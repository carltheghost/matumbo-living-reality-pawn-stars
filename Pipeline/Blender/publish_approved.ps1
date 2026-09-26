param([ValidateSet('all')][string]$Faction='all')
$ErrorActionPreference = 'Stop'

if ($env:TUMBO_FACE_PUBLICATION_APPROVED -ne 'YES') {
  throw 'Publication gate closed. Set TUMBO_FACE_PUBLICATION_APPROVED=YES only after Tumbo explicitly approves all 12 private previews.'
}

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
& (Join-Path $here 'preflight_phase1.ps1')
$repo = Resolve-Path (Join-Path $here '..\..')
$private = if ($env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT) {
  $env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT
} else {
  Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review'
}

$reviewManifestPath = Join-Path $private 'phase1-review-manifest.json'
if (-not (Test-Path $reviewManifestPath)) {
  throw "Missing private review manifest: $reviewManifestPath. Run run_phase1_review.ps1 first."
}

$review = Get-Content -Raw $reviewManifestPath | ConvertFrom-Json
if ($review.schemaVersion -ne 1 -or $review.gate -ne 'TUMBO_VISUAL_APPROVAL_REQUIRED') {
  throw 'Private review manifest is not a recognized Phase 1 review manifest.'
}

$pieces = @('king','queen','bishop','knight','rook','pawn')
$factions = @('black','white')
$slugs = @()
foreach ($side in $factions) {
  foreach ($piece in $pieces) {
    $slugs += "$side-$piece"
  }
}

if (@($review.outputs).Count -ne 12) {
  throw "Review manifest must contain exactly 12 outputs; found $(@($review.outputs).Count)."
}

$verified = @()
foreach ($slug in $slugs) {
  $rows = @($review.outputs | Where-Object { $_.piece -eq $slug })
  if ($rows.Count -ne 1) {
    throw "Review manifest must contain exactly one record for $slug; found $($rows.Count)."
  }
  $row = $rows[0]

  $expectedPreview = "preview-$slug.png"
  $expectedGlb = "$slug.glb"
  if ($row.preview -ne $expectedPreview -or $row.glb -ne $expectedGlb) {
    throw "$slug review manifest contains an unexpected artifact name. Path traversal and renamed artifacts are forbidden."
  }

  $preview = Join-Path $private $expectedPreview
  $glb = Join-Path $private $expectedGlb
  foreach ($required in @($preview,$glb)) {
    if (-not (Test-Path $required)) {
      throw "Approved artifact missing from private review set: $required"
    }
  }

  $previewInfo = Get-Item $preview
  $glbInfo = Get-Item $glb
  $previewHash = (Get-FileHash -Algorithm SHA256 $preview).Hash.ToLowerInvariant()
  $glbHash = (Get-FileHash -Algorithm SHA256 $glb).Hash.ToLowerInvariant()

  if ($previewHash -ne ([string]$row.previewSha256).ToLowerInvariant()) {
    throw "$slug PNG changed after review. Re-run review; publication is blocked."
  }
  if ($glbHash -ne ([string]$row.glbSha256).ToLowerInvariant()) {
    throw "$slug GLB changed after review. Re-run review; publication is blocked."
  }
  if ($previewInfo.Length -ne [int64]$row.previewBytes) {
    throw "$slug PNG byte size changed after review."
  }
  if ($glbInfo.Length -ne [int64]$row.glbBytes) {
    throw "$slug GLB byte size changed after review."
  }

  $verified += [pscustomobject]@{
    piece = $slug
    preview = $expectedPreview
    previewBytes = $previewInfo.Length
    previewSha256 = $previewHash
    glb = $expectedGlb
    glbBytes = $glbInfo.Length
    glbSha256 = $glbHash
  }
}

$final = Join-Path $repo 'chess\glb'
$stage = Join-Path $repo 'chess\.phase1-approved-staging'
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null

foreach ($row in $verified) {
  Copy-Item -Force (Join-Path $private $row.preview) (Join-Path $stage $row.preview)
  Copy-Item -Force (Join-Path $private $row.glb) (Join-Path $stage $row.glb)
}

$approvalManifest = [ordered]@{
  schemaVersion = 1
  approval = 'TUMBO_EXPLICIT_VISUAL_APPROVAL'
  promotedAt = (Get-Date).ToUniversalTime().ToString('o')
  sourceReviewManifestSha256 = (Get-FileHash -Algorithm SHA256 $reviewManifestPath).Hash.ToLowerInvariant()
  count = 12
  artifacts = $verified
}
$approvedManifestName = 'phase1-approved-manifest.json'
$approvalManifest | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 (Join-Path $stage $approvedManifestName)

$artifactNames = @($approvedManifestName)
foreach ($row in $verified) {
  $artifactNames += $row.preview
  $artifactNames += $row.glb
}

$missing = @($artifactNames | Where-Object { -not (Test-Path (Join-Path $stage $_)) })
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
    if (Test-Path $dest) {
      Copy-Item -Force $dest (Join-Path $backup $name)
    }
  }

  foreach ($name in $artifactNames) {
    Move-Item -Force (Join-Path $stage $name) (Join-Path $final $name)
    $installed += $name
  }

  Write-Host 'PASS: exact reviewed 12-piece artifact set promoted into chess\glb.'
  Write-Host 'No Blender rerender occurred during publication; hashes match the private review manifest.'
  Write-Host 'Review git diff and open/update the PR. Tumbo remains the only merger.'
}
catch {
  foreach ($name in $installed) {
    $dest = Join-Path $final $name
    if (Test-Path $dest) { Remove-Item -Force $dest }
  }
  foreach ($name in $artifactNames) {
    $saved = Join-Path $backup $name
    if (Test-Path $saved) {
      Copy-Item -Force $saved (Join-Path $final $name)
    }
  }
  throw
}
finally {
  if (Test-Path $backup) { Remove-Item -Recurse -Force $backup }
  if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
}
