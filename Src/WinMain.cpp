#include <stdio.h>
#include <stdbool.h>

#define GLEW_STATIC
#include "OpenGL/glew.h"

#include "App.h"
#include "App.cpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool running = true;
bool isFullScreen = false;

uint16_t SCREEN_WIDTH = 640;
uint16_t SCREEN_HEIGHT = 480;
uint16_t mSecsPerFrame = 1000 / 60;

struct AppInfo {
	HINSTANCE appInstance;
	HWND hwnd;
	WNDCLASSEX wc;
	HDC deviceContext;
	HGLRC openglRenderContext;
};
static AppInfo appInfo;

int16_t ReadShaderSrcFileFromDisk(const char* fileName, GLchar* buffer, uint16_t bufferLen) {
	FILE* file;
    errno_t e = fopen_s(&file, fileName, "r" );
    if( file == NULL || e != 0) {
        printf( "Could not load text file %s\n", fileName );
        return -1;
    }
    fseek( file, 0, SEEK_END );
    size_t fileSize = ftell( file );
    if(fileSize >= bufferLen) {
    	return fileSize;
    }

    fseek( file, 0, SEEK_SET );
    fread( buffer , 1, fileSize, file );
    fclose( file );
    return 0;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch (uMsg) {
		case WM_CLOSE: {
			running = false;
			wglMakeCurrent( appInfo.deviceContext, NULL );
			wglDeleteContext( appInfo.openglRenderContext );
			ReleaseDC( appInfo.hwnd, appInfo.deviceContext );
			DestroyWindow( appInfo.hwnd );
			break;
		}

		case WM_DESTROY: {
			running = false;
			wglMakeCurrent( appInfo.deviceContext, NULL );
			wglDeleteContext( appInfo.openglRenderContext );
			ReleaseDC( appInfo.hwnd, appInfo.deviceContext );
			PostQuitMessage( 0 );
			break;
		}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
	AllocConsole(); //create a console
	freopen("conin$","r",stdin);
	freopen("conout$","w",stdout);
	freopen("conout$","w",stderr);
	printf( "Program Started, console initialized\n" );

	appInfo.appInstance = hInstance;

	const char WindowName[] = "Wet Clay";

	appInfo.wc.cbSize = sizeof(WNDCLASSEX);
	appInfo.wc.style = 0;
	appInfo.wc.lpfnWndProc = WndProc;
	appInfo.wc.cbClsExtra = 0;
	appInfo.wc.cbWndExtra = 0;
	appInfo.wc.hInstance = appInfo.appInstance;
	appInfo.wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	appInfo.wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	appInfo.wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	appInfo.wc.lpszMenuName = NULL;
	appInfo.wc.lpszClassName = WindowName;
	appInfo.wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&appInfo.wc)) {
		printf( "Failed to register class\n" );
		running = false;
		return -1;
	}

	// center position of the window
	int posx = (GetSystemMetrics(SM_CXSCREEN) / 2) - (SCREEN_WIDTH / 2);
	int posy = (GetSystemMetrics(SM_CYSCREEN) / 2) - (SCREEN_HEIGHT / 2);
 
	// set up the window for a windowed application by default
	//long wndStyle = WS_OVERLAPPEDWINDOW;
 
	if( isFullScreen ) {
		posx = 0;
		posy = 0;

		// change resolution before the window is created
		//SysSetDisplayMode(width, height, SCRDEPTH);
		//TODO: implement
	}
 
	// at this point WM_CREATE message is sent/received
	// the WM_CREATE branch inside WinProc function will execute here
	appInfo.hwnd = CreateWindowEx(0, WindowName, "Wet Clay App", WS_BORDER, posx, posy, SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, appInfo.appInstance, NULL);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
	    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
		32,                       //Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		24,                       //Number of bits for the depthbuffer
	    8,                        //Number of bits for the stencilbuffer
		0,                        //Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0, 0, 0, 0
	};

	appInfo.deviceContext = GetDC(appInfo.hwnd);

	int  letWindowsChooseThisPixelFormat;
	letWindowsChooseThisPixelFormat = ChoosePixelFormat( appInfo.deviceContext, &pfd ); 
	SetPixelFormat( appInfo.deviceContext, letWindowsChooseThisPixelFormat, &pfd );

	appInfo.openglRenderContext = wglCreateContext( appInfo.deviceContext );
	if( wglMakeCurrent ( appInfo.deviceContext, appInfo.openglRenderContext ) == false ) {
		printf( "Couldn't make GL context current.\n" );
		return -1;
	}

	glewInit();

	SetWindowLong( appInfo.hwnd, GWL_STYLE, 0 );
	ShowWindow ( appInfo.hwnd, SW_SHOWNORMAL );
	UpdateWindow( appInfo.hwnd );

	MemorySlab gameSlab;
	gameSlab.slabSize = MEGABYTES(32);
	gameSlab.slabStart = VirtualAlloc( NULL, gameSlab.slabSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
	assert( gameSlab.slabStart != NULL );
	
	GameInit( &gameSlab );

	MSG Msg;
	do {
		while( PeekMessage( &Msg, NULL, 0, 0, PM_REMOVE ) ) {
			TranslateMessage( &Msg );
			DispatchMessage( &Msg );
		}

		static DWORD startTime;
		static DWORD endTime;
		startTime = GetTickCount();

		if(running) {
			running = Update( &gameSlab );
			Render( &gameSlab );
			SwapBuffers( appInfo.deviceContext );
		}

		endTime = GetTickCount();
		DWORD computeTime = endTime - startTime;
		if(computeTime < mSecsPerFrame ) {
			Sleep(mSecsPerFrame - computeTime);
		}

	} while( running );

	printf("Exitting\n");

	FreeConsole();

	return Msg.wParam;
}