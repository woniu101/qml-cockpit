param([switch]$DpiOnly)
$ErrorActionPreference='Stop'
Set-Location -LiteralPath (Split-Path -Parent $PSScriptRoot)
$env:PATH='D:\Qt\6.12.0\msvc2022_64\bin;'+$env:PATH
Remove-Item Env:QSG_RENDER_LOOP -ErrorAction SilentlyContinue
$env:QSG_INFO='1'
$scales=if($DpiOnly){@('1.25')}else{@('1','1.25')}
foreach($scale in $scales){
    $env:QT_SCALE_FACTOR=$scale
    $name=if($scale -eq '1'){'native'}else{'dpi125'}
    $env:CAR_TERMINAL_SMOKE_DIR="artifacts/v6/$name"
    if($scale -eq '1.25'){$env:CAR_TERMINAL_SKIP_SOAK='1'}else{Remove-Item Env:CAR_TERMINAL_SKIP_SOAK -ErrorAction SilentlyContinue}
    $workspace=(Get-Location).Path
    $run=Start-Process -FilePath "$workspace\build\CarTerminal.exe" -ArgumentList '--smoke-native' -WorkingDirectory $workspace -WindowStyle Normal -Wait -PassThru -RedirectStandardError "$workspace\artifacts\v6\$name.log" -RedirectStandardOutput "$workspace\artifacts\v6\$name-stdout.log"
    if($run.ExitCode -ne 0){throw "$name failed with exit code $($run.ExitCode); see artifacts/v6/$name.log"}
    Write-Output "$name passed"
}
