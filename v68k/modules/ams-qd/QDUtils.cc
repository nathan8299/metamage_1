/*
	QDUtils.cc
	----------
*/

#include "QDUtils.hh"

// Mac OS
#ifndef __RESOURCES__
#include <Resources.h>
#endif

// quickdraw
#include "qd/pack_bits.hh"

// ams-common
#include "QDGlobals.hh"


pascal void UnpackBits_patch( Ptr* src, Ptr* dst, short dstBytes )
{
	UInt8** tmp = (UInt8**) dst;
	
	quickdraw::unpack_bits( (const UInt8*&) *src, *tmp, dstBytes );
	
	dst = (Ptr*) tmp;
}

pascal unsigned char BitTst_patch( Ptr addr, long bit )
{
	return (*addr & (1 << 7 - bit)) != 0;
}

pascal void BitSet_patch( Ptr addr, long bit )
{
	const uint8_t mask = 1 << 7 - bit;
	
	*addr |= mask;
}

pascal void BitClr_patch( Ptr addr, long bit )
{
	const uint8_t mask = 1 << 7 - bit;
	
	*addr &= ~mask;
}

pascal short Random_patch()
{
	QDGlobals& qd = get_QDGlobals();
	
	qd.randSeed = UInt64( qd.randSeed * 16807ull ) % 0x7FFFFFFF;
	
	if ( (short) qd.randSeed == (short) 0x8000 )
	{
		return 0;
	}
	
	return qd.randSeed;
}

pascal long BitAnd_patch( long a, long b )
{
	return a & b;
}

static const char hex_table[ 32 ] =
{
	0,  10, 11, 12, 13, 14, 15, 0,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  1,  2,  3,  4,  5,  6,  7,
	8,  9,  0   // more zeros follow
};

const char xdigit_mask = 0x1F;

static inline 
unsigned char unhex( char xdigit )
{
	return hex_table[ xdigit & xdigit_mask ];
}

pascal void StuffHex_patch( char* dst, const unsigned char* srcHex )
{
	uint8_t len = *srcHex++;
	
	unsigned char c;
	
	while ( len > 0 )
	{
		c  = unhex( *srcHex++ ) << 4;
		c |= unhex( *srcHex++ );
		
		*dst++ = c;
		
		len -= 2;
	}
}

pascal void MapPt_patch( Point* pt, const Rect* src, const Rect* dst )
{
	const int srcHeight = src->bottom - src->top;
	const int dstHeight = dst->bottom - dst->top;
	
	const int srcWidth = src->right - src->left;
	const int dstWidth = dst->right - dst->left;
	
	pt->v = (pt->v - src->top ) * dstHeight / srcHeight + dst->top;
	pt->h = (pt->h - src->left) * dstHeight / srcHeight + dst->left;
}

pascal void MapRect_patch( Rect* r, const Rect* src, const Rect* dst )
{
	MapPt_patch( (Point*) &r->top,    src, dst );
	MapPt_patch( (Point*) &r->bottom, src, dst );
}

pascal PatHandle GetPattern_patch( short id )
{
	return (PatHandle) GetResource( 'PAT ', id );
}

pascal CursHandle GetCursor_patch( short id )
{
	return (CursHandle) GetResource( 'CURS', id );
}

pascal PicHandle GetPicture_patch( short id )
{
	return (PicHandle) GetResource( 'PICT', id );
}

pascal long DeltaPoint_patch( Point a, Point b )
{
	Point delta;
	
	delta.v = a.v - b.v;
	delta.h = a.h - b.h;
	
	return *(long*) &delta;
}
