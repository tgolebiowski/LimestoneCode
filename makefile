#object files
OBJS = Src/WinMain.cpp

#CC specifies which compiler we're using 
#CC = "C:\Program Files\LLVM\msbuild-bin\cl.exe" figure how to make clang work here
CC = gcc

#INCLUDE_PATHS specifies the additional include paths we'll need 
#INCLUDE_PATHS = -IC:\mingw_dev_lib\include\SDL2 
INCLUDE_PATHS = -IDependencies/include

#LIBRARY_PATHS specifies the additional library paths we'll need 
#LIBRARY_PATHS = -LC:\mingw_dev_lib\lib 
LIBRARY_PATHS = -LDependencies/lib/OpenGL  -LDependencies/lib/assimp

#COMPILER_FLAGS specifies the additional compilation options we're using
COMPILER_FLAGS = -Wall -ggdb -std=c99

#SDLFLAGS = -lSDL2main -lSDL2 $(sdl2-config --libs --cflags)
#LINKER_FLAGS specifies the libraries we're linking against 
GLFLAGS = -lglu32 -lglew32 -lopengl32
LIBRARIES = $(GLFLAGS) -lassimpd -lwinmm -lgdi32
# Put this at the end of linker flags to supress console output: -Wl,--subsystem,windows

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = go.exe

#This is the target that compiles our executable 
withGCC : $(OBJS) 
	gcc -Wall -ggdb -std=c99 $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(OBJS) $(LIBRARIES) -lmingw32 -o $(OBJ_NAME)
	@echo Wet Clay is ready.

#Saving this here, from m$ documentation website
#CL [option...] file... [option | file]... [lib...] [@command-file] [/link link-opt...]
#---option: one or more compiler options
#---file: The name of one or more source files, .obj files, or libraries. CL compiles source 
#files and passes the names of the .obj files and libraries to the linker.
#---lib: One or more library names.
#---command-file: A file that contains multiple options and filenames.
#---link-opt: One or more linker options. CL passes these options to the linker.

CLANG_CL_LIBRARIES = user32.lib glu32.lib glew32s.lib opengl32.lib libassimp.lib gdi32.lib
CLANG_CL_LINKER = /link -DEBUG -LIBPATH:Dependencies/lib/OpenGL/ -LIBPATH:Dependencies/lib/assimp/

withClang : $(OBJS)
	clang-cl -Wall -m32 $(INCLUDE_PATHS) $(OBJS) -o $(OBJ_NAME) $(CLANG_CL_LINKER) $(CLANG_CL_LIBRARIES)
	@echo Code clay is ready.

clean:
	$(RM) *.o *~