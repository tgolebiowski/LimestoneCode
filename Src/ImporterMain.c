#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "assimp/cimport.h"               // Assimp Plain-C interface
#include "assimp/scene.h"                 // Assimp Output data structure
#include "assimp/postprocess.h"           // Assimp Post processing flags

#include "Mesh.c"

int main(int argc, char* args[]) {
	// Start the import on the given file with some example postprocessing
    // Usually - if speed is not the most important aspect for you - you'll t
    // probably to request more postprocessing than we do in this example.
    const char* fileName = "Data/Pointy.fbx";
    unsigned int myFlags = 
    aiProcess_CalcTangentSpace         | // calculate tangents and bitangents if possible
	aiProcess_JoinIdenticalVertices    | // join identical vertices/ optimize indexing
    //aiProcess_ValidateDataStructure  | // perform a full validation of the loader's output
    aiProcess_Triangulate              | // Ensure all verticies are triangulated (each 3 vertices are triangle)
    aiProcess_ConvertToLeftHanded      | // convert everything to D3D left handed space (by default right-handed, for OpenGL)
	aiProcess_SortByPType              | // ?
	aiProcess_ImproveCacheLocality     | // improve the cache locality of the output vertices
	aiProcess_RemoveRedundantMaterials | // remove redundant materials
	aiProcess_FindDegenerates          | // remove degenerated polygons from the import
	aiProcess_FindInvalidData          | // detect invalid model data, such as invalid normal vectors
	aiProcess_GenUVCoords              | // convert spherical, cylindrical, box and planar mapping to proper UVs
	aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
	aiProcess_FindInstances            | // search for instanced meshes and remove them by references to one master
	aiProcess_LimitBoneWeights         | // limit bone weights to 4 per vertex
	aiProcess_OptimizeMeshes           | // join small meshes, if possible;
	aiProcess_SplitByBoneCount         | // split meshes with too many bones. Necessary for our (limited) hardware skinning shader
	0;

    const struct aiScene* scene = aiImportFile( fileName, myFlags );

    // If the import failed, report it
    if( !scene) {
        //DoTheErrorLogging( aiGetErrorString());
        printf( "Failed to import \n" );
        return false;
    }
    // Now we can access the file's contents
    printf( "Yay, loaded file: %s\n", fileName );
    printf( "%d Meshes in file\n", scene->mNumMeshes);
    for(uint8_t i = 0; i < scene->mNumMeshes; i++) {
    	const struct aiMesh* mesh = scene->mMeshes[i];
     	printf("Mesh %d: %d Vertices, %d Faces\n", i, mesh->mNumVertices, mesh->mNumFaces);

     	
    }

    // We're done. Release all resources associated with this import
    aiReleaseImport( scene);
    return true;
}

void LoadMesh(char* fileName) {

}