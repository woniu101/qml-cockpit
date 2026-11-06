param([ValidateSet('1','1.25')][string]$Scale='1')
$ErrorActionPreference='Stop'
$workspace=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $workspace
$env:PATH='D:\Qt\6.12.0\msvc2022_64\bin;'+$env:PATH
$env:QT_SCALE_FACTOR=$Scale
$env:QT_FORCE_STDERR_LOGGING='1'
$name=if($Scale -eq '1'){'native'}else{'dpi125'}
$env:CAR_TERMINAL_MEDIA_OUTPUT="artifacts/media/$name"
New-Item -ItemType Directory -Path $env:CAR_TERMINAL_MEDIA_OUTPUT -Force | Out-Null
Remove-Item -LiteralPath "$workspace\$($env:CAR_TERMINAL_MEDIA_OUTPUT)\verification.json" -ErrorAction SilentlyContinue
$run=Start-Process -FilePath "$workspace\build\QmlCockpit.exe" -ArgumentList '--media-smoke' -WorkingDirectory $workspace -WindowStyle Normal -Wait -PassThru -RedirectStandardError "$workspace\artifacts\media\$name.log"
if($run.ExitCode -ne 0){throw "Media center verification failed ($($run.ExitCode)); see artifacts/media/$name.log"}
Write-Output "PASS media center ($Scale): artifacts/media/$name"
