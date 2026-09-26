param([ValidateSet('all')][string]$Faction='all')
$ErrorActionPreference='Stop'

$here=Split-Path -Parent $MyInvocation.MyCommand.Path
$promoter=Join-Path $here 'promote_approved.ps1'
if(-not (Test-Path $promoter)){ throw "Missing promoter: $promoter" }

# Compatibility entrypoint. Publication never re-runs Blender: it promotes
# only the exact hash-locked private artifacts Tumbo already reviewed.
& $promoter -Faction $Faction
