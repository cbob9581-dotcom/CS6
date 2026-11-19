@echo off
setlocal
cd /d "%~dp0"
if not exist "%~dp0PendingUpdate\UETacticalPack.exe" goto launch
copy /b /y "%~dp0PendingUpdate\UETacticalPack.exe" "%~dp0Windows\UETacticalPack\Binaries\Win64\UETacticalPack.exe" >nul
if errorlevel 1 goto update_failed
del /q "%~dp0PendingUpdate\UETacticalPack.exe"
if not exist "%~dp0PendingUpdate\UETacticalPack.pdb" goto launch
copy /b /y "%~dp0PendingUpdate\UETacticalPack.pdb" "%~dp0Windows\UETacticalPack\Binaries\Win64\UETacticalPack.pdb" >nul
if errorlevel 1 goto launch
del /q "%~dp0PendingUpdate\UETacticalPack.pdb"
:launch
if not exist "%~dp0Windows\UETacticalPack.exe" goto missing_game
start "" "%~dp0Windows\UETacticalPack.exe" /Game/CS6/Maps/M_MilitaryBase -d3d11 -windowed -ResX=1600 -ResY=900 -CS6ManualLoadout
exit /b 0
:update_failed
echo Please close the running game, then run this launcher again.
pause
exit /b 1
:missing_game
echo Game executable missing: Windows\UETacticalPack.exe
pause
exit /b 1
