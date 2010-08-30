/*
	Genie/FS/gui/port/ADDR.hh
	-------------------------
*/

#ifndef GENIE_FS_GUI_PORT_ADDR_HH
#define GENIE_FS_GUI_PORT_ADDR_HH

// Genie
#include "Genie/FS/FSTree_Directory.hh"


namespace Pedestal
{
	
	class View;
	
}

namespace Genie
{
	
	extern const FSTree_Premapped::Mapping sys_port_ADDR_Mappings[];
	
	void notify_port_of_view_loss( const FSTree* port_key, const FSTree* view );
	
	void RemoveUserWindow( const FSTree* key );
	
	bool InvalidateWindow( const FSTree* key );
	
	void InstallViewInWindow    ( const boost::intrusive_ptr< Pedestal::View >& view, const FSTree* key );
	void UninstallViewFromWindow( const boost::intrusive_ptr< Pedestal::View >& view, const FSTree* key );
	
	const FSTree* GetWindowFocus( const FSTree* window );
	
	void SetWindowFocus( const FSTree* window, const FSTree* focus );
	
}

#endif

