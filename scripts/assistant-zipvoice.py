"""Generate local WAV replies with a resident ZipVoice model."""
import json
from pathlib import Path
import sys
import time
import wave

import numpy as np
import sherpa_onnx as sherpa

folder = Path(sys.argv[1])
model = sherpa.OfflineTtsModelConfig()
model.num_threads = 4
model.provider = "cpu"
model.zipvoice.encoder = str(folder / "encoder.int8.onnx")
model.zipvoice.decoder = str(folder / "decoder.int8.onnx")
model.zipvoice.vocoder = str(folder.parent / "vocos_24khz.onnx")
model.zipvoice.data_dir = str(folder / "espeak-ng-data")
model.zipvoice.tokens = str(folder / "tokens.txt")
model.zipvoice.lexicon = str(folder / "lexicon.txt")
tts = sherpa.OfflineTts(sherpa.OfflineTtsConfig(model=model))


def generate(request):
    start = time.monotonic()
    with wave.open(request["reference"], "rb") as audio:
        rate, channels = audio.getframerate(), audio.getnchannels()
        if audio.getsampwidth() != 2 or not 1 <= channels <= 2 or not 8000 <= rate <= 96000:
            raise ValueError("expected mono/stereo PCM16 WAV")
        if not rate <= audio.getnframes() <= 15 * rate:
            raise ValueError("reference duration must be 1–15 seconds")
        samples = np.frombuffer(audio.readframes(audio.getnframes()), dtype="<i2").reshape(-1, channels)
        samples = samples.mean(axis=1, dtype=np.float32) / 32768
        if len(samples) < rate or np.max(np.abs(samples)) < 0.005:
            raise ValueError("reference audio is empty or too quiet")
    config = sherpa.GenerationConfig()
    config.reference_audio = samples.tolist()
    config.reference_sample_rate = rate
    config.reference_text = request["transcript"]
    config.num_steps = 4
    result = tts.generate(request["text"], config)
    samples = np.asarray(result.samples, dtype=np.float32)
    if not samples.size or not np.isfinite(samples).all():
        raise ValueError("ZipVoice did not return a valid waveform")
    with wave.open(request["output"], "wb") as output:
        output.setparams((1, 2, result.sample_rate, 0, "NONE", "not compressed"))
        output.writeframes((np.clip(samples, -1, 1) * 32767).astype("<i2").tobytes())
    return {"seconds": samples.size / result.sample_rate, "elapsed_ms": round(1000 * (time.monotonic() - start))}


for line in sys.stdin.buffer:
    try:
        result = generate(json.loads(line))
    except Exception as error:
        result = {"error": str(error)[:200]}
    print(json.dumps(result, ensure_ascii=True, allow_nan=False), flush=True)
