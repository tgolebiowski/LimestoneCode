@echo off
SETLOCAL
SET VisualStudio=cl
SET VSFlags=/w /Od /Zi /Fego.exe
SET VSIncludes=-I..\WetClay_Library\Dependencies\include
SET VSLinkerFlags=/link /Profile

SET GlobalLibs=user32.lib glu32.lib opengl32.lib gdi32.lib xinput9_1_0.lib
SET LocalLibsInclude=-LIBPATH:..\WetClay_Library\Dependencies\lib\OpenGL\
SET LocalLibs=glew32s.lib

pushd ..\build
	REM call cl /w /Od /Zi /Fego.exe -I..\WetClay_Library\Dependencies\include ..\WetClay_Library\Config.cpp user32.lib glu32.lib opengl32.lib gdi32.lib xinput9_1_0.lib /link /Profile -LIBPATH:..\WetClay_Library\Dependencies\lib\OpenGL\ glew32s.lib
	call %VisualStudio% %VSFlags% %VSIncludes% ..\WetClay_Library\Config.cpp %GlobalLibs% %VSLinkerFlags% -LIBPATH:..\WetClay_Library\Dependencies\lib\OpenGL\ %LocalLibs%
popd
xcopy ..\build\go.exe go.exe /y /f