@echo off
pushd ..\build
call cl /w /Od /Zi /Fego.exe -I..\Dependencies\include ..\Src\WinMain.cpp user32.lib glu32.lib opengl32.lib gdi32.lib xinput9_1_0.lib /link /Profile -LIBPATH:..\Dependencies\lib\OpenGL\ glew32s.lib
popd
xcopy ..\build\go.exe go.exe /y /f
xcopy ..\build\go.pdb go.pdb /y /f
xcopy ..\build\vc120.pdb vc120.pdb /y