"""One local recognition request. Input audio is carried over stdin."""
import io
import json
import math
import sys
import wave
from pathlib import Path

import numpy as np
import sherpa_onnx
from scipy.signal import resample_poly


def read_audio(raw, format_args):
    if format_args == ['wav']:
        with wave.open(io.BytesIO(raw)) as stream:
            if stream.getsampwidth() != 2:
                raise ValueError('WAV replay requires PCM16')
            encoding, rate, channels = 's16', stream.getframerate(), stream.getnchannels()
            raw = stream.readframes(stream.getnframes())
    else:
        encoding, rate, channels = format_args[0], int(format_args[1]), int(format_args[2])
    if not 8000 <= rate <= 192000 or not 1 <= channels <= 8:
        raise ValueError('Invalid sample rate or channels')
    dtype, scale, offset = {'s16': ('<i2', 32768., 0), 's32': ('<i4', 2147483648., 0),
                            'f32': ('<f4', 1., 0), 'u8': ('u1', 128., 128)}[encoding]
    width = np.dtype(dtype).itemsize * channels
    count = len(raw) // width
    if not rate // 5 <= count <= rate * 10:
        raise ValueError('Unexpected recording length')
    signal = np.frombuffer(raw[:count * width], dtype=dtype).reshape(-1, channels).astype(np.float32)
    signal = (signal.mean(axis=1) - offset) / scale
    if not np.isfinite(signal).all():
        raise ValueError('Invalid audio samples')
    if rate != 16000:
        divisor = math.gcd(rate, 16000)
        signal = resample_poly(signal, 16000 // divisor, rate // divisor)
    return np.clip(signal, -1, 1).astype(np.float32)


def main():
    model = Path(sys.argv[1])
    audio = read_audio(sys.stdin.buffer.read(32 * 1024 * 1024 + 1), sys.argv[2:])
    recognizer = sherpa_onnx.OfflineRecognizer.from_paraformer(
        paraformer=str(model / 'model.int8.onnx'), tokens=str(model / 'tokens.txt'), num_threads=2)
    stream = recognizer.create_stream()
    stream.accept_waveform(16000, audio)
    recognizer.decode_stream(stream)
    sys.stdout.buffer.write(json.dumps({'text': stream.result.text}, ensure_ascii=False).encode('utf-8'))


if __name__ == '__main__':
    main()
