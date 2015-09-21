@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

set SAMPLESDIR=%~dp0..\..\..\samples

echo Samples dir: %SAMPLESDIR%

rem Enumerator all sample directories
for /R %SAMPLESDIR% %%s in (*_vc9.sln) do (
	set SAMPLEDIR=%%~dps
	call %~dp0\upgrade_sample.bat !SAMPLEDIR:~0,-1!
)
