#object files
OBJS = Src/NewMain.c

#CC specifies which compiler we're using 
CC = gcc

#INCLUDE_PATHS specifies the additional include paths we'll need 
#INCLUDE_PATHS = -IC:\mingw_dev_lib\include\SDL2 
INCLUDE_PATHS = -IDependencies\include\SDL -IDependencies\include\OpenGL -IDependencies\include\assimp

#LIBRARY_PATHS specifies the additional library paths we'll need 
#LIBRARY_PATHS = -LC:\mingw_dev_lib\lib 
LIBRARY_PATHS = -LDependencies\lib\SDL -LDependencies\lib\OpenGL  -LDependencies\lib\assimp
#-LDependencies\lib\assimp

#COMPILER_FLAGS specifies the additional compilation options we're using
COMPILER_FLAGS = -Wall -g -std=c99

#LINKER_FLAGS specifies the libraries we're linking against 
GLFLAGS = -lglu32 -lglew32 -lfreeglut_static -lopengl32
SDLFLAGS = -lSDL2main -lSDL2 $(sdl2-config --libs --cflags)
LINKER_FLAGS = -lmingw32 $(GLFLAGS) $(SDLFLAGS) -lassimpd -lwinmm -lgdi32

# Put this at the end of linker flags to supress console output: -Wl,--subsystem,windows

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = go
#This is the target that compiles our executable 
all : $(OBJS) 
	$(CC) $(OBJS) -D FREEGLUT_STATIC $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
	@echo Wet Clay is ready.

clean:
	$(RM) *.o *~