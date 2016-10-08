#include <stdio.h>
#include <cstdlib>

#include <string>
#include <assert.h>

#define NULL 0

#ifdef WIN32
#define LineEnding \n\r
#endif

char* ReadSourceFileIntoMemoryAndNullTerminate( char* filePath ) {
	FILE* file = NULL;
	fopen_s( &file, filePath, "r" );
	if( file ) {
		fseek( file, 0L, SEEK_END );
		int bytesInFile = ftell( file );
		fseek( file, 0L, SEEK_SET );
		char* mem = (char*)calloc( 1, bytesInFile + 1);

		fread( mem, bytesInFile - 1, 1, file );
		mem[ bytesInFile ] = 0;

		fclose(file);
		return mem;
	}
	return NULL;
}

enum TokenType {
	Identifier,
	OpenParen,
	CloseParen,
	OpenBracket,
	CloseBracket,
	OpenBrace,
	CloseBrace,
	SemiColon,
	Comma,
	Star,
	Ampersand,
	UndefinedToken,
	StringLiteral,
	Number,
	EndOfStream,

	kwStruct,
	kwStatic,
	MetaTag,
	poundDefine,
	poundInclude
};

struct MetaTagDataStorage;

struct Token {
	int tokenType;
	int tokenLength;
	char* tokenStart;
	Token* nextToken;
};

#define MAX_TOKENS 2148
struct Tokenizer {
	char* at;
	Token* tokens;
	int tokenCount;
	int tokensSinceLastMeta;
	MetaTagDataStorage* metaTagData;
};

inline bool IsNewlineValue( char c ) {
	return (
		c == '\n' ||
		c == '\r'
	);
}

inline bool IsWhiteSpace( char c ) {
	return (
		c == ' ' ||
		c == '\t' ||
		IsNewlineValue( c )
	);
}

void EatAllWhiteSpace( Tokenizer* tokenizer ) {
	while( tokenizer->at[0] ) {
		if( IsWhiteSpace( tokenizer->at[0] ) ) {
			++tokenizer->at;
		} else if( tokenizer->at[0] == '/' && tokenizer->at[1] == '/' ) {
			tokenizer->at += 2;
			while(tokenizer->at[0] && IsNewlineValue( tokenizer->at[0] ) ) {
				++tokenizer->at;
			}
		} else if( tokenizer->at[0] == '/' && tokenizer->at[1] == '*') {
			tokenizer->at += 2;
			while( tokenizer->at[0] && !(tokenizer->at[0] == '*' && tokenizer->at[1] == '/' ) ) {
				++tokenizer->at;
			}
			tokenizer->at += 2;
		} else {
			break;
		}
	}
}

inline bool IsStringMatch( char* c, const char* targetString ) {
	int targetStringLength = strlen( targetString );
	for( int i = 0; i < targetStringLength; ++i ) {
		if( !c || c[i] != targetString[i] ) {
			return false;
		}
	}

	return true;
}

inline bool IsNumeric( char c ) {
	return (
		c >= '0' && c <= '9'
	);
}

inline bool IsIdentifierValidCharacter( char c ) {
	return ( 
		( c >= 'a' && c <= 'z' ) ||
		( c >= 'A' && c <= 'Z' ) ||
		c == '_'
	);
}

Token* ReadNextToken( Tokenizer* tokenizer ) {
	EatAllWhiteSpace( tokenizer );

	Token* newToken = &tokenizer->tokens[ tokenizer->tokenCount ];

	assert( tokenizer->tokenCount <= MAX_TOKENS );
	newToken->tokenStart = tokenizer->at;
	newToken->tokenLength = 1;
	newToken->nextToken = NULL;
	switch( tokenizer->at[0] ) {
		case '{' : newToken->tokenType = TokenType::OpenBrace; ++tokenizer->at; break;
		case '}' : newToken->tokenType = TokenType::CloseBrace; ++tokenizer->at; break;
		case '[' : newToken->tokenType = TokenType::OpenBracket; ++tokenizer->at; break;
		case ']' : newToken->tokenType = TokenType::CloseBracket; ++tokenizer->at; break;
	    case '(' : newToken->tokenType = TokenType::OpenParen; ++tokenizer->at; break;
	    case ')' : newToken->tokenType = TokenType::CloseParen; ++tokenizer->at;  break;
	    case ';' : newToken->tokenType = TokenType::SemiColon; ++tokenizer->at; break;
	    case ',' : newToken->tokenType = TokenType::Comma; ++tokenizer->at; break;
	    case '*' : newToken->tokenType = TokenType::Star; ++tokenizer->at; break;
	    case '&' : newToken->tokenType = TokenType::Ampersand; ++tokenizer->at; break;
	    case '\0' : newToken->tokenType = TokenType::EndOfStream; break;
	    default: { 
	    	if( tokenizer->at[0] == '\"' ) {
	    		++tokenizer->at;
	    		while( tokenizer->at[0] && tokenizer->at[0] != '\"' ) {
	    			++tokenizer->at;
	    			if( tokenizer->at[0] == '\\' ) {
	    				tokenizer->at += 2;
	    			}
	    		}
	    		++tokenizer->at; //Chew through the last "
	    		newToken->tokenType = TokenType::StringLiteral;
	    	} else if ( tokenizer->at[0] == '#' ) {
	    		const char* defineString = "define";
	    		const char* includeString = "include";
	    		++tokenizer->at;

	    		if( IsStringMatch( tokenizer->at, defineString ) ) {
	    			newToken->tokenType = TokenType::poundDefine;
	    			tokenizer->at += 6;
	    		} else if( IsStringMatch( tokenizer->at, includeString ) ) {
	    			newToken->tokenType = TokenType::poundInclude;
	    			tokenizer->at += 7;
	    		} else {
	    			newToken->tokenType = TokenType::UndefinedToken;
	    			++tokenizer->at;
	    		}

	    	} else {
	    		const char* structString = "struct";
	    		const char* metatagString = "meta";
	    		const char* staticString = "static";

	    		//Compare to struct or meta, else identitfer or numeric
	    		if( IsStringMatch( tokenizer->at, structString ) ) {
	    			newToken->tokenType = TokenType::kwStruct;
	    			tokenizer->at += 6;
	    		} else if( IsStringMatch( tokenizer->at, metatagString ) ) {
	    			newToken->tokenType = TokenType::MetaTag;
	    			tokenizer->at += 4;
	    		} else if( IsStringMatch( tokenizer->at, staticString ) ) {
	    			newToken->tokenType == TokenType::kwStatic;
	    			tokenizer->at += 6;
	    		} else {
	    			if( IsIdentifierValidCharacter( tokenizer->at[0] ) ) {
	    				while( tokenizer->at[0] && ( IsIdentifierValidCharacter( tokenizer->at[0] ) || IsNumeric( tokenizer->at[0] ) ) )  {
	    					++tokenizer->at;
	    				}
	    				newToken->tokenType = TokenType::Identifier;
	    			} else if( IsNumeric( tokenizer->at[0] ) ) {
	    				while( IsNumeric( tokenizer->at[0] ) ) {
	    					++tokenizer->at;
	    				}
	    				newToken->tokenType = TokenType::Number;
	    			} else {
	    				newToken->tokenType = TokenType::UndefinedToken;
	    				++tokenizer->at;
	    			}
	    		}
	    	}

	    	newToken->tokenLength = tokenizer->at - newToken->tokenStart;

	    	//Edge case solution, StringLiterals were spitting out tokens like:
	    	// \"String\", if you wanted to compare their values, then you had to
	    	// make const string with escaped quotes
	    	if( newToken->tokenType == TokenType::StringLiteral ) {
	    		++newToken->tokenStart;
	    		newToken->tokenLength -= 2;
	    	}

	    	break;
	    }
	}
	++tokenizer->tokensSinceLastMeta;
	++tokenizer->tokenCount;
	return newToken;
}

struct TokenPattern {
	int* targetPattern;
	int patternLength;
};

struct PatternTrackingState {
	int* pattern;
	int patternLength;
	int trackingIndex;
};

struct MetaTagList {
	Token* tagTokens [6];
	int tagCount;
};

struct MetaTagDataStorage {
	MetaTagList* lists;
	int listCount;
	int maxListCount;
	PatternTrackingState metaTagPatternTracker;
};

struct Member {
	Token* typeToken;
	Token* memberNameToken;
	MetaTagList* metaData;
	int ptrLevel;
};

struct StructDefinition {
	Token* typeNameToken;
	Member memberDefinitions [15];
	int memberCount;
	MetaTagList* metaData;
};

struct StructDefinitionList {
	StructDefinition* definitions;
	int maxCount;
	int count;
};

bool UpdatePatternTracker( PatternTrackingState* state, Token* token ) {
	if( state->pattern[ state->trackingIndex ] == token->tokenType ) {
		++state->trackingIndex;
		if( state->trackingIndex == state->patternLength ) {
			state->trackingIndex = 0;
			return true;
		}
		return false;
	} else {
		state->trackingIndex = 0;
		return false;
	}
}

static FILE* outFile_H;
static FILE* outFile_CPP;
static inline void writeConstStringToFile( char* string, FILE* dest ) {
	int stringLength = strlen( string );
	fwrite( string, 1, stringLength, dest );
}

static inline void writeTokenDataToFile( Token* t, FILE* dest ) {
	fwrite( t->tokenStart, 1, t->tokenLength, dest );
}

#include "PreprocessorExt.h"

MetaTagList* ParseMetaTagValues( Tokenizer* tokenizer ) {
	MetaTagDataStorage* metaTagStorage = tokenizer->metaTagData;
	Token* token = ReadNextToken( tokenizer );

	MetaTagList* valueList = &metaTagStorage->lists[ metaTagStorage->listCount++ ];
	valueList->tagCount = 0;

	int tokenType = token->tokenType;
	while( tokenType != TokenType::CloseParen && tokenType != TokenType::EndOfStream ) {
		if( tokenType == TokenType::StringLiteral || tokenType == TokenType::Number) {
			valueList->tagTokens[ valueList->tagCount++ ] = token;
		}
		token = ReadNextToken( tokenizer );
		tokenType = token->tokenType;
	}

	tokenizer->tokensSinceLastMeta = 0;

	return valueList;
}

StructDefinition ParseStructDefinition( Tokenizer* tokenizer ) {
	StructDefinition definition = { };

	bool hasMetaData = tokenizer->tokensSinceLastMeta <= 3;
	MetaTagDataStorage* allMetaData = tokenizer->metaTagData;
	MetaTagList* metaData = &allMetaData->lists[ allMetaData->listCount - 1 ];
	definition.typeNameToken = &tokenizer->tokens[ tokenizer->tokenCount - 2 ];

	if( hasMetaData ) {
		definition.metaData = metaData;
	} else {
		definition.metaData = NULL;
	}

	int stdMemberPattern [3] = { 
		TokenType::Identifier, 
		TokenType::Identifier, 
		TokenType::SemiColon 
	};
	PatternTrackingState stdMemberChecking = { stdMemberPattern, 3, 0 };
	int ptrLevel = 0;

	MetaTagList* metaTags = NULL;
	while( tokenizer->at[0] ) {
		Token* token = ReadNextToken( tokenizer );
		if( token->tokenType == TokenType::CloseBrace ) break;

 		if( UpdatePatternTracker( &tokenizer->metaTagData->metaTagPatternTracker, token ) ) {
 			metaTags = ParseMetaTagValues( tokenizer );
 			ptrLevel = 0;
		}

		//The idea here, is skip any indicaters of pointers, just check for the
		//identifier - identifier - semicolon pattern but retain how deep
		//the pointer reference level is
		//(i.e. char vs char* vs char**  is 0 vs 1 vs 2, etc. )
		if( token->tokenType == TokenType::Star ) {
			++ptrLevel;
			//Skip updating the memberDef tracker
			continue;
		}

		//the 'Static' keyword should reset pattern tracking logic, since
		//members definitions only start with that, so for now we ignore such
		//modifiers
		if( token->tokenType == TokenType::kwStatic ) {
			stdMemberChecking.trackingIndex = 0;
			ptrLevel = 0;
			continue;
		}

		if( UpdatePatternTracker( &stdMemberChecking, token ) ) {
			int memberTypeTokenIndex = tokenizer->tokenCount - ptrLevel - 3;
			int memberNameTokenIndex = tokenizer->tokenCount - 2;

			definition.memberDefinitions[ definition.memberCount ] = { 
				&tokenizer->tokens[ memberTypeTokenIndex ],
				&tokenizer->tokens[ memberNameTokenIndex ],
				metaTags,
				ptrLevel
			};
			++definition.memberCount;
			metaTags = NULL;
		}

		if( token->tokenType == TokenType::SemiColon ) {
			ptrLevel = 0;
		}
	}

	//Forward declare struct type
	writeConstStringToFile( "\nstruct ", outFile_H );
	writeTokenDataToFile( definition.typeNameToken, outFile_H );
	writeConstStringToFile( ";", outFile_H );

	//Check for meta programming code generation flags
	if( hasMetaData ) {
		//Output introspection data, assume its functionality is needed
		GenerateIntrospectionCode( &definition );

		char* editableMetaFlag = "Editable";
		char* serializableMetaFlag = "Serializable";

		char* prevMetaString = metaData->tagTokens[0]->tokenStart;

		if( IsStringMatch( prevMetaString, editableMetaFlag ) ) {
			GenerateImguiEditor( &definition );
		} else if( IsStringMatch( prevMetaString, serializableMetaFlag ) ) {
			//GenerateSerializationCode( &definition );
		}
	}

	return definition;
}

void main( int argc, char** argv ) {
	//Reminder for self
	if( strcmp( argv[1], "-h" ) == 0 || argc < 3 ) {
		printf("Format: input src file, out file path------\n\n" );
		return;
	}

	char* headerName = "Meta.h";
	char* cppName = "Meta.cpp";

	char* fileToProcess = argv[1];
	char* outputBasePath = argv[2];

	int outputBasePathLen = strlen( outputBasePath );
	char generatedHeaderFilePath [128] = { };
	char generatedCPPFilePath [128] = { };
	memcpy( &generatedHeaderFilePath[0], outputBasePath, outputBasePathLen );
	memcpy( &generatedCPPFilePath[0], outputBasePath, outputBasePathLen );
	memcpy( 
		&generatedHeaderFilePath[ outputBasePathLen ], 
		headerName, 
		strlen( headerName ) 
	);
	memcpy(
		&generatedCPPFilePath[ outputBasePathLen ],
		cppName,
		strlen( cppName )
	);

	char* sourceFileToProcess = ReadSourceFileIntoMemoryAndNullTerminate( 
		fileToProcess
	);

	if( !sourceFileToProcess ) {
		printf("Error reading source file\n" );
		return;
	}

	outFile_H = fopen( generatedHeaderFilePath, "w" );
	outFile_CPP = fopen( generatedCPPFilePath, "w" );

	writeConstStringToFile( "#define meta(...)\n\n", outFile_H );
	writeConstStringToFile( "#if 1\n", outFile_H );
	writeConstStringToFile( "#if 1\n", outFile_CPP );


	int structPattern [3] = { 
		TokenType::kwStruct,
		TokenType::Identifier,
		TokenType::OpenBrace
	};
	PatternTrackingState structPatternTracking = { structPattern, 3, 0 };

	int metaTagPattern [2] = {
		TokenType::MetaTag,
		TokenType::OpenParen
	};
	MetaTagDataStorage metaTagStorage = {
		(MetaTagList*)calloc( 64, sizeof(MetaTagList) ),
		0,
		64,
		{ metaTagPattern, 2, 0 }
	};

	Tokenizer tokenizer = { };
	tokenizer.at = sourceFileToProcess;
	tokenizer.tokenCount = 0;
	tokenizer.tokens = (Token*)calloc( MAX_TOKENS, sizeof(Token) );
	tokenizer.tokensSinceLastMeta = 1;
	tokenizer.metaTagData = &metaTagStorage;

	StructDefinitionList sdList = {
		(StructDefinition*)calloc( 120, sizeof(StructDefinition) ),
		120,
		0
	};

	while( tokenizer.at[0] ) {
		Token* token = ReadNextToken( &tokenizer );

		if( UpdatePatternTracker( &metaTagStorage.metaTagPatternTracker, token ) ) {
			ParseMetaTagValues( &tokenizer );
		}
		if( UpdatePatternTracker( &structPatternTracking, token ) ) {
			StructDefinition structDefinition = 
			ParseStructDefinition( &tokenizer );

				//Add to list of definitions
			if( sdList.count < sdList.maxCount ) {
				sdList.definitions[ sdList.count++ ] = structDefinition;
			}
		}
	}

	writeConstStringToFile( "\n#endif", outFile_H );
	writeConstStringToFile( "\n#endif", outFile_CPP );

	fclose( outFile_H );
	fclose( outFile_CPP );
}