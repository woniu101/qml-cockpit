param([string]$SimulatorExe = '', [string]$ServerName = 'qml-cockpit.vehicle.v1')
$ErrorActionPreference = 'Stop'
$workspace = Split-Path -Parent $PSScriptRoot
if (-not $SimulatorExe) { $SimulatorExe = Join-Path (Split-Path -Parent $workspace) 'qml-cockpit-simulator\build\VehicleSimulator.exe' }
$cockpitExe = Join-Path $workspace 'build\CarTerminal.exe'
foreach ($path in @($SimulatorExe, $cockpitExe)) {
    if (-not (Test-Path -LiteralPath $path)) { throw "程序不存在，请先构建两个仓库：$path" }
}
if ($ServerName -notmatch '^[a-zA-Z0-9._-]{1,100}$') { throw '服务名只支持字母、数字、点、下划线和短横线。' }
$env:PATH = 'D:\Qt\6.12.0\msvc2022_64\bin;' + $env:PATH
# 复用已运行的模拟器；两个窗口独立退出。
$simulatorPath = (Resolve-Path -LiteralPath $SimulatorExe).Path
$existingSimulator = Get-Process VehicleSimulator -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $simulatorPath }
if (-not $existingSimulator -or $ServerName -ne 'qml-cockpit.vehicle.v1') {
    Start-Process -FilePath $simulatorPath -ArgumentList @('--server', $ServerName) -WorkingDirectory (Split-Path -Parent $simulatorPath) -WindowStyle Normal | Out-Null
}
Start-Process -FilePath $cockpitExe -ArgumentList @('--external', "--vehicle-server=$ServerName") -WorkingDirectory $workspace -WindowStyle Normal | Out-Null
