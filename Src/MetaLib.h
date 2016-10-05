#undef offsetof
#define offsetof(st, m) ((size_t)&(((st *)0)->m))

struct MemberIntrospectData {
	int metaDataCount;
	char* metaData [10];
	char* memberName;
	int offset;
	size_t typeSize;
	int ptrLevel;
};

struct StructIntrospectData {
	int memberCount;
	MemberIntrospectData members [10];
	int structMetaDataTagCount;
	char* metaData [10];
};

MemberIntrospectData* memberLookup( StructIntrospectData* s, char* lookupName ) {
	for( int i = 0; i < s->memberCount; ++i ) {
		if( strcmp( s->members[i].memberName, lookupName ) == 0 ) {
			return &s->members[i];
		}
	}
	return NULL;
}

static const char* MetaTagDelimiter = "-";
static const char* ArrayTag = "Array";
static const char* PtrToBufferTag = "PtrToBuffer";
static const char* ForeignTag = "Foreign";

void SerializeStruct( 
	    void* data, 
	    StructIntrospectData* s, 
	    FILE* f, 
	    StructIntrospectData** foreignIntrospectStructs = NULL,
	    void** foreignDataSrcs = NULL
	) {

	int foreignDataIndex = 0;
	fwrite( (void*)&s->memberCount, 1, sizeof( int ), f );

	for( int mIndex = 0; mIndex < s->memberCount; ++mIndex ) {
		MemberIntrospectData* m = &s->members[ mIndex ];
		for( int i = 0; i < m->metaDataCount; ++i ) {
			if( strcmp( m->metaData[i], "Ignore" ) == 0 ) {
				goto endWritingData;
			}
		}

		//Write the member name length, and the member name
		int memberNameLen = strlen( m->memberName );
		fwrite( &memberNameLen, 1, sizeof( int ), f );
		fwrite( m->memberName, memberNameLen, sizeof( char ), f );

		bool isArray = false;
		bool isPtrsToBuffer = false;
		int arrayLength = 0;
		void* baseBufferAddress = NULL;

		StructIntrospectData* currentIntrospect = s;
		void* currentData = data;

		fwrite( (void*)&m->metaDataCount, 1, sizeof( int ), f );
		for( int mdIndex = 0; mdIndex < m->metaDataCount; ++mdIndex ) {
			//write length of metaData string, and then the string itself
			char* metaData = m->metaData[ mdIndex ];
			int metaDataLen = strlen( metaData );
			fwrite( (void*)&metaDataLen, 1, sizeof( int ), f );
			fwrite( (void*)metaData, metaDataLen, 1, f );

			//Parse out the higher order struct of this member
			char metaCopy [64] = { };
			memcpy( metaCopy, metaData, metaDataLen );
			char* metaTagToken = strtok( metaCopy, MetaTagDelimiter );
			if( strcmp( metaTagToken, ArrayTag ) == 0 ) {
				metaTagToken = strtok( NULL, MetaTagDelimiter );

				isArray = true;
				arrayLength = atoi( metaTagToken );
			} else if( strcmp( metaTagToken, PtrToBufferTag ) == 0 ) {
				metaTagToken = strtok( NULL, MetaTagDelimiter );
	            
				isPtrsToBuffer = true;
				size_t memberOffset = memberLookup( currentIntrospect, metaTagToken )->offset;
				void** ptrToBaseBuffer = (void**)( ( (intptr)currentData ) + memberOffset );
				baseBufferAddress = *(ptrToBaseBuffer);
			} else if( strcmp( metaTagToken, ForeignTag ) == 0 ) {
				currentIntrospect = foreignIntrospectStructs[ foreignDataIndex ];
				currentData = foreignDataSrcs[ foreignDataIndex ];
				++foreignDataIndex;
			}
		}

		//Now write actual data out
		//General pattern: write data size, then write data payload
		if( isArray && !isPtrsToBuffer ) {
			int payloadSize = arrayLength * m->typeSize;
			void* arrayStartPtr = *( (void**)( (intptr)data + m->offset ) );

			if( arrayStartPtr == NULL ) {
				payloadSize = 1;
				uint8 nullPayload = 0;
				fwrite( (void*)&payloadSize, 1, sizeof( int ), f );
				fwrite( (void*)&nullPayload, 1, 1, f );
			} else {
				fwrite( (void*)&payloadSize, 1, sizeof( int ), f );
				fwrite( arrayStartPtr, 1, payloadSize, f );
			}
		} else if( isPtrsToBuffer ) {
			const int maxPtrElements = 256;
			ptrdiff_t offsetedPointers [ maxPtrElements ] = { };
			assert( arrayLength <= maxPtrElements );

			char** ptrToPtrs = *( (char***)( (char*)data + m->offset ) );
			size_t elementSize = m->typeSize;
			for( int ptrIndex = 0; ptrIndex < arrayLength; ++ptrIndex ) {
				char* ptr = ( ptrToPtrs[ptrIndex] );
				if( ptr < baseBufferAddress ) {
					offsetedPointers[ ptrIndex ] = -1;
				} else {
					//Assume null check has passed, 2 levels of dereferenced
					intptr targetAddress = (intptr)ptr;
					offsetedPointers[ ptrIndex ] = 
					(intptr)targetAddress - (intptr)baseBufferAddress;
				}
			}

			int payloadSize = sizeof( ptrdiff_t ) * arrayLength;
			fwrite( (void*)&payloadSize, 1, sizeof( int ), f );
			fwrite( (void*)&offsetedPointers, 1, payloadSize, f );
		} else {
			intptr memberData = ((intptr)data) + m->offset;
			fwrite( (void*)&m->typeSize, 1, sizeof ( size_t ), f );
			fwrite( (void*)memberData, 1, m->typeSize, f );
		}

		endWritingData:;
	}
}

void DeserializeStruct( void* data, StructIntrospectData* s, FILE* f, 
	StructIntrospectData** foreignIntrospectStructs = NULL, 
	void** foreignStructs = NULL ) {
	int savedMemberCount = 0;
	fread( &savedMemberCount, 1, sizeof( int ), f );

	int foreignDependentsUsed = 0;
	for( int memberIndex = 0; memberIndex < savedMemberCount; ++memberIndex ) {
		int nameLen = 0;
		char nameBuffer [64] = { };
		fread( &nameLen, 1, sizeof( int ), f );
		fread( nameBuffer, nameLen, sizeof( char ), f );

		MemberIntrospectData* m = memberLookup( s, nameBuffer );
		if( m != NULL ) {
			int arrayLength = 0;
			void* baseOfArrayAddress = NULL;

			StructIntrospectData* sid = s;
			void* datadata = data;

			int metaDataCount = 0;
			fread( &metaDataCount, 1, sizeof( int ), f );
			for( int mdIndex = 0; mdIndex < metaDataCount; ++mdIndex ) {
				char saveMetaDataBuffer [ 128 ] = { };
				int metaDataStringLen = 0;
				fread( &metaDataStringLen, 1, sizeof( int ), f );
				fread( saveMetaDataBuffer, metaDataStringLen, sizeof( char ), f );

				char* token = strtok( saveMetaDataBuffer, "-" );
				if( strcmp( token, ArrayTag ) == 0 ) {
					token = strtok( NULL, "-" );
					arrayLength = atoi( token );
				} else if( strcmp( token, PtrToBufferTag ) == 0 && arrayLength > 0 ) {
					token = strtok( NULL, "-" );
					void** ptrToArrayOfPtrs = (void**)(((char*)datadata) + memberLookup( sid, token )->offset);
					assert( ptrToArrayOfPtrs != NULL );
					baseOfArrayAddress = *ptrToArrayOfPtrs;
				} else if( strcmp( token, ForeignTag ) == 0 ) {
					token = strtok( NULL, "-" );
					datadata = foreignStructs[ foreignDependentsUsed ];
					sid = foreignIntrospectStructs[ foreignDependentsUsed ];
					++foreignDependentsUsed;
				}
			}

			if( baseOfArrayAddress != NULL ) {
				ptrdiff_t savedOffsets [256] = { };
				int payloadSize = 0;
				fread( &payloadSize, 1, sizeof( int ), f );
				fread( savedOffsets, 1, payloadSize, f );

				void** arrayStartPtr = *( (void***)( (intptr)data + m->offset ) );
				for( int index = 0; index < arrayLength; ++index ) {
					arrayStartPtr[ index ] = 
					(void*)( (char*)baseOfArrayAddress + savedOffsets[ index ] );

					//If savedOffsets[index] == 0, set arrayStartPtr to zero;
					if( savedOffsets[ index ] == -1 ) {
						arrayStartPtr[ index ] = 0;
					}
				}
			} else {
				int payloadSize = 0;
				void* ptrToMemberDataLocation = (void*)((intptr)data + m->offset );
				for( int ptrLevelIndex = 0; ptrLevelIndex < m->ptrLevel; ++ptrLevelIndex ) {
					ptrToMemberDataLocation = *(void**)ptrToMemberDataLocation;
				}
				fread( &payloadSize, 1, sizeof( int ), f );
				fread( ptrToMemberDataLocation, 1, payloadSize, f );
			}
		} else {
			//Just calculate how much data is there, "read" it and move on
			char blankBuffer [ 2 * 1024 ] = { };

			int metaDataCount = 0;
			fread( &metaDataCount, 1, sizeof( int ), f );
			for( int mdIndex = 0; mdIndex < metaDataCount; ++mdIndex ) {
				int metaDataStringLen = 0;
				fread( &metaDataStringLen, 1, sizeof( int ), f );
				fread( blankBuffer, metaDataStringLen, sizeof( char ), f );
			}

			int payloadSize = 0;
			fread( &payloadSize, 1, sizeof( int ), f );
			fread( blankBuffer, payloadSize, 1, f );
		}
	}
}