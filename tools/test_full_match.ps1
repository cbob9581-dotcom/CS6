$ErrorActionPreference='Stop'
$exe='D:/CS6/Build/Desktop/Windows/UETacticalPack/Binaries/Win64/UETacticalPack.exe'
$log='D:/CS6/reports/bomb-runtime-full-match.log'
# Fixed 60Hz world simulation runs without rendering; no forced scoring or damage.
# The local player remains idle. This exercises Bots and the actual round lifecycle.
$arguments='/Game/CS6/Maps/M_MilitaryBase -CS6FullMatch -nullrhi -unattended -nosplash -abslog="'+$log+'" -LogCmds="LogSkinnedMeshComp Error"'
$process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(600000)) {Stop-Process -Id $process.Id;throw 'Owned full-match test timed out.'}
if($process.ExitCode -ne 0){throw "Full-match test exited $($process.ExitCode)"}
& D:/Python310/python.exe D:/CS6/tools/validate_full_match.py $log
if($LASTEXITCODE -ne 0){throw 'Full-match validation failed.'}
Select-String $log -Pattern 'CS6 round ended|CS6 FULL MATCH'
