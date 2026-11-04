param([string]$BuildDirectory = 'build/assistant-dev', [switch]$External)
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$research = Join-Path (Split-Path -Parent $project) 'ai-assistant-test'
$app = Join-Path (Join-Path $project $BuildDirectory) 'CarTerminal.exe'
$env:PATH = 'D:\Qt\6.12.0\msvc2022_64\bin;' + $env:PATH
$env:CAR_TERMINAL_ASR_PYTHON = Join-Path $research '.venv/Scripts/python.exe'
$env:CAR_TERMINAL_ASR_WORKER = Join-Path $PSScriptRoot 'assistant-asr.py'
$env:CAR_TERMINAL_ASR_MODEL = Join-Path $research 'models/sherpa-onnx-paraformer-zh-small-2024-03-09'
$env:CAR_TERMINAL_KWS_WORKER = Join-Path $PSScriptRoot 'assistant-kws.py'
$env:CAR_TERMINAL_KWS_MODEL = Join-Path $research 'models/sherpa-onnx-kws-zipformer-zh-en-3M-2025-12-20'
$env:CAR_TERMINAL_NLU_PYTHON = Join-Path $research '.venv-train/Scripts/python.exe'
$env:CAR_TERMINAL_NLU_WORKER = Join-Path $PSScriptRoot 'assistant-nlu.py'
$env:CAR_TERMINAL_NLU_MODEL = Join-Path $research 'training/runs/rbtl3-v3'
$candidate = Join-Path $research 'training/runs/rbtl3-v4'
if (Test-Path (Join-Path $candidate 'cockpit-nlu.int8.onnx') -PathType Leaf) {
    $env:CAR_TERMINAL_NLU_CANDIDATE = $candidate
}
$env:CAR_TERMINAL_NLU_LOG = Join-Path $research 'reports/assistant-nlu-samples.jsonl'
$env:CAR_TERMINAL_TTS_WORKER = Join-Path $PSScriptRoot 'assistant-zipvoice.py'
$env:CAR_TERMINAL_TTS_MODEL = Join-Path $research 'models/sherpa-onnx-zipvoice-distill-int8-zh-en-emilia'
$env:CAR_TERMINAL_TTS_REFERENCE = Join-Path $research 'samples/zipvoice/dev-reference.wav'
$env:CAR_TERMINAL_TTS_TRANSCRIPT = Join-Path $research 'samples/zipvoice/dev-reference.txt'
foreach ($path in @($app, $env:CAR_TERMINAL_ASR_PYTHON, $env:CAR_TERMINAL_ASR_WORKER, (Join-Path $env:CAR_TERMINAL_ASR_MODEL 'model.int8.onnx'))) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing assistant development file: $path" }
}
$launch = @{ FilePath = $app; WorkingDirectory = $project; WindowStyle = 'Normal' }
if ($External) { $launch.ArgumentList = '--external' }
Start-Process @launch
