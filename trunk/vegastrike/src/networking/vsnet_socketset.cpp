#include <config.h>

#include "vsnet_socket.h"
#include "vsnet_socketset.h"
#include "const.h"

using namespace std;

SocketSet::SocketSet( )
{
    clear();
}

void SocketSet::autosetRead( VsnetSocketBase* s )
{
    _autoset.insert( s );
}

void SocketSet::autounsetRead( VsnetSocketBase* s )
{
    _autoset.erase( s );
}

void SocketSet::setRead( int fd )
{
    FD_SET( fd, &_read_set_select );
    if( fd >= _max_sock_select ) _max_sock_select = fd+1;
}

void SocketSet::setReadAlwaysTrue( int fd )
{
    FD_SET( fd, &_read_set_always_true );
    if( fd >= _max_sock_always_true ) _max_sock_always_true = fd+1;
}

bool SocketSet::is_set( int fd ) const
{
    COUT << "adding " << fd << " to set" << endl;
    return ( FD_ISSET( fd, &_read_set_select ) ||
             FD_ISSET( fd, &_read_set_always_true ) );
}

bool SocketSet::is_setRead( int fd ) const
{
    return ( FD_ISSET( fd, &_read_set_select ) );
}

void SocketSet::clear( )
{
    FD_ZERO( &_read_set_select );
    _max_sock_select = 0;
    FD_ZERO( &_read_set_always_true );
    _max_sock_always_true = -1;
}

int SocketSet::select( long sec, long usec )
{
    timeval tv;
    tv.tv_sec  = sec;
    tv.tv_usec = usec;
    return select( &tv );
}

int SocketSet::select( timeval* timeout )
{
#ifdef VSNET_DEBUG
    COUT << "enter " << __PRETTY_FUNCTION__ << " fds=";
#endif

    for( set<VsnetSocketBase*>::iterator it = _autoset.begin(); it != _autoset.end(); it++ )
    {
        int fd = (*it)->get_fd();
        if( fd >= 0 )
        {
            setRead( fd );
            if( (*it)->needReadAlwaysTrue() )
            {
                setReadAlwaysTrue( fd );
            }
        }
    }

#ifdef VSNET_DEBUG
    for( int i=0; i<_max_sock_select; i++ )
    {
        if( FD_ISSET(i,&_read_set_select) ) cout << i << " ";
    }
    if( timeout )
        cout << " t=" << timeout->tv_sec << ":"
                  << timeout->tv_usec << endl;
    else
        cout << " t=NULL (blocking)" << endl;
#endif

    if( _max_sock_always_true > 0 )
    {
        /* There is pending data in the read queue. We must not wait until
         * more data arrives. It is dumb that dontwait needs reinitialization
         * but the Linux version of select requires it.
         */
        static timeval dontwait;
        dontwait.tv_sec  = 0;
        dontwait.tv_usec = 0;
	    timeout = &dontwait;
#ifdef FIND_WIN_NBIO
        COUT << "Pending data in read queue: don't block" << endl;
#endif
    }

    int ret = ::select( _max_sock_select, &_read_set_select, 0, 0, timeout );
    if( ret == -1 )
    {
#if defined(_WIN32) && !defined(__CYGWIN__)
        if( WSAGetLastError()!=WSAEINVAL)
            COUT<<"WIN32 error : "<<WSAGetLastError()<<endl;
#else
        perror( "Select failed : ");
#endif
    }
    return ret;
}

