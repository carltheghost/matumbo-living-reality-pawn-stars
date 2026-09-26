param([ValidateSet('all')][string]$Faction='all')

$ErrorActionPreference = 'Stop'
if ($env:TUMBO_FACE_PUBLICATION_APPROVED -ne 'YES') {
  throw 'Publication gate closed. Tumbo must approve all 12 previews first.'
}

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = (Resolve-Path (Join-Path $here '..\..')).Path
$review = if ($env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT) {
  $env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT
} else {
  Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review'
}
if (-not (Test-Path $review)) { throw "Private review root not found: $review" }
$review = (Resolve-Path $review).Path

$repoPrefix = $repo.TrimEnd('\') + '\'
if ($review.StartsWith($repoPrefix,[System.StringComparison]::OrdinalIgnoreCase)) {
  throw 'Private review root must stay outside the repository.'
}

$approvalPath = Join-Path $review 'approval.json'
if (-not (Test-Path $approvalPath)) {
  throw 'Missing approval.json. Run approve_review.ps1 -IApproveAll12 after visual review.'
}
$approval = Get-Content -Raw $approvalPath | ConvertFrom-Json
if ($approval.schemaVersion -ne 1 -or $approval.approvedBy -ne 'Tumbo') {
  throw 'Invalid approval receipt.'
}
if ($approval.reviewRoot -and
    -not ([string]$approval.reviewRoot).Equals($review,[System.StringComparison]::OrdinalIgnoreCase)) {
  throw 'Approval receipt belongs to a different private review root.'
}

$pieces = @('king','queen','bishop','knight','rook','pawn')
$factions = @('black','white')
$verified = @()

foreach ($side in $factions) {
  foreach ($piece in $pieces) {
    $slug = "$side-$piece"
    $record = $approval.artifacts.$slug
    if ($null -eq $record) { throw "Approval receipt missing $slug" }

    $expectedPreview = "preview-$slug.png"
    $expectedGlb = "$slug.glb"
    $expectedQc = "qc-$slug.json"
    if ($record.preview.file -ne $expectedPreview -or
        $record.glb.file -ne $expectedGlb -or
        $record.qc.file -ne $expectedQc) {
      throw "$slug approval receipt has unexpected filenames. Renamed/path-traversal artifacts are forbidden."
    }

    $previewPath = Join-Path $review $expectedPreview
    $glbPath = Join-Path $review $expectedGlb
    $qcPath = Join-Path $review $expectedQc
    foreach ($required in @($previewPath,$glbPath,$qcPath)) {
      if (-not (Test-Path $required -PathType Leaf)) {
        throw "Approved artifact missing: $required"
      }
    }

    $previewHash = (Get-FileHash -Algorithm SHA256 $previewPath).Hash.ToLowerInvariant()
    $glbHash = (Get-FileHash -Algorithm SHA256 $glbPath).Hash.ToLowerInvariant()
    $qcHash = (Get-FileHash -Algorithm SHA256 $qcPath).Hash.ToLowerInvariant()

    if ($previewHash -ne ([string]$record.preview.sha256).ToLowerInvariant()) {
      throw "Approved hash mismatch: $slug/preview"
    }
    if ($glbHash -ne ([string]$record.glb.sha256).ToLowerInvariant()) {
      throw "Approved hash mismatch: $slug/glb"
    }
    if ($qcHash -ne ([string]$record.qc.sha256).ToLowerInvariant()) {
      throw "QC receipt changed after approval: $slug"
    }

    $qc = Get-Content -Raw $qcPath | ConvertFrom-Json
    if ($qc.status -ne 'PASS' -or $qc.piece -ne $slug -or $qc.privacy -ne 'private-review') {
      throw "QC no longer proves a private PASS: $slug"
    }
    if (([string]$qc.outputs.preview.sha256).ToLowerInvariant() -ne $previewHash -or
        ([string]$qc.outputs.glb.sha256).ToLowerInvariant() -ne $glbHash) {
      throw "QC output hashes no longer match approved files: $slug"
    }

    $verified += [pscustomobject]@{
      Piece = $slug
      PreviewSource = $previewPath
      PreviewName = $expectedPreview
      PreviewHash = $previewHash
      PreviewBytes = (Get-Item $previewPath).Length
      GlbSource = $glbPath
      GlbName = $expectedGlb
      GlbHash = $glbHash
      GlbBytes = (Get-Item $glbPath).Length
      QcHash = $qcHash
    }
  }
}

if ($verified.Count -ne 12) {
  throw "Expected 12 approved pieces; found $($verified.Count)."
}

$stage = Join-Path $repo 'chess\.phase1-approved-staging'
$final = Join-Path $repo 'chess\glb'
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null

foreach ($item in $verified) {
  Copy-Item -Force $item.PreviewSource (Join-Path $stage $item.PreviewName)
  Copy-Item -Force $item.GlbSource (Join-Path $stage $item.GlbName)

  $stagedPreviewHash = (Get-FileHash -Algorithm SHA256 (Join-Path $stage $item.PreviewName)).Hash.ToLowerInvariant()
  $stagedGlbHash = (Get-FileHash -Algorithm SHA256 (Join-Path $stage $item.GlbName)).Hash.ToLowerInvariant()
  if ($stagedPreviewHash -ne $item.PreviewHash -or $stagedGlbHash -ne $item.GlbHash) {
    throw "Staging hash mismatch: $($item.Piece)"
  }
}

$approvedManifestName = 'phase1-approved-manifest.json'
$approvedManifest = [ordered]@{
  schemaVersion = 1
  approval = 'TUMBO_EXPLICIT_VISUAL_APPROVAL'
  approvedAt = $approval.approvedAt
  promotedAt = (Get-Date).ToUniversalTime().ToString('o')
  approvalReceiptSha256 = (Get-FileHash -Algorithm SHA256 $approvalPath).Hash.ToLowerInvariant()
  count = 12
  artifacts = @($verified | ForEach-Object {
    [ordered]@{
      piece = $_.Piece
      preview = [ordered]@{ file = $_.PreviewName; bytes = $_.PreviewBytes; sha256 = $_.PreviewHash }
      glb = [ordered]@{ file = $_.GlbName; bytes = $_.GlbBytes; sha256 = $_.GlbHash }
      privateQcSha256 = $_.QcHash
    }
  })
}
$approvedManifest | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 (Join-Path $stage $approvedManifestName)

$artifactNames = @($approvedManifestName)
foreach ($item in $verified) {
  $artifactNames += $item.PreviewName
  $artifactNames += $item.GlbName
}
$missing = @($artifactNames | Where-Object { -not (Test-Path (Join-Path $stage $_) -PathType Leaf) })
if ($missing.Count -gt 0) {
  throw "Publication transaction incomplete. Missing staged files: $($missing -join ', ')"
}

New-Item -ItemType Directory -Force -Path $final | Out-Null
$backup = Join-Path $env:TEMP ("matumbo-chess-publish-backup-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $backup | Out-Null
$installed = @()

try {
  foreach ($name in $artifactNames) {
    $dest = Join-Path $final $name
    if (Test-Path $dest) { Copy-Item -Force $dest (Join-Path $backup $name) }
  }

  foreach ($name in $artifactNames) {
    Move-Item -Force (Join-Path $stage $name) (Join-Path $final $name)
    $installed += $name
  }

  foreach ($item in $verified) {
    $destPreviewHash = (Get-FileHash -Algorithm SHA256 (Join-Path $final $item.PreviewName)).Hash.ToLowerInvariant()
    $destGlbHash = (Get-FileHash -Algorithm SHA256 (Join-Path $final $item.GlbName)).Hash.ToLowerInvariant()
    if ($destPreviewHash -ne $item.PreviewHash -or $destGlbHash -ne $item.GlbHash) {
      throw "Post-promotion hash mismatch: $($item.Piece)"
    }
  }

  Write-Host 'PROMOTED: exact 12 reviewed GLBs + 12 reviewed preview PNGs copied transactionally to chess\glb.'
  Write-Host 'No Blender rerun occurred and face.jpg/private source assets were not copied.'
  Write-Host 'Review git diff and update the PR. Tumbo remains the only merger.'
}
catch {
  foreach ($name in $installed) {
    $dest = Join-Path $final $name
    if (Test-Path $dest) { Remove-Item -Force $dest }
  }
  foreach ($name in $artifactNames) {
    $saved = Join-Path $backup $name
    if (Test-Path $saved) { Copy-Item -Force $saved (Join-Path $final $name) }
  }
  throw
}
finally {
  if (Test-Path $backup) { Remove-Item -Recurse -Force $backup }
  if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
}
