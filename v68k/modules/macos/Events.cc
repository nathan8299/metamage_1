/*
	Events.cc
	---------
*/

#include "Events.hh"

// Mac OS
#ifndef __EVENTS__
#include <Events.h>
#endif
#ifndef __MACWINDOWS__
#include <MacWindows.h>
#endif
#ifndef __PROCESSES__
#include <Processes.h>
#endif

// POSIX
#include <unistd.h>
#include <sys/select.h>

// splode
#include "splode/splode.hh"

// macos
#include "Cursor.hh"
#include "Region-ops.hh"
#include "Regions.hh"
#include "options.hh"


UInt32 Ticks   : 0x016A;
Byte   MBState : 0x0172;
UInt16 KeyMods : 0x017A;
Point  Mouse   : 0x0830;


const unsigned long GetNextEvent_throttle = 2;  // minimum ticks between calls

static const timeval zero_timeout = { 0, 0 };

static timeval wait_timeout;

static
timeval timeval_from_ticks( unsigned long ticks )
{
	const long microseconds_per_tick = 1000 * 1000 / 60;
	
	const long seconds      = ticks / 60;
	const long microseconds = ticks % 60 * microseconds_per_tick;
	
	timeval tv = { seconds, microseconds };
	
	return tv;
}

static
bool wait_for_fd( int fd, timeval* timeout )
{
	fd_set readfds;
	FD_ZERO( &readfds );
	FD_SET( fd, &readfds );
	
	const int max_fd = fd;
	
	int selected = select( max_fd + 1, &readfds, NULL, NULL, timeout );
	
	return selected > 0;
}

static inline
ssize_t direct_read( int fd, unsigned char* buffer, size_t n )
{
	return read( fd, buffer, n );
}

static inline
ssize_t normal_read( int fd, unsigned char* buffer, size_t n )
{
	ssize_t result = read( fd, buffer, sizeof (char) );
	
	if ( result > 0 )
	{
		result = read( fd, buffer + 1, buffer[ 0 ] );
		
		if ( result >= 0 )
		{
			++result;
		}
	}
	
	return result;
}

static inline
ssize_t read( int fd, unsigned char* buffer, size_t n, bool direct )
{
	return direct ? direct_read( fd, buffer, n )
	              : normal_read( fd, buffer, n );
}

static
UInt16 keymods_from_modifiers_high_byte( uint8_t mod )
{
	UInt16 result = mod;
	
	result = result >> 1 | result << 15;
	
	return result;
}

static
void post_event( const splode::ascii_synth_buffer& buffer )
{
	const UInt32 message = buffer.ascii;
	
	PostEvent( keyDown, message );
	PostEvent( keyUp,   message );
}

static
void post_event( const splode::pointer_event_buffer& buffer )
{
	using namespace splode::modes;
	using namespace splode::pointer;
	using splode::uint8_t;
	
	const uint8_t mode_mask = Command | Shift | Option | Control;
	
	const uint8_t mod = buffer.modes & mode_mask;
	
	KeyMods = keymods_from_modifiers_high_byte( mod );
	
	const uint8_t action = buffer.attrs & action_mask;
	
	if ( action == 0 )
	{
		PostEvent( mouseDown, 0 );
		PostEvent( mouseUp,   0 );
		
		return;
	}
	
	MBState = action == 1 ? 0x00 : 0x80;
	
	const short what = action + (mouseDown - splode::pointer::down);
	
	PostEvent( what, 0 );
}

static
void post_event( const splode::ascii_event_buffer& buffer )
{
	using namespace splode::modes;
	using namespace splode::key;
	using splode::uint8_t;
	
	const UInt32 message = buffer.ascii;
	
	const uint8_t mode_mask = Command | Shift | Option | Control;
	const uint8_t attr_mask = Alpha;
	
	const uint8_t mod = (buffer.modes & mode_mask) | (buffer.attrs & attr_mask);
	
	KeyMods = keymods_from_modifiers_high_byte( mod );
	
	const uint8_t action = buffer.attrs & action_mask;
	
	if ( action == 0 )
	{
		PostEvent( keyDown, message );
		PostEvent( keyUp,   message );
		
		return;
	}
	
	const short what = action + (keyDown - splode::key::down);
	
	PostEvent( what, message );
}

static inline
void SetMouse( const splode::pointer_location_buffer& buffer )
{
	Mouse.h = buffer.x;
	Mouse.v = buffer.y;
	
	update_cursor_location();
}

static
void queue_event( int fd )
{
	unsigned char buffer[ 256 ];
	
	/*
		Reading 0 bytes from a normal stream yields 0, but an eventtap stream
		will fail with EINVAL.
	*/
	
	const bool direct = read( fd, buffer, 0 );
	
	const ssize_t n_read = read( fd, buffer, sizeof buffer, direct );
	
	const ssize_t len = buffer[ 0 ];
	
	if ( 1 + len != n_read )
	{
		ExitToShell();
	}
	
	switch ( len )
	{
		default:
		case 0:
			using splode::null_message_buffer;
			break;
		
		case 1:
			post_event( *(splode::ascii_synth_buffer*) buffer );
			break;
		
		case 3:
			post_event( *(splode::pointer_event_buffer*) buffer );
			break;
		
		case 4:
			post_event( *(splode::ascii_event_buffer*) buffer );
			break;
		
		case 5:
			using splode::pointer_location_buffer;
			
			SetMouse( *(pointer_location_buffer*) buffer );
			break;
	}
}

static
void wait_for_user_input()
{
	while ( wait_for_fd( events_fd, &wait_timeout ) )
	{
		wait_timeout = zero_timeout;
		
		queue_event( events_fd );
	}
}

static
void wait_for_user_input( unsigned long ticks )
{
	wait_timeout = timeval_from_ticks( ticks );
	
	wait_for_user_input();
}

static
void poll_user_input()
{
	wait_timeout = zero_timeout;
	
	wait_for_user_input();
}

static unsigned long next_sleep;

static
bool get_lowlevel_event( short eventMask, EventRecord* event )
{
	const short lowlevel_event_mask = mDownMask   | mUpMask
	                                | keyDownMask | keyUpMask | autoKeyMask
	                                | diskMask;
	
	eventMask &= lowlevel_event_mask;
	
	if ( eventMask != autoKeyMask )
	{
		if ( GetOSEvent( eventMask & ~autoKeyMask, event ) )
		{
			return true;
		}
	}
	
	if ( eventMask & autoKeyMask )
	{
		return GetOSEvent( eventMask, event );
	}
	
	return false;
}

pascal unsigned char GetNextEvent_patch( unsigned short  eventMask,
                                         EventRecord*    event )
{
	const unsigned long sleep = next_sleep;
	
	next_sleep = 0;
	
	poll_user_input();
	
	// TODO:  Check for activate events
	
	if ( get_lowlevel_event( eventMask, event ) )
	{
		return true;
	}
	
	if ( CheckUpdate( event ) )
	{
		return true;
	}
	
	wait_for_user_input( sleep );
	
	if ( get_lowlevel_event( eventMask, event ) )
	{
		return true;
	}
	
	/*
		If at any point we return a non-null event, leave next_sleep set to
		zero.  Otherwise, set it to a small but non-zero amount, so we only
		waste a little CPU in applications that call GetNextEvent() instead
		of WaitNextEvent(), rather than consuming one entirely.
	*/
	
	next_sleep = GetNextEvent_throttle;
	
	return false;
}

static inline
asm UInt32 add_pinned( UInt32 a : __D0, UInt32 b : __D1 ) : __D0
{
	ADD.L    D1,D0
	BCC.S    no_carry
	MOVEQ    #-1,D0
no_carry:
}

pascal unsigned char WaitNextEvent_patch( unsigned short  eventMask,
                                          EventRecord*    event,
                                          unsigned long   sleep,
                                          RgnHandle       mouseRgn )
{
	if ( mouseRgn != NULL  &&  EmptyRgn_patch( mouseRgn ) )
	{
		mouseRgn = NULL;
	}
	
	/*
		Time is fleeting.  Keep a local, non-volatile copy of Ticks.
		
		In theory, Ticks could be (future - 1) when compared to future and
		advance to (future + 1) when subtracted from future, since
		each load of Ticks makes a fresh call to gettimeofday() (and the
		same concern would apply it if were updated at interrupt time).
	*/
	
	UInt32 now = Ticks;
	
	/*
		Pin the addition of Ticks and sleep.  In the improbable (but very
		possible) event that an instance of macos runs for over a year,
		Ticks may exceed 2^31, at which point adding 0x7FFFFFFF (2^31 - 1)
		will overflow.  This is bad, because it means that WaitNextEvent()
		calls with a sleep argument of 0x7FFFFFFF (which are intended to
		sleep for an arbitrarily long time) will return immediately, and
		thereby (in all likelihood) consume all available CPU for the next
		year or so (until Ticks passes 2^32 - 1 and overflows on its own).
		
		By pinning the addition to 2^32 - 1, busy-polling is limited to
		one tick (1/60 of a second) after two years or so -- which isn't
		bad at all.  (Though you may have other issues if you let Ticks
		overflow.  I strongly don't recommend it...)
	*/
	
	const UInt32 future = add_pinned( now, sleep );
	
	/*
		First timeout is zero, so we can process update events.
	*/
	
	next_sleep = 0;
	
	while ( true )
	{
		const bool got = GetNextEvent( eventMask, event );
		
		if ( got  ||  event->what != nullEvent )
		{
			return got;
		}
		
		if ( mouseRgn != NULL  &&  ! PtInRgn_patch( event->where, mouseRgn ) )
		{
			event->what    = osEvt;
			event->message = mouseMovedMessage << 24;
			
			return true;
		}
		
		now = Ticks;
		
		if ( now >= future )
		{
			break;
		}
		
		next_sleep = future - now;
	}
	
	return false;
}
