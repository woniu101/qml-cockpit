"""Local keyword spotter. Live PCM and WAV tests use the same streaming decoder."""
import base64
import io
import json
import math
import re
import sys
import tempfile
import wave
from pathlib import Path

import numpy as np
import sherpa_onnx
from pypinyin import Style, pinyin
from scipy.signal import resample_poly


class WakeSpotter:
    def __init__(self, model):
        self.model = model
        self.tokens = {line.split()[0] for line in (model / "tokens.txt").read_text(encoding="utf-8").splitlines()}
        self.keyword = None
        self.spotter = None
        self.stream = None
        self.stream_id = None

    def configure(self, keyword):
        if keyword == self.keyword:
            return
        if not re.fullmatch(r"[\u3400-\u9fff]{4,8}", keyword):
            raise ValueError("唤醒词需为 4 到 8 个汉字")
        initials = pinyin(keyword, style=Style.INITIALS, strict=False)
        finals = pinyin(keyword, style=Style.FINALS_TONE, strict=False)
        tokens = [token for a, b in zip(initials, finals) for token in (a[0], b[0]) if token]
        if any(token not in self.tokens for token in tokens):
            raise ValueError("模型不支持该唤醒词的读音，请换一种说法")
        with tempfile.TemporaryDirectory() as tmp:
            file = Path(tmp) / "keywords.txt"
            file.write_text(" ".join(tokens) + " @" + keyword + "\n", encoding="utf-8")
            spotter = sherpa_onnx.KeywordSpotter(
                tokens=str(self.model / "tokens.txt"),
                encoder=str(self.model / "encoder-epoch-13-avg-2-chunk-8-left-64.int8.onnx"),
                decoder=str(self.model / "decoder-epoch-13-avg-2-chunk-8-left-64.onnx"),
                joiner=str(self.model / "joiner-epoch-13-avg-2-chunk-8-left-64.int8.onnx"),
                keywords_file=str(file), num_threads=2, keywords_score=1.0, keywords_threshold=0.25)
        self.spotter = spotter
        self.keyword = keyword
        self.stream = None
        self.stream_id = None

    def decode(self, stream, signal):
        stream.accept_waveform(16000, signal)
        while self.spotter.is_ready(stream):
            self.spotter.decode_stream(stream)
            if self.spotter.get_result(stream) == self.keyword:
                self.spotter.reset_stream(stream)
                return True
        return False

    def run(self, request):
        self.configure(request["keyword"])
        operation = request["operation"]
        if operation == "configure":
            return {"ok": True}
        raw = base64.b64decode(request["audio"], validate=True)
        if operation == "replay":
            with wave.open(io.BytesIO(raw), "rb") as audio:
                rate, channels = audio.getframerate(), audio.getnchannels()
                if audio.getsampwidth() != 2 or not rate // 5 <= audio.getnframes() <= rate * 10:
                    raise ValueError("请选择 0.2 到 10 秒的 PCM16 WAV")
                signal = pcm(audio.readframes(audio.getnframes()), "s16", rate, channels)
            signal = np.concatenate((signal, np.zeros(16000, dtype=np.float32)))
            stream = self.spotter.create_stream()
            hit = any(self.decode(stream, signal[i:i + 1280]) for i in range(0, len(signal), 1280))
            return {"ok": True, "hit": hit}
        if operation != "feed":
            raise ValueError("未知唤醒请求")
        stream_id = request["stream"]
        if self.stream is None or self.stream_id != stream_id:
            self.stream = self.spotter.create_stream()
            self.stream_id = stream_id
        signal = pcm(raw, request["encoding"], request["rate"], request["channels"])
        return {"ok": True, "hit": self.decode(self.stream, signal)}


def pcm(raw, encoding, rate, channels):
    if not 8000 <= rate <= 192000 or not 1 <= channels <= 8:
        raise ValueError("不支持的麦克风格式")
    dtype, scale, offset = {"s16": ("<i2", 32768, 0), "s32": ("<i4", 2147483648, 0),
                            "f32": ("<f4", 1, 0), "u8": ("u1", 128, 128)}[encoding]
    signal = np.frombuffer(raw, dtype=dtype).reshape(-1, channels).astype(np.float32)
    signal = (signal.mean(axis=1) - offset) / scale
    if rate != 16000:
        divisor = math.gcd(rate, 16000)
        signal = resample_poly(signal, 16000 // divisor, rate // divisor)
    return np.clip(np.nan_to_num(signal), -1, 1).astype(np.float32)


def main():
    spotter = WakeSpotter(Path(sys.argv[1]))
    for line in sys.stdin.buffer:
        try:
            result = spotter.run(json.loads(line))
        except (ValueError, RuntimeError, OSError, EOFError, wave.Error, KeyError) as error:
            result = {"error": str(error)[:160]}
        print(json.dumps(result, ensure_ascii=True), flush=True)


if __name__ == "__main__":
    main()
