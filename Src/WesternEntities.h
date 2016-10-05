#include "Meta.h"
#include "MetaLib.h"

meta("Editable")
struct Transform3D {
	Vec3 p;
	Quat q;
};

struct Static3DMesh {
	PtrToGpuMem vertexPosData;
	PtrToGpuMem vertexNormalData;
	uint32 vertexCount;
};

int LoadStaticMesh(
	char* file, 
	Stack* allocater, 
	Stack* transientMemory,
	ShaderProgram* meshShader,
	RenderDriver* renderDriver,
	FileSys* fileSys,
	Static3DMesh* loadSpace
) {
	Static3DMesh combinedData;

	void* data = fileSys->ReadWholeFile( file, transientMemory );
	MeshGeometryData meshData = { };
	bool colladaLoadSuccessful = renderDriver->ParseMeshDataFromCollada( 
		data, 
		transientMemory, 
		&meshData,
		NULL
	);
	FreeFromStack( transientMemory, data );

	if( !colladaLoadSuccessful ) {
		return -1;
	}

	combinedData.vertexPosData = renderDriver->CopyVertexDataToGpuMem(  
		renderDriver,
		meshData.vData,
		sizeof(float) * 3 * meshData.dataCount
	);
	combinedData.vertexNormalData = renderDriver->CopyVertexDataToGpuMem(
		renderDriver,
		meshData.vData,
		sizeof(float) * 3 * meshData.dataCount
	);

	combinedData.vertexCount = meshData.dataCount;
	*loadSpace = combinedData;

	FreeFromStack( transientMemory, meshData.vData );

	return 1;
}

void RenderStaticMesh( 
	Static3DMesh* mesh, 
	Mat4* cameraMatrix, 
	Mat4* modelMatrix, 
	ShaderProgram* meshShader,
	RenderDriver* renderDriver
) {
	RenderCommand command = { };
	command.shader = meshShader;
	command.elementCount = mesh->vertexCount;

	SetVertexInput( &command, "position", mesh->vertexPosData );
	SetVertexInput( &command, "normal", mesh->vertexNormalData );
	SetUniform( &command, "modelMatrix", (void*)modelMatrix );
	SetUniform( &command, "cameraMatrix", (void*)cameraMatrix );
	renderDriver->DrawMesh( 
		renderDriver, 
		&command 
	);
}

meta("Serializable")
struct MeshAssetList {
	#define MESHASSET_MAXMESHES 64
	#define MESHASSET_NAMBUFLEN 1024
	uint8 meshCount;
	uint16 nextCharIndex;

	meta( "Array-1024" )
	char* stringBuffer;
	meta( "Array-64", "PtrToBuffer-stringBuffer" )
	char** lookupNames;
	meta( "Array-64", "PtrToBuffer-stringBuffer" )
	char** fileNames;
	meta( "Array-64", "Ignore" )
	Static3DMesh* loadedMeshes;
};

static bool AddMeshToList( 
	MeshAssetList* list, 
	char* fileName, 
	char* lookupName, 
	Stack* allocater,
	Stack* transientMemory,
	ShaderProgram* meshShader,
	RenderDriver* renderDriver,
	FileSys* fileSys
) {
	size_t fileNameLen = strlen( fileName );
	size_t lookupNameLen = strlen( lookupName );

	char* storedLookupName = NULL;
	char* storedFileName = NULL;

	storedLookupName = &list->stringBuffer[ list->nextCharIndex ];
	strcpy( &list->stringBuffer[ list->nextCharIndex ], lookupName );
	list->nextCharIndex += ( lookupNameLen + 1 );
	storedFileName = &list->stringBuffer[ list->nextCharIndex ];
	strcpy( &list->stringBuffer[ list->nextCharIndex ], fileName );
	list->nextCharIndex += ( fileNameLen + 1 );

	int meshLoadReturn = LoadStaticMesh( 
		fileName, 
		allocater,
		transientMemory,
		meshShader,
		renderDriver,
		fileSys,
		&list->loadedMeshes[ list->meshCount ]
	);
	if( meshLoadReturn == 1 ) {
		list->lookupNames[ list->meshCount ] = storedLookupName;
		list->fileNames[ list->meshCount++ ] = storedFileName;
	} else {
		return false;
	}

	return true;
}

meta("Editable")
struct MVCue {
	float t;
	Vec3 pos;
	Quat rot;
};

static Static3DMesh* GetMeshByName( MeshAssetList* list, char* name ) {
	for( int meshIndex = 0; meshIndex < list->meshCount; ++meshIndex ) {
		if( strcmp( name, list->lookupNames[ meshIndex ] ) == 0 ) {
			return &list->loadedMeshes[ meshIndex ];
		}
	}

	return NULL;
}

struct Scene {
	MVCue cameraTrack [64];
};

#include "Meta.cpp"