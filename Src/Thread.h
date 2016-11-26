struct Job {
	enum STATUS {
		Available, Prepping, InQueue
	};
	int status;
	bool* doneFlag;
	//Data to pass to callback function
	void* dataIn;
	//Call back function, what the actual work is.
	void (*JobFunc)(void*);
};

//Only for platform layer code
#ifndef DLL_ONLY
struct WorkerThreadData {
	Job* jobs;
	int maxJobs;
};

static int maxJobs = 0;
static Job* workQueue = NULL;

static Job* GetJobSlot() { 
	
}

static void WorkThreadMain( void* data ) {

}
#endif