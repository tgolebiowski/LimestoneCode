@echo off
SETLOCAL

SET Dir=Limestone_Framework

SET VSFlags=/W0 /Od /Zi /wd4305 /wd4244 /wd4530
SET VSIncludes=-I..\%Dir%\Src\Dependencies\include
SET Libs=user32.lib glu32.lib opengl32.lib gdi32.lib xinput9_1_0.lib

pushd ..\build
	del *.pdb
	del *.exe

	call cl /W0 /Od /Zi /FeLimestoneMeta ..\%Dir%\CodeGeneration\Limestone_Preprocessor.cpp user32.lib /link
	REM call LimestoneMeta ..\%Dir%\Src\App.cpp ..\%Dir%\

	SET myTime=%TIME:~0,2%_%TIME:~3,2%_%TIME:~6,2%
	SET myTime=%myTime: =0%
	SET pdbName=App_%myTime%.pdb
	SET exportedFuncs=/EXPORT:MixSound /EXPORT:GameInit /EXPORT:UpdateAndRender

	call cl %VSFlags% /Fd%pdbName% %VSIncludes% /LD ..\%Dir%\Src\App.cpp /link /PDB:%pdbName% /incremental:no %exportedFuncs% /opt:noref
	call cl %VSFlags% /Fego %VSIncludes% ..\%Dir%\Src\WinMain.cpp %Libs% /link /Profile
popd
xcopy ..\build\go.exe go.exe /y /f
xcopy ..\build\App.dll App.dll /y /f