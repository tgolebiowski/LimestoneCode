#include "SDL/SDL.h"
#define GLEW_STATIC
#include "OpenGL/glew.h"
#include "SDL/SDL_opengl.h"
#include "OpenGL/glut.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "OpenGLContact.c"

#include "Math3D.c"

SDL_Window* window = 0;
SDL_GLContext gContext = 0;

uint16_t SCREEN_WIDTH = 640;
uint16_t SCREEN_HEIGHT = 480;
uint8_t FPS = 60;
uint16_t mSecsPerFrame = 60 / 1000;

ShaderProgram myProgram;
Mesh triMesh = { 0, 0, 0, 0};

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

void Update() {
    //Nothing here yet...
}

int main(int argc, char* args[]) {
    if( Init() == false ) {
        return 1;
    }

    //VBO data
    GLfloat vertexData[] =
    {
        -50.0f, -50.0f, 0.0f,
        50.0f, -50.0f, 0.0f,
        0.0f,  50.0f, 0.0f
    };
    uint16_t vertCount = 3;

    //IBO data
    GLuint indexData[] = { 0, 1, 2 };
    uint16_t indexCount = 3;

    CreateShader( &myProgram, "Data/Vert.vert", "Data/Frag.frag" );
    CreateMesh( &triMesh, &vertexData, &indexData, vertCount, indexCount );
    //LoadMesh( &triMesh, "Data/Pointy.fbx" );

    printf( "In main\n" );
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

        //Clear color buffer & depth buffer
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        static float angle = 0.0f;
        const float spinSpeed = 3.14159f / 64.0f;
        angle += spinSpeed;
        const Vec3 startingLookat = { 0.0f, 0.0f, 1.0f };
        Vec3 cameraPos = { cosf(angle), 0.0 , sinf(angle) };
        Vec3 zeroVec = { 0.0f, 0.0f, 0.0f };
        Vec3 lookAt = DiffVec( cameraPos, zeroVec );
        float lookAtAngle = AngleBetween( startingLookat, lookAt );
        Matrix4 rotationMatrix;
        SetRotation( &rotationMatrix, 0.0f, 1.0f, 0.0f, lookAtAngle );
        Matrix4 translationMatrix;
        SetTranslation( &translationMatrix, cameraPos.x, cameraPos.y, cameraPos.z );
        Matrix4 netMatrix = MultMatrix( &rotationMatrix, &translationMatrix );

        //glMatrixMode( GL_MODELVIEW );
        //glLoadIdentity();
        //glLoadMatrixf( &netMatrix );

        RenderMesh(&triMesh, &myProgram);
        //Swap the framebuffer
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
    printf("Quittin\n");
    return 0;
}