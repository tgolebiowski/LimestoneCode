#include <stdio.h>
#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <functional>
#include "tinyxml2/tinyxml2.h"
#include "tinyxml2/tinyxml2.cpp"

#define GLEW_STATIC
#include "OpenGL/glew.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb/stb_image.h"

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

void GetMousePosition( float* x, float* y ) {
	POINT mousePosition;
	GetCursorPos( &mousePosition );
	*x = ((float)(mousePosition.x - appInfo.windowPosX)) / ( (float)SCREEN_WIDTH / 2.0f ) - 1.0f;
	*y = (((float)(mousePosition.y - appInfo.windowPosY)) / ( (float)SCREEN_HEIGHT / 2.0f ) - 1.0f) * -1.0f;
}

bool IsKeyDown( uint8 keyChar ) {
	if( keyChar >= 'a' && keyChar <= 'z' ) {
		keyChar -= ( 'a' - 'A');
	}
	short keystate = GetAsyncKeyState( keyChar );

	return ( 1 << 16 ) & keystate;
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

void TextToNumberConversion( char* textIn, float* numbersOut ) {
	char* start;
	char* end; 
	start = textIn; 
	end = start;
	uint16 index = 0;
	do {
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

void LoadMeshDataFromDisk( const char* fileName, MeshGeometryData* storage ) {
	tinyxml2::XMLDocument colladaDoc;
	colladaDoc.LoadFile( fileName );
	//if( colladaDoc == NULL ) {
		//printf( "Could not load mesh: %s\n", fileName );
		//return;
	//}

	tinyxml2::XMLElement* meshNode = colladaDoc.FirstChildElement( "COLLADA" )->FirstChildElement( "library_geometries" )
	->FirstChildElement( "geometry" )->FirstChildElement( "mesh" );

	char* colladaTextBuffer = NULL;
	size_t textBufferLen = 0;

	uint16 vCount = 0;
	uint16 nCount = 0;
	uint16 uvCount = 0;
	uint16 indexCount = 0;
	float* rawColladaVertexData;
	float* rawColladaNormalData;
	float* rawColladaUVData;
	float* rawIndexData;
	///Basic Mesh Geometry Data
	{
		tinyxml2::XMLNode* colladaVertexArray = meshNode->FirstChildElement( "source" );
		tinyxml2::XMLElement* vertexFloatArray = colladaVertexArray->FirstChildElement( "float_array" );
		tinyxml2::XMLNode* colladaNormalArray = colladaVertexArray->NextSibling();
		tinyxml2::XMLElement* normalFloatArray = colladaNormalArray->FirstChildElement( "float_array" );
		tinyxml2::XMLNode* colladaUVMapArray = colladaNormalArray->NextSibling();
		tinyxml2::XMLElement* uvMapFloatArray = colladaUVMapArray->FirstChildElement( "float_array" );
		tinyxml2::XMLElement* meshSrc = meshNode->FirstChildElement( "polylist" );
		tinyxml2::XMLElement* colladaIndexArray = meshSrc->FirstChildElement( "p" );

		int count;
		const char* colladaVertArrayVal = vertexFloatArray->FirstChild()->Value();
		vertexFloatArray->QueryAttribute( "count", &count );
		vCount = count;
		const char* colladaNormArrayVal = normalFloatArray->FirstChild()->Value();
		normalFloatArray->QueryAttribute( "count", &count );
		nCount = count;
		const char* colladaUVMapArrayVal = uvMapFloatArray->FirstChild()->Value();
		uvMapFloatArray->QueryAttribute( "count", &count );
		uvCount = count;
		const char* colladaIndexArrayVal = colladaIndexArray->FirstChild()->Value();
		meshSrc->QueryAttribute( "count", &count );
		//Assume this is already triangulated
		indexCount = count * 3 * 3;

		///TODO: replace this with fmaxf?
		std::function< size_t (size_t, size_t) > sizeComparison = []( size_t size1, size_t size2 ) -> size_t {
			if( size1 >= size2 ) return size1;
			return size2;
		};

		textBufferLen = strlen( colladaVertArrayVal );
		textBufferLen = sizeComparison( strlen( colladaNormArrayVal ), textBufferLen );
		textBufferLen = sizeComparison( strlen( colladaUVMapArrayVal ), textBufferLen );
		textBufferLen = sizeComparison( strlen( colladaIndexArrayVal ), textBufferLen );
		colladaTextBuffer = (char*)alloca( textBufferLen );
		memset( colladaTextBuffer, 0, textBufferLen );
		rawColladaVertexData = (float*)alloca( sizeof(float) * vCount );
		rawColladaNormalData = (float*)alloca( sizeof(float) * nCount );
		rawColladaUVData = (float*)alloca( sizeof(float) * uvCount );
		rawIndexData = (float*)alloca( sizeof(float) * indexCount );

		//Reading Vertex position data
		strcpy( colladaTextBuffer, colladaVertArrayVal );
		rawColladaVertexData = (float*)alloca( sizeof(float) * vCount );
		TextToNumberConversion( colladaTextBuffer, rawColladaVertexData );

	    //Reading Normals data
		memset( colladaTextBuffer, 0, textBufferLen );
		strcpy( colladaTextBuffer, colladaNormArrayVal );
		rawColladaNormalData = (float*)alloca( sizeof(float) * nCount );
		TextToNumberConversion( colladaTextBuffer, rawColladaNormalData );

	    //Reading UV map data
		memset( colladaTextBuffer, 0, textBufferLen );
		strcpy( colladaTextBuffer, colladaUVMapArrayVal );
		rawColladaUVData = (float*)alloca( sizeof(float) * uvCount );
		TextToNumberConversion( colladaTextBuffer, rawColladaUVData );

	    //Reading index data
		memset( colladaTextBuffer, 0, textBufferLen );
		strcpy( colladaTextBuffer, colladaIndexArrayVal );
		rawIndexData = (float*)alloca( sizeof(float) * indexCount );
		TextToNumberConversion( colladaTextBuffer, rawIndexData );
	}

	float* boneWeightData = NULL;
	uint8* boneIndexData = NULL;
	//Skinning Data
	{
		tinyxml2::XMLElement* libControllers = colladaDoc.FirstChildElement( "library_controllers" );
		if( libControllers == NULL ) goto skinningExit;
		tinyxml2::XMLElement* controllerElement = libControllers->FirstChildElement( "controllers" );
		if( controllerElement == NULL ) goto skinningExit;


	}
	skinningExit:
	
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

void LoadMeshSkinningDataFromDisk( const char* fileName, Armature* armature ) {
	tinyxml2::XMLDocument colladaDoc;
	colladaDoc.LoadFile( fileName );

	//Armature
	{
		tinyxml2::XMLElement* visualScenesNode = colladaDoc.FirstChildElement( "COLLADA" )->FirstChildElement( "library_visual_scenes" )
		->FirstChildElement( "visual_scene" )->FirstChildElement( "node" );
		tinyxml2::XMLElement* armatureNode = NULL;

		while( visualScenesNode != NULL ) {
			if( visualScenesNode->FirstChildElement( "node" ) != NULL && 
				visualScenesNode->FirstChildElement( "node" )->Attribute( "type", "JOINT" ) != NULL ) {
				armatureNode = visualScenesNode;
			    break;
			} else {
				visualScenesNode = visualScenesNode->NextSibling()->ToElement();
			}
		}
		if( armatureNode == NULL ) return;

		uint8 stackTracker = 0;
		//Note: the same index for bone and sibling stack is like this:
		//boneStack[ i ] = parent
		//siblingStack[ i ] next sibling to branch from, whose parent bone is "parent"
		Bone* boneStack[ MAXBONES ];
		tinyxml2::XMLElement* siblingStack[ MAXBONES ];

		//TODO: clean up into one big thing that uses recursion
		//TODO: combine parsing recursive function, and bind pose thing into one thing
		//TODO: recursive lambda rather than messy stack stuff
		//Parsing basic data from XML
		armature->boneCount = 0;
		tinyxml2::XMLElement* boneElement = armatureNode->FirstChildElement( "node" ); //->FirstChildElement( "node" );
		while( boneElement != NULL ) {
			Bone* armatureBone = &armature->allBones[ armature->boneCount ];
			armatureBone->childCount = 0;
			armatureBone->currentTransform = &armature->boneTransforms[ armature->boneCount ];
			if( stackTracker > 0) { 
				Bone* parentBone = boneStack[ stackTracker - 1 ];
				armatureBone->parent = parentBone;
				parentBone->children[ parentBone->childCount ] = armatureBone;
				parentBone->childCount++;
			}
			else { 
				armatureBone->parent = NULL; 
				armature->rootBone = armatureBone;
			}
			armature->boneCount++;

			strcpy( &armatureBone->name[0], boneElement->Attribute( "name" ) );

			float matrixData[16];
			char matrixTextData [512];
			tinyxml2::XMLNode* matrixElement = boneElement->FirstChildElement("matrix");
			strcpy( &matrixTextData[0], matrixElement->FirstChild()->ToText()->Value() );

			TextToNumberConversion( matrixTextData, matrixData );
			//Note: this is only local transform data, but its being saved in inverse bind pose matrix for now
			Mat4 m;
			memcpy( &m.m[0][0], &matrixData[0], sizeof(float) * 16 );
			armatureBone->inverseBindPose = TransposeMatrix( m );

			if( boneElement->FirstChildElement( "node" ) != NULL ) {
				tinyxml2::XMLElement* nextElement = boneElement->FirstChildElement( "node" );
				boneStack[ stackTracker ] = armatureBone;
				siblingStack[ stackTracker ] = nextElement;
				stackTracker++;
				boneElement = nextElement;
			} else {
				while( stackTracker > 0 ) {
					tinyxml2::XMLElement* prev = siblingStack[ stackTracker - 1 ];
					if( prev->NextSibling() != NULL ) {
						boneElement = prev->NextSibling()->ToElement();
						siblingStack[ stackTracker - 1 ] = boneElement;
						break;
					} else {
						stackTracker--;
					}
				};
				if( stackTracker == 0 ) {
					boneElement = NULL;
				}
			}
		};

		//Calculating Bind Pose matricies
		std::function< void ( Bone*, Mat4 ) > SetBindMatrix = [&]( Bone* bone, Mat4 parentBindPoseMatrix ) {
			//This is taking the local transform to the bind pose
			bone->inverseBindPose = MultMatrix( parentBindPoseMatrix, bone->inverseBindPose );

			for( uint8 childIndex = 0; childIndex < bone->childCount; childIndex++ ) {
				SetBindMatrix( bone->children[ childIndex ], bone->inverseBindPose );
			}
		};
		Mat4 i; SetToIdentity( &i );
		SetBindMatrix( armature->rootBone, i );

		//Actually invert matricies so they are finally inverse-bind-pose
		for( uint8 boneIndex = 0; boneIndex < armature->boneCount; boneIndex++ ) {
			Bone* bone = &armature->allBones[ boneIndex ];
			bone->inverseBindPose = InverseMatrix( bone->inverseBindPose );
		}
	}
}

void LoadTextureDataFromDisk( const char* fileName, TextureData* storage ) {
    //unsigned char* data = stbi_load( fileName, &texData->width, &texData->height, &n, 0 );
    storage->texData = (uint8*)stbi_load( fileName, (int*)&storage->width, (int*)&storage->height, (int*)&storage->channelsPerPixel, 0 );
    if( storage->texData == NULL ) {
        printf( "Could not load file: %s\n", fileName );
    }
    printf( "Loaded file: %s\n", fileName );
    printf( "Width: %d, Height: %d, Channel count: %d\n", storage->width, storage->height, storage->channelsPerPixel );
}