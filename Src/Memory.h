#define KILOBYTES(value) value * 1024
#define MEGABYTES(value) KILOBYTES(value) * 1024
#define GIGABYTES(value) MEGABYTES(value) * 1024

struct GlobalMem {
	void* slabStart;
	void* current;
	uint64 slabSize;
};

struct Stack {
	void* start;
	void* end;
	void* current;
};

Stack AllocateStackInGlobalMem( GlobalMem* slab, uint64 bytes ) {
	Stack slabSub = { 0, 0, 0 };
	ptrdiff_t bytesLeft = slab->slabSize - ((uintptr)slab->current - (uintptr)slab->slabStart);
	if( bytesLeft > bytes ) {
		slabSub.start = slab->current;
		slabSub.current = slab->current;
		slab->current = (void*)( (intptr)slab->current + bytes );
		slabSub.end = slab->current;
	}
	return slabSub;
}

void* StackAlloc( Stack* stack, uint64 sizeInBytes ) {
	//Check how much space is left in the sub stack
	ptrdiff_t bytesLeft = (intptr)stack->end - (intptr)stack->current;
	//Allocate if there's enough space left
	if( bytesLeft > sizeInBytes ) {
		void* returnValue = stack->current;
		stack->current = (void*)( (intptr)stack->current + sizeInBytes );
		return returnValue;
	} else {
		assert(false);
		return NULL;
	}
}

//Naive implementation based on examples in "Game Engine Architechture"
void* StackAllocA( Stack* stack, uint64 size, uint8 alignment = 8 ) {
	//Add some padding so we have enough room to align
	uint64 expandedSize = size + alignment;

	//Determine how "off" the basic allocated pointer is
	uintptr rawAddress = (uintptr)StackAlloc( stack, expandedSize );
	uintptr alignmentMask = ( alignment - 1 );
	uintptr misalignment = ( rawAddress & alignmentMask );

	//Adjust the pointer to be aligned and return that
	uintptr adjustment = alignment - misalignment;
	uintptr alignedAddress = rawAddress + adjustment;
	return (void*)alignedAddress;
}

void ClearSubStack( Stack* stack ) {
	stack->current = stack->start;
}

//TODO: This "leaks" memory due to padding, track amount of padding in byte ptr - 1
//And free the other stuff too
void FreeFromStack( Stack* stack, void* ptr ) {
	if( (intptr)ptr > (intptr)stack->current ) {
		stack->current = ptr;
	}
}