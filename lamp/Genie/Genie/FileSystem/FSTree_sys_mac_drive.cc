/*	=======================
 *	FSTree_sys_mac_drive.cc
 *	=======================
 */

#if !TARGET_API_MAC_CARBON

#include "Genie/FileSystem/FSTree_sys_mac_drive.hh"

#ifndef __DISKS__
#include <Disks.h>
#endif

// POSeven
#include "POSeven/Errno.hh"

// Genie
#include "Genie/FileSystem/FSTree_QueryFile.hh"
#include "Genie/FileSystem/ResolvePathname.hh"


namespace Genie
{
	
	namespace N = Nitrogen;
	namespace NN = Nucleus;
	namespace p7 = poseven;
	
	
	static const DrvQEl* FindDrive( UInt16 driveNumber )
	{
		N::DriveQueue_Container drives = N::DriveQueue();
		
		typedef N::DriveQueue_Container::const_iterator Iter;
		
		for ( Iter it = drives.begin();  it != drives.end();  ++it )
		{
			if ( it->dQDrive == driveNumber )
			{
				return &*it;
			}
		}
		
		return NULL;
	}
	
	std::string DriveNumber_KeyName_Traits::NameFromKey( const Key& key )
	{
		return Int_KeyName_Traits::NameFromKey( key );
	}
	
	DriveNumber_KeyName_Traits::Key DriveNumber_KeyName_Traits::KeyFromName( const std::string& name )
	{
		int n = Int_KeyName_Traits::KeyFromName( name );
		
		if ( n <= 0  ||  SInt16( n ) != n )
		{
			p7::throw_errno( ENOENT );
		}
		
		N::FSVolumeRefNum driveNum = N::FSVolumeRefNum( n );
		
		return driveNum;
	}
	
	bool DriveNumber_KeyName_Traits::KeyIsValid( const Key& key )
	{
		return FindDrive( key ) != NULL;
	}
	
	
	extern const Functional_Traits< DriveNumber_KeyName_Traits::Key >::Mapping sys_mac_drive_N_Mappings[];
	
	FSTreePtr sys_mac_drive_Details::GetChildNode( const FSTreePtr&    parent,
		                                           const std::string&  name,
		                                           const Key&          key )
	{
		return Premapped_Factory< Key, sys_mac_drive_N_Mappings >( parent, name, key );
	}
	
	
	class FSTree_Driver_Link : public FSTree
	{
		private:
			typedef N::DriverRefNum Key;
			
			Key itsKey;
		
		public:
			FSTree_Driver_Link( const FSTreePtr&    parent,
			                    const Key&          key,
			                    const std::string&  name ) : FSTree( parent, name ),
			                                                 itsKey ( key )
			{
			}
			
			bool IsLink() const  { return true; }
			
			std::string ReadLink() const;
			
			FSTreePtr ResolveLink() const
			{
				return ResolvePathname( ReadLink() );
			}
	};
	
	std::string FSTree_Driver_Link::ReadLink() const
	{
		UnitNumber unit = ~itsKey;
		
		std::string name = NN::Convert< std::string >( unit );
		
		return "/sys/mac/unit/" + name;
	}
	
	
	static FSTreePtr Link_Factory( const FSTreePtr&                 parent,
	                               const std::string&               name,
	                               DriveNumber_KeyName_Traits::Key  key )
	{
		const DrvQEl* el = FindDrive( key );
		
		if ( el == NULL )
		{
			return FSNull();
		}
		
		N::DriverRefNum refNum = N::DriverRefNum( el->dQRefNum );
		
		return MakeFSTree( new FSTree_Driver_Link( parent, refNum, name ) );
	}
	
	struct GetDriveSize
	{
		typedef UInt32 Result;
		
		UInt32 operator()( const DrvQEl& drive ) const
		{
			UInt32 size = drive.dQDrvSz;
			
			if ( drive.qType != 0 )
			{
				size += drive.dQDrvSz2 << 16;
			}
			
			return size;
		}
	};
	
	struct GetDriveFSID
	{
		typedef SInt16 Result;
		
		SInt16 operator()( const DrvQEl& drive ) const
		{
			return drive.dQFSID;
		}
	};
	
	template < class Get >
	class sys_mac_drive_N_Query
	{
		private:
			typedef UInt16 Key;
			
			Key itsKey;
		
		public:
			sys_mac_drive_N_Query( const Key& key ) : itsKey( key )
			{
			}
			
			std::string operator()() const
			{
				const DrvQEl* el = FindDrive( itsKey );
				
				if ( el == NULL )
				{
					return "";
				}
				
				std::string output = NN::Convert< std::string >( Get()( *el ) ) + "\n";
				
				return output;
			}
	};
	
	template < class Get >
	static FSTreePtr Query_Factory( const FSTreePtr&                 parent,
	                                const std::string&               name,
	                                DriveNumber_KeyName_Traits::Key  key )
	{
		typedef sys_mac_drive_N_Query< Get > Query;
		
		typedef FSTree_QueryFile< Query > QueryFile;
		
		return MakeFSTree( new QueryFile( parent, name, Query( key ) ) );
	}
	
	const Functional_Traits< DriveNumber_KeyName_Traits::Key >::Mapping sys_mac_drive_N_Mappings[] =
	{
		{ "driver", &Link_Factory },
		
		{ "fsid", &Query_Factory< GetDriveFSID > },
		{ "size", &Query_Factory< GetDriveSize > },
		
		{ NULL, NULL }
	};
	
}

#endif

