/*
	OSUtils.cc
	----------
*/

#include "OSUtils.hh"

// Mac OS
#ifndef __MACERRORS__
#include <MacErrors.h>
#endif
#ifndef __OSUTILS__
#include <OSUtils.h>
#endif

// POSIX
#include <unistd.h>

// ams-common
#include "time.hh"

// ams-core
#include "reactor-core.hh"


uint32_t Ticks : 0x016A;
uint32_t Time  : 0x020C;


#pragma mark -
#pragma mark Date and Time Operations
#pragma mark -

pascal short ReadDateTime_patch( long* secs : __A0 ) : __D0
{
	*secs = Time;
	
	return noErr;
}

static const char month_days[] =
{
	31, 28, 31,
	30, 31, 30,
	31, 31, 30,
	31, 30, 31,
};

pascal DateTimeRec* Secs2Date_patch( unsigned long  secs : __D0,
                                     DateTimeRec*   date : __A0 ) : __A0
{
	const unsigned long days_per_quad = 365 * 4 + 1;
	
	unsigned long mins, hour, days;
	
	date->second = secs % 60;
	mins         = secs / 60;
	date->minute = mins % 60;
	hour         = mins / 60;
	date->hour   = hour % 24;
	days         = hour / 24;
	
	const unsigned short quads = days / days_per_quad;
	days                       = days % days_per_quad;
	
	date->dayOfWeek = (days + 5) % 7 + 1;  // 1904-01-01 is a Friday
	
	const unsigned short feb29 = 31 + 28;
	
	const short leap_diff = days - feb29;
	
	days -= leap_diff > 0;
	
	unsigned short years = quads * 4;
	
	years += days / 365;
	days   = days % 365;
	
	unsigned short month = 1;
	
	for ( int i = 0;  i < 12;  ++i )
	{
		if ( days < month_days[ i ] )
		{
			break;
		}
		
		days -= month_days[ i ];
		
		++month;
	}
	
	days += 1 + (leap_diff == 0);
	
	date->year  = 1904 + years;
	date->month = month;
	date->day   = days;
	
	return date;
}

#pragma mark -
#pragma mark Queue Manipulation
#pragma mark -

asm
pascal void Enqueue_patch( QElem* qEntry : __A0, QHdr* queue : __A1 )
{
	CLR.L    (A0)     // qEntry->qLink = NULL;
	
	JSR      0xFFFFFFFA  // enter_supervisor_mode()
	ORI      #0x0700,SR  // mask all interrupts (except NMI)
	ANDI     #0xDFFF,SR  // exit supervisor mode
	
	// old SR (with old interrupt mask) is in D0
	
	MOVE.L   A0,D2    // Move qEntry to D2 (freeing up A0 for next pointer)
	ADDQ.L   #2,A1    // Point to qHead field of queue
	
	MOVEA.L  A1,A0    // next = &queue->qHead;
	TST.L    (A1)+    // if (queue->qHead)  // advances A1 to &queue->qTail
	BEQ.S    set_link
	
	MOVE.L   (A1),A0  // next = &queue->qTail->qLink;
	
set_link:
	
	MOVE.L   D2,(A0)  // *next        = qEntry;
	MOVE.L   D2,(A1)  // queue->qTail = qEntry;
	
	MOVE.L   D0,D1       // move old SR to D1
	JSR      0xFFFFFFFA  // enter_supervisor_mode()
	MOVE     D1,SR       // restore old SR (with old interrupt mask)
	
	RTS
}

asm
pascal short Dequeue_patch( QElem* qEntry : __A0, QHdr* queue : __A1 ) : __D0
{
	MOVEQ.L  #qErr,D0
	MOVE.L   A0,D2       // Null-check qEntry (and move to D2 for later)
	BEQ.S    bail
	
	CLR.W    -(SP)       // noErr
	
	ADDQ.L   #2,A1       // Point to qHead field of queue
	MOVE.L   A1,D1       // Copy &queue->qHead to D1
	
	JSR      0xFFFFFFFA  // enter_supervisor_mode()
	ORI      #0x0700,SR  // mask all interrupts (except NMI)
	ANDI     #0xDFFF,SR  // exit supervisor mode
	
	// old SR (with old interrupt mask) is in D0
	
	CMPA.L   (A1)+,A0    // if ( qEntry == queue->qHead )
	BNE.S    loop
	
	CMP.L    (A1),A0     // if ( qEntry == queue->qTail )
	BNE.S    first_but_not_last
	
	CLR.L    (A1)        // queue->qTail = NULL;
	
first_but_not_last:
	
	MOVE.L   (A0),-(A1)  // queue->qHead = qEntry->qLink;
	
	BRA.S    cleanup
	
not_found:
	
	SUBQ.W   #1,(SP)     // qErr: "Entry not in specified queue"
	BRA.S    cleanup
	
loop:
	
	MOVEA.L  D1,A0       // prev = next;         // or &queue->qHead, once
	MOVE.L   (A0),D1     // next = next->qLink;  // or queue->qHead, once
	
	BEQ.S    not_found   // while ( next != NULL )
	
	CMP.L    D2,D1       // if ( next == qEntry )
	BNE.S    loop
	
	CMP.L    (A1),D2     // if ( qEntry == queue->qTail )
	BNE.S    not_last
	
	MOVE.L   A0,(A1)     // queue->qTail = prev;
	
not_last:
	
	MOVEA.L  D2,A1
	MOVE.L   (A1),(A0)   // prev->qLink = qEntry->qLink;
	
cleanup:
	
	MOVE.L   D0,D1       // move old SR to D1
	JSR      0xFFFFFFFA  // enter_supervisor_mode()
	MOVE     D1,SR       // restore old SR (with old interrupt mask)
	
	MOVE.W   (SP)+,D0
	
bail:
	RTS
}

#pragma mark -
#pragma mark Miscellaneous Utilities
#pragma mark -

static inline
bool reactor_wait( uint64_t dt )
{
	timeval timeout = { dt / 1000000, dt % 1000000 };
	
	return reactor_wait( &timeout );
}

pascal long Delay_patch( long numTicks : __A0 ) : __D0
{
	const uint64_t start = time_microseconds();
	
	uint64_t dt = (uint64_t) numTicks * tick_microseconds;
	
	const uint64_t end_time = start + dt;
	
	// If numTicks is negative, return after one reactor-wait.
	
	while ( reactor_wait( dt )  &&  numTicks >= 0 )
	{
		dt = end_time - time_microseconds();
	}
	
	return Ticks;
}

pascal void SysBeep_patch( short duration )
{
	char c = 0x07;
	
	write( STDOUT_FILENO, &c, sizeof c );
}
