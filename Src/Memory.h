#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

struct MemorySlab {
	void* slabStart;
	uint64 slabSize;
};