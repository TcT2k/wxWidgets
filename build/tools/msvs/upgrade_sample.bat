@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

set SNAME=%~n1
set SPATH=%~1
set BASE_SLN=%SNAME%_vc9.sln
set BASE_PROJ=%SPATH%\%SNAME%_vc9.vcproj
set INPUT_SLN=%SPATH%\%BASE_SLN%

if NOT EXIST %INPUT_SLN% (
	echo Error: Could not find %INPUT_SLN%; bakefiles not generated?
	exit /b 1
)

echo Upgrading sample: %SNAME% (%SPATH%)

rem Clean dir from potential previous run
if exist %SPATH%\Backup rmdir /s /q %SPATH%\Backup
del %SPATH%\*.vcxproj >nul 2>&1
del %SPATH%\*.vcxproj.filters >nul 2>&1

rem Upgrade SLN and VCproj
devenv %INPUT_SLN% /upgrade

rem Rename new project files
for %%f in (%SPATH%\*.vcxproj %SPATH%\*.vcxproj.filters) do (
	set FN=%%~nxf
	rename %SPATH%\!FN! !FN:_vc9=!
)

rem Recreate new solution with updated filename
set ORG_SLN=%SPATH%\%BASE_SLN%

call %~dp0\upgrade_convert_sln.bat 10 %ORG_SLN% %SPATH%\%SNAME%_vc10.sln
call %~dp0\upgrade_convert_sln.bat 11 %ORG_SLN% %SPATH%\%SNAME%_vc11.sln
call %~dp0\upgrade_convert_sln.bat 12 %ORG_SLN% %SPATH%\%SNAME%_vc12.sln
call %~dp0\upgrade_convert_sln.bat 14 %ORG_SLN% %SPATH%\%SNAME%_vc14.sln

rem Remove .SUO
del /a %SPATH%\%SNAME%_vc9*.suo>nul 2>&1

rem Restore original SLN
move %SPATH%\Backup\%BASE_SLN% %SPATH%\%BASE_SLN%>nul 2>&1
rmdir /S /Q %SPATH%\Backup

del %SPATH%\UpgradeLog.htm
