// Carbonate/CFBase.cc


#ifndef __CFBASE__
#include <CFBase.h>
#endif

#include "Carbonate/CFBase.hh"

#if TARGET_API_MAC_CARBON
#error Configuration error:  This file is for classic only
#endif

/*
#if ACCESSOR_CALLS_ARE_FUNCTIONS
// Compile the Carbon accessors as extern pascal functions.
#define CARBONATE_LINKAGE pascal
#include "Carbonate/Events.hh"
#endif
*/

CFTypeRef CFRetain( CFTypeRef cf )
{
	CFObject* object = CFType_Cast< CFObject* >( cf );
	
	++object->retainCount;
	
	return cf;
}

void CFRelease( CFTypeRef cf )
{
	CFObject* object = CFType_Cast< CFObject* >( cf );
	
	if ( --object->retainCount == 0 )
	{
		delete object;
	}
}

CFIndex CFGetRetainCount( CFTypeRef cf )
{
	CFObject* object = CFType_Cast< CFObject* >( cf );
	
	return object->retainCount;
}

