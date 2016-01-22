#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "Math3D.c"

#include "SDL/SDL.h"
#define GLEW_STATIC
#include "OpenGL/glew.h"
#include "SDL/SDL_opengl.h"
#include "OpenGL/glut.h"

#include "assimp/cimport.h"               // Assimp Plain-C interface
#include "assimp/scene.h"                 // Assimp Output data structure
#include "assimp/postprocess.h"           // Assimp Post processing flags

    // const char* fileName = "Data/Tri_Scene.fbx";
    // unsigned int myFlags = 
    // aiProcess_CalcTangentSpace         | // calculate tangents and bitangents if possible
    // aiProcess_JoinIdenticalVertices    | // join identical vertices/ optimize indexing
    // //aiProcess_ValidateDataStructure  | // perform a full validation of the loader's output
    // aiProcess_Triangulate              | // Ensure all verticies are triangulated (each 3 vertices are triangle)
    // //aiProcess_ConvertToLeftHanded      | // convert everything to D3D left handed space (by default right-handed, for OpenGL)
    // aiProcess_SortByPType              | // ?
    // aiProcess_ImproveCacheLocality     | // improve the cache locality of the output vertices
    // aiProcess_RemoveRedundantMaterials | // remove redundant materials
    // aiProcess_FindDegenerates          | // remove degenerated polygons from the import
    // aiProcess_FindInvalidData          | // detect invalid model data, such as invalid normal vectors
    // aiProcess_GenUVCoords              | // convert spherical, cylindrical, box and planar mapping to proper UVs
    // aiProcess_TransformUVCoords        | // preprocess UV transformations (scaling, translation ...)
    // aiProcess_FindInstances            | // search for instanced meshes and remove them by references to one master
    // aiProcess_LimitBoneWeights         | // limit bone weights to 4 per vertex
    // aiProcess_OptimizeMeshes           | // join small meshes, if possible;
    // aiProcess_SplitByBoneCount         | // split meshes with too many bones. Necessary for our (limited) hardware skinning shader
    // 0;

    // const struct aiScene* scene = aiImportFile( fileName, myFlags );

    // // If the import failed, report it
    // if( !scene) {
    //     //DoTheErrorLogging( aiGetErrorString());
    //     printf( "Failed to import \n" );
    //     return;// false;
    // }
    // // Now we can access the file's contents
    // printf( "Yay, loaded file: %s\n", fileName );
    // printf( "%d Meshes in file\n", scene->mNumMeshes);
    // const struct aiMesh* aimesh = scene->mMeshes[0];
    // uint16_t vertCount = aimesh->mNumVertices;
    // float vertData[vertCount * 3];
    // for(uint16_t i = 0; i < vertCount; i++) {
    //     vertData[i * 3 + 0] = aimesh->mVertices[i].x * 20.0f;
    //     vertData[i * 3 + 1] = aimesh->mVertices[i].z * 20.0f;
    //     vertData[i * 3 + 2] = aimesh->mVertices[i].y * 20.0f;
    //     printf("Vertex: %f, %f, %f\n", vertData[i * 3 + 0], vertData[i * 3 + 1], vertData[i * 3 + 2]);
    // }

    // uint16_t intCount = aimesh->mNumFaces * 3;
    // GLuint indData[intCount];
    // for(uint16_t i = 0; i < aimesh->mNumFaces; i++) {
    //     struct aiFace aiface = aimesh->mFaces[i];
    //     indData[i * 3 + 0] = aiface.mIndices[0];
    //     indData[i * 3 + 1] = aiface.mIndices[1];
    //     indData[i * 3 + 2] = aiface.mIndices[2];
    //     printf("Index set: %d, %d, %d\n", aiface.mIndices[0], aiface.mIndices[1], aiface.mIndices[2]);
    // }

    // // We're done. Release all resources associated with this import
    // aiReleaseImport( scene );