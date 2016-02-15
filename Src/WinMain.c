#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#define GLEW_STATIC
#include <glew.h>
#include <glut.h>

#include "App.c"

bool running = true;
bool isFullscreen = false;

uint16_t SCREEN_WIDTH = 640;
uint16_t SCREEN_HEIGHT = 480;
uint16_t mSecsPerFrame = 1000 / 60;

typedef struct AppInfo {
	HINSTANCE appInstance;
	HWND hwnd;
	WNDCLASSEX wc;
	HDC deviceContext;
	HGLRC openglRenderContext;
}AppInfo;
static AppInfo appInfo;

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

bool CreateClayWindow() {
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
		return false;
	}

	// center position of the window
	int posx = (GetSystemMetrics(SM_CXSCREEN) / 2) - (SCREEN_WIDTH / 2);
	int posy = (GetSystemMetrics(SM_CYSCREEN) / 2) - (SCREEN_HEIGHT / 2);
 
	// set up the window for a windowed application by default
	//long wndStyle = WS_OVERLAPPEDWINDOW;
 
	if (isFullscreen)	// create a full-screen application if requested
	{
		//wndStyle = WS_POPUP;
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
		32,                        //Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		24,                        //Number of bits for the depthbuffer
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
		return false;
	}

	glewInit();

	SetWindowLong( appInfo.hwnd, GWL_STYLE, 0 );
	ShowWindow ( appInfo.hwnd, SW_SHOWNORMAL );
	UpdateWindow( appInfo.hwnd );

	return true;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {
	appInfo.appInstance = hInstance;
	CreateClayWindow();
	Init();

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
			Update();
			Render();
			SwapBuffers( appInfo.deviceContext );
		}

		endTime = GetTickCount();
		DWORD computeTime = endTime - startTime;
		if(computeTime < mSecsPerFrame ) {
			Sleep(mSecsPerFrame - computeTime);
		}

	} while( running );

	printf("Exitting\n");

	return Msg.wParam;
}