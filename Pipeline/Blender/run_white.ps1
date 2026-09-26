$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = Resolve-Path (Join-Path $here '..\..')
$manifest = Join-Path $here 'forge_manifest.json'
$forge = Join-Path $here 'chess_forge.py'

if (-not $env:BLENDER_EXE) {
  $candidates = @(
    'C:\Program Files\Blender Foundation\Blender 4.5\blender.exe',
    'C:\Program Files\Blender Foundation\Blender 4.4\blender.exe',
    'C:\Program Files\Blender Foundation\Blender 4.3\blender.exe',
    'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe'
  )
  $env:BLENDER_EXE = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $env:BLENDER_EXE -or -not (Test-Path $env:BLENDER_EXE)) { throw 'Set BLENDER_EXE to Blender 4.2+ blender.exe' }
if (-not $env:TUMBO_CHESS_SOURCE_ROOT) { throw 'Set TUMBO_CHESS_SOURCE_ROOT to the local sculpt/MetaHuman export asset pack.' }
if (-not $env:TUMBO_FACE_IMAGE -or -not (Test-Path $env:TUMBO_FACE_IMAGE)) { throw 'Set TUMBO_FACE_IMAGE to the private local face.jpg. It is never copied into the repo.' }

$out = Join-Path $repo 'chess\glb'
$private = if ($env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT) { $env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT } else { Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review' }
& $env:BLENDER_EXE --background --python $forge -- --manifest $manifest --faction white --piece all --source-root $env:TUMBO_CHESS_SOURCE_ROOT --output-root $out --private-review-root $private --face-image $env:TUMBO_FACE_IMAGE
if ($LASTEXITCODE -ne 0) { throw "White army forge failed: $LASTEXITCODE" }
Write-Host "White army private review complete: $private"
