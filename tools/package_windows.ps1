param([ValidateSet('Development','Shipping')][string]$Configuration='Development',[switch]$Iterative)
$ErrorActionPreference='Stop'
$workspace=Split-Path $PSScriptRoot -Parent
& (Join-Path $PSScriptRoot 'sync_runtime_sources.ps1')
$project=Join-Path $workspace 'Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/UETacticalPack.uproject'
$report=Join-Path $workspace 'reports/military-gameplay-setup.json'
if(-not (Test-Path -LiteralPath $report)){throw 'Military gameplay setup has not completed.'}
$setup=Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
if(-not $setup.navigation_built){throw 'Navigation has not passed its build check.'}
$output=Join-Path $workspace 'Build/Desktop'
$extraCook=@()
if($Iterative){$extraCook+='-iterate'}
& 'C:/Program Files/Epic Games/UE_5.4/Engine/Build/BatchFiles/RunUAT.bat' BuildCookRun "-project=$project" -nop4 -platform=Win64 "-clientconfig=$Configuration" -build -cook -stage -pak -archive "-archivedirectory=$output" '-map=/Game/CS6/Maps/M_MilitaryBase' -utf8output @extraCook
if($LASTEXITCODE -ne 0){throw "Desktop package failed: $LASTEXITCODE"}
