#define GLEW_STATIC
#include "OpenGL/glew.h"
#include "OpenGL/wglew.h"

#include "assimp/cimport.h"               // Assimp Plain-C interface
#include "assimp/scene.h"                 // Assimp Output data structure
#include "assimp/postprocess.h"           // Assimp Post processing flags

#include "Math3D.c"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

typedef struct OpenGLTexture {
    GLuint textureID;
    GLenum pixelFormat;
    int width;
    int height;
    GLuint* data;
}OpenGLTexture;

typedef struct ShaderProgram {
    GLuint programID;

    uint8_t uniformCount;
    GLuint uniformPtrs[8];
    GLenum uniformTypes[8];
    GLchar* uniformNames[8];

    GLuint positionAttribute;
    GLuint texCoordAttribute;

    uint8_t samplerCount;
    GLuint samplerPtrs[4];
    GLchar* samplerNames[4];

    GLchar uniformNameBuffer[256];
}ShaderProgram;

typedef struct ShaderTextureSet {
    uint8_t count;
    ShaderProgram* associatedShader;
    GLuint shaderSamplerPtrs[4];
    GLuint texturePtrs[4];
}ShaderTextureSet;

typedef struct OpenGLMesh {
	Matrix4 m;
	GLuint elementCount;
	GLuint vbo;
	GLuint ibo;
    GLuint uvBuffer;
}OpenGLMesh;

typedef struct MeshData {
	GLfloat* vertexData;
    GLfloat* uvData;
	GLuint* indexData;
	uint16_t indexCount;
	uint16_t vertexCount;
}MeshData;

static Matrix4 spinMatrix;
//static Matrix4 cameraMatrix;
static ShaderProgram myProgram;
static MeshData tinyMeshData;
static OpenGLMesh renderMesh;
static OpenGLTexture myTexture;
static OpenGLTexture otherTexture;
static ShaderTextureSet myTextureSet;

static GLuint frameBufferPtr;
static OpenGLTexture frameBufferTexture;
static GLuint framebuffVBO;
static GLuint framebuffIBO;
static GLuint framebuffUVBuff;
static ShaderProgram framebufferShader;

bool LoadTextureFromFile( OpenGLTexture* texData, const char* fileName ) {
    //Load data from file
    int n;
    unsigned char* data = stbi_load( fileName, &texData->width, &texData->height, &n, 0 );
    if( data == NULL ) {
        printf( "Could not load file: %s\n", fileName );
        return false;
    }
    printf( "Loaded file: %s\n", fileName );
    printf( "Width: %d, Height: %d, Components per channel: %d\n", texData->width, texData->height, n );

    if( n == 3 ) {
        texData->pixelFormat = GL_RGB;
    } else if( n == 4 ) {
        texData->pixelFormat = GL_RGBA;
    }

    //Generate Texture and bind pointer
    glGenTextures( 1, &texData->textureID );
    glBindTexture( GL_TEXTURE_2D, texData->textureID );
    printf( "Created texture from file %s with id: %d\n", fileName, texData->textureID );

    //Set texture wrapping
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //Set Texture filtering
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); // Use nearest filtering all the time
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

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
    glTexImage2D( GL_TEXTURE_2D, 0, n, texData->width, texData->height, 0, texData->pixelFormat, GL_UNSIGNED_BYTE, data );

    glBindTexture( GL_TEXTURE_2D, 0 );
    stbi_image_free( data );

    return true;
}

bool CreateEmptyTexture( OpenGLTexture* texture, const uint16_t width, const uint16_t height ) {
    if(texture == NULL) {
        return false;
    }

    size_t spaceNeeded = sizeof(GLuint) * width * height;
    if( ( texture->data = (GLuint*)malloc( spaceNeeded ) ) == NULL) {
        return false;
    }
    memset( texture->data, 0, spaceNeeded );

    texture->height = height;
    texture->width = width;
    texture->pixelFormat = GL_RGBA;

    //Generate Texture and bind pointer
    glGenTextures( 1, &texture->textureID );
    if( texture->textureID == -1 ){
        return false;
    }

    glBindTexture( GL_TEXTURE_2D, texture->textureID );

    //Set texture wrapping
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //Set Texture filtering
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST ); // Use nearest filtering all the time
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    //Create Texture in Memory, parameters in order:
    //1 - Which Texture binding
    //2 - # of MipMaps
    //3 - # of components per pixel, 4 means RGBA, 3 means RGB & so on
    //4 - Texture Width
    //5 - Texture Height
    //6 - OpenGL Reference says this value must be 0 :I whatta great library
    //7 - How are pixels organized
    //8 - size of each component
    //9 - ptr to texture data
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->data );

    glBindTexture( GL_TEXTURE_2D, 0 );

    return true;
}

void CreateFrameBuffer( OpenGLTexture* texture ) {
    const GLfloat uvData[4 * 2] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f };
    const GLuint iData[4] = { 3, 2, 1, 0 };
    GLfloat vData[4 * 3] = { 0.0f, 0.0f, 0.0f, 
                            (float)texture->width, 0.0f, 0.0f,
                            (float)texture->width, (float)texture->height, 0.0f, 
                            0.0f, (float)texture->height, 0.0f };

    //Create VBO
    glGenBuffers( 1, &framebuffVBO );
    glBindBuffer( GL_ARRAY_BUFFER, framebuffVBO );
    glBufferData( GL_ARRAY_BUFFER, 3 * 4 * sizeof(GLfloat), &vData, GL_STATIC_DRAW );

    //Create IBO
    glGenBuffers( 1, &framebuffIBO );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, framebuffIBO );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), &iData, GL_STATIC_DRAW );

    //Create UV Buffer
    glGenBuffers( 1, &framebuffUVBuff );
    glBindBuffer( GL_ARRAY_BUFFER, framebuffUVBuff );
    glBufferData( GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), &uvData, GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );
}

void RenderFramebufferTexture( OpenGLTexture* texture ) {
    glBindTexture( GL_TEXTURE_2D, texture->textureID );

    //Bind Shader
    glUseProgram( framebufferShader.programID );

    //Set vertex data
    glBindBuffer( GL_ARRAY_BUFFER, framebuffVBO );
    glEnableVertexAttribArray( framebufferShader.positionAttribute );
    glVertexAttribPointer( framebufferShader.positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set UV data
    glBindBuffer( GL_ARRAY_BUFFER, framebuffUVBuff );
    glEnableVertexAttribArray( framebufferShader.texCoordAttribute );
    glVertexAttribPointer( framebufferShader.texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    glBindTexture( GL_TEXTURE_2D, texture->textureID );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, framebuffIBO );    //Bind index data
    glDrawElements( GL_QUADS, 4, GL_UNSIGNED_INT, NULL );     //Render da quad

    //clear shader
    glUseProgram( (GLuint)NULL );

    //Unbind textures
    glBindTexture( GL_TEXTURE_2D, 0 );
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
    0;

    const struct aiScene* scene = aiImportFile( fileName, myFlags );

    // If the import failed, report it
    if( !scene ) {
        printf( "Failed to import %s\n", fileName );
        return false;
    }

    // Now we can access the file's contents
    uint16_t numMeshes = scene->mNumMeshes;
    printf( "Loaded file: %s\n", fileName );
    printf( "%d Meshes in file\n", numMeshes );
    if(numMeshes == 0) {
        return false;
    }

    //Read data for each mesh
    for( uint8_t i = 0; i < numMeshes; i++ ) {
        const struct aiMesh* mesh = scene->mMeshes[i];
        printf( "Mesh %d: %d Vertices, %d Faces\n", i, mesh->mNumVertices, mesh->mNumFaces );

        //Read vertex data
        uint16_t vertexCount = mesh->mNumVertices;
        data->uvData = (GLfloat*)calloc( 1, vertexCount * sizeof(GLfloat) * 2);
        data->vertexData = (GLfloat*)calloc( 1, vertexCount * sizeof(GLfloat) * 3 );    //Allocate space for vertex buffer
        //printf("Vertex Pointer data: %p\n", data->vertexData );
        for( uint16_t j = 0; j < vertexCount; j++ ) {
            data->vertexData[j * 3 + 0] = mesh->mVertices[j].x * 50.0f;
            data->vertexData[j * 3 + 1] = mesh->mVertices[j].z * 50.0f;
            data->vertexData[j * 3 + 2] = mesh->mVertices[j].y * 50.0f;

            data->uvData[j * 2 + 0] = mesh->mTextureCoords[0][j].x;
            data->uvData[j * 2 + 1] = mesh->mTextureCoords[0][j].y;
            //printf("UV Data: %f, %f\n", data->uvData[j * 2 + 0], data->uvData[j * 2 + 1]);
            //printf( "Vertex Data %d: %f, %f, %f\n", j, data->vertexData[j * 3 + 0], data->vertexData[j * 3 + 1], data->vertexData[j * 3 + 2]);
        }
        data->vertexCount = vertexCount;

        //Read index data
        uint16_t indexCount = mesh->mNumFaces * 3;
        data->indexData = (GLuint*)calloc( 1, indexCount * sizeof(GLuint) );    //Allocate space for index buffer
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
    aiReleaseImport( scene );
    return true; 
}

void CreateRenderMesh(OpenGLMesh* renderMesh, MeshData* meshData) {
    //Create VBO
    glGenBuffers( 1, &renderMesh->vbo );
    glBindBuffer( GL_ARRAY_BUFFER, renderMesh->vbo );
    glBufferData( GL_ARRAY_BUFFER, 3 * meshData->vertexCount * sizeof(GLfloat), meshData->vertexData, GL_STATIC_DRAW );

    //Create IBO
    glGenBuffers( 1, &renderMesh->ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, renderMesh->ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, meshData->indexCount * sizeof(GLuint), meshData->indexData, GL_STATIC_DRAW );

    //Create UV Buffer
    glGenBuffers( 1, &renderMesh->uvBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, renderMesh->uvBuffer );
    glBufferData( GL_ARRAY_BUFFER, 2 * meshData->vertexCount * sizeof(GLfloat), meshData->uvData, GL_STATIC_DRAW );

    glBindBuffer( GL_ARRAY_BUFFER, (GLuint)NULL ); 
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, (GLuint)NULL );

    renderMesh->elementCount = meshData->indexCount;
    //printf("Created render mesh\n");
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
    *srcPtr = (char*)calloc( fileSize + 1, sizeof( GLchar ) );
    //Actually read data into allocated space
    fread( *srcPtr , 1, fileSize, file );
    //Close source file
    fclose(file);
}

void CreateShader(ShaderProgram* program, const char* vertShaderFile, const char* fragShaderFile) {
    GLuint vertexShader;
    GLuint fragShader;
    GLchar* vertexShaderSrc;
    GLchar* fragShaderSrc;

    //Create program
    program->programID = glCreateProgram();

    //Create vertex shader component
    vertexShader = glCreateShader( GL_VERTEX_SHADER );
    //Bind source to program
    LoadShaderSrc( vertShaderFile, &vertexShaderSrc );
    glShaderSource( vertexShader, 1, &vertexShaderSrc, NULL );

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
    LoadShaderSrc( fragShaderFile, &fragShaderSrc );
    glShaderSource( fragShader, 1, &fragShaderSrc, NULL );

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
    } else {
        printf( "Shader Program Linked Successfully\n");
    }

    //clean up space allocated for shader program source files
    free( vertexShaderSrc );
    free( fragShaderSrc );

    //Now we're going to cache all the uniforms and attributes in the shader
    uintptr_t nameBufferOffset = 0;
    GLint total = -1;
    glGetProgramiv( program->programID, GL_ACTIVE_UNIFORMS, &total ); 
    program->uniformCount = 0;
    program->samplerCount = 0;

    glUseProgram(program->programID);

    for(uint8_t i = 0; i < total; ++i)  {
        //Allocate space for searching
        GLint name_len = -1;
        GLsizei size = -1;
        GLenum type = GL_ZERO;
        char name[100];

        //Get uniform info
        glGetActiveUniform( program->programID, i, sizeof(name)-1, &name_len, &size, &type, &name[0] );
        name[name_len] = 0;

        if( type == GL_SAMPLER_2D ) {
            GLuint samplerLocation = glGetUniformLocation( program->programID, &name[0] );
            glUniform1i( samplerLocation, program->samplerCount);
            program->samplerNames[program->samplerCount] = &program->uniformNameBuffer[nameBufferOffset];
            program->samplerPtrs[program->samplerCount] = samplerLocation;
            printf( "Got info for Sampler Uniform: %s, ptr %d, bound to unit %d\n", name, samplerLocation, program->samplerCount );
            program->samplerCount++;
        }else {
            //Copy info into program struct
            //Get uniform ptr
            program->uniformPtrs[program->uniformCount] = glGetUniformLocation( program->programID, &name[0] );
            program->uniformNames[program->uniformCount] = &program->uniformNameBuffer[nameBufferOffset];   //Set ptr to name
            printf( "Got info for Shader Uniform: %s, type: %d, size: %d, ptr %d\n", name, type, size, program->uniformPtrs[program->uniformCount] );
            program->uniformCount++;
        }

        memcpy(&program->uniformNameBuffer[nameBufferOffset], &name[0], name_len * sizeof(char));
        nameBufferOffset += name_len + 1;
    }

    program->positionAttribute = glGetAttribLocation( program->programID, "position" );
    program->texCoordAttribute = glGetAttribLocation( program->programID, "texCoord" );

    glUseProgram(0);

    glDeleteShader( vertexShader ); 
    glDeleteShader( fragShader );
}

void RenderMesh( OpenGLMesh* mesh, ShaderProgram* program ) {
    //Flush errors
    //while( glGetError() != GL_NO_ERROR ){};

    //Bind Shader
    glUseProgram( program->programID );
    glUniformMatrix4fv( glGetUniformLocation(program->programID, "modelMatrix"), 1, false, (const float*)&mesh->m.m[0] );

    //Set vertex data
    glBindBuffer( GL_ARRAY_BUFFER, mesh->vbo );
    glEnableVertexAttribArray( program->positionAttribute );
    glVertexAttribPointer( program->positionAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //Set UV data
    glBindBuffer( GL_ARRAY_BUFFER, mesh->uvBuffer );
    glEnableVertexAttribArray( program->texCoordAttribute );
    glVertexAttribPointer( program->texCoordAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0 );

    glBindTexture( GL_TEXTURE_2D, myTextureSet.texturePtrs[0] );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, myTextureSet.texturePtrs[1] );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, mesh->ibo );                            //Bind index data
    glDrawElements( GL_TRIANGLES, mesh->elementCount, GL_UNSIGNED_INT, NULL );     ///Render, assume its all triangles

    //Unbind textures
    glBindTexture( GL_TEXTURE_2D, 0 );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    //clear shader
    glUseProgram( (GLuint)NULL );
}

bool InitOpenGLRenderer( const float screen_w, const float screen_h ) {
    //Initialize clear color
    glClearColor( 0.1f, 0.1f, 0.1f, 1.0f );

    glEnable( GL_DEPTH_TEST );
    //Configure Texturing, setting some nice perspective correction
    //glEnable( GL_TEXTURE_2D );
    //glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
    //Set Blending
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //Configure Back Face Culling
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    GLint k;
    glGetIntegerv( GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &k );
    printf("Max texture units: %d\n", k);

    //Set viewport
    glViewport( 0.0f, 0.0f, screen_w, screen_h );

    const float halfHeight = screen_h * 0.5f;
    const float halfWidth = screen_w * 0.5f;
    //Initialize Projection Matrix
    glMatrixMode(GL_PROJECTION);
    glOrtho( -halfWidth, halfWidth, -halfHeight, halfHeight, -halfWidth, halfWidth );
    //SetOrthoProjection( &cameraMatrix, -halfWidth, halfWidth, halfHeight, -halfHeight, -halfWidth, halfWidth );

    //Initialize framebuffer
    CreateEmptyTexture( &frameBufferTexture, 1024, 1024 );
    CreateFrameBuffer( &frameBufferTexture );
    glGenFramebuffers( 1, &frameBufferPtr );

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
    if( InitOpenGLRenderer( 640, 480 ) == false ) return false;

    LoadTextureFromFile( &myTexture, "Data/Textures/pink_texture.png" );
    LoadTextureFromFile( &otherTexture, "Data/Textures/green_texture.png" );
    LoadMeshFromFile( &tinyMeshData, "Data/Pointy.fbx" );
    CreateRenderMesh( &renderMesh, &tinyMeshData );
    CreateShader( &myProgram, "Data/Shaders/Vert.vert", "Data/Shaders/Frag.frag" );
    CreateShader( &framebufferShader, "Data/Shaders/Framebuffer.vert", "Data/Shaders/Framebuffer.frag" );
    myTextureSet.count = 2;
    myTextureSet.associatedShader = &myProgram;
    myTextureSet.texturePtrs[0] = myTexture.textureID;
    myTextureSet.texturePtrs[1] = otherTexture.textureID;
    myTextureSet.shaderSamplerPtrs[0] = myProgram.samplerPtrs[0];
    myTextureSet.shaderSamplerPtrs[1] = myProgram.samplerPtrs[1];

    Identity( &renderMesh.m );
    const float spinSpeed = 3.1415926 / 64.0f;
    SetRotation( &spinMatrix, 0.0f, 1.0f, 0.0f, spinSpeed );

    //memset( &cameraMatrix.m[0], 0.0f, sizeof(float) * 16 );

    printf("Init went well\n");
    return true;
}

void Render() {
    glBindFramebuffer( GL_FRAMEBUFFER, frameBufferPtr );
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameBufferTexture.textureID, 0 );

    //Clear color buffer & depth buffer
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    RenderMesh( &renderMesh, &myProgram );

    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    RenderFramebufferTexture( &frameBufferTexture );
}

bool Update() {
    renderMesh.m = MultMatrix( renderMesh.m, spinMatrix );
    return true;
}