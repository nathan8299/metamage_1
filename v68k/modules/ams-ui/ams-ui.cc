/*
	ams-ui.cc
	---------
*/

// Mac OS
#ifndef __TRAPS__
#include <Traps.h>
#endif
#ifndef __QUICKDRAW__
#include <Quickdraw.h>
#endif

// POSIX
#include <unistd.h>

// ams-ui
#include "Controls.hh"
#include "Drag.hh"
#include "Icons.hh"
#include "Menus.hh"
#include "StrUtils.hh"
#include "Windows.hh"


#define STR_LEN( s )  "" s, (sizeof s - 1)

#define PROGRAM  "ams-ui"

#define WARN( msg )  write( STDERR_FILENO, STR_LEN( PROGRAM ": " msg "\n" ) )


BitMap IconBitmap : 0x0A0E;

void* toolbox_trap_table[] : 3 * 1024;

#define TBTRAP( Proc )  (toolbox_trap_table[ _##Proc & 0x03FF ] = &Proc##_patch)

enum
{
	_BeginUpdate = _BeginUpDate,
	_CheckUpdate = _CheckUpDate,
	_EndUpdate   = _EndUpDate,
};


static
void initialize_low_memory_globals()
{
	IconBitmap.rowBytes = 4;
	IconBitmap.bounds.top  = 0;
	IconBitmap.bounds.left = 0;
	IconBitmap.bounds.bottom = 32;
	IconBitmap.bounds.right  = 32;
}

static
void install_Windows()
{
	TBTRAP( DrawGrowIcon  );  // A904
	TBTRAP( DragGrayRgn   );  // A905
	TBTRAP( NewString     );  // A906
	TBTRAP( SetString     );  // A907
	
	TBTRAP( CalcVis       );  // A909
	TBTRAP( CalcVBehind   );  // A90A
	TBTRAP( ClipAbove     );  // A90B
	TBTRAP( PaintOne      );  // A90C
	TBTRAP( PaintBehind   );  // A90D
	
	TBTRAP( CheckUpdate   );  // A911
	TBTRAP( InitWindows   );  // A912
	TBTRAP( NewWindow     );  // A913
	TBTRAP( DisposeWindow );  // A914
	
	TBTRAP( GetWTitle     );  // A919
	TBTRAP( SetWTitle     );  // A91A
	TBTRAP( MoveWindow    );  // A91B
	TBTRAP( HiliteWindow  );  // A91C
	
	TBTRAP( SizeWindow    );  // A91D
	TBTRAP( TrackGoAway   );  // A91E
	TBTRAP( SelectWindow  );  // A91F
	TBTRAP( BringToFront  );  // A920
	
	TBTRAP( BeginUpdate   );  // A922
	TBTRAP( EndUpdate     );  // A923
	TBTRAP( FrontWindow   );  // A924
	TBTRAP( DragWindow    );  // A925
	TBTRAP( DragTheRgn    );  // A926
	TBTRAP( InvalRgn      );  // A927
	TBTRAP( InvalRect     );  // A928
	TBTRAP( ValidRgn      );  // A929
	TBTRAP( ValidRect     );  // A92A
	
	TBTRAP( GrowWindow    );  // A92B
	TBTRAP( FindWindow    );  // A92C
	TBTRAP( CloseWindow   );  // A92D
}

static
void install_Menus()
{
	TBTRAP( InitMenus    );  // A930
	TBTRAP( DrawMenuBar  );  // A937
	TBTRAP( FlashMenuBar );  // A94C
}

static void install_Controls()
{
	TBTRAP( NewControl     );  // A954
	TBTRAP( DisposeControl );  // A955
	TBTRAP( KillControls   );  // A956
	
	TBTRAP( TrackControl   );  // A968
	TBTRAP( DrawControls   );  // A969
	
	TBTRAP( FindControl    );  // A96C
}

static
void install_IconUtilities()
{
	TBTRAP( IconDispatch );  // ABC9
}

static
asm void module_suspend()
{
	JSR      0xFFFFFFF8
}

int main( int argc, char** argv )
{
	if ( argc > 0 )
	{
		char* const* args = ++argv;
		
		if ( *args != NULL )
		{
			WARN( "no arguments allowed" );
			
			_exit( 1 );
		}
	}
	
	initialize_low_memory_globals();
	
	install_Windows();
	install_Menus();
	install_Controls();
	
	install_IconUtilities();
	
	module_suspend();  // doesn't return
}
