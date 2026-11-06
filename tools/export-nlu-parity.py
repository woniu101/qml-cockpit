"""Generate reference fixtures with the training environment, never at runtime."""
import argparse
import json
from pathlib import Path
import random
import sys


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', type=Path, required=True)
    parser.add_argument('--corpus', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--precision', choices=['fp32','int8'],default='int8')
    parser.add_argument('--limit', type=int, default=600)
    args = parser.parse_args()
    model = args.model.resolve()
    sys.path.insert(0, str(model / 'source-snapshot'))
    import numpy as np
    import onnxruntime as ort
    from model import tokenizer
    from evaluation import predictions
    tok = tokenizer(model)
    options = ort.SessionOptions()
    options.intra_op_num_threads = 2
    engine = ort.InferenceSession(str(model / f'cockpit-nlu.{args.precision}.onnx'), options, providers=['CPUExecutionProvider'])
    threshold = json.loads((model / 'calibration.json').read_text())['threshold']
    rows = []
    for split in ('test', 'challenge'):
        path = args.corpus / f'{split}.jsonl'
        rows.extend(json.loads(line) for line in path.read_text(encoding='utf-8').splitlines())
    random.Random(20261106).shuffle(rows)
    rows = rows[:args.limit]
    rows.extend({'text': text, 'input': 'text'} for text in ('播放 Café 的歌', 'SOC还有多少？', '查一下右后车窗开度', '主驾温度调高到二十八度', '不要把主驾温度调高到二十八度', '😀左前车窗开了多少', '打开[CLS]车窗', 'ÀBCD１２３'))
    results = []
    for row in rows:
        text = row['text']
        feed = {k: v.astype(np.int64) for k, v in tok(text, return_tensors='np').items()}
        if feed['input_ids'].shape[1] > 64:
            continue
        prediction = predictions(tok, [row], engine.run(None, feed), threshold)[0]
        results.append({'text': text, 'input': row.get('input', 'text'), 'ids': feed['input_ids'][0].tolist(),
                        'offsets': tok(text, return_offsets_mapping=True)['offset_mapping'], 'expected': prediction})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(results, ensure_ascii=False), encoding='utf-8')
    print(f'Exported {len(results)} reference predictions')


if __name__ == '__main__':
    main()
