@echo off
pushd build
call cl /w /Od /Zi /Fego.exe -I..\Dependencies\include ..\Src\WinMain.cpp user32.lib glu32.lib opengl32.lib gdi32.lib /link -LIBPATH:..\Dependencies\lib\OpenGL\ -LIBPATH:..\Dependencies\lib\assimp\ glew32s.lib libassimp.lib
popd
move build\go.exe go.exe
REM clang++ --analyze -IDependencies/include Src\WinMain.cpp