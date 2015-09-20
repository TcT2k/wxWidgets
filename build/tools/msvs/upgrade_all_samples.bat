@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

set SAMPLESDIR=%~dp0..\..\..\samples

echo Samples dir: %SAMPLESDIR%

cd /d %SAMPLESDIR%

rem Enumerator all sample directories

for /D %%s in (*.*) do (
	if exist %%s\%%s_vc9.vcproj (
		call %~dp0\upgrade_sample.bat %%~fs
	)
)
