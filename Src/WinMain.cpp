#include <stdio.h>
#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include "tinyxml2.h"
#include "tinyxml2.cpp"

#define GLEW_STATIC
#include "OpenGL/glew.h"

#include "App.h"
#include "GLRenderer.cpp"
#include "App.cpp"

//Win32 function prototypes, allows the entry point to be the first function
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

struct AppInfo {
	HINSTANCE appInstance;
	HWND hwnd;
	WNDCLASSEX wc;
	HDC deviceContext;

	HGLRC openglRenderContext;
	LPRECT windowRect;

	uint16 windowPosX;
	uint16 windowPosY;
	bool running = true;
	bool isFullScreen = false;
	DWORD mSecsPerFrame = 1000 / 60;
};
static AppInfo appInfo;

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
		appInfo.running = false;
		return -1;
	}

	// center position of the window
	appInfo.windowPosX = (GetSystemMetrics(SM_CXSCREEN) / 2) - (SCREEN_WIDTH / 2);
	appInfo.windowPosY = (GetSystemMetrics(SM_CYSCREEN) / 2) - (SCREEN_HEIGHT / 2);
 
	// set up the window for a windowed application by default
	//long wndStyle = WS_OVERLAPPEDWINDOW;
 
	if( appInfo.isFullScreen ) {
		appInfo.windowPosX = 0;
		appInfo.windowPosY = 0;

		// change resolution before the window is created
		//SysSetDisplayMode(width, height, SCRDEPTH);
		//TODO: implement
	}
 
	// at this point WM_CREATE message is sent/received
	// the WM_CREATE branch inside WinProc function will execute here
	appInfo.hwnd = CreateWindowEx(0, WindowName, "Wet Clay App", WS_BORDER, appInfo.windowPosX, appInfo.windowPosY, 
		SCREEN_WIDTH, SCREEN_HEIGHT, NULL, NULL, appInfo.appInstance, NULL);

	GetClientRect( appInfo.hwnd, appInfo.windowRect );

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

	int letWindowsChooseThisPixelFormat;
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

		if(appInfo.running) {
			appInfo.running = Update( &gameSlab );
			Render( &gameSlab );
			SwapBuffers( appInfo.deviceContext );
		}

		endTime = GetTickCount();
		DWORD computeTime = endTime - startTime;
		if(computeTime <= appInfo.mSecsPerFrame ) {
			Sleep(appInfo.mSecsPerFrame - computeTime);
		} else {
			printf("Didn't sleep, compute was %ld, target: %ld \n", computeTime, appInfo.mSecsPerFrame );
		}

	} while( appInfo.running );

	printf("Exitting\n");

	FreeConsole();

	return Msg.wParam;
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam ) {
	switch (uMsg) {
		case WM_CLOSE: {
			appInfo.running = false;
			wglMakeCurrent( appInfo.deviceContext, NULL );
			wglDeleteContext( appInfo.openglRenderContext );
			ReleaseDC( appInfo.hwnd, appInfo.deviceContext );
			DestroyWindow( appInfo.hwnd );
			break;
		}

		case WM_DESTROY: {
			appInfo.running = false;
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

/*----------------------------------------------------------------------------------------
                         App.h function prototype implementations
-----------------------------------------------------------------------------------------*/

void GetMousePosition( float* x, float * y ) {
	POINT mousePosition;
	GetCursorPos( &mousePosition );
	*x = ((float)(mousePosition.x - appInfo.windowPosX)) / ( (float)SCREEN_WIDTH / 2.0f ) - 1.0f;
	*y = (((float)(mousePosition.y - appInfo.windowPosY)) / ( (float)SCREEN_HEIGHT / 2.0f ) - 1.0f) * -1.0f;
}

/*----------------------------------------------------------------------------------------
                       Renderer.h function prototype implementations
-----------------------------------------------------------------------------------------*/

int16 ReadShaderSrcFileFromDisk(const char* fileName, char* buffer, uint16 bufferLen) {
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

void LoadMeshData( const char* fileName, MeshDataStorage* storage ) {
	tinyxml2::XMLDocument colladaDoc;
	colladaDoc.LoadFile( fileName );

	tinyxml2::XMLElement* meshNode = colladaDoc.FirstChildElement( "COLLADA" )->FirstChildElement( "library_geometries" )
	->FirstChildElement( "geometry" )->FirstChildElement( "mesh" );

	auto TextToNumberConversion = [] ( char* textIn, float* numbersOut ) {
		char* start;
		char* end; 
		start = textIn; 
		end = start;
		uint16 index = 0;
		do{
			do {
				end++;
			} while( *end != ' ' && *end != 0 );

			char buffer [16];
			memcpy( &buffer[0], start, end - start);
			buffer[end - start] = 0;
			start = end;

			numbersOut[index] = atof( buffer );
			index++;
		} while( *end != 0 );
	};

	int count;
	char colladaTextBuffer [1024];
	uint16 vCount = 0;
	uint16 nCount = 0;
	uint16 uvCount = 0;
	uint16 indexCount = 0;
	///TODO: use stack allocation for these to reduce uneeded overhead or support huge files
	float rawColladaVertexData[1024];
	float rawColladaNormalData[1024];
	float rawColladaUVData[1024];
	float rawIndexData[1024];

	///TODO: do more error handling on this, rather than all hard-codey expecting where data is and by what name
	///and things like that
	{
	    //Vertex p data
		tinyxml2::XMLNode* colladaVertexArray = meshNode->FirstChild();
		colladaVertexArray->FirstChildElement( "float_array" )->QueryAttribute( "count", &count );
		vCount = count / 3;
		strcpy( &colladaTextBuffer[0], colladaVertexArray->FirstChild()->FirstChild()->ToText()->Value() );
		TextToNumberConversion( (char*)&colladaTextBuffer[0], (float*)&rawColladaVertexData );
		memset( &colladaTextBuffer[0], 0, sizeof(char) * 1024 );

	    //Normals data
		tinyxml2::XMLNode* colladaNormalArray = colladaVertexArray->NextSibling();
		colladaNormalArray->FirstChildElement( "float_array" )->QueryAttribute( "count", &count );
		nCount = count / 3;
		strcpy( &colladaTextBuffer[0], colladaNormalArray->FirstChild()->FirstChild()->ToText()->Value() );
		TextToNumberConversion( (char*)&colladaTextBuffer[0], (float*)&rawColladaNormalData );
		memset( &colladaTextBuffer[0], 0, sizeof(char) * 1024 );

	    //UV map data
		tinyxml2::XMLNode* colladaUVMapArray = colladaNormalArray->NextSibling();
		colladaUVMapArray->FirstChildElement( "float_array" )->QueryAttribute( "count", &count );
		uvCount = count / 2;
		strcpy( &colladaTextBuffer[0], colladaUVMapArray->FirstChild()->FirstChild()->ToText()->Value() );
		TextToNumberConversion( (char*)&colladaTextBuffer[0], (float*)&rawColladaUVData );
		memset( &colladaTextBuffer[0], 0, sizeof(char) * 1024 );

	    //Reading index data
		tinyxml2::XMLElement* meshSrc = meshNode->FirstChildElement( "polylist" );
		meshSrc->QueryAttribute( "count", &count );
		indexCount = count * 3 * 3;
		tinyxml2::XMLElement* colladaIndexArray = meshSrc->FirstChildElement( "p" );
		strcpy( &colladaTextBuffer[0], colladaIndexArray->FirstChild()->ToText()->Value() );
		TextToNumberConversion( (char*)&colladaTextBuffer[0], (float*)&rawIndexData );
	}
	
	storage->dataCount = 0;
	uint16 counter = 0;
	while( counter < indexCount ) {
		Vec3 v, n;
		float uv_x, uv_y;

		uint16 vertIndex = rawIndexData[ counter++ ];
		uint16 normalIndex = rawIndexData[ counter++ ];
		uint16 uvIndex = rawIndexData[ counter++ ];

		v.x = rawColladaVertexData[ vertIndex * 3 + 0 ];
		v.z = -rawColladaVertexData[ vertIndex * 3 + 1 ];
		v.y = rawColladaVertexData[ vertIndex * 3 + 2 ];

		n.x = rawColladaNormalData[ normalIndex * 3 + 0 ];
		n.z = -rawColladaNormalData[ normalIndex * 3 + 1 ];
		n.y = rawColladaNormalData[ normalIndex * 3 + 2 ];

		uv_x = rawColladaUVData[ uvIndex * 2 ];
		uv_y = rawColladaUVData[ uvIndex * 2 + 1 ];

		///TODO: check for exact copies of data, use to index to first instance instead
		uint32 storageIndex = storage->dataCount;
		storage->vData[ storageIndex ] = v;
		storage->normalData[ storageIndex ] = n;
		storage->uvData[ storageIndex * 2 ] = uv_x;
		storage->uvData[ storageIndex * 2 + 1 ] = uv_y;
		storage->iData[ storageIndex ] = storageIndex;
		storage->dataCount++;
	};
}