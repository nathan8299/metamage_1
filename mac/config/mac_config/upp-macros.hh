/*
	upp-macros.hh
	-------------
*/

#ifndef MACCONFIG_UPPMACROS_HH
#define MACCONFIG_UPPMACROS_HH


#ifndef TARGET_RT_MAC_CFM
#error  "TARGET_RT_MAC_CFM undefined.  Include any Mac OS header first."
#endif


#if ! TARGET_RT_MAC_CFM
	
	/*
		Null definitions for classic 68K and Mach-O.
		(UPPs are plain functions, not routine descriptors.)
	*/
	
	#define DEFINE_UPP2( UPPTypeName, proc )  /**/
	
	#define UPP_ARG( proc )  &proc
	
#else
	
	/*
		CFM:  Create a routine descriptor at static initialization time.
	*/
	
	#define UPP_VARNAME( proc )  proc ## _upp
	
	#define DEFINE_UPP2( UPPTypeName, proc )  \
	static UPPTypeName UPP_VARNAME( proc ) = New ## UPPTypeName( &proc );
	
	#define UPP_ARG( proc )  UPP_VARNAME( proc )
	
#endif
	
	#define DEFINE_UPP( BaseName, proc )  DEFINE_UPP2( BaseName ## UPP, proc )
	
#if TARGET_CPU_PPC  &&  ! OPAQUE_UPP_TYPES
	
	/*
		Non-Carbon PPC uses plain function pointers for TPPs.
	*/
	
	#define DEFINE_TPP( BaseName, proc )  /**/
	
	#define TPP_ARG( proc )  &proc
	
#else
	
	/*
		Carbon and CFM-68K use UPPs (routine descriptors) for TPPs.
		Non-Carbon PPC with OPAQUE_UPP_TYPES enabled also uses UPPs.
		In Classic 68K, UPPs are plain function pointers, so it's moot.
	*/
	
	#define DEFINE_TPP( BaseName, proc )  DEFINE_UPP( BaseName, proc )
	
	#define TPP_ARG( proc )  UPP_ARG( proc )
	
#endif
	
#if ! TARGET_RT_MAC_CFM  ||  TARGET_API_MAC_CARBON
	
	/*
		For non-CFM, use the null macro above.
		For Carbon CFM, create a routine descriptor as usual.
	*/
	
	#define DEFINE_CARBON_UPP( BaseName, proc )  DEFINE_UPP( BaseName, proc )
	
#else
	
	/*
		For non-Carbon CFM, declare the UPP variable extern, but don't
		define it.  Since these are Carbon event handlers, installing them
		would be a run-time error, so make it a link error instead, and don't
		waste resources creating the routine descriptors.
	*/
	
	#define DEFINE_CARBON_UPP( BaseName, proc )  \
	extern BaseName ## UPP UPP_VARNAME( proc );
	
#endif

#endif
