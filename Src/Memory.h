#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

struct SlabSubsection {
	void* start;
	void* end;
};

struct MemorySlab {
	void* slabStart;
	void* current;
	uint64 slabSize;
};

SlabSubsection CarveNewSubsection( MemorySlab* slab, uint64 bytes ) {

	return { };
}