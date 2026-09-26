$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$repo = (Resolve-Path (Join-Path $here '..\..')).Path
$manifestPath = Join-Path $here 'forge_manifest.json'

function Test-PathUnder([string]$Child, [string]$Parent) {
  $childFull = [IO.Path]::GetFullPath($Child).TrimEnd('\','/')
  $parentFull = [IO.Path]::GetFullPath($Parent).TrimEnd('\','/')
  return $childFull.StartsWith($parentFull + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
         $childFull.Equals($parentFull, [StringComparison]::OrdinalIgnoreCase)
}

if (-not $env:BLENDER_EXE) {
  $candidates = @(
    'C:\Program Files\Blender Foundation\Blender 4.5\blender.exe',
    'C:\Program Files\Blender Foundation\Blender 4.4\blender.exe',
    'C:\Program Files\Blender Foundation\Blender 4.3\blender.exe',
    'C:\Program Files\Blender Foundation\Blender 4.2\blender.exe'
  )
  $env:BLENDER_EXE = $candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $env:BLENDER_EXE -or -not (Test-Path $env:BLENDER_EXE)) {
  throw 'Phase 1 preflight: Blender 4.2+ not found. Set BLENDER_EXE.'
}
if (-not $env:TUMBO_CHESS_SOURCE_ROOT -or -not (Test-Path $env:TUMBO_CHESS_SOURCE_ROOT -PathType Container)) {
  throw 'Phase 1 preflight: set TUMBO_CHESS_SOURCE_ROOT to the external cinematic source pack.'
}
if (-not $env:TUMBO_FACE_IMAGE -or -not (Test-Path $env:TUMBO_FACE_IMAGE -PathType Leaf)) {
  throw 'Phase 1 preflight: set TUMBO_FACE_IMAGE to the private local face.jpg reference.'
}

$sourceRoot = (Resolve-Path $env:TUMBO_CHESS_SOURCE_ROOT).Path
$faceImage = (Resolve-Path $env:TUMBO_FACE_IMAGE).Path
if (Test-PathUnder $sourceRoot $repo) {
  throw 'Phase 1 preflight: TUMBO_CHESS_SOURCE_ROOT must stay outside the Git repository because it contains the private MetaHuman likeness asset.'
}
if (Test-PathUnder $faceImage $repo) {
  throw 'Phase 1 preflight: TUMBO_FACE_IMAGE must stay outside the Git repository.'
}

$manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
$required = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)

foreach ($pieceProp in $manifest.pieces.PSObject.Properties) {
  $piece = $pieceProp.Value
  foreach ($rel in $piece.sources) { [void]$required.Add([string]$rel) }
  if (-not $piece.rigSource -or -not ($piece.sources -contains $piece.rigSource)) {
    throw "Phase 1 preflight: $($pieceProp.Name) rigSource is missing or not in sources."
  }
}
foreach ($faction in @('black','white')) {
  foreach ($surface in @('armor','gold','cloth')) {
    $spec = $manifest.materials.$faction.$surface
    foreach ($texProp in $spec.textures.PSObject.Properties) {
      [void]$required.Add([string]$texProp.Value)
    }
  }
}

$missing = @()
foreach ($rel in $required) {
  $candidate = Join-Path $sourceRoot $rel
  if (-not (Test-Path $candidate -PathType Leaf)) { $missing += $rel }
}
if ($missing.Count -gt 0) {
  $pretty = ($missing | Sort-Object) -join [Environment]::NewLine
  throw "Phase 1 source pack is incomplete. Missing files:$([Environment]::NewLine)$pretty"
}

Write-Host "PASS: Phase 1 preflight found Blender, private likeness reference, and $($required.Count) required source/texture files."
Write-Host 'PASS: private source pack and face.jpg are outside the repository.'
