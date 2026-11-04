"""Local JSON-lines adapter for a pinned rbtl3 research export."""
import json
import os
from pathlib import Path
import sys

os.environ["HF_HUB_OFFLINE"] = "1"
os.environ["TOKENIZERS_PARALLELISM"] = "false"
folder = Path(sys.argv[1])
sys.path.insert(0, str(folder / "source-snapshot"))

import numpy as np
import onnxruntime as ort
from evaluation import predictions
from model import tokenizer
from schema import MAX_LENGTH

tok = tokenizer(folder)
options = ort.SessionOptions()
options.intra_op_num_threads = 2
options.inter_op_num_threads = 1
engine = ort.InferenceSession(str(folder / "cockpit-nlu.int8.onnx"), options, providers=["CPUExecutionProvider"])
threshold = json.loads((folder / "calibration.json").read_text(encoding="utf-8"))["threshold"]

for line in sys.stdin.buffer:
    try:
        request = json.loads(line)
        text = request["text"]
        feed = {k: v.astype(np.int64) for k, v in tok(text, return_tensors="np").items()}
        if feed["input_ids"].shape[1] > MAX_LENGTH:
            result = {"block_reason": "too many tokens", "proposal": None}
        else:
            result = predictions(tok, [{"text": text}], engine.run(None, feed), threshold)[0]
        result["model"] = folder.name
    except Exception as error:
        result = {"error": str(error)[:200]}
    print(json.dumps(result, ensure_ascii=True, allow_nan=False), flush=True)
