param([string]$PackageDir = 'dist/QmlCockpit')
$ErrorActionPreference='Stop'
Set-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
$workspace=(Get-Location).Path
$built=Get-FileHash -LiteralPath 'build/QmlCockpit.exe' -Algorithm SHA256
$package=(Resolve-Path -LiteralPath $PackageDir).Path
$exe=Join-Path $package 'QmlCockpit.exe'
$packaged=Get-FileHash -LiteralPath $exe -Algorithm SHA256
if($built.Hash -ne $packaged.Hash){throw 'Packaged executable differs from the current build'}
$env:PATH="$env:SystemRoot\System32;$env:SystemRoot"
foreach($key in @('QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QML_IMPORT_PATH','QML2_IMPORT_PATH','QSG_RENDER_LOOP','QSG_INFO')){Remove-Item -LiteralPath "Env:$key" -ErrorAction SilentlyContinue}
$env:QT_SCALE_FACTOR='1'
$env:CAR_TERMINAL_SKIP_SOAK='1'
$env:CAR_TERMINAL_SMOKE_DIR=Join-Path $workspace 'artifacts/v6/release'
New-Item -ItemType Directory -Force -Path $env:CAR_TERMINAL_SMOKE_DIR | Out-Null
Remove-Item Env:COCKPIT_ASSISTANT_MANIFEST -ErrorAction SilentlyContinue
$run=Start-Process -FilePath $exe -ArgumentList '--smoke-native' -WorkingDirectory $package -PassThru -Wait -RedirectStandardError "$workspace\artifacts\v6\release.log" -RedirectStandardOutput "$workspace\artifacts\v6\release-stdout.log"
if($run.ExitCode -ne 0){throw "Packaged app failed: exit code $($run.ExitCode); see artifacts/v6/release.log"}
Write-Output "Clean PATH release test passed; SHA256 $($packaged.Hash)"
