#ifndef VS_NETUI_H
#define VS_NETUI_H

/* 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
  netUI - Network Interface - written by Stephane Vaxelaire <svax@free.fr>
*/

#include "const.h"
#include "vsnet_socket.h"

struct ServerSocket;
class SocketSet;

class NetUITCP
{
public:
    static SOCKETALT     createSocket( char* host, unsigned short port, SocketSet& set );
    static ServerSocket* createServerSocket( unsigned short port, SocketSet& set );
};

class NetUIUDP
{
public:
    static SOCKETALT		createSocket( char * host, unsigned short port, SocketSet& set );
    static ServerSocket*	createServerSocket( unsigned short port, SocketSet& set );
};

#endif /* VS_NETUI_H */

