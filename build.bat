@ECHO OFF

rem ================================
rem       CONFIGURATION BLOCK
rem ================================

set BRANCHNAME=enhanced
set BRANCHNAMEPROPER=Enhanced

set ASSETSFOLDER=ojp%BRANCHNAME%
set PK3ASSETS=ojp_%BRANCHNAME%stuff
set PK3DLL=ojp_%BRANCHNAME%dlls

set ZIP="C:\Program Files\7-Zip\7z.exe"

set RESOURCES=resources
set OUTPUTFOLDER=build

rem ================================
rem        SCRIPT START
rem ================================

ECHO.
ECHO Cleaning old build folder...
IF NOT EXIST %OUTPUTFOLDER% mkdir %OUTPUTFOLDER%
IF EXIST %OUTPUTFOLDER%\%PK3ASSETS%.pk3 DEL %OUTPUTFOLDER%\%PK3ASSETS%.pk3
IF EXIST %OUTPUTFOLDER%\%PK3DLL%.pk3 DEL %OUTPUTFOLDER%\%PK3DLL%.pk3

ECHO.
ECHO Creating PK3s...

rem Создаём pk3 с ассетами
%ZIP% a -tzip %OUTPUTFOLDER%\%PK3ASSETS%.pk3 .\%ASSETSFOLDER%\* -xr!.screenshots\ -xr!.svn\ -x!*.dll -x!*.so -x!.\%ASSETSFOLDER%\*.* -xr!*.nav -mx9
IF ERRORLEVEL 1 GOTO ERROR

rem Создаём pk3 с dll
%ZIP% a -tzip %OUTPUTFOLDER%\%PK3DLL%.pk3 .\%ASSETSFOLDER%\*.dll -mx9
IF ERRORLEVEL 1 GOTO ERROR

ECHO.
ECHO Copying resources to build folder...

xcopy /E /Y /I %RESOURCES%\* %OUTPUTFOLDER%\

ECHO.
ECHO ==========================
ECHO     BUILD COMPLETE
ECHO ==========================
GOTO END

:ERROR
ECHO.
ECHO ================================================
ECHO Error while building PK3s or copying resources!
ECHO ================================================
GOTO END

:END