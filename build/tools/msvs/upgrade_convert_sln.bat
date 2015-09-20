@echo off
setlocal enableextensions
setlocal enabledelayedexpansion

rem Syntax upgrade_convert_sln (10|11|12|14) <input_vc.sln> <output.sln>
rem input file has to be VS10+

rem Write header first
if %1==10 (
	echo Microsoft Visual Studio Solution File, Format Version 11.00>%3
	echo # Visual Studio 2010>>%3
)
if %1==11 (
	echo Microsoft Visual Studio Solution File, Format Version 12.00>%3
	echo # Visual Studio 2012>>%3
)
if %1==12 (
	echo Microsoft Visual Studio Solution File, Format Version 12.00>%3
	echo # Visual Studio 2013>>%3
	echo VisualStudioVersion = 12.0>>%3
	echo MinimumVisualStudioVersion = 10.0.40219.1>>%3
)
if %1==14 (
	echo Microsoft Visual Studio Solution File, Format Version 12.00>%3
	echo # Visual Studio 14>>%3
	echo VisualStudioVersion = 14.0.23107.0>>%3
	echo MinimumVisualStudioVersion = 10.0.40219.1>>%3
)

for /f "skip=2 delims=" %%t in (%2) do (
	set LINE=%%t
	if NOT "!LINE:~0,21!"=="VisualStudioVersion =" (
		if NOT "!LINE:~0,28!"=="MinimumVisualStudioVersion =" (
			echo !LINE:_vc9=!>> %3
		)
	)
)
