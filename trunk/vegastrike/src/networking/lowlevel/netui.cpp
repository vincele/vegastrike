#include <config.h>

#include "networking/const.h"
#include "netui.h"
#include "vsnet_serversocket.h"
#include "vsnet_oss.h"
#include "vsnet_debug.h"
#include "vsnet_headers.h"

static void static_initNetwork( )
{
#if defined(_WIN32) && !defined(__CYGWIN__)
    static bool first_time = true;
    if( first_time )
    {
        first_time = false;

        COUT <<"Initializing Winsock"<<std::endl;
        WORD wVersionRequested = MAKEWORD( 1, 1 );
        WSADATA wsaData; 
        int res = WSAStartup(wVersionRequested,&wsaData);
        if( res != 0)
            COUT <<"Error initializing Winsock"<<std::endl;
    }
#endif
}

AddressIP NetUIBase::lookupHost(const char* host, unsigned short port)
{
    static_initNetwork( );

    AddressIP      remote_ip;
    memset( &remote_ip, 0, sizeof(AddressIP) );

    struct hostent  *he = NULL;
	
    // Gets the host info for host
    if( host[0]<48 || host[0]>57)
    {
        COUT <<"Resolving host name... ";
        if( (he = gethostbyname( host)) == NULL)
        {
            COUT << "Could not resolve hostname" << std::endl;
            return AddressIP();
        }
        memcpy( &remote_ip.sin_addr.s_addr, he->h_addr_list[0], he->h_length);
        COUT <<"found : "<<inet_ntoa( remote_ip.sin_addr)<<std::endl;
    }
    else
    {
        if( VsnetOSS::inet_aton( host, &remote_ip.sin_addr) == 0)
        {
            COUT << "Error inet_aton" << std::endl;
            return AddressIP();
        }
    }
    // Store it in srv_ip struct
    remote_ip.sin_port   = htons( port );
    remote_ip.sin_family = AF_INET;
	return remote_ip;
}

int NetUIBase::createClientSocket(const AddressIP &remote_ip, bool isTCP)
{
    static_initNetwork( );

    int            local_fd;

    if( (local_fd = VsnetOSS::socket( PF_INET, isTCP ? SOCK_STREAM : SOCK_DGRAM, 0)) == -1)
    {
        COUT << "Could not create socket" << std::endl;
        return -1;
    }

    if (isTCP) {
        COUT << "Connecting to " << inet_ntoa( remote_ip.sin_addr) << " on port " << remote_ip.sin_port << std::endl;
        VsnetOSS::set_blocking( local_fd, false ); // Connect may hang... we don't want that.
#if defined(_WIN32) && !defined(__CYGWIN__)
        if( ::connect( local_fd, (sockaddr *)&remote_ip, sizeof( struct sockaddr))==SOCKET_ERROR)
#else
        if( ::connect( local_fd, (sockaddr *)&remote_ip, sizeof( struct sockaddr)) < 0 )
#endif
        {
            if
#if defined(_WIN32) && !defined(__CYGWIN__)
                ( WSAGetLastError() != WSAEINPROGRESS && WSAGetLastError() != WSAEWOULDBLOCK)
#else
                ( errno != EINPROGRESS && errno!= EWOULDBLOCK)
#endif
            {
                perror( "Can't connect to server ");
                VsnetOSS::close_socket( local_fd );
                return -1;
            }
       }
    } else {
        // binds socket
        if( bind( local_fd, (sockaddr *)&remote_ip, sizeof(struct sockaddr_in))==SOCKET_ERROR )
        {
            perror( "Can't bind socket" );
            VsnetOSS::close_socket( local_fd );
            return -1;
        }
    }
	
	return local_fd;
}

int NetUIBase::createServerSocket(const AddressIP &local_ip, bool isTCP)
{
    if (!isTCP)
        return createClientSocket(local_ip, false);
    
    static_initNetwork( );
    
    int       local_fd;

    if( (local_fd = VsnetOSS::socket( PF_INET, SOCK_STREAM, 0)) == -1 )
    {
        COUT << "Could not create socket" << std::endl;
        return -1;
    }

    // binds socket
    COUT << "Bind on " << ntohl(local_ip.sin_addr.s_addr) << ", port "
         << ntohs( local_ip.sin_port) << std::endl;
    if( bind( local_fd, (sockaddr *)&local_ip, sizeof( struct sockaddr_in) )==SOCKET_ERROR )
    {
        perror( "Problem binding socket" );
        VsnetOSS::close_socket( local_fd );
        return -1;
    }

    COUT << "Accepting max : " << SOMAXCONN << std::endl;
    if( listen( local_fd, SOMAXCONN)==SOCKET_ERROR)
    {
        perror( "Problem listening on socket" );
        VsnetOSS::close_socket( local_fd );
        return -1;
    }

	return local_fd;
}

/**************************************************************/
/**************************************************************/
/**** Create (and bind) a socket on host                   ****/
/**************************************************************/

// Creates and bind the socket designed to receive coms
// host == NULL -> localhost

SOCKETALT NetUITCP::createSocket( const char * host, unsigned short srv_port, SocketSet& set )
{
    COUT << "enter " << __PRETTY_FUNCTION__ << std::endl;

    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( srv_port == 0 )
    {
        srv_port = SERVER_PORT;
    }
	
	AddressIP remote_ip = NetUIBase::lookupHost(host, srv_port);
	if (remote_ip.sin_port == 0 ) {
		SOCKETALT ret;
		return ret;
	}
    int local_fd = NetUIBase::createClientSocket(remote_ip, true);
    if (local_fd==-1) {
        SOCKETALT ret; // ( -1, SOCKETALT::TCP, remote_ip );
        return ret;
    }
	
    COUT << "Connected to " << inet_ntoa( remote_ip.sin_addr) << ":" << srv_port << std::endl;
	
    SOCKETALT ret( local_fd, SOCKETALT::TCP, remote_ip, set );
    if( ret.set_nonblock() == false )
    {
        COUT << "WARNING: TCP client socket may be in blocking mode" << std::endl;
    }
    COUT << "SOCKETALT n� : " << ret.get_fd() << std::endl;
    return ret;
}

ServerSocket* NetUITCP::createServerSocket( unsigned short port, SocketSet& set )
{
    COUT << "enter " << __PRETTY_FUNCTION__ << std::endl;

    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( port == 0 ) port = SERVER_PORT;
	
	AddressIP local_ip = NetUIBase::lookupHost("0.0.0.0", port);
	if (local_ip.sin_port == 0 ) {
		return NULL;
	}
    int local_fd = NetUIBase::createServerSocket(local_ip, true);
    if (local_fd==-1) {
        return NULL;
    }

    COUT << "Listening on socket " << local_fd << std::endl
         << "*** ServerSocket n� : " << local_fd << std::endl;
    return new ServerSocketTCP( local_fd, local_ip, set );
}

/**************************************************************/
/**** Create (and bind) a socket on host                   ****/
/**************************************************************/

// Creates and bind the socket designed to receive coms
// host == NULL -> localhost

SOCKETALT NetUIUDP::createSocket( const char * host, unsigned short srv_port, unsigned short clt_port, SocketSet& set )
{
    COUT << " enter " << __PRETTY_FUNCTION__ << std::endl;
    static_initNetwork( );

    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( srv_port==0 ) srv_port = SERVER_PORT;
    if( clt_port==0 ) clt_port = SERVER_PORT;
	
    AddressIP local_ip = NetUIBase::lookupHost("0.0.0.0", clt_port);
    AddressIP remote_ip = NetUIBase::lookupHost(host, srv_port);
    int       local_fd = NetUIBase::createClientSocket( local_ip, false );

    if (local_fd == -1) {
        SOCKETALT ret;
        return ret;
    }

    SOCKETALT ret( local_fd, SOCKETALT::UDP, remote_ip, set );

    if( ret.set_nonblock() == false )
    {
		COUT << "Could not set socket to nonblocking state";
        //ret.disconnect( "Could not set socket to nonblocking state" );
        //SOCKETALT ret; // ( -1, SOCKETALT::UDP, remote_ip );
        //return ret;
    }
    COUT << "Bind on localhost, " << ret << std::endl;
    return ret;
}

SOCKETALT NetUIUDP::createServerSocket( unsigned short port, SocketSet& set )
{
    COUT << "enter " << __PRETTY_FUNCTION__ << std::endl;
    static_initNetwork( );

    // If port is not given, use the defaults ones --> do not work with specified ones yet... well, didn't try
    if( port == 0 ) port = SERVER_PORT;
	
    AddressIP local_ip = NetUIBase::lookupHost( "0.0.0.0", port );
    int       local_fd = NetUIBase::createServerSocket( local_ip, false );

    if (local_fd == -1) {
        SOCKETALT ret;
        return ret;
    }

	SOCKETALT ret( local_fd, SOCKETALT::UDP, local_ip, set );

    if( ret.set_nonblock() == false )
    {
		COUT << "Setting server socket mode to nonblocking failed";
        //ret.disconnect( "Setting server socket mode to nonblocking failed", true );
        //SOCKETALT ret;
        //return ret;
    }

    COUT << "Bind on localhost, " << ret << std::endl;
    return ret;
}
/*
// This is as simple as NetUI (factory) functions *SHOULD* be.
SOCKETALT NetUIHTTP::createSocket(const char* uri, SocketSet &set)
{
    COUT << "enter " << __PRETTY_FUNCTION__ << std::endl;
	SOCKETALT ret( uri, set );
	return ret;
}
*/
