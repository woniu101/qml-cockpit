"""Editable media artboards. Displayed playback and library values are design samples."""
from pathlib import Path
from html import escape
import base64
import json
import re

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[1]
SCREENS = ROOT / 'screens'
PALETTES = {
    'day': dict(bg='#f1f3ee', ink='#23332f', muted='#6f7d73', green='#557a65', active='#e3ebe2', surface='#ffffff', secondary='#f0f3ed', line='#dfe5da', dock='#fafbf8'),
    'night': dict(bg='#18231f', ink='#e5eee5', muted='#a9b8aa', green='#acd2ac', active='#38513d', surface='#24342b', secondary='#304434', line='#3b4d3d', dock='#202e25'),
}
C = PALETTES['day']
CUSTOM = {
    'radio': '<rect x="3" y="7" width="18" height="14" rx="3"/><path d="m6 7 12-5M7 11h10M8 15v2m4-2v2m4-2v2"/>',
    'video': '<rect x="3" y="4" width="18" height="16" rx="3"/><path d="m10 8 6 4-6 4Z"/>',
    'heart': '<path d="M12 21S2 15 2 8a5 5 0 0 1 10-2 5 5 0 0 1 10 2c0 7-10 13-10 13Z"/>',
    'folder': '<path d="M3 7V4h7l3 3h8v13H3Z"/>',
    'expand': '<path d="M3 9V3h6m6 0h6v6M3 15v6h6m6 0h6v-6"/>',
    'stop': '<rect x="6" y="6" width="12" height="12" rx="2"/>',
    'headphones': '<path d="M4 14v-3a8 8 0 0 1 16 0v3"/><rect x="3" y="12" width="4" height="8" rx="2"/><rect x="17" y="12" width="4" height="8" rx="2"/>',
    'back': '<path d="M20 12H4m6-6-6 6 6 6"/>',
    'clock': '<circle cx="12" cy="12" r="9"/><path d="M12 7v5l3 2"/>',
    'network-off': '<path d="m3 3 18 18M3 8a15 15 0 0 1 18 0M6 12a10 10 0 0 1 8-2m-5 6a5 5 0 0 1 6 0"/>',
}
ICONS = {}
for path in (REPO / 'assets/ui/icons').glob('*.svg'):
    match = re.search(r'<svg[^>]*>(.*)</svg>', path.read_text(encoding='utf-8'), re.S)
    if match:
        ICONS[path.stem] = match.group(1)
ICONS.update(CUSTOM)


def txt(x, y, value, size=20, color=None, weight=400, anchor='start'):
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{color or C["ink"]}" font-weight="{weight}" text-anchor="{anchor}">{escape(str(value))}</text>'


def rect(x, y, w, h, radius=24, color=None, stroke='none'):
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{radius}" fill="{color or C["surface"]}" stroke="{stroke}"/>'


def line(x, y, x2, y2, color=None, width=1):
    return f'<path d="M{x} {y}H{x2}" stroke="{color or C["line"]}" stroke-width="{width}"/>' if y == y2 else f'<path d="M{x} {y}L{x2} {y2}" stroke="{color or C["line"]}" stroke-width="{width}"/>'


def ico(name, x, y, size=25, color=None):
    content = ICONS[name].replace('currentColor', color or C['ink'])
    return f'<g transform="translate({x} {y}) scale({size/24})" fill="none" stroke="{color or C["ink"]}" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">{content}</g>'


def button(x, y, w, label, icon=None, active=False, primary=False, h=48):
    fill = C['green'] if primary else C['active'] if active else C['surface']
    color = C['bg'] if primary else C['green'] if active else C['ink']
    content = rect(x, y, w, h, h / 2, fill)
    if icon:
        if not label:
            return content + ico(icon, x + (w - 24) / 2, y + (h - 24) / 2, 24, color)
        text_width = sum(19 if ord(char) > 255 else 10 for char in label)
        left = x + (w - text_width - 34) / 2
        return content + ico(icon, left, y + (h - 24) / 2, 24, color) + txt(left + 34, y + h / 2 + 7, label, 19, color)
    return content + txt(x + w / 2, y + h / 2 + 7, label, 19, color, anchor='middle')


def art(kind, x, y, w, h, radius=20):
    scale = max(w / 800, h / 450)
    s = f'<g transform="translate({(w-800*scale)/2} {(h-450*scale)/2}) scale({scale})">'
    if kind == 'city':
        s += '<rect width="800" height="450" fill="#cbd8d7"/><circle cx="610" cy="95" r="52" fill="#e8dbc2"/><path d="M0 330V190h95v-60h90v140h45V95h70v190h60V155h100v-45h80v160h70V180h80v90h70v-75h40v255H0Z" fill="#849c98"/><path d="M0 330h800v120H0Z" fill="#7b9695"/><path d="M0 390Q220 340 400 385T800 365" fill="none" stroke="#b5cdca" stroke-width="5"/><path d="M0 430Q250 380 500 425T800 415" fill="none" stroke="#b5cdca" stroke-width="3"/>'
    elif kind == 'forest':
        s += '<rect width="800" height="450" fill="#d9ded0"/><circle cx="610" cy="100" r="55" fill="#f4e8cb"/><path d="M0 250Q150 110 330 245T800 170V450H0Z" fill="#9eaf91"/><path d="M0 360Q180 185 420 335T800 245V450H0Z" fill="#5e7e69"/><path d="M0 440Q190 310 440 430T800 380V450H0Z" fill="#3a5a4c"/>'
        for i in range(11):
            a = i * 84 - 20
            s += f'<path d="M{a} 370l32-115 32 115Z" fill="#3f6251"/>'
    else:
        s += '<rect width="800" height="450" fill="#ded5c1"/><circle cx="630" cy="107" r="62" fill="#f6ecd4"/><path d="M0 275Q160 95 370 270T800 160V450H0Z" fill="#a8b19a"/><path d="M0 350Q210 180 430 350T800 270V450H0Z" fill="#6e8976"/><path d="M240 450Q330 355 420 322T495 240" fill="none" stroke="#d5d5be" stroke-width="63"/><path d="M240 450Q330 355 420 322T495 240" fill="none" stroke="#9d9f8a" stroke-width="48"/><path d="M240 450Q330 355 420 322T495 240" fill="none" stroke="#eee8d2" stroke-width="3" stroke-dasharray="17 17"/>'
    # Embed each illustration as its own SVG image: its viewport clips the scene
    # consistently in browsers and Qt's restricted SVG renderer.
    embedded = f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">' + s + '</g></svg>'
    encoded = base64.b64encode(embedded.encode('utf-8')).decode()
    return f'<image x="{x}" y="{y}" width="{w}" height="{h}" href="data:image/svg+xml;base64,{encoded}"/>'


def slider(x, y, w, value=0.36, color=None):
    return rect(x, y, w, 4, 2, C['line']) + rect(x, y, w * value, 4, 2, color or C['green']) + f'<circle cx="{x+w*value}" cy="{y+2}" r="7" fill="{color or C["green"]}"/>'


def shell(tab='电台', night=False, gear='P', home=False):
    global C
    C = PALETTES['night' if night else 'day']
    s = f'<svg xmlns="http://www.w3.org/2000/svg" width="1600" height="900" viewBox="0 0 1600 900"><title>QML Cockpit 媒体中心 — {tab}</title><rect width="1600" height="900" fill="{C["bg"]}"/><g font-family="Microsoft YaHei UI, Microsoft YaHei, sans-serif">'
    s += txt(64, 44, '21:08' if night else '16:28', 22, C['muted']) + line(166, 23, 166, 44)
    s += ico('moon' if night else 'sun', 188, 22, 24, C['green']) + txt(224, 44, '24°', 20, C['muted']) + txt(285, 44, '上海', 20, C['muted'])
    s += button(370, 13, 176, 'P · 驻车' if gear == 'P' else 'D · 40 km/h', 'car', active=True, h=44)
    s += ico('layers', 627, 23, 25, C['green']) + txt(663, 44, '联调 · 已连接', 17, C['green'])
    s += txt(1168, 44, '90%', 19, C['muted']) + rect(1223, 28, 34, 14, 3, C['bg'], C['muted']) + rect(1226, 31, 26, 8, 1, C['green'])
    s += ico('bluetooth', 1289, 22, 24, C['muted']) + ico('ethernet', 1349, 22, 24, C['green']) + txt(1383, 43, '有线', 14, C['green']) + ico('bell', 1443, 22, 24)
    s += f'<circle cx="1534" cy="35" r="18" fill="{C["surface"]}" stroke="{C["line"]}"/>' + txt(1534, 42, '林', 18, C['green'], anchor='middle')
    if not home:
        s += txt(56, 130, '媒体', 34)
        s += rect(207, 92, 534, 52, 26, C['secondary'])
        for i, (name, icon) in enumerate([('音乐', 'music'), ('电台', 'radio'), ('视频', 'video'), ('手机蓝牙', 'bluetooth')]):
            x = 213 + i * 124
            s += button(x, 96, 118 if i < 3 else 146, name, None, active=name == tab, h=44)
        s += txt(57, 179, '收藏喜欢的声音，继续未完的旅程' if tab == '电台' else '让每一段停留，都有喜欢的内容', 17, C['muted'])
        if tab != '视频播放':
            s += button(1325, 97, 220, '目录管理' if tab in ['音乐', '视频'] else '音频输出', 'folder' if tab in ['音乐', '视频'] else 'volume')
    s += rect(0, 790, 1600, 110, 0, C['dock']) + line(0, 790, 1600, 790)
    for i, (name, icon) in enumerate([('首页', 'home'), ('导航', 'nav'), ('车辆', 'car'), ('媒体', 'music')]):
        x = 30 + i * 83
        if (home and i == 0) or (not home and i == 3):
            s += rect(x, 803, 69, 79, 24, C['active'])
        s += ico(icon, x + 23, 815, 25, C['green'] if i == (0 if home else 3) else C['ink']) + txt(x + 35, 868, name, 17, C['green'] if i == (0 if home else 3) else C['ink'], anchor='middle')
    for x in [383, 993, 1225]:
        s += line(x, 818, x, 872)
    for x, label in [(490, '主驾'), (1105, '副驾')]:
        s += txt(x, 823, label, 15, C['muted'], anchor='middle') + txt(x, 865, '22.0°', 38, anchor='middle') + ico('chevron-left', x - 87, 838, 24) + ico('chevron-right', x + 62, 838, 24)
    for x, label, icon in [(640, 'AUTO', 'fan'), (733, 'A/C', 'snow'), (827, '除雾', 'defrost'), (920, '座椅', 'seat')]:
        s += ico(icon, x - 13, 817, 26, C['green']) + txt(x, 868, label, 15, C['muted'], anchor='middle')
    return s + ico('volume', 1275, 830, 25, C['muted']) + slider(1324, 844, 108, 0.45) + ico('apps', 1495, 831, 26)


def channel_tile(x, y, initials, large=False):
    size = 182 if large else 52
    s = rect(x, y, size, size, 32 if large else 14, C['active'])
    if large:
        s += ico('radio', x + 65, y + 35, 52, C['green']) + txt(x + size / 2, y + 128, initials, 30, C['green'], anchor='middle')
    else:
        s += txt(x + size / 2, y + 34, initials, 20, C['green'], anchor='middle')
    return s


def radio(state='playing', night=False):
    s = shell(night=night)
    s += rect(56, 215, 925, 535) + rect(1005, 215, 540, 535)
    s += channel_tile(90, 252, '央广', True)
    s += txt(314, 279, '央广 · 新闻综合', 17, C['muted']) + txt(312, 336, '中国之声', 44)
    s += button(314, 361, 111, '网络直播', active=True, h=34) + button(442, 361, 118, '已收藏', 'heart', h=34)
    if state == 'offline':
        s += ico('network-off', 316, 431, 28, C['muted']) + txt(356, 455, '网络连接已断开', 25)
        s += txt(314, 494, '恢复连接后，可重新收听当前电台', 18, C['muted'])
        s += button(314, 554, 229, '重新连接', 'refresh', primary=True, h=58) + button(563, 554, 164, '切换电台', h=58)
    elif state == 'loading':
        s += ico('refresh', 316, 431, 28, C['green']) + txt(356, 455, '正在连接电台…', 25)
        s += txt(314, 494, '等待直播音频，可随时取消', 18, C['muted'])
        s += button(314, 554, 229, '取消连接', 'close', h=58)
    else:
        s += f'<circle cx="322" cy="446" r="5" fill="{C["green"]}"/>' + txt(341, 453, '正在收听', 21, C['green'])
        s += txt(314, 492, '直播内容由电台实时提供', 18, C['muted'])
        s += button(314, 554, 229, '停止收听', 'stop', primary=True, h=58)
        s += button(565, 554, 68, '', 'previous', h=58) + button(647, 554, 68, '', 'skip', h=58)
    s += line(90, 657, 947, 657) + ico('headphones', 92, 689, 23, C['muted']) + txt(126, 709, 'Windows 默认输出', 17, C['muted'])
    for i, (label, width) in enumerate([('全部', 88), ('收藏', 88), ('最近', 88)]):
        s += button(1032 + i * 99, 237, width, label, active=i == 0, h=38)
    s += ico('search', 1484, 245, 23, C['muted'])
    rows = [('声', '中国之声', '央广 · 新闻综合'), ('经', '经济之声', '央广 · 财经资讯'), ('乐', '音乐之声', '央广 · 音乐'), ('港', '香港电台第一台', '香港 · 综合')]
    for i, (initials, name, description) in enumerate(rows):
        y = 295 + i * 97
        if i == 0:
            s += rect(1027, y, 496, 85, 20, C['secondary'])
        s += channel_tile(1043, y + 16, initials) + txt(1111, y + 37, name, 22) + txt(1111, y + 65, description, 16, C['muted'])
        s += ico('stop' if i == 0 and state == 'playing' else 'play', 1471, y + 28, 24, C['green'])
    return s + txt(1036, 725, '共 4 个电台', 15, C['muted'])


def music():
    s = shell('音乐')
    s += art('road', 56, 216, 310, 310) + txt(56, 577, '沿途', 34) + txt(56, 617, '本地演示音源', 20, C['muted']) + button(56, 682, 148, '已收藏', 'heart', active=True)
    s += rect(414, 216, 568, 359) + txt(698, 291, '轻音乐精选', 18, C['muted'], anchor='middle')
    s += txt(698, 368, '让风景，慢一点经过', 29, C['green'], anchor='middle') + txt(698, 417, '在喜欢的旋律里继续前行', 21, C['muted'], anchor='middle') + txt(698, 532, '此音源无歌词', 15, C['muted'], anchor='middle')
    s += slider(428, 612, 540, 0.39) + txt(428, 647, '00:25', 16, C['muted']) + txt(968, 647, '01:04', 16, C['muted'], anchor='end')
    s += button(566, 682, 61, '', 'previous', h=58) + button(656, 674, 76, '', 'pause', active=True, h=74) + button(761, 682, 61, '', 'skip', h=58)
    s += rect(1005, 215, 540, 535) + txt(1034, 259, '音乐库', 26) + button(1362, 232, 156, '导入音乐', 'plus', h=43)
    s += rect(1031, 284, 487, 44, 16, C['secondary']) + ico('search', 1047, 295, 22, C['muted']) + txt(1082, 313, '搜索歌曲、歌手或专辑', 17, C['muted'])
    for i, (name, kind) in enumerate([('沿途', 'road'), ('晚风', 'city'), ('山间', 'forest')]):
        y = 348 + i * 95
        if i == 0:
            s += rect(1027, y, 496, 83, 18, C['secondary'])
        s += art(kind, 1041, y + 12, 60, 60, 13) + txt(1120, y + 36, name, 23) + txt(1120, y + 64, '本地演示音源', 16, C['muted']) + ico('play', 1471, y + 29, 24, C['green'])
    return s + line(1035, 672, 1515, 672) + button(1034, 688, 220, '播放队列 · 3 首', h=40) + button(1271, 688, 246, '列表循环', h=40)


def video_library(empty=False):
    s = shell('视频')
    s += button(56, 216, 147, '全部视频', active=True, h=44) + button(56, 279, 147, '最近观看', 'clock', h=44) + button(56, 342, 147, '按文件夹', 'folder', h=44)
    s += line(227, 216, 227, 747)
    s += txt(265, 245, '本地视频', 26) + txt(1543, 245, '0 个视频' if empty else '3 个视频', 16, C['muted'], anchor='end')
    if empty:
        s += rect(265, 274, 1280, 474)
        s += ico('video', 862, 365, 63, C['green']) + txt(905, 477, '把喜欢的视频，带到旅途中', 29, anchor='middle')
        s += txt(905, 519, '添加本地文件或目录，支持包含子文件夹', 19, C['muted'], anchor='middle') + button(786, 567, 238, '添加视频目录', 'plus', primary=True, h=55)
        return s
    names = [('山海之间', '自驾风景 / 沿途', 'road', '12:34', '继续观看 · 04:38'), ('城市慢行', '城市影像', 'city', '08:21', '尚未观看'), ('走进山野', '自然 / 旅行', 'forest', '06:18', '尚未观看')]
    for i, (name, path, kind, duration, progress) in enumerate(names):
        x = 265 + i * 435
        s += rect(x, 276, 410, 350) + art(kind, x + 12, 288, 386, 220, 17)
        s += rect(x + 324, 466, 60, 28, 10, '#23332f') + txt(x + 354, 486, duration, 15, '#ffffff', anchor='middle')
        s += txt(x + 24, 548, name, 25) + txt(x + 24, 580, path, 16, C['muted']) + txt(x + 24, 608, progress, 16, C['green'] if i == 0 else C['muted'])
    return s + ico('folder', 268, 687, 24, C['muted']) + txt(304, 708, '本地文件 · 支持目录与子目录', 17, C['muted']) + button(1307, 674, 238, '添加视频目录', 'plus', h=50)


def video_playback(locked=False):
    s = shell('视频', gear='D' if locked else 'P')
    s += button(56, 216, 113, '返回', 'back', h=42) + txt(193, 246, '山海之间', 24)
    s += rect(56, 276, 1104, 468, 25, '#17251f')
    if locked:
        s += ico('lock', 565, 366, 55, '#c3d5c6') + txt(608, 479, '请在驻车后观看视频', 30, '#e5eee5', anchor='middle')
        s += txt(608, 523, '视频已暂停，当前进度已保留', 19, '#a9b8aa', anchor='middle')
    else:
        s += art('road', 192, 276, 832, 468, 0)
        s += rect(76, 644, 1064, 82, 18, '#23332f') + ico('pause', 99, 668, 28, '#ffffff') + txt(148, 688, '04:38 / 12:34', 16, '#e5eee5')
        s += slider(306, 679, 514, 0.37, '#c1d7c7') + txt(859, 688, '1.0×', 18, '#ffffff') + ico('volume', 944, 667, 26, '#ffffff') + ico('expand', 1079, 666, 28, '#ffffff')
    s += rect(1184, 276, 361, 468) + txt(1211, 319, '播放列表', 24)
    for i, (name, kind, duration) in enumerate([('山海之间', 'road', '12:34'), ('城市慢行', 'city', '08:21'), ('走进山野', 'forest', '06:18')]):
        y = 348 + i * 111
        if i == 0:
            s += rect(1200, y, 329, 94, 17, C['secondary'])
        s += art(kind, 1213, y + 14, 98, 65, 10) + txt(1329, y + 39, name, 20) + txt(1329, y + 69, duration, 16, C['muted'])
    return s


def bluetooth():
    s = shell('手机蓝牙') + rect(56, 215, 925, 535) + rect(1005, 215, 540, 535)
    s += ico('bluetooth', 109, 341, 137, C['green']) + txt(315, 332, '手机蓝牙音乐', 38) + txt(317, 381, 'vivo X200 Pro', 23, C['muted'])
    s += txt(317, 448, '手机音频已接通', 25, C['green']) + txt(317, 494, '播放、切歌与音量请在手机控制', 19, C['muted'])
    s += button(317, 557, 345, '查看手机音乐连接', 'bluetooth', active=True, h=58)
    s += txt(1035, 261, '手机连接', 26) + txt(1035, 342, '声音输出至 Windows 默认设备', 21) + txt(1035, 392, '本地音乐库与观看记录保持保留', 18, C['muted'])
    return s + button(1035, 464, 480, '蓝牙设备与音乐连接', 'bluetooth', h=55) + button(1035, 546, 480, '系统声音设置', 'volume', h=55)


def video_fullscreen():
    global C
    C = PALETTES['night']
    s = '<svg xmlns="http://www.w3.org/2000/svg" width="1600" height="900" viewBox="0 0 1600 900"><title>视频全屏</title><g font-family="Microsoft YaHei UI, Microsoft YaHei, sans-serif">'
    s += art('road', 0, 0, 1600, 900, 0)
    s += rect(32, 28, 410, 64, 22, '#23332f') + ico('back', 53, 47, 27, '#ffffff') + txt(101, 69, '山海之间', 25, '#ffffff')
    s += rect(56, 749, 1488, 110, 24, '#23332f') + ico('pause', 87, 789, 31, '#ffffff')
    s += txt(145, 813, '04:38 / 12:34', 20, '#e5eee5') + slider(338, 798, 771, 0.37, '#c1d7c7')
    return s + txt(1157, 813, '1.0×', 20, '#ffffff') + ico('volume', 1275, 787, 29, '#ffffff') + txt(1370, 812, '退出全屏', 20, '#ffffff')


def home():
    s = shell(home=True) + txt(56, 135, '下午好，林先生', 34) + txt(57, 175, '11月3日 周二', 18, C['muted'])
    s += txt(59, 267, '90', 52, '#267f78') + txt(123, 267, '%', 23, C['muted']) + line(197, 222, 197, 300) + txt(233, 267, '468', 52) + txt(342, 267, 'km', 22, C['muted'])
    s += txt(59, 303, '电量 · 高电量', 17, '#267f78') + txt(233, 303, '预计续航', 18, C['muted'])
    for x, name, icon in [(718, '车锁', 'lock'), (837, '后备厢', 'trunk'), (956, '充电口', 'charge')]:
        s += button(x, 216, 75, '', icon, h=64) + txt(x + 37, 314, name, 17, C['muted'], anchor='middle')
    car = 'data:image/png;base64,' + base64.b64encode((REPO / 'assets/ui/vehicle-pearl.png').read_bytes()).decode()
    s += f'<image x="130" y="352" width="825" height="385" href="{car}"/>'
    s += rect(1110, 93, 438, 278) + ico('search', 1137, 119, 25, C['green']) + txt(1178, 147, '想去哪里？', 27)
    s += txt(1138, 193, '常用目的地', 18, C['muted']) + button(1138, 215, 174, '回家', 'home', active=True, h=64) + button(1330, 215, 188, '公司', 'briefcase', active=True, h=64)
    s += line(1138, 303, 1518, 303) + txt(1328, 345, '附近充电站', 20, anchor='middle')
    s += rect(1110, 393, 438, 235) + channel_tile(1138, 422, '声') + txt(1211, 447, '中国之声', 27) + txt(1212, 481, '网络电台', 17, C['muted'])
    s += f'<circle cx="1146" cy="537" r="4" fill="{C["green"]}"/>' + txt(1162, 544, '正在收听', 18, C['green']) + button(1137, 566, 382, '停止收听', 'stop', active=True, h=41)
    return s + rect(1110, 650, 438, 112) + txt(1138, 711, '场景模式', 18, C['muted']) + txt(1315, 711, '小憩', 21) + txt(1450, 711, '露营', 21)


PAGES = [
    ('01-music', '音乐 · 本地音乐库', music),
    ('02-radio', '电台 · 正在收听', radio),
    ('03-video-library', '视频 · 本地视频库', video_library),
    ('04-video-player', '视频 · 宽屏播放', video_playback),
    ('05-radio-connecting', '电台 · 连接与取消', lambda: radio('loading')),
    ('06-radio-offline', '电台 · 断线与恢复', lambda: radio('offline')),
    ('07-video-empty', '视频 · 初次导入', lambda: video_library(True)),
    ('08-video-parked-only', '视频 · 驻车限制', lambda: video_playback(True)),
    ('09-radio-night', '电台 · 夜间模式', lambda: radio(night=True)),
    ('10-home-radio', '首页 · 电台媒体卡', home),
    ('11-phone-bluetooth', '媒体 · 手机蓝牙', bluetooth),
    ('12-video-fullscreen', '视频 · 沉浸全屏', video_fullscreen),
]


def main():
    SCREENS.mkdir(parents=True, exist_ok=True)
    entries = []
    for name, title, build in PAGES:
        (SCREENS / f'{name}.svg').write_text(build() + '</g></svg>', encoding='utf-8')
        entries.append(dict(id=name, title=title, svg=f'screens/{name}.svg', png=f'screens/{name}.png'))
    (ROOT / 'artboards.json').write_text(json.dumps(entries, ensure_ascii=False, indent=2), encoding='utf-8')
    options = ''.join(f'<button data-id="{entry["id"]}">{entry["title"]}</button>' for entry in entries)
    html = '''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>QML Cockpit · 媒体设计评审</title><meta name="viewport" content="width=device-width,initial-scale=1"><style>
*{box-sizing:border-box}body{margin:0;background:#e8ede7;color:#23332f;font:15px 'Microsoft YaHei UI',sans-serif}header{padding:22px 28px;background:#fafbf8;border-bottom:1px solid #cfd9cd}h1{font-size:23px;margin:0 0 8px}p{margin:0;color:#6f7d73;line-height:1.7}main{display:grid;grid-template-columns:230px minmax(0,1fr);gap:22px;padding:22px}nav{display:flex;flex-direction:column;gap:9px}button,select,a{font:inherit}button{border:1px solid transparent;border-radius:14px;padding:13px 12px;text-align:left;background:#f7f9f4;color:#23332f;cursor:pointer}button[aria-current=true]{background:#557a65;color:white}.viewer{min-width:0}.toolbar{display:flex;gap:16px;align-items:center;margin-bottom:14px}.toolbar a{color:#557a65}select{padding:8px;border:1px solid #cfd9cd;border-radius:10px;background:#fafbf8}figure{margin:0;overflow:auto;background:#dce3d9;border-radius:16px}img{display:block;width:100%;height:auto}figcaption{padding:15px 4px;color:#6f7d73}#notice{padding-top:8px;font-size:13px}@media(max-width:850px){main{grid-template-columns:1fr}nav{flex-direction:row;overflow:auto}nav button{white-space:nowrap}header{padding:18px}}
</style><header><h1>QML Cockpit · 媒体中心</h1><p>阶段 1A 静态设计稿 · 1600 × 900 · 2026-11-03</p><p id="notice">页面数据为设计示例。点击左侧切换画板；页面内按钮尚未接入功能。台源验证结果另见 README。</p></header><main><nav>OPTIONS</nav><section class="viewer"><div class="toolbar"><label>查看尺寸 <select id="size"><option value="fit">适应窗口</option><option value="1600">1600 × 900</option><option value="1120">1120 × 630</option><option value="2000">125% / 2000 × 1125</option></select></label><a id="svg" target="_blank">打开可编辑 SVG</a></div><figure><img id="preview" alt="媒体中心设计稿"></figure><figcaption id="caption"></figcaption></section></main><script>
const names=ENTRIES;let current=names[1];const image=document.querySelector('#preview');function show(entry){current=entry;const size=document.querySelector('#size').value;image.src='screens/'+entry.id+(size==='1120'?'@min':size==='2000'?'@125':'')+'.png';image.style.width=size==='fit'?'100%':size+'px';document.querySelector('#svg').href=entry.svg;document.querySelector('#caption').textContent=entry.title;document.querySelectorAll('nav button').forEach(b=>b.setAttribute('aria-current',b.dataset.id===entry.id))}document.querySelectorAll('nav button').forEach(b=>b.onclick=()=>show(names.find(n=>n.id===b.dataset.id)));document.querySelector('#size').onchange=()=>show(current);show(current);
</script></html>'''.replace('OPTIONS', options).replace('ENTRIES', json.dumps(entries, ensure_ascii=False))
    (ROOT / 'index.html').write_text(html, encoding='utf-8')
    print(f'Generated {len(entries)} editable artboards and local review gallery.')


if __name__ == '__main__':
    main()
