param(
  [string]$ReviewRoot = $env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT,
  [switch]$IApproveAll12
)

$ErrorActionPreference='Stop'
if(-not $IApproveAll12){
  throw 'Approval not recorded. Eyeball all 12 preview PNGs first, then rerun with -IApproveAll12.'
}

$here=Split-Path -Parent $MyInvocation.MyCommand.Path
$repo=(Resolve-Path (Join-Path $here '..\..')).Path
if(-not $ReviewRoot){
  $ReviewRoot=Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review'
}
if(-not (Test-Path $ReviewRoot)){ throw "Private review root not found: $ReviewRoot" }
$review=(Resolve-Path $ReviewRoot).Path

$repoPrefix=$repo.TrimEnd('\') + '\'
if($review.StartsWith($repoPrefix,[System.StringComparison]::OrdinalIgnoreCase)){
  throw 'Private review root must stay outside the repository.'
}

$pieces=@('king','queen','bishop','knight','rook','pawn')
$factions=@('black','white')
$artifacts=[ordered]@{}

foreach($side in $factions){
  foreach($piece in $pieces){
    $slug="$side-$piece"
    $qcPath=Join-Path $review "qc-$slug.json"
    if(-not (Test-Path $qcPath)){ throw "Missing QC receipt: $qcPath" }

    $qc=Get-Content -Raw $qcPath | ConvertFrom-Json
    if($qc.status -ne 'PASS' -or $qc.piece -ne $slug -or $qc.privacy -ne 'private-review'){
      throw "QC receipt is not an approved private PASS for $slug"
    }

    $previewPath=Join-Path $review ([string]$qc.outputs.preview.file)
    $glbPath=Join-Path $review ([string]$qc.outputs.glb.file)
    if(-not (Test-Path $previewPath)){ throw "Missing preview for $slug: $previewPath" }
    if(-not (Test-Path $glbPath)){ throw "Missing GLB for $slug: $glbPath" }

    $previewHash=(Get-FileHash -Algorithm SHA256 $previewPath).Hash.ToLowerInvariant()
    $glbHash=(Get-FileHash -Algorithm SHA256 $glbPath).Hash.ToLowerInvariant()
    if($previewHash -ne ([string]$qc.outputs.preview.sha256).ToLowerInvariant()){
      throw "Preview changed after forge/QC for $slug"
    }
    if($glbHash -ne ([string]$qc.outputs.glb.sha256).ToLowerInvariant()){
      throw "GLB changed after forge/QC for $slug"
    }

    $artifacts[$slug]=[ordered]@{
      preview=[ordered]@{ file=[IO.Path]::GetFileName($previewPath); sha256=$previewHash }
      glb=[ordered]@{ file=[IO.Path]::GetFileName($glbPath); sha256=$glbHash }
      qc=[ordered]@{
        file=[IO.Path]::GetFileName($qcPath)
        sha256=(Get-FileHash -Algorithm SHA256 $qcPath).Hash.ToLowerInvariant()
      }
    }
  }
}

if($artifacts.Count -ne 12){ throw "Approval requires exactly 12 pieces; found $($artifacts.Count)." }

$approval=[ordered]@{
  schemaVersion=1
  approvedBy='Tumbo'
  approvedAt=(Get-Date).ToUniversalTime().ToString('o')
  reviewRoot=$review
  rule='Approval covers only these exact hashed private-review GLBs and preview PNGs.'
  artifacts=$artifacts
}
$approvalPath=Join-Path $review 'approval.json'
$approval | ConvertTo-Json -Depth 10 | Set-Content -Encoding UTF8 $approvalPath

Write-Host "APPROVED: exact 12-piece review set locked at $approvalPath"
Write-Host 'face.jpg remains outside the repository; this receipt approves only the hashed rendered/exported artifacts.'
