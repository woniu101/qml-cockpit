param(
    [string]$BuildDirectory = 'build',
    [string]$ResearchRoot = '',
    [string]$ParityFixture = ''
)
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
if (!$ResearchRoot) { $ResearchRoot = Join-Path (Split-Path -Parent $project) 'ai-assistant-test' }
if (!$ParityFixture) { $ParityFixture = Join-Path $ResearchRoot 'reports/native-active-nlu-parity.json' }
$manifest = Join-Path $project 'runtime/assistant/models.json'
$config = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
if ($config.schema -ne 'cockpit-native-models-v1') { throw 'Unsupported native model manifest' }
$runtime = Split-Path -Parent $manifest
$required = @($ParityFixture, (Join-Path $project "$BuildDirectory/NativeInferenceTests.exe"))
foreach ($role in @('asr','wake','voice','nlu','nluCandidate','defaultReference','defaultTranscript')) {
    $required += Join-Path $runtime $config.$role
}
foreach ($path in $required) {
    if (!(Test-Path -LiteralPath $path)) { throw "Missing assistant validation input: $path" }
}
$env:PATH = 'D:/Qt/6.12.0/msvc2022_64/bin;' + $env:PATH
$env:QT_QPA_PLATFORM = 'offscreen'
$env:QT_FORCE_STDERR_LOGGING = '1'
$env:CAR_TERMINAL_NO_SETTINGS = '1'
$env:COCKPIT_ASSISTANT_MANIFEST = $manifest
$env:COCKPIT_NLU_PARITY = $ParityFixture
$env:COCKPIT_NATIVE_PARITY_REPORT = Join-Path $ResearchRoot 'reports/native-active-nlu-differences.json'
$env:COCKPIT_NATIVE_ASR_WAV = Join-Path $ResearchRoot 'samples/assistant-controls/driver-full.wav'
$roles = @{ ASR='asr'; KWS='wake'; TTS='voice'; NLU='nlu'; NLU_CANDIDATE='nluCandidate' }
foreach ($role in $roles.Keys) {
    $name = if ($role -eq 'NLU_CANDIDATE') { 'CAR_TERMINAL_NLU_CANDIDATE' } else { "CAR_TERMINAL_${role}_MODEL" }
    [Environment]::SetEnvironmentVariable($name, (Join-Path $runtime $config.($roles[$role])), 'Process')
}
$env:CAR_TERMINAL_TTS_REFERENCE = Join-Path $runtime $config.defaultReference
$env:CAR_TERMINAL_TTS_TRANSCRIPT = Join-Path $runtime $config.defaultTranscript
$env:CAR_TERMINAL_NLU_WINDOW_PILOT = if ($config.nluWindowPilot) { '1' } else { '0' }
$sampleSets = @{
    CAR_TERMINAL_ASSISTANT_WAKE_WAVS='synthetic'; CAR_TERMINAL_ASSISTANT_DIALOGUE_WAVS='assistant-dialogue'
    CAR_TERMINAL_ASSISTANT_CONTROL_WAVS='assistant-controls'; CAR_TERMINAL_ASSISTANT_EXTENDED_WAVS='assistant-extended'
    CAR_TERMINAL_ASSISTANT_TEST_WAVS='assistant-replay'; CAR_TERMINAL_NLU_WINDOW_WAVS='nlu-v7'
    CAR_TERMINAL_NLU_READONLY_WAVS='nlu-readonly'; CAR_TERMINAL_ASSISTANT_SHORT_MP3='ui-redesign/short.mp3'
}
foreach ($name in $sampleSets.Keys) {
    $path = Join-Path $ResearchRoot ('samples/' + $sampleSets[$name])
    if (!(Test-Path -LiteralPath $path)) { throw "Missing assistant test sample: $path" }
    [Environment]::SetEnvironmentVariable($name, $path, 'Process')
}
$parent = Split-Path -Parent $project
$env:CAR_TERMINAL_NLU_READONLY_CORPUS = Join-Path $ResearchRoot 'training/data-v6'
$env:CAR_TERMINAL_ASSISTANT_TEST_SONG = Join-Path $parent 'media/music/周杰伦/一路向北 - 周杰伦.mp3'
$env:CAR_TERMINAL_ASSISTANT_TEST_VIDEO = Join-Path $parent 'media/video/七里香MV.mp4'
$env:CAR_TERMINAL_ASSISTANT_TEST_SIMULATOR = Join-Path $parent 'qml-cockpit-simulator/build/QmlCockpitSimulator.exe'
foreach ($path in @($env:CAR_TERMINAL_ASSISTANT_TEST_SONG, $env:CAR_TERMINAL_ASSISTANT_TEST_VIDEO,
                    $env:CAR_TERMINAL_ASSISTANT_TEST_SIMULATOR, $env:CAR_TERMINAL_NLU_READONLY_CORPUS)) {
    if (!(Test-Path -LiteralPath $path)) { throw "Missing integration test input: $path" }
}
& 'D:/Qt/Tools/CMake_64/bin/ctest.exe' --test-dir (Join-Path $project $BuildDirectory) --output-on-failure --parallel 2
exit $LASTEXITCODE
