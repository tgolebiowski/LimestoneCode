#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "App.c"

bool running = true;
bool isFullscreen = false;
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
		case WM_CREATE:
		{
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

			appInfo.deviceContext = GetDC(hWnd);

			int  letWindowsChooseThisPixelFormat;
			letWindowsChooseThisPixelFormat = ChoosePixelFormat( appInfo.deviceContext, &pfd ); 
			SetPixelFormat( appInfo.deviceContext, letWindowsChooseThisPixelFormat, &pfd );

			appInfo.openglRenderContext = wglCreateContext(appInfo.deviceContext);
			wglMakeCurrent (appInfo.deviceContext, appInfo.openglRenderContext);

			MessageBoxA(0,(char*)glGetString(GL_VERSION), "OPENGL VERSION",0);
		}
		break;

		case WM_CLOSE:
		DestroyWindow( appInfo.hwnd );
		running = false;
		break;

		case WM_DESTROY:
		wglDeleteContext( appInfo.openglRenderContext );
		PostQuitMessage( 0 );
		running = false;
		break;
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

	int width = 640;
	int height = 480;

	// center position of the window
	int posx = (GetSystemMetrics(SM_CXSCREEN) / 2) - (width / 2);
	int posy = (GetSystemMetrics(SM_CYSCREEN) / 2) - (height / 2);
 
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
	appInfo.hwnd = CreateWindowEx(0, WindowName, "Wet Clay App", WS_BORDER, posx, posy, 640, 480, NULL, NULL, appInfo.appInstance, NULL);
	SetWindowLong(appInfo.hwnd, GWL_STYLE, 0);
	ShowWindow(appInfo.hwnd, SW_SHOWNORMAL);
	UpdateWindow(appInfo.hwnd);

	// if ((appInfo.hdc = GetDC(appInfo.hwnd)) == NULL) {	// get device context
	// 	printf( "Failed to grab device context\n" );
	// 	running = false;
	// 	return false;
	// }

	// const PIXELFORMATDESCRIPTOR pfd = {
	// 	sizeof(PIXELFORMATDESCRIPTOR),
	// 	1,
	//     PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
	//     PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
	//     32,                        //Colordepth of the framebuffer.
	//     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //Padding...?
	//     24,                        //Number of bits for the depthbuffer
	//     8,                        //Number of bits for the stencilbuffer
	//     0,                        //Number of Aux buffers in the framebuffer.
	//     PFD_MAIN_PLANE,
	//     0, 0, 0, 0                //More padding
	// };

	// int iPixelFormat = ChoosePixelFormat(appInfo.hdc, &pfd); 
	// SetPixelFormat( appInfo.hdc, iPixelFormat, &pfd );

	// appInfo.hglrc = wglCreateContext( appInfo.hdc );

	// if( wglMakeCurrent ( appInfo.hdc, appInfo.hglrc ) == false ) {
	// 	printf( "Couldn't make GL context current.\n" );
	// 	return false;
	// }

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
		Update();
		Render();
		SwapBuffers( appInfo.deviceContext );
	} while( running );
	return Msg.wParam;
}