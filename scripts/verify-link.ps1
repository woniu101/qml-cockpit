param(
    [string]$SimulatorExe = '',
    [ValidateSet('1','1.25')][string]$Scale = '1'
)
$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $workspace
if (-not $SimulatorExe) {
    $SimulatorExe = Join-Path (Split-Path -Parent $workspace) 'qml-cockpit-simulator\build\VehicleSimulator.exe'
}
if (-not (Test-Path -LiteralPath $SimulatorExe)) { throw '请先构建 qml-cockpit-simulator，或用 -SimulatorExe 指定程序。' }
if (-not (Test-Path -LiteralPath 'build\CarTerminal.exe')) { throw '请先执行 scripts/build.cmd。' }
$env:PATH = 'D:\Qt\6.12.0\msvc2022_64\bin;' + $env:PATH
$env:QML_COCKPIT_SIMULATOR_EXE = (Resolve-Path -LiteralPath $SimulatorExe).Path
$env:QT_SCALE_FACTOR = $Scale
$env:QT_FORCE_STDERR_LOGGING = '1'
Remove-Item Env:QSG_RENDER_LOOP -ErrorAction SilentlyContinue
$name = if ($Scale -eq '1') { 'native' } else { 'dpi125' }
$env:CAR_TERMINAL_LINK_OUTPUT = "artifacts/link/$name"
New-Item -ItemType Directory -Path $env:CAR_TERMINAL_LINK_OUTPUT -Force | Out-Null
$run = Start-Process -FilePath "$workspace\build\CarTerminal.exe" -ArgumentList '--link-smoke' -WorkingDirectory $workspace -WindowStyle Normal -Wait -PassThru -RedirectStandardError "$workspace\artifacts\link\$name.log" -RedirectStandardOutput "$workspace\artifacts\link\$name-stdout.log"
if ($run.ExitCode -ne 0) { throw "双进程联调失败：$($run.ExitCode)，详见 artifacts/link/$name.log" }
Write-Output "PASS dual-process integration ($Scale): artifacts/link/$name"
