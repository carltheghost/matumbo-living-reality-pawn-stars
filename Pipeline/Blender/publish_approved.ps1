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

$final=Join-Path $repo 'chess\glb'
$stage=Join-Path $repo 'chess\.phase1-approved-staging'
$private=if($env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT){$env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT}else{Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review'}
$pieces=@('king','queen','bishop','knight','rook','pawn')
$factions=@('black','white')
$expected=@()
foreach($side in $factions){
  foreach($piece in $pieces){
    $expected += "$side-$piece.glb"
    $expected += "preview-$side-$piece.png"
  }
}

if(Test-Path $stage){ Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null

try {
  & $env:BLENDER_EXE --background --python $forge -- --manifest $manifest --faction all --piece all --source-root $env:TUMBO_CHESS_SOURCE_ROOT --output-root $stage --private-review-root $private --repository-root $repo --face-image $env:TUMBO_FACE_IMAGE --publish-approved
  if ($LASTEXITCODE -ne 0) { throw "Approved publication forge failed: $LASTEXITCODE" }

  $missing=@($expected | Where-Object { -not (Test-Path (Join-Path $stage $_)) })
  if($missing.Count -gt 0){
    throw "Publication transaction incomplete. Missing: $($missing -join ', ')"
  }

  New-Item -ItemType Directory -Force -Path $final | Out-Null
  $backup=Join-Path $env:TEMP ("matumbo-chess-publish-backup-" + [guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Force -Path $backup | Out-Null
  $installed=@()

  try {
    foreach($name in $expected){
      $dest=Join-Path $final $name
      if(Test-Path $dest){ Copy-Item -Force $dest (Join-Path $backup $name) }
    }
    foreach($name in $expected){
      Move-Item -Force (Join-Path $stage $name) (Join-Path $final $name)
      $installed += $name
    }
  } catch {
    foreach($name in $installed){
      $dest=Join-Path $final $name
      if(Test-Path $dest){ Remove-Item -Force $dest }
    }
    foreach($name in $expected){
      $saved=Join-Path $backup $name
      if(Test-Path $saved){ Copy-Item -Force $saved (Join-Path $final $name) }
    }
    throw
  } finally {
    if(Test-Path $backup){ Remove-Item -Recurse -Force $backup }
  }

  Write-Host 'Approved 12-piece publication transaction complete. Review git diff before commit.'
} finally {
  if(Test-Path $stage){ Remove-Item -Recurse -Force $stage }
}
