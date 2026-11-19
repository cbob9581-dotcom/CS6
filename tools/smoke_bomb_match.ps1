param([switch]$Render,[switch]$DefuseScenario,[switch]$WeaponScenario,[switch]$DeathScenario,[switch]$UseLauncher)
$ErrorActionPreference='Stop'
if(([int]$DefuseScenario.IsPresent+[int]$WeaponScenario.IsPresent+[int]$DeathScenario.IsPresent) -gt 1){throw 'Choose one controlled scenario at a time.'}
$exe='D:/CS6/Build/Desktop/Windows/UETacticalPack/Binaries/Win64/UETacticalPack.exe'
if($UseLauncher){$exe='D:/CS6/Build/Desktop/Windows/UETacticalPack.exe'}
$name=if($DeathScenario){'bomb-runtime-death'}elseif($WeaponScenario){'bomb-runtime-weapons'}elseif($DefuseScenario){'bomb-runtime-defuse'}elseif($Render){'bomb-runtime-render'}else{'bomb-runtime-headless'}
$log='D:/CS6/reports/'+$name+'.log'
$arguments='/Game/CS6/Maps/M_MilitaryBase -CS6Smoke -unattended -nosplash -abslog="'+$log+'"'
if($Render){$arguments+=' -d3d11 -windowed -ResX=1280 -ResY=720 -CS6Capture'}else{$arguments+=' -nullrhi -nosound'}
if($DefuseScenario){$arguments+=' -CS6DefuseScenario'}
if($WeaponScenario){$arguments+=' -CS6WeaponScenario'}
if($DeathScenario){$arguments+=' -CS6DeathScenario'}
$process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(180000)) {
    Stop-Process -Id $process.Id
    throw 'Runtime smoke timed out; owned test process stopped.'
}
if($process.ExitCode -ne 0){throw "Runtime smoke exited $($process.ExitCode). Inspect $log"}
if(-not (Select-String -LiteralPath $log -Pattern 'CS6 SMOKE PASS' -Quiet)){throw 'Runtime did not emit the smoke completion marker.'}
& 'D:/Python310/python.exe' 'D:/CS6/tools/validate_runtime_positions.py' $log
if($LASTEXITCODE -ne 0){throw 'Runtime ground-height validation failed.'}
if($WeaponScenario){
    & 'D:/Python310/python.exe' 'D:/CS6/tools/validate_weapon_actions.py' $log
    if($LASTEXITCODE -ne 0){throw 'Player weapon action validation failed.'}
}
if($DeathScenario){
    & 'D:/Python310/python.exe' 'D:/CS6/tools/validate_death_recovery.py' $log
    if($LASTEXITCODE -ne 0){throw 'Death and recovery validation failed.'}
}
if(-not $DefuseScenario -and -not $WeaponScenario -and -not $DeathScenario){
    & 'D:/Python310/python.exe' 'D:/CS6/tools/validate_bot_activity.py' $log
    if($LASTEXITCODE -ne 0){throw 'Autonomous bot movement validation failed.'}
}
$scriptErrors=Select-String -LiteralPath $log -Pattern 'LogScript: Warning: Script Msg:|LogScript: Warning: Accessed None|LogBlueprintUserMessages:.*Error|LogScript: Error:'
$knownDuplicateHud=$scriptErrors | Where-Object {$_.Line -match "LogScript: Warning: Script Msg: The widget 'WB_KillerInfo_C_\d+' was already added to the screen\."}
if($knownDuplicateHud){Write-Warning "Existing kit KillerInfo duplicate-add warnings: $(@($knownDuplicateHud).Count); functional death checks remain mandatory."}
$scriptErrors=$scriptErrors | Where-Object {$_.Line -notmatch "LogScript: Warning: Script Msg: The widget 'WB_KillerInfo_C_\d+' was already added to the screen\."}
if($scriptErrors){$scriptErrors | Select-Object -First 10;throw 'Template script errors occurred during smoke.'}
Select-String -LiteralPath $log -Pattern 'CS6 template boot|CS6 AUDIT|CS6 SCENARIO|CS6 SMOKE'
