/*
	Handles.cc
	----------
*/

#include "Handles.hh"

// Standard C
#include <stdlib.h>
#include <string.h>


enum
{
	kHandleIsResourceMask = 0x20,
	kHandlePurgeableMask  = 0x40,
	kHandleLockedMask     = 0x80
};

const short noErr        = 0;
const short paramErr     = -50;
const short memFullErr   = -108;
const short nilHandleErr = -109;
const short memSCErr     = -116;

short MemErr : 0x0220;


const unsigned long Handle_prologue = 0xC7C7C7C7;  // left guillemots
const unsigned long Handle_epilogue = 0xC8C8C8C8;  // right guillemots

struct master_pointer
{
	char*  alloc;
	long   flags;
};

struct Handle_header
{
	long  size;
	long  capacity;
	
	master_pointer* backlink;
	
	unsigned long  prologue;
};

struct Handle_footer
{
	unsigned long  epilogue;
};


static inline
unsigned long padded( size_t size )
{
	return size + 3 & ~3;
}

static inline
Handle_header* get_header( char* p )
{
	return (Handle_header*) p - 1;
}

static inline
Handle_footer* get_footer( Handle_header* header )
{
	return (Handle_footer*) ((char*) &header[ 1 ] + padded( header->size ));
}

static inline
void set_epilogue( Handle_header* header )
{
	get_footer( header )->epilogue = Handle_epilogue;
}

static inline
Handle_footer* get_footer( char* p )
{
	return get_footer( get_header( p ) );
}

static inline
short size_error( unsigned long size : __D0 )
{
	const unsigned extra_size = sizeof (Handle_header)
	                          + sizeof (Handle_footer);  // 16 bytes
	
	if ( size > 0x7FFFFFFF - extra_size )
	{
		return memSCErr;
	}
	
	return noErr;
}

static
Handle_header* allocate_Handle_mem( long   size      : __D0,
                                    short  trap_word : __D1 )
{
	const unsigned extra_size = sizeof (Handle_header)
	                          + sizeof (Handle_footer);  // 16 bytes
	
	const unsigned long padded_size = padded( size );
	
	if (( MemErr = size_error( size ) ))
	{
		return NULL;
	}
	
	const long full_size = padded_size + extra_size;
	
	void* alloc = trap_word & 0x0200 ? calloc( 1, full_size )
	                                 : malloc(    full_size );
	
	if ( alloc == NULL )
	{
		MemErr = memFullErr;
		
		return NULL;
	}
	
	Handle_header* header = (Handle_header*) alloc;
	Handle_footer* footer = (Handle_footer*) ((char*) &header[1] + padded_size);
	
	header->size     = size;
	header->capacity = padded_size;
	
	header->backlink = NULL;
	
	header->prologue = Handle_prologue;
	footer->epilogue = Handle_epilogue;
	
	return header;
}


static
char** new_empty_handle()
{
	MemErr = memFullErr;
	
	void* alloc = malloc( sizeof (master_pointer) );  // 8 bytes
	
	if ( alloc != NULL )
	{
		master_pointer* h = (master_pointer*) alloc;
		
		h->alloc = NULL;
		h->flags = 0;
		
		MemErr = noErr;
	}
	
	return (char**) alloc;
}

static
char** new_handle( long size : __D0, short trap_word : __D1 )
{
	char** h = new_empty_handle();
	
	if ( h != NULL )
	{
		ReallocateHandle_patch( h, size, trap_word );
		
		if ( *h != NULL )
		{
			return h;
		}
		
		free( h );
	}
	
	return NULL;
}

asm char** NewHandle_patch( long size : __D0, short trap_word : __D1 )
{
	JSR      new_handle
	MOVE.W   MemErr,D0  // Includes the effect of TST.W D0
	
	RTS
}

asm
char** NewEmptyHandle_patch()
{
	JSR      new_empty_handle
	MOVE.W   MemErr,D0
	
	RTS
}

short HLock_patch( char** h : __A0 )
{
	if ( h == NULL )
	{
		return MemErr = paramErr;
	}
	
	*(char*) &h[1] |= kHandleLockedMask;
	
	return MemErr = noErr;
}

short HUnlock_patch( char** h : __A0 )
{
	if ( h == NULL )
	{
		return MemErr = paramErr;
	}
	
	*(char*) &h[1] &= ~kHandleLockedMask;
	
	return MemErr = noErr;
}

short HPurge_patch( char** h : __A0 )
{
	if ( h == NULL )
	{
		return MemErr = paramErr;
	}
	
	*(char*) &h[1] |= kHandlePurgeableMask;
	
	return MemErr = noErr;
}

short HNoPurge_patch( char** h : __A0 )
{
	if ( h == NULL )
	{
		return MemErr = paramErr;
	}
	
	*(char*) &h[1] &= ~kHandlePurgeableMask;
	
	return MemErr = noErr;
}

short DisposeHandle_patch( char** h : __A0 )
{
	if ( h == NULL )
	{
		return MemErr = nilHandleErr;
	}
	
	if ( *h != NULL )
	{
		Handle_header* header = get_header( *h );
	
		free( header );
	}
	
	free( h );
	
	return MemErr = noErr;
}

short SetHandleSize_patch( char**  h         : __A0,
                           long    size      : __D0,
                           short   trap_word : __D1 )
{
	if ( h == NULL  ||  *h == NULL )
	{
		return MemErr = nilHandleErr;
	}
	
	if ( size < 0 )
	{
		return MemErr = memSCErr;
	}
	
	Handle_header* header = get_header( *h );
	
	if ( size <= header->capacity )
	{
		header->size = size;
	}
	else
	{
		Handle_header* new_header = allocate_Handle_mem( size, trap_word );
		
		if ( new_header == NULL )
		{
			return MemErr = memFullErr;
		}
		
		memcpy( new_header + 1, header + 1, header->size );
		
		new_header->backlink = header->backlink;
		
		*h = (char*) &new_header[1];
		
		free( header );
		
		header = new_header;
	}
	
	set_epilogue( header );
	
	return MemErr = noErr;
}

long GetHandleSize_patch( char** h : __A0 )
{
	if ( h == NULL  ||  *h == NULL )
	{
		MemErr = nilHandleErr;
		
		return 0;
	}
	
	const Handle_header* header = get_header( *h );
	
	return header->size;
}

short ReallocateHandle_patch( char**  h         : __A0,
                              long    size      : __D0,
                              short   trap_word : __D1 )
{
	if ( h == NULL )
	{
		return MemErr = nilHandleErr;
	}
	
	if ( *h != NULL )
	{
		Handle_header* header = get_header( *h );
		
		free( header );
		
		*h = NULL;
	}
	
	Handle_header* header = allocate_Handle_mem( size, trap_word );
	
	if ( header == NULL )
	{
		return MemErr;  // set in allocate_Handle_mem()
	}
	
	*h = (char*) &header[1];
	
	header->backlink = (master_pointer*) h;
	
	return MemErr = noErr;
}

short EmptyHandle_patch( char** h : __A0 )
{
	if ( h == NULL )
	{
		return MemErr = nilHandleErr;
	}
	
	if ( *h != NULL )
	{
		Handle_header* header = get_header( *h );
		
		free( header );
		
		*h = NULL;
	}
	
	return MemErr = noErr;
}

short SetApplLimit_patch( char* p : __A0 )
{
	return MemErr = noErr;
}

void MoreMasters_patch()
{
	MemErr = noErr;
}

asm void MaxMem_patch()
{
	SUBA.L   A0,A0
	MOVE.L   #0x100000,D0
	RTS
}

short ReserveMem_patch( long needed : __D0, short trap_word : __D1 )
{
	return MemErr = noErr;
}

void MaxApplZone_patch()
{
}
