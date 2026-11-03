param([ValidateSet('1','1.25')][string]$Scale='1')
$ErrorActionPreference='Stop'
$workspace=Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $workspace
$env:PATH='D:\Qt\6.12.0\msvc2022_64\bin;'+$env:PATH
$env:QT_FORCE_STDERR_LOGGING='1'
$env:QT_SCALE_FACTOR=$Scale
Remove-Item Env:CAR_TERMINAL_MUSIC_LIBRARY_FILE -ErrorAction SilentlyContinue
$name=if($Scale -eq '1'){'native'}else{'dpi125'}
$env:CAR_TERMINAL_MUSIC_OUTPUT="artifacts/music/$name"
New-Item -ItemType Directory -Path $env:CAR_TERMINAL_MUSIC_OUTPUT -Force | Out-Null
$run=Start-Process -FilePath "$workspace\build\CarTerminal.exe" -ArgumentList '--music-smoke' -WorkingDirectory $workspace -WindowStyle Normal -Wait -PassThru -RedirectStandardError "$workspace\artifacts\music\$name.log"
if($run.ExitCode -ne 0){throw "Music UI failed: $($run.ExitCode), see artifacts/music/$name.log"}
$fixture="$workspace\artifacts\music\$name\fixtures\歌手\专辑\测试音源.wav"
$fixtureUri=([Uri]::new($fixture)).AbsoluteUri
$profile="$workspace\artifacts\music\$name\resume-library.json"
@{version=1;tracks=@(@{url=$fixtureUri;title='Local Music Test';source='本地测试音源';available=$true});lastUrl=$fixtureUri;position=6000;roots=@();excluded=@();offsets=@{}} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $profile -Encoding utf8
$env:CAR_TERMINAL_MUSIC_LIBRARY_FILE=$profile
$resume=Start-Process -FilePath "$workspace\build\CarTerminal.exe" -ArgumentList '--music-resume-check' -WorkingDirectory $workspace -WindowStyle Normal -Wait -PassThru -RedirectStandardError "$workspace\artifacts\music\$name-resume.log"
Remove-Item Env:CAR_TERMINAL_MUSIC_LIBRARY_FILE -ErrorAction SilentlyContinue
if($resume.ExitCode -ne 0){throw "Music resume failed: $($resume.ExitCode), see artifacts/music/$name-resume.log"}
Write-Output "PASS music UI ($Scale): artifacts/music/$name"
