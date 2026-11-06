param([string]$BuildDirectory = 'build', [switch]$External)
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$app = Join-Path (Join-Path $project $BuildDirectory) 'QmlCockpit.exe'
$manifest = Join-Path $project 'runtime/assistant/models.json'
$env:PATH = 'D:\Qt\6.12.0\msvc2022_64\bin;' + $env:PATH
foreach ($path in @($app, $manifest)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing native assistant file: $path" }
}
$config = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
if ($config.schema -ne 'cockpit-native-models-v1') { throw 'Unsupported native model manifest' }
$env:COCKPIT_ASSISTANT_MANIFEST = $manifest
$env:CAR_TERMINAL_NLU_WINDOW_PILOT = if ($config.nluWindowPilot) { '1' } else { '0' }
$logs = Join-Path $project 'artifacts/assistant'
New-Item -ItemType Directory -Force $logs | Out-Null
$env:CAR_TERMINAL_NLU_LOG = Join-Path $logs 'samples.jsonl'
$launch = @{ FilePath = $app; WorkingDirectory = $project; WindowStyle = 'Normal' }
if ($External) { $launch.ArgumentList = '--external' }
Start-Process @launch
