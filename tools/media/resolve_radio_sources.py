"""Read public broadcaster player configuration; ephemeral URLs stay in artifacts."""
import argparse
import json
import re
import urllib.request
from datetime import datetime
from pathlib import Path
from zoneinfo import ZoneInfo


def fetch(url, data=None):
    request = urllib.request.Request(url, data=data, headers={
        'User-Agent': 'QmlCockpit-MediaValidation/0.1',
        'Content-Type': 'application/json',
    })
    with urllib.request.urlopen(request, timeout=15) as response:
        return response.read(2_000_000)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    stations, errors = [], []
    try:
        # Endpoint and POST body are published by www.cnr.cn's own player script.
        catalog = json.loads(fetch('https://apppc.cnr.cn/national', b'{}'))
        channels = catalog['data']['categories'][0]['detail']
        for channel in channels:
            if channel['id'] in {'1', '2', '4'}:
                streams = channel.get('other_info11', [])
                url = next((s['url'] for s in streams if s.get('url', '').startswith('https://')), '')
                if url:
                    stations.append({'id': 'cnr-' + channel['id'], 'name': channel['name'],
                                     'url': url, 'originPage': 'https://www.cnr.cn/',
                                     'resolver': 'https://apppc.cnr.cn/national'})
    except Exception as error:
        errors.append({'provider': 'cnr', 'error': str(error)})
    try:
        origin = 'https://www.rthk.hk/radio/radio1'
        html = fetch(origin).decode('utf-8', 'replace')
        url = re.search(r'https://rthkradio1-live[^\s"<>]+\.m3u8', html).group(0)
        stations.append({'id': 'rthk-1', 'name': '香港电台第一台', 'url': url, 'originPage': origin})
    except Exception as error:
        errors.append({'provider': 'rthk', 'error': str(error)})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({'resolvedAt': datetime.now(ZoneInfo('Asia/Shanghai')).isoformat(),
                                     'stations': stations, 'resolutionErrors': errors}, ensure_ascii=False, indent=2), encoding='utf-8')
    print(f'Resolved {len(stations)} live sources; {len(errors)} provider errors. Temporary URLs saved only to {args.output}')
    return 0 if len(stations) >= 2 else 1


if __name__ == '__main__':
    raise SystemExit(main())
