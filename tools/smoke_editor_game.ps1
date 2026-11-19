param([string]$Scenario='',[switch]$CaptureBuy)
$ErrorActionPreference='Stop'
$project='D:/CS6/Game/Ultimate Multiplayer FPS Kit UE5.4/UltimateMultiplayerFPSKit/UETacticalPack.uproject'
$log='D:/CS6/reports/cs2-editor-game.log'
$arguments='"'+$project+'" /Game/CS6/Maps/M_MilitaryBase -game -d3d11 -windowed -ResX=1280 -ResY=720 -CS6Smoke -CS6Capture -unattended -nosplash -abslog="'+$log+'" -LogCmds="LogSkinnedMeshComp Error"'
if($Scenario){$arguments+=' -'+$Scenario}
$process=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.4/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(300000)){Stop-Process -Id $process.Id;throw 'Owned uncooked game test timed out.'}
if($process.ExitCode -ne 0){throw "Uncooked game exited $($process.ExitCode)"}
if(-not (Select-String $log -Pattern 'CS6 SMOKE PASS' -Quiet)){throw 'Uncooked game did not pass.'}
if(Select-String $log -Pattern 'Accessed None|LogTemp: Error:|LogScript: Error:|Fatal error!' -Quiet){throw 'Runtime errors in uncooked game.'}
Select-String $log -Pattern 'CS6 enrolled|CS6 EQUIP|CS6 BUY|CS6 AUDIT|CS6 SMOKE'
