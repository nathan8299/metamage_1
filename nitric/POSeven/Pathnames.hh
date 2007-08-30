// Pathnames.hh
// ------------
//
// Maintained by Joshua Juran

// Part of the Nitrogen project.
//
// Written 2007 by Joshua Juran.
//
// This code was written entirely by the above contributor, who places it
// in the public domain.


#ifndef POSEVEN_PATHNAMES_HH
#define POSEVEN_PATHNAMES_HH

// Standard C++
#include <string>

// POSIX
#include <unistd.h>

// Io
#include "io/files.hh"

// POSeven
#include "POSeven/Errno.hh"


namespace POSeven
{
	
	inline void Unlink( const char* pathname )
	{
		ThrowPOSIXResult( ::unlink( pathname ) );
	}
	
	inline void Unlink( const std::string& pathname )
	{
		Unlink( pathname.c_str() );
	}
	
	inline void RemoveDir( const char* pathname )
	{
		ThrowPOSIXResult( ::rmdir( pathname ) );
	}
	
	inline void RemoveDir( const std::string& pathname )
	{
		RemoveDir( pathname.c_str() );
	}
	
}

namespace poseven
{
	
	inline void unlink( const char* pathname )
	{
		POSeven::Unlink( pathname );
	}
	
	inline void unlink( const std::string& pathname )
	{
		POSeven::Unlink( pathname );
	}
	
	inline void rmdir( const char* pathname )
	{
		POSeven::RemoveDir( pathname );
	}
	
	inline void rmdir( const std::string& pathname )
	{
		POSeven::RemoveDir( pathname );
	}
	
}

namespace io
{
	
	inline std::string get_filename( const std::string& pathname, overload = overload() )
	{
		std::string result = pathname;
		
		std::size_t last_slash = result.find_last_of( "/" );
		
		if ( last_slash == result.npos )
		{
			return result;
		}
		
		if ( last_slash == result.size() - 1 )
		{
			result.resize( last_slash );
			
			last_slash = result.find_last_of( "/" );
		}
		
		return result.substr( last_slash + 1, result.npos );
	}
	
	inline std::string get_filename_string( const std::string& pathname, overload = overload() )
	{
		return get_filename( pathname );
	}
	
	inline std::string get_preceding_directory( const std::string& pathname, overload = overload() )
	{
		std::string result = pathname;
		
		std::size_t last_slash = result.find_last_of( "/" );
		
		if ( last_slash == result.npos )
		{
			return ".";
		}
		
		if ( last_slash == result.size() - 1 )
		{
			result.resize( last_slash );
			
			last_slash = result.find_last_of( "/" );
		}
		
		result.resize( last_slash + 1 );
		
		return result;
	}
	
	inline std::string get_parent_directory_of_directory( const std::string& pathname, overload = overload() )
	{
		std::string result = pathname;
		
		if ( *result.rbegin() != '/' )
		{
			result += '/';
		}
		
		result += "..";
		
		return result;
	}
	
	// Path descent
	
	inline std::string path_descent( std::string path, const char* name, std::size_t length )
	{
		if ( *path.rbegin() != '/' )
		{
			path += '/';
		}
		
		path.append( name, length );
		
		return path;
	}
	
	namespace path_descent_operators
	{
		
		inline std::string operator/( const std::string& path, const std::string& name )
		{
			return path_descent( path, name.data(), name.size() );
		}
		
		inline std::string operator/( const std::string& path, const char* name )
		{
			return path_descent( path, name, std::strlen( name ) );
		}
		
	}
	
	// Delete
	
	inline void delete_file( const std::string& file, overload = overload() )
	{
		POSeven::Unlink( file );
	}
	
	inline void delete_file_only( const std::string& file, overload = overload() )
	{
		delete_file( file );
	}
	
	inline void delete_empty_directory( const std::string& dir, overload = overload() )
	{
		POSeven::RemoveDir( dir );
	}
	
	inline void delete_empty_directory_only( const std::string& dir, overload = overload() )
	{
		delete_empty_directory( dir );
	}
	
}

#endif

