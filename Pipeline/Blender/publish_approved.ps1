param([ValidateSet('all')][string]$Faction='all')
$ErrorActionPreference='Stop'
if ($env:TUMBO_FACE_PUBLICATION_APPROVED -ne 'YES') {
  throw 'Publication gate closed. Set TUMBO_FACE_PUBLICATION_APPROVED=YES only after Tumbo explicitly approves the 12 local previews.'
}
$here=Split-Path -Parent $MyInvocation.MyCommand.Path
$repo=Resolve-Path (Join-Path $here '..\..')
$manifest=Join-Path $here 'forge_manifest.json'
$forge=Join-Path $here 'chess_forge.py'
if (-not $env:BLENDER_EXE -or -not (Test-Path $env:BLENDER_EXE)) { throw 'Set BLENDER_EXE.' }
if (-not $env:TUMBO_CHESS_SOURCE_ROOT) { throw 'Set TUMBO_CHESS_SOURCE_ROOT.' }
if (-not $env:TUMBO_FACE_IMAGE -or -not (Test-Path $env:TUMBO_FACE_IMAGE)) { throw 'Set TUMBO_FACE_IMAGE.' }
$out=Join-Path $repo 'chess\glb'
$private=if($env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT){$env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT}else{Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review'}
& $env:BLENDER_EXE --background --python $forge -- --manifest $manifest --faction $Faction --piece all --source-root $env:TUMBO_CHESS_SOURCE_ROOT --output-root $out --private-review-root $private --repository-root $repo --face-image $env:TUMBO_FACE_IMAGE --publish-approved
if ($LASTEXITCODE -ne 0) { throw "Approved publication forge failed: $LASTEXITCODE" }
Write-Host "Approved outputs written to $out. Review git diff before commit."
