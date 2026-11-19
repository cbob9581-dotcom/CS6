param([Parameter(Mandatory=$true)][string]$Script,[Parameter(Mandatory=$true)][string]$Log,[switch]$TraceLoads)
$ErrorActionPreference='Stop'
$project='D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/UETacticalPack.uproject'
$arguments='"'+$project+'" -ExecutePythonScript="'+$Script+'" -nullrhi -unattended -nosplash -nop4 -abslog="'+$Log+'"'
if($TraceLoads){$arguments+=' -CS6TraceLoads'}
$editor=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
if($editor.ExitCode -ne 0){throw "Editor failed: $($editor.ExitCode)"}
if(Select-String -LiteralPath $Log -Pattern 'LogPython: Error:|Fatal error!' -Quiet){throw 'Editor script failed; inspect log.'}
$global:LASTEXITCODE=0
