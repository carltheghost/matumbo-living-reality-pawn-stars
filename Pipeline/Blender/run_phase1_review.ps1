$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path

$private = if ($env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT) {
  $env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT
} else {
  Join-Path $env:LOCALAPPDATA 'maTumbo\chess-review'
}
$env:TUMBO_CHESS_PRIVATE_REVIEW_ROOT = $private
New-Item -ItemType Directory -Force -Path $private | Out-Null

Write-Host '=== maTumbo Cinematic Chess Phase 1: private 12-piece review forge ==='
Write-Host "Review root: $private"

& (Join-Path $here 'preflight_phase1.ps1')
if ($LASTEXITCODE -ne 0) { throw "Phase 1 preflight failed: $LASTEXITCODE" }

& (Join-Path $here 'run_black.ps1')
if ($LASTEXITCODE -ne 0) { throw "Black-army forge failed: $LASTEXITCODE" }

& (Join-Path $here 'run_white.ps1')
if ($LASTEXITCODE -ne 0) { throw "White-army forge failed: $LASTEXITCODE" }

$pieces = @('king','queen','bishop','knight','rook','pawn')
$factions = @('black','white')
$records = @()

foreach ($side in $factions) {
  foreach ($piece in $pieces) {
    $slug = "$side-$piece"
    $preview = Join-Path $private "preview-$slug.png"
    $glb = Join-Path $private "$slug.glb"
    $qc = Join-Path $private "qc-$slug.json"

    foreach ($required in @($preview,$glb,$qc)) {
      if (-not (Test-Path $required)) {
        throw "Private review is incomplete. Missing: $required"
      }
    }

    $previewInfo = Get-Item $preview
    $glbInfo = Get-Item $glb
    if ($previewInfo.Length -lt 100000) {
      throw "Preview below review size floor: $preview"
    }
    if ($glbInfo.Length -lt 1000000) {
      throw "GLB below cinematic size floor: $glb"
    }

    $qcJson = Get-Content -Raw $qc | ConvertFrom-Json
    if ($qcJson.status -ne 'PASS' -or $qcJson.piece -ne $slug -or $qcJson.privacy -ne 'private-review') {
      throw "QC receipt failed or mismatched: $qc"
    }

    $previewHash = (Get-FileHash -Algorithm SHA256 $preview).Hash.ToLowerInvariant()
    $glbHash = (Get-FileHash -Algorithm SHA256 $glb).Hash.ToLowerInvariant()
    if ($previewHash -ne ([string]$qcJson.outputs.preview.sha256).ToLowerInvariant()) {
      throw "Preview/QC hash mismatch before review: $slug"
    }
    if ($glbHash -ne ([string]$qcJson.outputs.glb.sha256).ToLowerInvariant()) {
      throw "GLB/QC hash mismatch before review: $slug"
    }

    $records += [pscustomobject]@{
      piece = $slug
      preview = [IO.Path]::GetFileName($preview)
      previewBytes = $previewInfo.Length
      previewSha256 = $previewHash
      glb = [IO.Path]::GetFileName($glb)
      glbBytes = $glbInfo.Length
      glbSha256 = $glbHash
      qc = [IO.Path]::GetFileName($qc)
    }
  }
}

$reviewManifest = [ordered]@{
  schemaVersion = 1
  gate = 'TUMBO_VISUAL_APPROVAL_REQUIRED'
  generatedAt = (Get-Date).ToUniversalTime().ToString('o')
  count = $records.Count
  outputs = $records
}
$manifestPath = Join-Path $private 'phase1-review-manifest.json'
$reviewManifest | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $manifestPath

$cards = foreach ($r in $records) {
  $label = $r.piece.ToUpperInvariant()
  @"
<article class="card">
  <img src="$($r.preview)" alt="$label cinematic chess preview">
  <h2>$label</h2>
  <p>PNG $([math]::Round($r.previewBytes / 1MB, 2)) MB · GLB $([math]::Round($r.glbBytes / 1MB, 2)) MB</p>
  <p class="hash">PNG SHA256 $($r.previewSha256)</p>
</article>
"@
}

$html = @"
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>maTumbo Phase 1 — 12 Piece Private Review</title>
<style>
  :root { color-scheme: dark; font-family: Segoe UI, Arial, sans-serif; }
  body { margin: 24px; background: #08090c; color: #eee; }
  header { max-width: 1100px; margin: 0 auto 24px; }
  .warning { color: #f3c76b; font-weight: 700; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit,minmax(320px,1fr)); gap: 18px; }
  .card { background: #12141a; border: 1px solid #2c3038; padding: 12px; border-radius: 10px; }
  img { width: 100%; aspect-ratio: 1; object-fit: contain; background: #030407; border-radius: 6px; }
  h2 { margin: 10px 0 4px; font-size: 17px; letter-spacing: .08em; }
  p { margin: 4px 0; color: #bbb; }
  .hash { font: 10px Consolas, monospace; overflow-wrap: anywhere; color: #777; }
</style>
</head>
<body>
<header>
  <h1>maTumbo Cinematic Chess — Phase 1 Private Review</h1>
  <p class="warning">NOT APPROVED FOR DEMO OR PUBLICATION. Tumbo must visually approve all twelve.</p>
  <p>Black: obsidian + gold. White: ivory + gold. Review face likeness, silhouette law, centaur anatomy, castle-staff scale, filigree, cloth, weapons, and framing at full resolution.</p>
</header>
<main class="grid">
$($cards -join [Environment]::NewLine)
</main>
</body>
</html>
"@

$reviewPath = Join-Path $private 'phase1-review.html'
$html | Set-Content -Encoding UTF8 $reviewPath

Write-Host ''
Write-Host 'PASS: all 12 private GLBs, PNG previews, and QC receipts exist.'
Write-Host "Review manifest: $manifestPath"
Write-Host "Visual review board: $reviewPath"
Write-Host 'Publication remains CLOSED until Tumbo approves every piece.'
Write-Host "NEXT: open $reviewPath, inspect all 12 at full resolution, then run:"
Write-Host '  .\Pipeline\Blender\approve_review.ps1 -IApproveAll12'
