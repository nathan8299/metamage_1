// spew.hh
// -------
//
// Maintained by Joshua Juran

// Part of the Nitrogen project.
//
// Written 2008 by Joshua Juran.
//
// This code was written entirely by the above contributor, who places it
// in the public domain.


#ifndef POSEVEN_EXTRAS_SPEW_HH
#define POSEVEN_EXTRAS_SPEW_HH

// Standard C++
#include <string>

// Io
#include "io/spew.hh"

// Nucleus
#include "Nucleus/Flattener.h"

// POSeven
#include "POSeven/FileDescriptor.hh"
#include "POSeven/functions/open.hh"


namespace poseven
{
	
	inline void spew( fd_t fd, const char* buffer, std::size_t length )
	{
		return io::spew_output( fd, buffer, length );
	}
	
	inline void spew( const std::string& path, const char* buffer, std::size_t length )
	{
		return io::spew_file( path, buffer, length );
	}
	
	inline void spew( const std::string& path, const std::string& stuff )
	{
		return io::spew_file< Nucleus::StringFlattener< std::string > >( path, stuff );
	}
	
	
	inline void spew( const char* path, const char* buffer, std::size_t length )
	{
		return io::spew_file( path, buffer, length );
	}
	
	inline void spew( const char* path, const std::string& stuff )
	{
		return io::spew_output< Nucleus::StringFlattener< std::string > >( open( path, o_wronly | o_trunc ).get(), stuff );
	}
	
}

#endif

