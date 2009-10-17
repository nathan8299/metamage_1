// closedir.hh
// -----------
//
// Maintained by Joshua Juran

// Part of the Nitrogen project.
//
// Written 2007 by Joshua Juran.
//
// This code was written entirely by the above contributor, who places it
// in the public domain.


#ifndef POSEVEN_FUNCTIONS_CLOSEDIR_HH
#define POSEVEN_FUNCTIONS_CLOSEDIR_HH

// POSIX
#include <dirent.h>

// Nucleus
#include "nucleus/owned.hh"

// poseven
#include "poseven/types/dir_t.hh"
#include "poseven/types/errno_t.hh"


namespace nucleus
{
	
	template <>
	struct disposer< poseven::dir_t > : public std::unary_function< poseven::dir_t, void >
	{
		void operator()( poseven::dir_t dir ) const
		{
			poseven::handle_destruction_posix_result( ::closedir( dir ) );
		}
	};
	
}

namespace poseven
{
	
	inline void closedir( nucleus::owned< dir_t > dir )
	{
		throw_posix_result( ::closedir( dir.release() ) );
	}
	
}

#endif

