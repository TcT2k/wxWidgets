@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

set SNAME=%~n1
set SPATH=%~1
set BASE_SLN=%SNAME%_vc9.sln
set BASE_PROJ=%SPATH%\%SNAME%_vc9.vcproj
set NEW_PROJ=%SPATH%\%SNAME%.vcxproj

if NOT EXIST %BASE_PROJ% (
	echo Error: Could not find %BASE_PROJ%
	exit /b 1
)

if EXIST %NEW_PROJ% (
	echo Error: %NEW_PROJ% already updated
	exit /b 1
)

echo Upgrading sample: %SNAME% (%SPATH%)

rem Remove potential Backup dir
if exist %SPATH%\Backup rmdir /s /q %SPATH%\Backup

rem Upgrade SLN and VCproj
devenv %BASE_PROJ% /upgrade

rem Rename new project files
rename %SPATH%\%SNAME%_vc9.vcxproj %SNAME%.vcxproj
rename %SPATH%\%SNAME%_vc9.vcxproj.filters %SNAME%.vcxproj.filters

rem Recreate new solution with updated filename
set ORG_SLN=%SPATH%\%BASE_SLN%

call %~dp0\upgrade_convert_sln.bat 10 %ORG_SLN% %SPATH%\%SNAME%_vc10.sln
call %~dp0\upgrade_convert_sln.bat 11 %ORG_SLN% %SPATH%\%SNAME%_vc11.sln
call %~dp0\upgrade_convert_sln.bat 12 %ORG_SLN% %SPATH%\%SNAME%_vc12.sln
call %~dp0\upgrade_convert_sln.bat 14 %ORG_SLN% %SPATH%\%SNAME%_vc14.sln

rem Remove .SUO
del /a %SPATH%\%SNAME%_vc9*.suo

rem Restore original SLN
move %SPATH%\Backup\%BASE_SLN% %SPATH%\%BASE_SLN%
rmdir /S /Q %SPATH%\Backup

del %SPATH%\UpgradeLog.htm
