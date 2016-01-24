#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "SDL.h"
#define GLEW_STATIC
#include "glew.h"
#include "SDL_opengl.h"
#include "glut.h"

#include "cimport.h"               // Assimp Plain-C interface
#include "scene.h"                 // Assimp Output data structure
#include "postprocess.h"           // Assimp Post processing flags

#include "Math3D.c"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

typedef struct ShaderProgram {
    GLuint programID;
    uint8_t uniformCount;
    GLuint uniformPtrs[16];
    GLchar* uniformNames[16];
    GLchar* vertexShaderSrc;
    GLchar* fragShaderSrc;
    GLchar uniformNameBuffer[512];
}ShaderProgram;

typedef struct OpenGLTexture {
    GLuint textureID;
    uint16_t width;
    uint16_t height;
}OpenGLTexture;

typedef struct OpenGLMesh {
	Matrix4 m;
	GLuint elementCount;
	GLuint vbo;
	GLuint ibo;
}OpenGLMesh;

typedef struct MeshData {
	GLfloat* vertexData;
	GLuint* indexData;
	uint16_t indexCount;
	uint16_t vertexCount;
}MeshData;

uint16_t SCREEN_WIDTH = 640;
uint16_t SCREEN_HEIGHT = 480;
uint16_t mSecsPerFrame = 60 / 1000;
SDL_Window* window;
SDL_GLContext gContext;

Matrix4 spinMatrix;
ShaderProgram myProgram;
MeshData tinyMeshData;
OpenGLMesh renderMesh;
OpenGLTexture myTexture;

bool InitOpenGLRenderer( const float screen_w, const float screen_h ) {
	//Set viewport
	glViewport( 0.0f, 0.0f, screen_w, screen_h );

	float halfHeight = screen_h * 0.5f;
	float halfWidth = screen_w * 0.5f;
    //Initialize Projection Matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( -halfWidth, halfWidth, -halfHeight, halfHeight, halfWidth, -halfWidth );

    //Initialize Modelview Matrix
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    //Initialize clear color
    glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

    //Set blending
    glEnable( GL_BLEND );
    glHint(GL_PERSPECTIVE_CORRECTION_HINT , GL_NICEST);
    glDisable( GL_DEPTH_TEST );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	//Initialize framebuffer //RETURN TO LAZY FOO LESSON 27 on Framebuffers
    //glGenFramebuffers( 1, &frameBuffer.gFBO );
    //if( gFBOTexture.getTextureID() == 0 ) { 
    //Create it 
    //gFBOTexture.createPixels32( SCREEN_WIDTH, SCREEN_HEIGHT ); 
    //gFBOTexture.padPixels32(); 
    //gFBOTexture.loadTextureFromPixels32(); } 
    //Bind texture 
    //glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gFBOTexture.getTextureID(), 0 );

    //Check for error
    GLenum error = glGetError();
    if( error != GL_NO_ERROR ) {
        printf( "Error initializing OpenGL! %s\n", gluErrorString( error ) );
        return false;
    } else {
    	printf( "Initialized OpenGL\n" );
    }

    return true;
}

bool Init() {
    // initialize SDL
    if( SDL_Init( SDL_INIT_VIDEO ) >= 0 ) {

        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
        SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
        printf("Set Attributes\n");

        window = SDL_CreateWindow("Wet Clay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
        if(window == NULL) {
            printf("Failed to create window\n");
            return false;
        } else {
            printf("created window\n");

            gContext = SDL_GL_CreateContext( window );
            if( gContext == NULL ) {
                printf("could not create GL context\n");
            } else {
                printf("created GL context\n");
            }

            if( SDL_GL_SetSwapInterval( 1 ) < 0 ) {
                printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
            }
        }
    } else {
        printf( "Could not init SDL video. SDL Error: %s\n", SDL_GetError() );
        return false; // sdl could not initialize
    }

    //Initialize GL stuff
    glewExperimental = GL_TRUE;
    GLenum error = glewInit();
    if( error != GLEW_OK ) {
        printf( "Failed to init GLEW\n" );
    }

    //Initialize OpenGL
    if( !InitOpenGLRenderer( SCREEN_WIDTH, SCREEN_HEIGHT ) ) {
    	printf( "Unable to initialize OpenGL!\n" );
    	return false;
    }

    return true;
}

bool LoadTextureFromFile( OpenGLTexture* texData, const char* fileName ) {
    //Get ptr to currently bound ptr (probably SDL's screen/frame texture buffer whatever)
    GLint prevBoundTex;
    glGetIntegerv( GL_TEXTURE_BINDING_2D, &prevBoundTex );
    printf( "Previously Bound Texture Ptr: %d\n", prevBoundTex );

    //Load data from file
    uint8_t n;
    unsigned char* data = stbi_load( fileName, &texData->width, &texData->height, &n, 0 );
    if( data == NULL ) {
        printf( "Could not load file: %s\n", fileName );
        return false;
    }
    printf( "Loaded file: %s\n", fileName );
    printf( "Width: %d, Height: %d, Components per channel: %d\n", texData->width, texData->height, n );

    //Generate Texture and bind pointer
    glGenTextures( 1, &texData->textureID );
    glBindTexture( GL_TEXTURE_2D, texData->textureID );

    //Create Texture in Memory, parameters in order:
    //1 - Which Texture binding
    //2 - # of MipMaps
    //3 - # of components per pixel, 4 means RGBA, 3 means RGB & so on
    //4 - Texture Width
    //5 - Texture Height
    //6 - OpenGL Reference says this value must be 0 :I great library
    //7 - How are pixels organized
    //8 - size of each component
    //9 - ptr to texture data
    glTexImage2D( GL_TEXTURE_2D, 0, n, texData->width, texData->height, 0, GL_RGB, GL_UNSIGNED_BYTE, data );

    //Set image filtering
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ); // Use linear interoplation when the texture is squished
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST ); // Use nearest filter when texture is stretched

    printf( "Closing Image\n" );
    stbi_image_free( data );

    glBindTexture( GL_TEXTURE_2D, prevBoundTex );
    return true;
}

bool LoadMeshFromFile( MeshData* data, const char* fileName ) {
    // Start the import on the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll t
    // probably to request more postprocessing than we do in this example.
    unsigned int myFlags = 
    aiProcess_CalcTangentSpace         | // calculate tangents and bitangents if possible
    aiProcess_JoinIdenticalVertices    | // join identical vertices/ optimize indexing
    aiProcess_Triangulate              | // Ensure all verticies are triangulated (each 3 vertices are triangle)
    aiProcess_ImproveCacheLocality     | // improve the cache locality of the output vertices
    aiProcess_FindDegenerates          | // remove degenerated polygons from the import
    aiProcess_FindInvalidData          | // detect invalid model data, such as invalid normal vectors
    aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
    aiProcess_LimitBoneWeights         | // limit bone weights to 4 per vertex
    aiProcess_OptimizeMeshes           | // join small meshes, if possible;
    0;

    const struct aiScene* scene = aiImportFile( fileName, myFlags );

    // If the import failed, report it
    if( !scene ) {
        //DoTheErrorLogging( aiGetErrorString());
        printf( "Failed to import %s\n", fileName );
        return false;
    }

    // Now we can access the file's contents
    uint16_t numMeshes = scene->mNumMeshes;
    printf( "Yay, loaded file: %s\n", fileName );
    //printf( "%d Meshes in file\n", numMeshes );
    if(numMeshes == 0) {
        return false;
    }

    //Read data for each mesh
    for( uint8_t i = 0; i < numMeshes; i++ ) {
        const struct aiMesh* mesh = scene->mMeshes[i];
        //printf( "Mesh %d: %d Vertices, %d Faces\n", i, mesh->mNumVertices, mesh->mNumFaces );

        //Read vertex data
        uint16_t vertexCount = mesh->mNumVertices;
        data->vertexData = calloc( 1, vertexCount * sizeof(GLfloat) * 3 );    //Allocate space for vertex buffer
        //printf("Vertex Pointer data: %p\n", data->vertexData );
        for( uint16_t j = 0; j < vertexCount; j++ ) {
            data->vertexData[j * 3 + 0] = mesh->mVertices[j].x * 50.0f;
            data->vertexData[j * 3 + 1] = mesh->mVertices[j].z * 50.0f;
            data->vertexData[j * 3 + 2] = mesh->mVertices[j].y * 50.0f;
            //printf( "Vertex Data %d: %f, %f, %f\n", j, data->vertexData[j * 3 + 0], data->vertexData[j * 3 + 1], data->vertexData[j * 3 + 2]);
        }
        data->vertexCount = vertexCount;

        //Read index data
        uint16_t indexCount = mesh->mNumFaces * 3;
        data->indexData = calloc( 1, indexCount * sizeof(GLuint) );    //Allocate space for index buffer
        //printf("Index Pointer data: %p\n", data->indexData );
        for( uint16_t j = 0; j < mesh->mNumFaces; j++ ) {
            const struct aiFace face = mesh->mFaces[j];
            data->indexData[j * 3 + 0] = face.mIndices[0];
            data->indexData[j * 3 + 1] = face.mIndices[1];
            data->indexData[j * 3 + 2] = face.mIndices[2];
            //printf( "Index Set: %d, %d, %d\n", data->indexData[j * 3 + 0], data->indexData[j * 3 + 1], data->indexData[j * 3 + 2]);
        }
        data->indexCount = indexCount;
    }

    // We're done. Release all resources associated with this import
    aiReleaseImport( scene);
    return true;
}

void CreateRenderMesh(OpenGLMesh* renderMesh, MeshData* meshData) {
	//Create VBO
    glGenBuffers( 1, &renderMesh->vbo );
    glBindBuffer( GL_ARRAY_BUFFER, renderMesh->vbo );
    //printf( "Vertex Array Data - count %d, pointer: %p\n", meshData->vertexCount, meshData->vertexData );
    glBufferData( GL_ARRAY_BUFFER, 3 * meshData->vertexCount * sizeof(GLfloat), meshData->vertexData, GL_STATIC_DRAW );

    //Create IBO
    glGenBuffers( 1, &renderMesh->ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, renderMesh->ibo );
    //printf( "Index Array Data - count %d, pointer: %p\n", meshData->indexCount, meshData->indexData );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, meshData->indexCount * sizeof(GLuint), meshData->indexData, GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );

    renderMesh->elementCount = meshData->indexCount;
}

void RenderMesh( OpenGLMesh* mesh, ShaderProgram* program ) {
	glEnableClientState( GL_VERTEX_ARRAY );
	glUseProgram( program->programID );

	glMatrixMode( GL_MODELVIEW );
	glMultMatrixf( &mesh->m.m[0] );

    //Set vertex data
	glBindBuffer( GL_ARRAY_BUFFER, mesh->vbo );
	glVertexPointer( 3, GL_FLOAT, 0, 0 );
    //Set index data and render
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->ibo );

    //Assume we're doing just GL_Triangles
	glDrawElements( GL_TRIANGLES, mesh->elementCount, GL_UNSIGNED_INT, NULL );

    //Disable vertex arrays
	glDisableClientState( GL_VERTEX_ARRAY );
    //clear shader
	glUseProgram( (GLuint)NULL );
}

void printShaderLog( GLuint shader ) {
    //Make sure name is shader
    if( glIsShader( shader ) ) {
        //Shader log length
        int infoLogLength = 0;
        int maxLength = infoLogLength;
        
        //Get info string length
        glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
        
        //Allocate string
        GLchar infoLog[ maxLength ];
        
        //Get info log
        glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
        if( infoLogLength > 0 ) {
            //Print Log
            printf( "%s\n", infoLog );
        }
    } else {
        printf( "Name %d is not a shader\n", shader );
    }
}

void LoadShaderSrc(const char* fileName, char** srcPtr){
    FILE* file = fopen( fileName, "r" );
    if( file == NULL ) {
        printf( "Could not load vert source file\n" );
        return;
    }
    //Seek till end of file, and get size
    fseek( file, 0, SEEK_END );
    uint16_t fileSize = ftell( file );
    printf( "Opened vert src file, length is: %d\n", fileSize );
    //Reset to beginning
    fseek( file, 0, SEEK_SET );
    //Allocate space for src
    *srcPtr = calloc( fileSize + 1, sizeof( GLchar ) );
    //Actually read data into allocated space
    fread( *srcPtr , 1, fileSize, file );
    //Close source file
    fclose(file);
}

void CreateShader(ShaderProgram* program, const char* vertShaderFile, const char* fragShaderFile) {
    GLuint vertexShader;
    GLuint fragShader;

    //Create program
    program->programID = glCreateProgram();

    //Create vertex shader component
    vertexShader = glCreateShader( GL_VERTEX_SHADER );
    //Bind source to program
    LoadShaderSrc( vertShaderFile, &program->vertexShaderSrc );
    glShaderSource( vertexShader, 1, &program->vertexShaderSrc, NULL );

    //Compile Vertex Shader
    glCompileShader( vertexShader );
    //Check for errors
    GLint compiled = GL_FALSE;
    glGetShaderiv( vertexShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Could not compile Vertex Shader\n" );
        printShaderLog( vertexShader );
    } else {
        printf( "Vertex Shader %s compiled\n", vertShaderFile );
        //Actually attach it if it compiled
        glAttachShader( program->programID, vertexShader );
    }

    //Create fragment shader component
    fragShader = glCreateShader( GL_FRAGMENT_SHADER );
    //Load source and bind to GL program
    LoadShaderSrc( fragShaderFile, &program->fragShaderSrc );
    glShaderSource( fragShader, 1, &program->fragShaderSrc, NULL );

    //Compile fragment source
    glCompileShader( fragShader );
    //Check for errors
    compiled = GL_FALSE;
    glGetShaderiv( fragShader, GL_COMPILE_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Unable to compile fragment shader\n" );
        printShaderLog( fragShader );
    } else {
        printf( "Frag Shader %s compiled\n", fragShaderFile );
        //Actually attach it if it compiled
        glAttachShader( program->programID, fragShader );
    }

    //Link program
    glLinkProgram( program->programID );
    //Check for errors
    compiled = GL_TRUE;
    glGetProgramiv( program->programID, GL_LINK_STATUS, &compiled );
    if( compiled != GL_TRUE ) {
        printf( "Error linking program\n" );
    }

    uintptr_t nameBufferOffset = 0;
    GLint total = -1;
    glGetProgramiv( program->programID, GL_ACTIVE_UNIFORMS, &total ); 
    for(uint8_t i = 0; i < total; ++i)  {
    	//Allocate space for searching
    	GLint name_len = -1;
    	GLsizei size = -1;
    	GLenum type = GL_ZERO;
    	char name[100];

    	//Get uniform info
    	glGetActiveUniform( program->programID, i, sizeof(name)-1, &name_len, &size, &type, &name[0] );
    	name[name_len] = 0;

    	//Copy info into program struct
    	//Get uniform ptr
    	program->uniformPtrs[i] = glGetUniformLocation( program->programID, &name[0] );
    	//Set ptr to name
    	program->uniformNames[i] = &program->uniformNameBuffer[nameBufferOffset];
    	//Copy name
    	memcpy(&program->uniformNameBuffer[nameBufferOffset], &name[0], name_len * sizeof(char));
    	nameBufferOffset += name_len + 1;

        //printf("Got info for Shader Uniform: %s\n", name);
    }

    glDeleteShader( vertexShader ); 
    glDeleteShader( fragShader );
}

void LoadData() {

}

void Update() {
	renderMesh.m = MultMatrix( renderMesh.m, spinMatrix );
}

void Render() {

    //Enable texturing
    glEnable( GL_TEXTURE_2D );

	//Clear color buffer & depth buffer
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	//Clear model view matrix info
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	RenderMesh( &renderMesh, &myProgram );
}

int main(int argc, char** argv) {
    LoadTextureFromFile( &myTexture, "Data/Checkerboard.png" );
    if(!Init()) {
		return 0;
	}
	printf("SDL setup seems to have worked.\n");

    LoadMeshFromFile( &tinyMeshData, "Data/Pointy.fbx" );
    CreateRenderMesh( &renderMesh, &tinyMeshData );
    CreateShader( &myProgram, "Data/Vert.vert", "Data/Frag.frag" );

    printf( "Loading and creation complete\n" );

    Identity( &renderMesh.m );
	const float spinSpeed = 3.1415926 / 64.0f;
    SetRotation( &spinMatrix, 0.0f, 1.0f, 0.0f, spinSpeed );

    bool quit = false;
    SDL_Event e;
    while( !quit ) {
        uint16_t startTime = SDL_GetTicks();

        while( SDL_PollEvent( &e ) != 0 ) {
            if( e.type == SDL_QUIT ) {
                quit = true;
            }
        }

        Update();

        Render();

        SDL_GL_SwapWindow( window );

        uint16_t endTime = SDL_GetTicks();
        uint16_t totalTime = endTime - startTime;
        if( totalTime < mSecsPerFrame ) {
            SDL_Delay(mSecsPerFrame - totalTime);
        }
    }
    //Destroy window 
    SDL_DestroyWindow( window ); 
    //Quit SDL subsystems 
    SDL_Quit(); 
    printf("Quit");

	return 1;
}