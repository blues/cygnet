@echo off
setlocal enabledelayedexpansion

:: Get current date components
for /f "tokens=2 delims==" %%I in ('"wmic os get localdatetime /value"') do set datetime=%%I

:: Extract YYYY-MM-DD from WMIC format (yyyymmddHHMMSS)
set YYYY=!datetime:~0,4!
set MM=!datetime:~4,2!
set DD=!datetime:~6,2!

:: Construct the new filename
set PREFIXED_NAME=!YYYY!-!MM!-!DD!-cygnet.bin

:: Copy with new name
copy Exe\cygnet.bin "z:\blues Dropbox\Firmware\Cygnet\!PREFIXED_NAME!"
