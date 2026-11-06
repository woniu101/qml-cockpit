param([string]$DownloadCache = '')
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$sdk = Join-Path $project '.deps/assistant-native'
if (!$DownloadCache) { $DownloadCache = Join-Path $project '.deps/downloads' }
New-Item -ItemType Directory -Force $sdk, $DownloadCache | Out-Null
$version = '2.0.1'
$archive = "sherpa-onnx-v$version-win-x64-shared-MD-Release-lib.tar.bz2"
$specs = @(
    @{Name=$archive; Url="https://github.com/k2-fsa/sherpa-onnx/releases/download/v$version/$archive"; Sha='C3C78220DB8BEBF97D5F932496165BB3897B163410427D7C021013439C80F183'},
    @{Name='sherpa-c-api.h'; Url="https://raw.githubusercontent.com/k2-fsa/sherpa-onnx/v$version/sherpa-onnx/c-api/c-api.h"; Sha='5698F38BA09AD2BDF085C56A6CBBD7741ACF15E25F4CE5C7097B9702CADF437F'},
    @{Name='onnxruntime_c_api.h'; Url='https://raw.githubusercontent.com/microsoft/onnxruntime/v1.20.1/include/onnxruntime/core/session/onnxruntime_c_api.h'; Sha='573A117AE6B83EAD7F53DA6CEDAA0D7B7D9CDB45C0BA4726BAC9081E67696A82'}
)
foreach ($spec in $specs) {
    $path = Join-Path $DownloadCache $spec.Name
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) {
        & curl.exe --ssl-revoke-best-effort --fail --location --retry 2 --output $path $spec.Url
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $($spec.Name)" }
    }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $spec.Sha) { throw "SHA256 mismatch: $path" }
}
& tar.exe -xf (Join-Path $DownloadCache $archive) -C $DownloadCache
if ($LASTEXITCODE -ne 0) { throw 'Cannot extract inference SDK' }
$include = Join-Path $sdk 'include/sherpa-onnx/c-api'
New-Item -ItemType Directory -Force $include | Out-Null
Copy-Item -LiteralPath (Join-Path $DownloadCache 'sherpa-c-api.h') -Destination (Join-Path $include 'c-api.h') -Force
Copy-Item -LiteralPath (Join-Path $DownloadCache 'onnxruntime_c_api.h') -Destination (Join-Path $sdk 'include/onnxruntime_c_api.h') -Force
Copy-Item -LiteralPath (Join-Path $DownloadCache "sherpa-onnx-v$version-win-x64-shared-MD-Release-lib/lib") -Destination $sdk -Recurse -Force
$specs | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $sdk 'sources.json') -Encoding utf8
Write-Host "Native SDK ready: $sdk"
