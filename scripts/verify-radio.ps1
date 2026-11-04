$ErrorActionPreference='Stop'
$workspace=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $workspace
$env:PATH='D:\Qt\6.12.0\msvc2022_64\bin;'+$env:PATH
python tools/media/resolve_radio_sources.py artifacts/radio/sources.json
if($LASTEXITCODE -ne 0){throw 'Could not resolve enough official live sources'}
& .\build\media-tools\RadioStreamProbe.exe artifacts/radio/sources.json artifacts/radio/verification.json 2> artifacts/radio/probe.log
if($LASTEXITCODE -ne 0){throw 'Radio capability check had failures; see artifacts/radio/verification.json'}
Write-Output 'PASS muted radio stream decoding, stop, pending-load cancellation and HTTP error reporting'
