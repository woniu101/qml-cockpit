"""Training/export tool. The desktop runtime never invokes this script."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')
    temporary.replace(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--research', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--nlu-model', type=Path)
    parser.add_argument('--candidate-model', type=Path)
    parser.add_argument('--nlu-precision', choices=['fp32','int8'], default='fp32')
    args = parser.parse_args()
    research, output = args.research.resolve(), args.output.resolve()
    models = research / 'models'
    definitions = {
        'asr': (models / 'sherpa-onnx-paraformer-zh-small-2024-03-09', ['model.int8.onnx', 'tokens.txt']),
        'wake': (models / 'sherpa-onnx-kws-zipformer-zh-en-3M-2025-12-20', [
            'encoder-epoch-13-avg-2-chunk-8-left-64.int8.onnx',
            'decoder-epoch-13-avg-2-chunk-8-left-64.onnx',
            'joiner-epoch-13-avg-2-chunk-8-left-64.int8.onnx', 'tokens.txt']),
        'voice': (models / 'sherpa-onnx-zipvoice-distill-int8-zh-en-emilia', [
            'encoder.int8.onnx', 'decoder.int8.onnx', 'tokens.txt', 'lexicon.txt']),
        'nlu': ((args.nlu_model or research / 'training/runs/rbtl3-v8-classifier').resolve(), [
            f'cockpit-nlu.{args.nlu_precision}.onnx', 'vocab.txt', 'labels.json', 'calibration.json', 'tokenizer_config.json']),
        'nluCandidate': ((args.candidate_model or research / 'training/runs/rbtl3-v7').resolve(), [
            'cockpit-nlu.int8.onnx', 'vocab.txt', 'labels.json', 'calibration.json', 'tokenizer_config.json']),
    }
    manifest = {'schema': 'cockpit-native-models-v1', 'nluWindowPilot': False}
    for role, (source, names) in definitions.items():
        destination = output / source.name
        destination.mkdir(parents=True, exist_ok=True)
        for name in names:
            src, dst = source / name, destination / name
            if not src.exists():
                raise FileNotFoundError(src)
            if src.is_dir():
                shutil.copytree(src, dst, dirs_exist_ok=True)
            else:
                shutil.copy2(src, dst)
        manifest[role] = source.name
        if role in ('nlu','nluCandidate'):
            precision = args.nlu_precision if role == 'nlu' else 'int8'
            write_json(destination / 'inference.json', {'schema':'cockpit-nlu-engine-v1',
                'file':f'cockpit-nlu.{precision}.onnx','precision':precision})
    # Export the exact pronunciation convention previously used for custom KWS.
    from pypinyin import Style, pinyin
    dictionary = {}
    for code in range(0x3400, 0xa000):
        char = chr(code)
        initial = pinyin(char, style=Style.INITIALS, strict=False)[0][0]
        final = pinyin(char, style=Style.FINALS_TONE, strict=False)[0][0]
        if final != char:
            dictionary[char] = [part for part in (initial, final) if part]
    write_json(output / manifest['wake'] / 'pronunciations.json', dictionary)
    # Preserve the pilot's exact model-fingerprint approval, not an automatic release.
    approval_path = definitions['nluCandidate'][0] / 'single-window-release.json'
    if approval_path.is_file():
        approval = json.loads(approval_path.read_text(encoding='utf-8-sig'))
        digest = hashlib.sha256((output / manifest['nluCandidate'] / 'cockpit-nlu.int8.onnx').read_bytes()).hexdigest()
        manifest['nluWindowPilot'] = approval.get('scope') == 'single-window-confirmed' and approval.get('sha256', '').lower() == digest
    shutil.copy2(models / 'vocos_24khz.onnx', output / manifest['voice'] / 'vocos_24khz.onnx')
    for role in definitions:
        directory = output / manifest[role]
        files = {p.relative_to(directory).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in directory.rglob('*') if p.is_file() and p.name != 'bundle.json'}
        write_json(directory / 'bundle.json', {'schema': 'cockpit-native-bundle-v1', 'role': role, 'files': files})
    reference = output / 'default-voice'
    reference.mkdir(exist_ok=True)
    shutil.copy2(research / 'samples/zipvoice/dev-reference.wav', reference / 'reference.wav')
    shutil.copy2(research / 'samples/zipvoice/dev-reference.txt', reference / 'transcript.txt')
    manifest.update(defaultReference='default-voice/reference.wav', defaultTranscript='default-voice/transcript.txt')
    # Publish the manifest last so incomplete exports cannot be selected.
    write_json(output / 'models.json', manifest)
    print(json.dumps({'output': str(output), 'roles': list(definitions), 'python_required_at_runtime': False}))


if __name__ == '__main__':
    main()
