#include <config.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iomanip>

#include "packetmem.h"

using std::ostream;
using std::endl;

/***********************************************************************
 * PacketMem - definition
 ***********************************************************************/

PacketMem::PacketMem( )
{
    MAKE_VALID
    _buffer  = NULL;
    _len     = 0;
    _cnt     = NULL;
}

PacketMem::PacketMem( const PacketMem& orig )
{
    MAKE_VALID
    _buffer  = orig._buffer;
    _len     = orig._len;
    _cnt     = orig._cnt;

    if( _cnt ) ref();
}

PacketMem::PacketMem( size_t bytesize )
{
    MAKE_VALID
    _buffer  = new char[bytesize];
    _len     = bytesize;
    _cnt     = new size_t;
    *_cnt    = 1;
}

PacketMem::PacketMem( const void* buffer, size_t size )
{
    MAKE_VALID
    if( buffer != NULL )
    {
        _buffer  = new char[size];
	_len     = size;
        _cnt     = new size_t;
	*_cnt    = 1;

	memcpy( _buffer, buffer, size );
    }
    else
    {
        _buffer  = NULL;
	_len     = 0;
        _cnt     = NULL;
    }
}

PacketMem::PacketMem( void* buffer, size_t size, bool own )
{
    MAKE_VALID
    inner_set( buffer, size, own );
}

void PacketMem::set( void* buffer, size_t size, bool own )
{
    release( );
    inner_set( buffer, size, own );
}

PacketMem::~PacketMem( )
{
    release( );
    MAKE_INVALID
}

PacketMem& PacketMem::operator=( const PacketMem& orig )
{
    release( );

    _buffer  = orig._buffer;
    _len     = orig._len;
    _cnt     = orig._cnt;

    if( _cnt ) ref( );

    return *this;
}

void PacketMem::release( )
{
    if( _cnt == NULL ) return;

    unref( );
    if( *_cnt != 0 ) return;

    delete _cnt;
    if( _buffer == NULL ) return;

    delete [] _buffer;
}

void PacketMem::inner_set( void* buffer, size_t size, bool own )
{
    if( buffer != NULL )
    {
	if( own == false )
	{
            _buffer  = new char[size];
	    _len     = size;
            _cnt     = new size_t;
	    *_cnt    = 1;

	    memcpy( _buffer, buffer, size );
	}
	else
	{
            _buffer  = (char*)buffer;
            _len     = size;
            _cnt     = new size_t;
	    *_cnt    = 1;
	}
    }
    else
    {
        _buffer  = NULL;
	_len     = 0;
        _cnt     = NULL;
    }
}

void PacketMem::dump( ostream& ostr, size_t indent_depth ) const
{
    static const size_t LEN = 15;

    char x[LEN];
    char* buf  = _buffer;
    size_t len = _len;
    while( len > 0 )
    {
        for( size_t i=0; i<indent_depth; i++ ) ostr << " ";
	memset( x, -1, LEN );
	for( size_t i=0; i<LEN && len>0; i++ )
	{
	    x[i] = *buf;
	    buf++;
	    len--;
	}
	for( size_t i=0; i<LEN; i++ )
	{
	    ostr << " " << std::setw(2) << std::hex << (((unsigned int)x[i])&0xff);
	}
	ostr << "   ";
	for( size_t i=0; i<LEN; i++ )
	{
	    if( isprint(x[i]) ) ostr << x[i]; else ostr << "@";
	}
	ostr << endl;
    }
    ostr << std::dec;
}

bool PacketMem::operator==( const PacketMem& r ) const
{
    if( _buffer == r._buffer ) return true;
    if( _len != r._len ) return false;
    if( memcmp( _buffer, r._buffer, _len ) == 0 ) return true;
    return false;
}

size_t PacketMem::len() const
{
    return _len;
}

const char* PacketMem::getConstBuf() const
{
    return _buffer;
}

char* PacketMem::getVarBuf()
{
    return _buffer;
}

void PacketMem::ref( )
{
    assert( _cnt );
    (*_cnt)++;
}

void PacketMem::unref( )
{
    assert( _cnt );
    assert( *_cnt > 0 );
    (*_cnt)--;
}

