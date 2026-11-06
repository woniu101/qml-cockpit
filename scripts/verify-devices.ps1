param([ValidateSet('1','1.25')][string]$Scale='1')
$ErrorActionPreference='Stop'
$workspace=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $workspace
$env:PATH='D:\Qt\6.12.0\msvc2022_64\bin;'+$env:PATH
$env:QT_FORCE_STDERR_LOGGING='1'
$env:QT_SCALE_FACTOR=$Scale
Remove-Item Env:QSG_RENDER_LOOP -ErrorAction SilentlyContinue
$name=if($Scale -eq '1'){'native'}else{'dpi125'}
$env:CAR_TERMINAL_DEVICE_OUTPUT="artifacts/devices/$name"
New-Item -ItemType Directory -Path $env:CAR_TERMINAL_DEVICE_OUTPUT -Force | Out-Null
$run=Start-Process -FilePath "$workspace\build\QmlCockpit.exe" -ArgumentList '--device-smoke' -WorkingDirectory $workspace -WindowStyle Normal -Wait -PassThru -RedirectStandardError "$workspace\artifacts\devices\$name.log" -RedirectStandardOutput "$workspace\artifacts\devices\$name-out.log"
if($run.ExitCode -ne 0){throw "Device UI failed: $($run.ExitCode), see artifacts/devices/$name.log"}
Write-Output "PASS devices UI ($Scale): artifacts/devices/$name"
