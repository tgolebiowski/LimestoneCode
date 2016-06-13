@echo off
SETLOCAL
SET VisualStudio=cl
SET VSFlags=/W1 /Od /Zi /Fego.exe
SET VSIncludes=-I..\Limestone_Framework\Src\Dependencies\include
SET VSLinkerFlags=/link /Profile

SET GlobalLibs=user32.lib glu32.lib opengl32.lib gdi32.lib xinput9_1_0.lib
SET LocalLibsInclude=-LIBPATH:..\Limestone_Framework\Src\Dependencies\lib\OpenGL\
SET LocalLibs=glew32s.lib

pushd ..\build
	call %VisualStudio% %VSFlags% %VSIncludes% ..\Limestone_Framework\Config.cpp %GlobalLibs% %VSLinkerFlags% -LIBPATH:..\Limestone_Framework\Src\Dependencies\lib\OpenGL\ %LocalLibs%
popd
xcopy ..\build\go.exe go.exe /y /f