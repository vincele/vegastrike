#include <config.h>

#include <list>
#include <fcntl.h>
#include "vsnet_socket.h"
#include "const.h"

using std::cout;
using std::cerr;
using std::endl;

#if defined(_WIN32) && !defined(__CYGWIN__)
#else
  #ifndef SOCKET_ERROR
  #define SOCKET_ERROR -1
  #endif
#endif

LOCALCONST_DEF(SOCKETALT,bool,TCP,1)
LOCALCONST_DEF(SOCKETALT,bool,UDP,0)

/***********************************************************************
 * VsnetUDPSocket - declaration
 ***********************************************************************/
 
class VsnetUDPSocket : public VsnetSocket
{
public:
    VsnetUDPSocket( ) { }

    VsnetUDPSocket( int sock, const AddressIP& remote_ip )
        : VsnetSocket( sock, remote_ip )
    { }

    VsnetUDPSocket( const VsnetUDPSocket& orig )
        : VsnetSocket( orig )
    { }

    VsnetUDPSocket& operator=( const VsnetUDPSocket& orig )
    {
        VsnetSocket::operator=( orig );
        return *this;
    }

    virtual bool isTcp() const { return false; }

    virtual int  sendbuf( PacketMem& packet, const AddressIP* to);
    virtual int  recvbuf( void *buffer, unsigned int &len, AddressIP *from);
    virtual void ack( );

    virtual void disconnect( const char *s, bool fexit );

    virtual void dump( std::ostream& ostr );

    virtual bool isActive( SocketSet& set ) { return set.is_set(fd); }
};

/***********************************************************************
 * VsnetTCPSocket - declaration
 ***********************************************************************/
 
class VsnetTCPSocket : public VsnetSocket
{
public:
    VsnetTCPSocket( )
        : _incomplete_packet( 0 )
	, _incomplete_len_field( 0 )
	, _connection_closed( false )
    { }

    VsnetTCPSocket( int sock, const AddressIP& remote_ip )
        : VsnetSocket( sock, remote_ip )
        , _incomplete_packet( 0 )
	, _incomplete_len_field( 0 )
	, _connection_closed( false )
    { }

    VsnetTCPSocket( const VsnetTCPSocket& orig )
        : VsnetSocket( orig )
    { }

    VsnetTCPSocket& operator=( const VsnetTCPSocket& orig )
    {
        VsnetSocket::operator=( orig );
        return *this;
    }

    virtual ~VsnetTCPSocket( );

    virtual bool isTcp() const { return true; }

    virtual int  sendbuf( PacketMem& packet, const AddressIP* to);
    virtual int  recvbuf( void *buffer, unsigned int &len, AddressIP *from);
    virtual void ack( );

    virtual void disconnect( const char *s, bool fexit );

    virtual void dump( std::ostream& ostr );

    virtual bool isActive( SocketSet& set );

private:
    struct blob
    {
        char*  buf;
	size_t present_len;
	size_t expected_len;

	blob( ) : buf(0), present_len(0), expected_len(0) { }

	blob( size_t len ) : present_len(0), expected_len(len)
	{
	    buf = new char[len];
	}
	
	~blob( )
	{
	    delete [] buf;
	}

    private:
        blob( const blob& orig );             // forbidden
        blob& operator=( const blob& orig );  // forbidden
    };

    /** if we have received part of a TCP packet but not the complete packet,
     *  the expected length and received number of bytes are stored in
     *  _incomplete_packet. If several packets have been received at once, but
     *  the application processes them one at a time, the received, unprocessed
     *  packets are stored in the list.
     */
    std::list<blob*> _complete_packets;
    blob*       _incomplete_packet;

    /** We send 4 bytes as a packet length indicator. Unfortunately, even these
     *  4 bytes may be split by TCP. These two variables are needed for collecting
     *  the 4 bytes.
     *  Note: for the obvious reason that this happens rarely, the collection code
     *        can not be considered tested.
     */
    int         _incomplete_len_field;
    char        _len_field[4];

    /** Closed connections are noticed in isActive but evaluated by the application
     *  after recvbuf. So, we remember the situation here until the application
     *  notices it.
     */
    bool        _connection_closed;
};

/***********************************************************************
 * SOCKETALT
 ***********************************************************************/
 
SOCKETALT::SOCKETALT( )
{
}

SOCKETALT::SOCKETALT( int sock, bool mode, const AddressIP& remote_ip )
{
    if( mode == TCP )
        _sock = new VsnetTCPSocket( sock, remote_ip );
    else
        _sock = new VsnetUDPSocket( sock, remote_ip );
}

SOCKETALT::SOCKETALT( const SOCKETALT& orig )
{
    _sock = orig._sock;
}

SOCKETALT& SOCKETALT::operator=( const SOCKETALT& orig )
{
    _sock = orig._sock;
    return *this;
}

void SOCKETALT::set_fd( int sock, bool mode )
{
    if( _sock.isNull() )
    {
	_sock = (mode  ? (VsnetSocket*)new VsnetTCPSocket : (VsnetSocket*)new VsnetUDPSocket );
        _sock->set_fd( sock );
    }
    else
    {
        if( (mode==TCP && _sock->isTcp()) || (mode==UDP && !_sock->isTcp()) )
        {
	    _sock->set_fd( sock );
        }
        else
        {
	    _sock = (mode  ? (VsnetSocket*)new VsnetTCPSocket : (VsnetSocket*)new VsnetUDPSocket );
	    _sock->set_fd( sock );
        }
    }
}

std::ostream& operator<<( std::ostream& ostr, const SOCKETALT& s )
{
    if( !s._sock.isNull() ) s._sock->dump( ostr );
    else ostr << "NULL";
    return ostr;
}

bool operator==( const SOCKETALT& l, const SOCKETALT& r )
{
    if( l._sock.isNull() )
    {
        return ( r._sock.isNull() ? true : false );
    }
    else if( r._sock.isNull() )
    {
        return false;
    }
    else
    {
        return l._sock->eq(*r._sock);
    }
}

/***********************************************************************
 * VsnetSocket - definition
 ***********************************************************************/

VsnetSocket::VsnetSocket( )
    : fd(0)
{
}

VsnetSocket::VsnetSocket( int sock, const AddressIP& remote_ip )
    : fd( sock )
    , _remote_ip( remote_ip )
{
}

VsnetSocket::VsnetSocket( const VsnetSocket& orig )
    : fd( orig.fd )
    , _remote_ip( orig._remote_ip )
{
}

VsnetSocket::~VsnetSocket( )
{
}

VsnetSocket& VsnetSocket::operator=( const VsnetSocket& orig )
{
    fd     = orig.fd;
    _remote_ip = orig._remote_ip;
    return *this;
}

int VsnetSocket::get_fd() const
{
    return fd;
}

void VsnetSocket::set_fd( int sock )
{
    fd   = sock;
}

bool VsnetSocket::valid() const
{
    return (fd>0);
}

void VsnetSocket::watch( SocketSet& set )
{
    set.set(fd);
}

bool VsnetSocket::eq( const VsnetSocket& r )
{
    const VsnetSocket* r2 = (const VsnetSocket*)&r;
    return ( (isTcp() == r2->isTcp()) && (fd == r2->fd) && (_remote_ip==r2->_remote_ip) );
}

/***********************************************************************
 * VsnetUDPSocket - definition
 ***********************************************************************/
 
int VsnetUDPSocket::sendbuf( PacketMem& packet, const AddressIP* to)
// int VsnetUDPSocket::sendbuf( void *buffer, unsigned int len, const AddressIP* to)
{
    COUT << "enter " << __PRETTY_FUNCTION__ << endl;
    int numsent;

    // In UDP mode, always send on this->sock
    const sockaddr_in* dest = to;
    if( dest == NULL ) dest = &_remote_ip;

    assert( dest != NULL );

    // if( (numsent = sendto( fd, buffer, len, 0, (sockaddr*) dest, sizeof(struct sockaddr_in)))<0)
    if( (numsent = sendto( fd, packet.getConstBuf(), packet.len(), 0, (sockaddr*) dest, sizeof(struct sockaddr_in)))<0)
    {
#if defined(_WIN32) && !defined(__CYGWIN__)
    // if( (numsent = sendto( fd, (char *) buffer, len, 0, (sockaddr*) dest, sizeof(struct sockaddr_in)))<0)
        if( WSAGetLastError()!=WSAEINVAL)
            cout << "WIN32 error : " << WSAGetLastError() << endl;
        return -1;
#else
        perror( "Error sending ");
        return -1;
#endif
    }
    cout<<"Sent "<<numsent<<" bytes"<<" -> "<<inet_ntoa( dest->sin_addr)<<":"<<ntohs(dest->sin_port)<<endl;
    return numsent;
}

void VsnetUDPSocket::ack( )
{
    /* as soon as windows have been introduced, these ACKs will get meaning again */
}

int VsnetUDPSocket::recvbuf( void *buffer, unsigned int& len, AddressIP* from)
{
    COUT << " enter " << __PRETTY_FUNCTION__ << " with buffer " << buffer
         << " len=" << len << endl;

    int       ret = 0;

#if defined (_WIN32) || defined (__APPLE__)
    int len1;
#else
    socklen_t len1;
#endif

    AddressIP dummy;
    if( from == NULL ) from = &dummy;

    // In UDP mode, always receive data on sock
    len1 = sizeof(sockaddr_in);
    ret = recvfrom( fd, (char *)buffer, len, 0, (sockaddr*)(sockaddr_in*)from, &len1 );
    if( ret < 0 )
    {
        COUT << " fd=" << fd << " error receiving: ";
#if defined(_WIN32) && !defined(__CYGWIN__)
        cout << WSAGetLastError() << endl;
#else
        cout << strerror(errno) << endl;
#endif
        ret = -1;
    }
    else if( ret == 0 )
    {
        cout << __FILE__ << ":" << __LINE__;
        cout << " Received " << ret << " bytes : " << buffer << " (UDP socket closed, strange)" << endl;
        ret = -1;
    }
    else
    {
        len = ret;
	COUT << "NETUI : Recvd " << len << " bytes" << " <- " << *from << endl;
    }
    return ret;
}

void VsnetUDPSocket::disconnect( const char *s, bool fexit )
{
    if( fexit )
        exit(1);
}

void VsnetUDPSocket::dump( std::ostream& ostr )
{
    ostr << "( s=" << fd << " UDP r=" << _remote_ip << " )";
}

/***********************************************************************
 * VsnetTCPSocket - definition
 ***********************************************************************/
 
VsnetTCPSocket::~VsnetTCPSocket( )
{
    while( !_complete_packets.empty() )
    {
        blob* b = _complete_packets.front();
	_complete_packets.pop_front();
	delete b;
    }

    if( _incomplete_packet )
    {
	delete _incomplete_packet;
    }
}

// int VsnetTCPSocket::sendbuf( void *buffer, unsigned int len, const AddressIP* to)
int VsnetTCPSocket::sendbuf( PacketMem& packet, const AddressIP* to)
{
    COUT << "enter " << __PRETTY_FUNCTION__ << endl;
    int numsent;

#if 1
    //
    // DEBUG block - remove soon
    //
    {
        COUT << "trying to send buffer with len " << packet.len() << ": " << endl;
	packet.dump( cout, 0 );
    }
#endif

    unsigned int len = htonl( packet.len() );
    if( (numsent=send( fd, (char *)&len, 4, 0 )) < 0 )
    {
        perror( "\tsending TCP packet len : ");
        if( errno == EBADF) return -1;
    }
    if( (numsent=send( fd, packet.getConstBuf(), packet.len(), 0))<0)
    {
        perror( "\tsending TCP data : ");
        if( errno == EBADF) return -1;
    }
    return numsent;
}

void VsnetTCPSocket::ack( )
{
    /* as soon as windows have been introduced, these ACKs will get meaning again */
}

int VsnetTCPSocket::recvbuf( void *buffer, unsigned int& len, AddressIP* from)
{
    if( _complete_packets.empty() == false )
    {
        blob* b = _complete_packets.front();
        _complete_packets.pop_front();
        assert( b );
        assert( b->present_len == b->expected_len );
        if( len > b->present_len ) len = b->present_len;
        memcpy( buffer, b->buf, len );
        delete b;
	assert( len > 0 );
        return len;
    }
    else
    {
        if( _connection_closed )
	{
	    COUT << __PRETTY_FUNCTION__ << " connection is closed" << endl;
	    return 0;
	}
        return -1;
    }
}

void VsnetTCPSocket::disconnect( const char *s, bool fexit )
{
    if( fd > 0 )
    {
#if defined(_WIN32) && !defined(__CYGWIN__)
        closesocket( fd );
#else
        close( fd );
#endif
    }
    cout << __FILE__ << ":" << __LINE__ << " "
         << s << " :\tWarning: disconnected" << strerror(errno) << endl;
    if( fexit )
        exit(1);
}

void VsnetTCPSocket::dump( std::ostream& ostr )
{
    ostr << "( s=" << fd << " TCP r=" << _remote_ip << " )";
}

bool VsnetTCPSocket::isActive( SocketSet& set )
{
    COUT << "enter " << __FUNCTION__ << endl;

    if( _complete_packets.empty() == false )
    {
        COUT << "leave " << __FUNCTION__ << endl;
        return true;
    }

    /* True is the correct answer: the app must call recvbuf once after this to receive 0
     * and start close processing.
     */
    if( _connection_closed )
    {
        COUT << "leave " << __FUNCTION__ << endl;
        return true;
    }

    if( set.is_set(fd) == false )
    {
        COUT << "leave " << __FUNCTION__ << endl;
        return false;
    }

    bool endless   = true;
    bool gotpacket = false;

#if !defined(_WIN32) || defined(__CYGWIN__)
    int arg = fcntl( fd, F_GETFL, 0 );
    if( arg == -1 )
    {
        COUT << "Can't very blocking mode, assume blocking" << endl;
        endless = false;
    }
    else if( (arg & O_NONBLOCK) == false )
    {
	// for some obscure reason, we forgot to set this socket non-blocking
	COUT << "Blocking socket" << endl;
        endless = false;
    }
    else
    {
        COUT << "Received socket status " << std::hex << arg << std::dec << endl;
    }
#endif

    do
    {
	if( ( _incomplete_len_field > 0 ) ||
	    ( _incomplete_len_field == 0 && _incomplete_packet == 0 ) )
	{
	    assert( _incomplete_packet == 0 );   // we expect a len, can not have data yet
	    assert( _incomplete_len_field < 4 ); // len is coded in 4 bytes
	    int len = 4 - _incomplete_len_field;
	    int ret = recv( fd, &_len_field[_incomplete_len_field], len, 0 );
	    assert( ret <= len );
	    if( ret <= 0 )
	    {
	        if( ret == 0 )
		{
		    _connection_closed = true;
                    COUT << "leave " << __FUNCTION__ << endl;
		    return ( _complete_packets.empty() == false );
		}
#if !defined(_WIN32) || defined(__CYGWIN__)
	        if( errno == EWOULDBLOCK )
#else
			 if( WSAGetLastError()==WSAEWOULDBLOCK)
#endif
		{
                    COUT << "leave " << __FUNCTION__ << endl;
		    return ( _complete_packets.empty() == false );
		}
		else
		{
                    COUT << "leave " << __FUNCTION__ << endl;
	            return false;
		}
	    }
	    if( ret > 0 ) _incomplete_len_field += ret;
	    if( _incomplete_len_field == 4 )
	    {
	        _incomplete_len_field = 0;
		len = ntohl( *(unsigned int*)_len_field );
		COUT << "Next packet to receive has length " << len << endl;
		_incomplete_packet = new blob( len );
	    }
	}
        if( _incomplete_packet != 0 )
	{
	    int len = _incomplete_packet->expected_len - _incomplete_packet->present_len;
	    int ret = recv( fd, _incomplete_packet->buf, len, 0 );
	    assert( ret <= len );
	    if( ret <= 0 )
	    {
	        if( ret == 0 )
		{
		    _connection_closed = true;
                    COUT << "leave " << __FUNCTION__ << endl;
		    return ( _complete_packets.empty() == false );
		}
#if !defined(_WIN32) || defined(__CYGWIN__)
	        if( errno == EWOULDBLOCK )
#else
			 if( WSAGetLastError()==WSAEWOULDBLOCK)
#endif
		{
                    COUT << "leave " << __FUNCTION__ << endl;
		    return ( _complete_packets.empty() == false );
		}
		else
		{
                    COUT << "leave " << __FUNCTION__ << endl;
	            return false;
		}
	    }
	    if( ret > 0 ) _incomplete_packet->present_len += len;
	    if( ret == len )
	    {
		assert( _incomplete_packet->expected_len == _incomplete_packet->present_len );
	        _complete_packets.push_back( _incomplete_packet );
		gotpacket = true;
#if 1
                //
                // DEBUG block - remove soon
                //
                {
		    blob* b = _incomplete_packet;
                    COUT << "received buffer with len " << b->present_len << ": " << endl;
		    PacketMem m( b->buf, b->present_len, PacketMem::LeaveOwnership );
		    m.dump( cout, 3 );
                }
#endif

		_incomplete_packet = 0;
	    }
        }
    }
    while( endless );  // exit only for EWOULDBLOCK or closed socket

    COUT << "leave " << __FUNCTION__ << " blocking socket" << endl;

    return gotpacket;
}

