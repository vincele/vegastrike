#include <config.h>
#include <sstream>

#include "vsnet_headers.h"
#include "vsnet_socket.h"
#include "vsnet_socketset.h"
#include "vsnet_pipe.h"
#include "vsnet_debug.h"
#include "const.h"

using namespace std;

SocketSet::SocketSet( bool blockmainthread )
    : VSThread( false )
    , _blockmain( blockmainthread )
    , _blockmain_pending( 0 )
{
#ifndef USE_NO_THREAD
    _thread_end = false;
#endif
}

SocketSet::~SocketSet( )
{
#ifndef USE_NO_THREAD
    _thread_mx.lock( );
    _thread_end = true;
    _blockmain  = false; // signalling would be dangerous
    _thread_wakeup.closewrite();
    _thread_cond.wait( _thread_mx );
    _thread_wakeup.closeread();
    _thread_mx.unlock( );
#endif
}

void SocketSet::set( VsnetSocketBase* s )
{
    _autoset.insert( s );
    private_wakeup( );
}

void SocketSet::unset( VsnetSocketBase* s )
{
    _autoset.erase( s );
    private_wakeup( );
}

#ifdef USE_NO_THREAD
void SocketSet::wait( )
{
    assert( _blockmain ); // can't call wait if we haven't ordered the feature
    if( _blockmain_pending == 0 )
    {
        private_select( NULL );
    }
    else
    {
#ifdef VSNET_DEBUG
        std::ostringstream ostr;
        for( int i=0; i<_blockmain_pending; i++ )
        {
            if( FD_ISSET( i, &_blockmain_set ) ) ostr << " " << i;
        }
        COUT << "something pending for sockets:"
             << ostr.str()
             << " (" << _blockmain_pending << ")" << endl;
#endif
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = 0;
        private_select( &tv );
    }
}
#else
void SocketSet::wait( )
{
    assert( _blockmain ); // can't call wait if we haven't ordered the feature
    _blockmain_mx.lock( );
    if( _blockmain_pending == 0 )
    {
        _blockmain_cond.wait( _blockmain_mx );
    }
#ifdef VSNET_DEBUG
    else
    {
        std::ostringstream ostr;
        for( int i=0; i<_blockmain_pending; i++ )
        {
            if( FD_ISSET( i, &_blockmain_set ) ) ostr << " " << i;
        }
        COUT << "something pending for sockets:"
             << ostr.str()
             << " (" << _blockmain_pending << ")" << endl;
    }
#endif
    _blockmain_mx.unlock( );
}
#endif

void SocketSet::add_pending( int fd )
{
    if( _blockmain )
    {
        _blockmain_mx.lock( );
	    FD_SET( fd, &_blockmain_set );
	    if( fd >= _blockmain_pending ) _blockmain_pending = fd + 1;
        _blockmain_mx.unlock( );
    }
}

void SocketSet::rem_pending( int fd )
{
    if( _blockmain )
    {
        _blockmain_mx.lock( );
	    FD_CLR( fd, &_blockmain_set );
	    if( fd == _blockmain_pending-1 )
	    {
	        while( _blockmain_pending > 0 )
	        {
	            if( FD_ISSET( _blockmain_pending-1, &_blockmain_set ) )
		            break;
                _blockmain_pending -= 1;
	        }
        }
        _blockmain_mx.unlock( );
    }
}

void SocketSet::private_addset( int fd, fd_set& fds, int& maxfd )
{
    FD_SET( fd, &fds );
    if( fd >= maxfd ) maxfd = fd+1;
}

int SocketSet::private_select( timeval* timeout )
{
    fd_set read_set_select;
    fd_set write_set_select;
    int    max_sock_select = 0;

    FD_ZERO( &read_set_select );
    FD_ZERO( &write_set_select );

    private_test_dump_request_sets( timeout );

    for( Set::iterator it = _autoset.begin(); it != _autoset.end(); it++ )
    {
	    VsnetSocketBase* b = (*it);
        int fd = b->get_fd();
        if( fd >= 0 )
        {
            private_addset( fd, read_set_select, max_sock_select );
	        if( b->need_test_writable( ) )
	        {
                private_addset( b->get_write_fd(),
                                write_set_select,
                                max_sock_select );
	        }
        }
    }

#ifndef USE_NO_THREAD
    private_addset( _thread_wakeup.getread(),
                    read_set_select,
                    max_sock_select );
#endif

    int ret = ::select( max_sock_select,
	                &read_set_select, &write_set_select, 0, timeout );

    if( ret == -1 )
    {
#if defined(_WIN32) && !defined(__CYGWIN__)
        if( WSAGetLastError()!=WSAEINVAL)
            COUT<<"WIN32 error : "<<WSAGetLastError()<<endl;
#else
        perror( "Select failed : ");
#endif
    }
    else if( ret == 0 )
    {
    }
    else
    {
        private_test_dump_active_sets( read_set_select, write_set_select );

        for( Set::iterator it = _autoset.begin(); it != _autoset.end(); it++ )
        {
	        VsnetSocketBase* b = (*it);
            int fd = b->get_fd();
	        if( fd >= 0 )
	        {
                if( FD_ISSET(fd,&read_set_select) )
                    b->lower_selected( );

                if( FD_ISSET(b->get_write_fd(),&write_set_select) )
                    b->lower_sendbuf( );
	        }
        }

#ifndef USE_NO_THREAD
        if( FD_ISSET( _thread_wakeup.getread(), &read_set_select ) )
        {
            char c;
            _thread_wakeup.read( &c, 1 );
        }
#endif
    }

    if( _blockmain )
    {
        // whatever the reason for leaving select, if we have been asked
        // to signal the main thread on wakeup, we do it
        _blockmain_mx.lock( );
        _blockmain_cond.signal( );
        _blockmain_mx.unlock( );
    }

    return ret;
}

void SocketSet::wakeup( )
{
    private_wakeup( );
}

#ifdef USE_NO_THREAD
/// unthreaded case
void SocketSet::private_wakeup( )
{
}

/// unthreaded case
void SocketSet::waste_time( long sec, long usec )
{
    struct timeval tv;
    tv.tv_sec  = sec;
    tv.tv_usec = usec;
    private_select( &tv );
}
#else
/// threaded case
void SocketSet::private_wakeup( )
{
    char c = 'w';
    _thread_wakeup.write( &c, 1 );
}

/// threaded case
void SocketSet::waste_time( long sec, long usec )
{
    struct timeval tv;
    tv.tv_sec  = sec;
    tv.tv_usec = usec;
    select( 0, NULL, NULL, NULL, &tv );
}
#endif

void SocketSet::run( )
{
#ifndef USE_NO_THREAD
    while( !_thread_end )
    {
        private_select( NULL );
    }
    _thread_mx.lock( );
    _thread_cond.signal( );
    _thread_mx.unlock( );
#endif
}

void SocketSet::private_test_dump_active_sets( const fd_set& read_set_select,
                                               const fd_set& write_set_select )
{
#ifdef VSNET_DEBUG
    std::ostringstream ostr;
    for( Set::iterator it = _autoset.begin(); it != _autoset.end(); it++ )
    {
        VsnetSocketBase* b = (*it);
        int fd = b->get_fd();
        if( fd >= 0 )
        {
            if( FD_ISSET(fd,&read_set_select) )
            {
                ostr << fd << " ";
            }
            if( FD_ISSET(b->get_write_fd(),&write_set_select) )
            {
                ostr << "+" << b->get_write_fd() << " ";
            }
        }
    }

#ifndef USE_NO_THREAD
    if( FD_ISSET( _thread_wakeup.getread(), &read_set_select ) )
    {
        ostr << _thread_wakeup.getread() << "(w)";
    }
#endif

    ostr << ends;
    COUT << "select saw activity on fds=" << ostr.str() << endl;
#endif
}

void SocketSet::private_test_dump_request_sets( timeval* timeout )
{
#ifdef VSNET_DEBUG
    std::ostringstream ostr;
    ostr << "calling select with fds=";
    for( Set::iterator it = _autoset.begin(); it != _autoset.end(); it++ )
    {
	    VsnetSocketBase* b = (*it);
        int fd = b->get_fd();
        if( fd >= 0 )
        {
            ostr << fd << " ";
	        if( b->need_test_writable( ) )
	        {
                ostr << "+" << b->get_write_fd() << " ";
	        }
        }
    }

#ifndef USE_NO_THREAD
    ostr << _thread_wakeup.getread() << "(w)";
#endif
    if( timeout )
        ostr << " t=" << timeout->tv_sec << ":" << timeout->tv_usec;
    else
        ostr << " t=NULL (blocking)";
    ostr << ends;

    if( !timeout || timeout->tv_sec >= 1 )
    {
        COUT << ostr.str() << endl;
    }
    else
    {
        static long waitabit = 0;
        waitabit += 1;
        if( waitabit > 100 )
        {
            COUT << ostr.str() << endl;
            waitabit = 0;
        }
    }
#endif
}

