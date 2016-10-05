struct FileSys {
	void* (*ReadWholeFile) (char*, Stack*);
	int (*GetMostRecentMatchingFile) (char*, char*);
	int (*TrackFileUpdates)( char* filePath );
	bool (*DidFileUpdate)( int trackingIndex );
};