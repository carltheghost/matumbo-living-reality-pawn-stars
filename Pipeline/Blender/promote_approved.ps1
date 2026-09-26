param([ValidateSet('all')][string]$Faction='all')

$ErrorActionPreference='Stop'
if($env:TUMBO_FACE_PUBLICATION_APPROVED -ne 'YES'){
  throw 'Publication gate closed. Tumbo must approve all 12 previews first.'
}

$here=Split-Path -Parent $MyInvocation.MyCommand.Path
$repo=(Resolve-Path (Join-Path $here '..\..')).Path
$review=if($env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT){$env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT}else{Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review'}
if(-not (Test-Path $review)){ throw "Private review root not found: $review" }
$review=(Resolve-Path $review).Path

$approvalPath=Join-Path $review 'approval.json'
if(-not (Test-Path $approvalPath)){
  throw 'Missing approval.json. Run approve_review.ps1 -IApproveAll12 after visual review.'
}
$approval=Get-Content -Raw $approvalPath | ConvertFrom-Json
if($approval.schemaVersion -ne 1 -or $approval.approvedBy -ne 'Tumbo'){
  throw 'Invalid approval receipt.'
}

$pieces=@('king','queen','bishop','knight','rook','pawn')
$factions=@('black','white')
$approved=@()

foreach($side in $factions){
  foreach($piece in $pieces){
    $slug="$side-$piece"
    $record=$approval.artifacts.$slug
    if($null -eq $record){ throw "Approval receipt missing $slug" }

    foreach($kind in @('preview','glb')){
      $entry=$record.$kind
      $source=Join-Path $review ([string]$entry.file)
      if(-not (Test-Path $source)){ throw "Approved artifact missing: $source" }
      $actual=(Get-FileHash -Algorithm SHA256 $source).Hash.ToLowerInvariant()
      $expected=([string]$entry.sha256).ToLowerInvariant()
      if($actual -ne $expected){ throw "Approved hash mismatch: $slug/$kind" }
      $approved += [pscustomobject]@{ Source=$source; Name=[IO.Path]::GetFileName($source); Hash=$actual }
    }

    $qcEntry=$record.qc
    $qcPath=Join-Path $review ([string]$qcEntry.file)
    if(-not (Test-Path $qcPath)){ throw "QC receipt missing: $slug" }
    $qcHash=(Get-FileHash -Algorithm SHA256 $qcPath).Hash.ToLowerInvariant()
    if($qcHash -ne ([string]$qcEntry.sha256).ToLowerInvariant()){ throw "QC receipt changed: $slug" }
    $qc=Get-Content -Raw $qcPath | ConvertFrom-Json
    if($qc.status -ne 'PASS' -or $qc.piece -ne $slug -or $qc.privacy -ne 'private-review'){
      throw "QC no longer proves a private PASS: $slug"
    }
  }
}

if($approved.Count -ne 24){ throw "Expected 24 approved files; found $($approved.Count)." }

$final=Join-Path $repo 'chess\glb'
New-Item -ItemType Directory -Force -Path $final | Out-Null
foreach($item in $approved){
  $dest=Join-Path $final $item.Name
  Copy-Item -Force $item.Source $dest
  $copied=(Get-FileHash -Algorithm SHA256 $dest).Hash.ToLowerInvariant()
  if($copied -ne $item.Hash){ throw "Post-copy hash mismatch: $($item.Name)" }
}

Write-Host 'PROMOTED: exact 12 reviewed GLBs + 12 reviewed preview PNGs copied to chess\glb.'
Write-Host 'No Blender rerun occurred and face.jpg was not copied.'
