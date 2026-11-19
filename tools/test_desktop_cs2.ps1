param([ValidateSet('','CS6ShopScenario','CS6ControlScenario','CS6DefuseScenario')][string]$Scenario='',[switch]$Headless)
$ErrorActionPreference='Stop'
$name=if($Scenario){$Scenario}else{'default-entry'}
$log="D:/CS6/reports/cs2-desktop-$name.log"
$started=Get-Date
$arguments='-CS6Smoke -unattended -nosplash -abslog="'+$log+'" -LogCmds="LogSkinnedMeshComp Error"'
if($Headless){$arguments+=' -nullrhi'}else{$arguments+=' -d3d11 -windowed -ResX=1280 -ResY=720 -CS6Capture'}
if($Scenario){$arguments+=' -'+$Scenario}
$process=Start-Process -FilePath 'D:/CS6/Build/Desktop/Windows/UETacticalPack.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(180000)){Stop-Process -Id $process.Id;throw 'Owned desktop test timed out.'}
if($process.ExitCode -ne 0){throw "Desktop exited $($process.ExitCode)"}
if(-not (Test-Path -LiteralPath $log) -or (Get-Item -LiteralPath $log).LastWriteTime -lt $started){throw 'No fresh desktop runtime log.'}
if(-not (Select-String $log -Pattern 'CS6 SMOKE PASS' -Quiet)){throw "Desktop did not pass: $log"}
if(Select-String $log -Pattern 'Accessed None|LogTemp: Error:|LogScript: Error:|Fatal error!' -Quiet){throw 'Desktop runtime error.'}
& D:/Python310/python.exe D:/CS6/tools/validate_spawn_recovery.py $log
if($LASTEXITCODE){throw 'Standing placement check failed.'}
if($Scenario -eq 'CS6ControlScenario'){
    & D:/Python310/python.exe D:/CS6/tools/validate_takeover_scenario.py $log
    if($LASTEXITCODE){throw 'Takeover validation failed.'}
}elseif($Scenario -eq 'CS6ShopScenario'){
    & D:/Python310/python.exe D:/CS6/tools/validate_weapon_presentation.py $log
    if($LASTEXITCODE){throw 'Supplied gun presentation validation failed.'}
    foreach($marker in @('CS6 SHOP supplied_model_reload=1','CS6 SHOP drop=1 pickup_kept_ammo=1 dropped_refund_rejected=1')){if(-not(Select-String -LiteralPath $log -Pattern $marker -SimpleMatch -Quiet)){throw "Missing $marker"}}
}elseif($Scenario -eq 'CS6DefuseScenario'){
    foreach($marker in @('CS6 OBJECTIVE plant=1 reward=300','CS6 OBJECTIVE plant=0 reward=300','CS6 SCENARIO resolved defuse=1')){if(-not(Select-String -LiteralPath $log -Pattern $marker -SimpleMatch -Quiet)){throw "Missing $marker"}}
}else{
    & D:/Python310/python.exe D:/CS6/tools/validate_bot_activity.py $log
    if($LASTEXITCODE){throw 'Individual Bot validation failed.'}
}
Write-Output "PASS desktop $name : $log"
