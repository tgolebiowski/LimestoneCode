struct WriteState {
	int bufferLength;
	char* buffer;
	int writeIndex;
};

void sprintTokenToIndexedBuffer( char* targetBuffer, int* index, Token* token ) {
	int sprintfReturn = sprintf( 
		&targetBuffer[ *index ],
		"%.*s",
		token->tokenLength,
		token->tokenStart
	);

	*index += sprintfReturn;
}

void sprintConstToIndexedBuffer( char* buffer, int* index, char* string ) {
	int sprintfReturn = sprintf (
		&buffer[ *index ],
		"%s",
		string
	);
	*index += sprintfReturn;
}

void GenerateIntrospectionCode( StructDefinition* d ) {
	char buffer [1024] = { };
	WriteState ws = { 1024, buffer, 0 };
	#define write(...) ws.writeIndex += sprintf( \
	&ws.buffer[ ws.writeIndex ], __VA_ARGS__ );

	write( "\nstatic StructIntrospectData %.*sIntroData = {\n",
		d->typeNameToken->tokenLength, d->typeNameToken->tokenStart
	)
	write( "\t%d,\n", d->memberCount )

	write( "\t{\n")
    for( int mIndex = 0; mIndex < d->memberCount; ++mIndex ) {
    	Member* m = &d->memberDefinitions[ mIndex ];

    	if( mIndex > 0 ) {
    		write( ",\n")
    	}
    	if( m->metaData ) {
    		write( "\t\t{ %d, { ", m->metaData->tagCount )
    		for( int mdIndex = 0; mdIndex < m->metaData->tagCount; ++mdIndex ) {
    			Token* metaTagToken = m->metaData->tagTokens[ mdIndex ];

    			if( mdIndex > 0 ) { write( ", " ) }
    			write( "\"%.*s\"",
    				metaTagToken->tokenLength, metaTagToken->tokenStart
    			)
    		}
    		write( " }, " )
    	} else {
    		write( "\t\t{ 0, { }, ")
    	}

    	write( "\"%.*s\", ",
    		m->memberNameToken->tokenLength, m->memberNameToken->tokenStart
    	)

    	write( "offsetof( %.*s, %.*s ), ",
    		d->typeNameToken->tokenLength, d->typeNameToken->tokenStart,
    		m->memberNameToken->tokenLength, m->memberNameToken->tokenStart
    	)
    	//Member data size
    	write( "sizeof( %.*s",
    		m->typeToken->tokenLength, m->typeToken->tokenStart
    	)
    	for( int ptrLevel = 0; ptrLevel < m->ptrLevel - 1; ++ptrLevel ) {
    		write( "*" )
    	}
    	write( " )" )

    	write( ", %d", m->ptrLevel );
    	write( " }")


    }
    write( "\n\t}, \n")

    if( d->metaData != NULL ) {
    	write( "\t%d,\n", d->metaData->tagCount )
    	write( "\t{ " )
    	for( int mdIndex = 0; mdIndex < d->metaData->tagCount; ++mdIndex ) {
    		Token* mdTagToken = d->metaData->tagTokens[ mdIndex ];
    		if( mdIndex > 0 ) write( ", " )
    		write("\"%.*s\"", mdTagToken->tokenLength, mdTagToken->tokenStart	)
    	}
    	write( " }\n" )
	} else {
		write( "\t0,\n" )
		write( "\t{ }\n" )
	}

    write( "};\n" )

    write( "StructIntrospectData* getIntrospectionData( %.*s* s ) {\n",
    	d->typeNameToken->tokenLength, d->typeNameToken->tokenStart
    )
    write( "\treturn &%.*sIntroData;\n",
    	d->typeNameToken->tokenLength, d->typeNameToken->tokenStart
    )
    write( "}\n" )

	writeConstStringToFile( buffer, outFile_CPP );
}

void GenerateImguiEditor( StructDefinition* d ) {
	char* baseEditorDefinition = "\nvoid ImGuiEdit( ";
	char* baseEditorDefineClose = "* s ) {";
	
	//This section outputs: \n
	//void ImGuiEdit( TYPENAME* s ) {\n
	{
		writeConstStringToFile( baseEditorDefinition, outFile_CPP );
		writeTokenDataToFile( d->typeNameToken, outFile_CPP );
		writeConstStringToFile( baseEditorDefineClose, outFile_CPP );
	}

	for( int memberIndex = 0; memberIndex < d->memberCount; ++memberIndex ) {
		Member* m = &d->memberDefinitions[ memberIndex ];
		char* floatPtrCast = "(float*)";
		char* closeMethodCall = " );";

		char* matchingEditor = NULL;
		char* printType = NULL;
		{
			char* KnownQuatType = "Quat";
			char* KnownVec3Type = "Vec3";
			char* floatPrimitive = "float";
			char* Float3Editor = "\n\tImGui::InputFloat3( ";
			char* Float4Editor = "\n\tImGui::InputFloat4( ";
			char* Float1Editor = "\n\tImGui::InputFloat( ";

			char* tStart = m->typeToken->tokenStart;
	        if( IsStringMatch( tStart, KnownVec3Type ) ) {
	        	matchingEditor = Float3Editor;
	        	printType = KnownVec3Type;
	        } else if( IsStringMatch( tStart, KnownQuatType ) ) {
	        	matchingEditor = Float4Editor;
	        	printType = KnownQuatType;
	        } else if( IsStringMatch( tStart, floatPrimitive ) ) {
	        	matchingEditor = Float1Editor;
	        	printType = floatPrimitive;
	        }
	    }

		if( matchingEditor && printType ) {
			//Method call opening
			writeConstStringToFile( matchingEditor, outFile_CPP );
			//UI display name
			writeConstStringToFile("\"", outFile_CPP );
			writeTokenDataToFile( m->memberNameToken, outFile_CPP );
			writeConstStringToFile("\",", outFile_CPP );
		    //Ptr to data for editor
			writeConstStringToFile( floatPtrCast, outFile_CPP );
			writeConstStringToFile( "&s->", outFile_CPP );
			writeTokenDataToFile( m->memberNameToken, outFile_CPP );

			writeConstStringToFile( closeMethodCall, outFile_CPP );
		}
	}
	writeConstStringToFile( "\n}", outFile_CPP );
}

#if 0
void GenerateSerializationCode( StructDefinition* d ) {
	//Set up write buffers...
	char srlzSrcBuffer [ 1024 ] = { };
	int srlzWriteIndex = 0;
	char desrlzBuffer [ 1024 ] = { };
	int deSrlzWriteIndex = 0;

	#define SrlzWrite(...) srlzWriteIndex += sprintf( \
	&srlzSrcBuffer[ srlzWriteIndex ], __VA_ARGS__ );
	#define DesrlzWrite(...) deSrlzWriteIndex += sprintf( \
	&desrlzBuffer[ deSrlzWriteIndex ], __VA_ARGS__ );

	//Create string for including arguements to pass data from other structs
	char foreignDependencyArgs [128] = { };
	char foreignIntrospectStructs [512] = { };
	char foreignDataPtrs [ 256 ] = { };
	int foreignDependentMembers = 0;
	{
		int dependencyCount = 0;
		int writeIndex = 0;
		int writeIndex2 = 0;
		int writeIndex3 = 0;
		for( int mIndex = 0; mIndex < d->memberCount; ++mIndex ) {
			Member* member = &d->memberDefinitions[ mIndex ];
			MetaTagList* m = member->metaData;
			if( m == NULL ) continue;
			for( int tagIndex = 0; tagIndex < m->tagCount; ++tagIndex ) {
				Token* tagToken = m->tagTokens[ tagIndex ];
				char metaTagCopy [64] = { };
				assert( tagToken->tokenLength < 64 );
				memcpy( metaTagCopy, tagToken->tokenStart, tagToken->tokenLength );
				char* token = strtok( metaTagCopy, "-" );

				//Setting up foreign dependency logic
				if( strcmp( token, "Foreign" ) == 0 ) {
					token = strtok( NULL, "-" );

					//Write to buffer for function arguments
					writeIndex += sprintf(
						&foreignDependencyArgs[ writeIndex ],
						", %s* __%s__", 
						token,
						token 
					);
					++dependencyCount;

					//Write to buffer for foreign introspection struct list
					if( foreignDependentMembers > 1 ) {
						writeIndex2 += sprintf(
							&foreignIntrospectStructs[ writeIndex2 ],
							"\n,"
						);
					}
					writeIndex2 += sprintf(
						&foreignIntrospectStructs[ writeIndex2 ],
						"\t\tgetIntrospectionData( __%s__ )",
						token
					);

					//Write to buffer for foreign struct pointers in void* form
					if( foreignDependentMembers > 1 ) {
						writeIndex3 += sprintf(
							&foreignDataPtrs[ writeIndex3 ], ",\n"
						);
					}
					writeIndex3 += sprintf(
						&foreignDataPtrs[ writeIndex3 ],
						"\t\t(void*)__%s__", token
					);

					++foreignDependentMembers;
				}
			}
		}
	}

	//Write out function definitions, and #include any higher level dependencies
	writeConstStringToFile( "\n#include <time.h>\n", outFile_H );
	writeConstStringToFile( "\nvoid Serialize( ", outFile_H );
	writeTokenDataToFile( d->typeNameToken, outFile_H );
	writeConstStringToFile( "* s", outFile_H );
	writeConstStringToFile( foreignDependencyArgs, outFile_H );
	writeConstStringToFile( ", FILE* f = NULL );\n", outFile_H );

	writeConstStringToFile( "\nvoid Deserialize( ", outFile_H );
	writeTokenDataToFile( d->typeNameToken, outFile_H );
	writeConstStringToFile( "* s", outFile_H );
	writeConstStringToFile( foreignDependencyArgs, outFile_H );
	writeConstStringToFile(
		", FILE* f = NULL, FileSys* fileSys = NULL );\n",
	    outFile_H
	);

	//Write Serialze logic
	SrlzWrite( "void Serialize( %.*s* s%s, FILE* f ) {\n",
		d->typeNameToken->tokenLength, d->typeNameToken->tokenStart,
		foreignDependencyArgs
	)
	SrlzWrite( "\tStructIntrospectData* introData = getIntrospectionData( s );\n\n" )
	SrlzWrite( "\tif( !f ) {\n" )
	SrlzWrite( "\t\tchar* baseFileName = \"%.*s\";\n", 
		d->typeNameToken->tokenLength, d->typeNameToken->tokenStart 
	)
	const char* versioningLogic = 
	"\t\ttime_t t;\n"
	"\t\ttime( &t );\n"
	"\t\tstruct tm* current = localtime( &t );\n"
	"\t\tchar fileName[128] = { };\n"
	"\t\tint writeIndex = sprintf( &fileName[0], \"%s--\", baseFileName );\n"
	"\t\twriteIndex += sprintf( &fileName[ writeIndex ], \"%d_%d-%d-%d\", \n"
	"\t\t\tcurrent->tm_mday, current->tm_hour, current->tm_min, current->tm_sec );\n"
	"\t\tsprintf( &fileName[ writeIndex ], \".save\" );\n";
	SrlzWrite( "%s", versioningLogic )
	SrlzWrite( "\t\tf = fopen( fileName, \"w\" );\n",
		d->typeNameToken->tokenLength, d->typeNameToken->tokenStart 
	)
	SrlzWrite( "\t}\n\n" )

	if( foreignDependentMembers > 0 ) {
		SrlzWrite( "\tStructIntrospectData* foreignIntrospectStructs [%d] = { \n",
			foreignDependentMembers
		)
		SrlzWrite( "%s", foreignIntrospectStructs )
		SrlzWrite( "\n\t};\n" )

		SrlzWrite( "\tvoid* foreignDataPtrs [%d] = { \n",
			foreignDependentMembers
		)
		SrlzWrite( "%s", foreignDataPtrs )
		SrlzWrite( "\n\t};\n" )

		SrlzWrite( "\n" )
		SrlzWrite( "\tSerializeStruct( (void*)s, introData, f, foreignIntrospectStructs, foreignDataPtrs );\n" )
	} else {
		SrlzWrite( "\tSerializeStruct( (void*)s, introData, f );\n" )
	}

	SrlzWrite( "\tfclose( f );\n" )
	SrlzWrite( "}\n" )

	//Write Deserialize logic
	DesrlzWrite( "void Deserialize( %.*s* s%s, FILE* f, FileSys* fileSys ) {\n",
		d->typeNameToken->tokenLength, d->typeNameToken->tokenStart,
		foreignDependencyArgs
	)
	DesrlzWrite( "\tStructIntrospectData* introData = getIntrospectionData( s );\n" )
	DesrlzWrite( "\tif( !f ){\n" )
	DesrlzWrite( "\t\tchar* structAsString = \"%.*s\";\n",
		d->typeNameToken->tokenLength, d->typeNameToken->tokenStart
	)
	const char* desrlzVersioningLogic =
	"\t\tint searchReturn = 0;\n"
	"\t\tchar targetFile [128] = { };\n"
	"\t\tsearchReturn = fileSys->GetMostRecentMatchingFile( structAsString, targetFile );\n"
	"\t\tif( searchReturn != 0 ) { \n"
	"\t\t\treturn;\n"
	"\t\t}\n";
	DesrlzWrite( "%s", desrlzVersioningLogic )
	DesrlzWrite( "\t\tf = fopen( targetFile, \"r\" );\n" )
	DesrlzWrite( "\t}\n" )
	if( foreignDependentMembers > 0 ) {
		DesrlzWrite( "\tStructIntrospectData* foreignIntrospectStructs [%d] = { \n",
			foreignDependentMembers
		)
		DesrlzWrite( "%s", foreignIntrospectStructs )
		DesrlzWrite( "\n\t};\n" )

		DesrlzWrite( "\tvoid* foreignDataPtrs [%d] = { \n",
			foreignDependentMembers
		)
		DesrlzWrite( "%s", foreignDataPtrs )
		DesrlzWrite( "\n\t};\n" )

		DesrlzWrite( "\n" )
		DesrlzWrite( "\tDeserializeStruct( (void*)s, introData, f, foreignIntrospectStructs, foreignDataPtrs );\n" )
	} else {
		DesrlzWrite( "\tDeserializeStruct( (void*)s, introData, f );\n")
	}
	DesrlzWrite( "\tfclose( f );\n" )
	DesrlzWrite( "}\n" )

	writeConstStringToFile( srlzSrcBuffer, outFile_CPP );
	writeConstStringToFile( desrlzBuffer, outFile_CPP );
}
#endif