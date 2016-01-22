#object files
OBJS = Src/main.c
IMP = Src/ImporterMain.c

#CC specifies which compiler we're using 
CC = gcc

#INCLUDE_PATHS specifies the additional include paths we'll need 
#INCLUDE_PATHS = -IC:\mingw_dev_lib\include\SDL2 
INCLUDE_PATHS = -IDependencies\include 
#\assimp -IDependencies\include\SDL -IDependencies\include\OpenGL

#LIBRARY_PATHS specifies the additional library paths we'll need 
#LIBRARY_PATHS = -LC:\mingw_dev_lib\lib 
LIBRARY_PATHS = -LDependencies\lib\SDL -LDependencies\lib\OpenGL -LDependencies\lib\assimp

#COMPILER_FLAGS specifies the additional compilation options we're using
COMPILER_FLAGS = -Wall -g -std=c99

#LINKER_FLAGS specifies the libraries we're linking against 
GLFLAGS = -lglu32 -lglew32 -lfreeglut_static -lopengl32
SDLFLAGS = -lSDL2main -lSDL2 $(sdl2-config --libs --cflags)
ASSIMPFLAG = -lassimpd
LINKER_FLAGS = -lmingw32 $(GLFLAGS) $(SDLFLAGS) $(ASSIMPFLAG) -lwinmm -lgdi32
IMPORTER_FLAGS = -lmingw32 $(GLFLAGS) $(SDLFLAGS) $(ASSIMPFLAG) -lwinmm -lgdi32
# Put this at the end of linker flags to supress console output: -Wl,--subsystem,windows

#OBJ_NAME specifies the name of our exectuable
OBJ_NAME = go
IMP_NAME = importer

#This is the target that compiles our executable 
all : $(OBJS) 
	$(CC) $(OBJS) -D FREEGLUT_STATIC $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
	@echo Wet Clay is ready.

importer :$(IMP)
	$(CC) $(IMP) -D FREEGLUT_STATIC $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(IMPORTER_FLAGS) -o $(IMP_NAME)
	@echo Clay importer is ready

clean:
	$(RM) *.o *~