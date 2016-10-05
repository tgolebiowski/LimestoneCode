#if 1

static StructIntrospectData Transform3DIntroData = {
	2,
	{
		{ 0, { }, "p", offsetof( Transform3D, p ), sizeof( Vec3 ), 0 },
		{ 0, { }, "q", offsetof( Transform3D, q ), sizeof( Quat ), 0 }
	}, 
	1,
	{ "Editable" }
};
StructIntrospectData* getIntrospectionData( Transform3D* s ) {
	return &Transform3DIntroData;
}

void ImGuiEdit( Transform3D* s ) {
	ImGui::InputFloat3( "p",(float*)&s->p );
	ImGui::InputFloat4( "q",(float*)&s->q );
}
static StructIntrospectData MeshAssetListIntroData = {
	6,
	{
		{ 0, { }, "meshCount", offsetof( MeshAssetList, meshCount ), sizeof( uint8 ), 0 },
		{ 0, { }, "nextCharIndex", offsetof( MeshAssetList, nextCharIndex ), sizeof( uint16 ), 0 },
		{ 1, { "Array-1024" }, "stringBuffer", offsetof( MeshAssetList, stringBuffer ), sizeof( char ), 1 },
		{ 2, { "Array-64", "PtrToBuffer-stringBuffer" }, "lookupNames", offsetof( MeshAssetList, lookupNames ), sizeof( char* ), 2 },
		{ 2, { "Array-64", "PtrToBuffer-stringBuffer" }, "fileNames", offsetof( MeshAssetList, fileNames ), sizeof( char* ), 2 },
		{ 2, { "Array-64", "Ignore" }, "loadedMeshes", offsetof( MeshAssetList, loadedMeshes ), sizeof( Static3DMesh ), 1 }
	}, 
	1,
	{ "Serializable" }
};
StructIntrospectData* getIntrospectionData( MeshAssetList* s ) {
	return &MeshAssetListIntroData;
}
void Serialize( MeshAssetList* s, FILE* f ) {
	StructIntrospectData* introData = getIntrospectionData( s );

	if( !f ) {
		char* baseFileName = "MeshAssetList";
		time_t t;
		time( &t );
		struct tm* current = localtime( &t );
		char fileName[128] = { };
		int writeIndex = sprintf( &fileName[0], "%s--", baseFileName );
		writeIndex += sprintf( &fileName[ writeIndex ], "%d_%d-%d-%d", 
			current->tm_mday, current->tm_hour, current->tm_min, current->tm_sec );
		sprintf( &fileName[ writeIndex ], ".save" );
		f = fopen( fileName, "w" );
	}

	SerializeStruct( (void*)s, introData, f );
	fclose( f );
}
void Deserialize( MeshAssetList* s, FILE* f, FileSys* fileSys ) {
	StructIntrospectData* introData = getIntrospectionData( s );
	if( !f ){
		char* structAsString = "MeshAssetList";
		int searchReturn = 0;
		char targetFile [128] = { };
		searchReturn = fileSys->GetMostRecentMatchingFile( structAsString, targetFile );
		if( searchReturn != 0 ) { 
			return;
		}
		f = fopen( targetFile, "r" );
	}
	DeserializeStruct( (void*)s, introData, f );
	fclose( f );
}

static StructIntrospectData MVCueIntroData = {
	3,
	{
		{ 0, { }, "t", offsetof( MVCue, t ), sizeof( float ), 0 },
		{ 0, { }, "pos", offsetof( MVCue, pos ), sizeof( Vec3 ), 0 },
		{ 0, { }, "rot", offsetof( MVCue, rot ), sizeof( Quat ), 0 }
	}, 
	1,
	{ "Editable" }
};
StructIntrospectData* getIntrospectionData( MVCue* s ) {
	return &MVCueIntroData;
}

void ImGuiEdit( MVCue* s ) {
	ImGui::InputFloat( "t",(float*)&s->t );
	ImGui::InputFloat3( "pos",(float*)&s->pos );
	ImGui::InputFloat4( "rot",(float*)&s->rot );
}
#endif