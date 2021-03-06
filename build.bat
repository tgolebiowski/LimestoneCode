@echo off
SETLOCAL
SET Dir=Limestone_Framework

SET VSFlags=/W0 /Od /Zi /wd4305 /wd4244 /wd4530
SET VSIncludes=-I..\%Dir%\Src\Dependencies\include
SET Libs=user32.lib glu32.lib opengl32.lib gdi32.lib xinput9_1_0.lib

REM unless this was started with "-all" then only build and copy the dll
if NOT "%1" == "-all" GOTO BuildApp

REM Build exe and code generator, and run the code generator
pushd ..\build
	del *.exe
	del *.pdb
	del *.dll
	
	echo building code gen...
	call cl /W0 /Od /Zi /FeLimestoneMeta ..\%Dir%\CodeGeneration\Limestone_Preprocessor.cpp user32.lib /link
	REM echo running code gen...
	REM call LimestoneMeta ..\%Dir%\Src\App.cpp ..\%Dir%\

	echo building exe...
	call cl %VSFlags% /Fego %VSIncludes% ..\%Dir%\Src\WinMain.cpp %Libs% /link /Profile
popd
xcopy ..\build\go.exe go.exe /y /f

:BuildApp

REM build the dll
echo building dll...
pushd ..\build
	SET myTime=%TIME:~0,2%_%TIME:~3,2%_%TIME:~6,2%
	SET myTime=%myTime: =0%
	SET pdbName=App_%myTime%.pdb
	SET exportedFuncs=/EXPORT:MixSound /EXPORT:GameInit /EXPORT:UpdateAndRender

	call cl %VSFlags% /Fd%pdbName% %VSIncludes% /LD ..\%Dir%\Src\App.cpp /link /PDB:%pdbName% /incremental:no %exportedFuncs% /opt:noref
popd
xcopy ..\build\App.dll App.dll /y /f