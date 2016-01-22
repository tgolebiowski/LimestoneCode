#include <SDL/SDL.h>
#define GLEW_STATIC
#include <OpenGL/glew.h>
#include <SDL/SDL_opengl.h>
#include <OpenGL/glut.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "Math3D.c"
//#include "Mesh.c"

typedef struct ShaderProgram {
    GLuint programID;
    uint8_t uniformCount;
    GLuint uniformPtrs[16];
    GLchar* uniformNames[16];
    GLchar* vertexShaderSrc;
    GLchar* fragShaderSrc;
    GLchar uniformNameBuffer[512];
}ShaderProgram;

typedef struct FrameBuffer {
	GLuint gFBO;
	//LTexture gFBOTexture;
}FrameBuffer;

FrameBuffer frameBuffer = { 0 };

typedef struct Mesh {
    GLuint VBO;
    GLuint IBO;
    uint16_t vertexCount;
    uint16_t indexCount;
    Matrix4 transformMatrix;
}Mesh;

void CreateMesh(Mesh* mesh, const GLfloat* vData, const GLuint* iData, const uint16_t vCount, const uint16_t iCount) {
    if(mesh == NULL) return;

    // mesh->vertexData = calloc(vCount, sizeof(GLfloat));
    // mesh->vertexCount = vCount;
    // memcpy( &mesh->vertexData, vData, vCount * sizeof(GLfloat) );

    // mesh->indexData = calloc(iCount, sizeof(GLfloat));
    // mesh->indexCount = iCount;
    // memcpy( &mesh->indexData, iData, vCount * sizeof(GLuint) );

    //Create VBO
    glGenBuffers( 1, &mesh->VBO );
    glBindBuffer( GL_ARRAY_BUFFER, mesh->VBO );
    //glBufferData( GL_ARRAY_BUFFER, 3 * vertCount * sizeof(GLfloat), &vertData, GL_STATIC_DRAW );
    glBufferData( GL_ARRAY_BUFFER, 3 * vCount * sizeof(GLfloat), &vData, GL_STATIC_DRAW );

    printf("VBO address %d\n", mesh->VBO);
    printf("Vertex info\n");
    for(int i = 0; i < vCount * 3; i += 3) {
        printf( "%f, %f, %f\n", vData[i + 0], vData[i + 1], vData[i + 2] );
    }

    //Create IBO
    glGenBuffers( 1, &mesh->IBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->IBO );
    //glBufferData( GL_ELEMENT_ARRAY_BUFFER, intCount * sizeof(GLuint), &indData, GL_STATIC_DRAW );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, iCount * sizeof(GLuint), &iData, GL_STATIC_DRAW );

    printf("IBO address %d\n", mesh->IBO);
    printf("Index info\n");
    for(int i = 0; i < iCount; i += 3) {
        printf( "%d, %d, %d\n", iData[i + 0], iData[i + 1], iData[i + 2] );
    }

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );
}

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

    //Enable texturing
    glEnable( GL_TEXTURE_2D );

    //Set blending
    glEnable( GL_BLEND );
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
    }

    return true;
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
        printf( "Vertex Shader compiled\n" );
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
        printf( "Frag Shader compiled\n" );
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
    }else {
    	printf( "Shader Linked\n" );
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

        printf("Got info for Shader Uniform: %s\n", name);
    }

    glDeleteShader( vertexShader ); 
    glDeleteShader( fragShader );
}

void RenderMesh(const Mesh* mesh, const ShaderProgram* program) {
    static float angle = 0.0f;
    const float tick = 3.14159 / 50.0f;
    Matrix4 scaleMatrix;
    SetScale( &scaleMatrix, sinf(angle) * 2.0 + 1.0, sinf(angle) * 2.0 + 1.0, 1.0);
    //glMatrixMode( GL_MODELVIEW );
    //glMultMatrixf( &scaleMatrix );
    angle += tick;

    //Bind program
    glUseProgram( program->programID );
    //comment
    glEnableClientState( GL_VERTEX_ARRAY );

    //Set vertex data
    glBindBuffer( GL_ARRAY_BUFFER, mesh->VBO );
    glVertexPointer( 3, GL_FLOAT, 0, 0 );
    //Set index data and render
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->IBO );

    //Assume we're doing just GL_Triangles
    glDrawElements( GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, NULL );

    //Disable vertex arrays
    glDisableClientState( GL_VERTEX_ARRAY );
    //clear shader
    glUseProgram( (GLuint)NULL );
}