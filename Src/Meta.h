#define meta(...)

#if 1

struct Transform3D;
struct Static3DMesh;
struct MeshAssetList;
#include <time.h>

void Serialize( MeshAssetList* s, FILE* f = NULL );

void Deserialize( MeshAssetList* s, FILE* f = NULL, FileSys* fileSys = NULL );

struct MVCue;
struct Scene;
#endif