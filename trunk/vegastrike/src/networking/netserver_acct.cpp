#include "netserver.h"
#include "vsnet_debug.h"
#include "vs_path.h"
#include "savenet_util.h"
#include "netbuffer.h"

extern int acct_con;

/**************************************************************/
/**** Check account server activity                        ****/
/**************************************************************/

void	NetServer::checkAcctMsg( SocketSet& sets )
{
	int len=0;
	AddressIP	ipadr, ip2;
	ClientPtr   clt;
	unsigned char cmd=0;

	// Watch account server socket
	// Get the number of active clients
	if( acct_sock.isActive( ))
	{
		//COUT<<"Net activity !"<<endl;
		// Receive packet and process according to command

		PacketMem mem;
		if( (len=acct_sock.recvbuf( mem, &ip2 ))>0 )
		{
			// Maybe copy that in a "else" condition too if when it fails we have to disconnect a client

			// Here we get the latest client which asked for a login
			// Since coms between game servers and account server are TCP the order of request/answers
			// should be ok and we can use a "queue" for waiting clients
			if( waitList.size()==0)
			{
				cerr<<"Error : trying to remove client on empty waitList"<<" - len="<<len<<endl;
				exit( 1);
			}
            WaitListEntry entry( waitList.front() );
			if( entry.tcp )
			{
			    clt = entry.t;
			    COUT << "Got response for TCP client" << endl;
			}
			else
			{
			    ipadr = entry.u;
				COUT << "Got response for client IP : " << ipadr << endl;
			}
			waitList.pop();

			Packet p( mem );
			packeta = p;
			switch( packeta.getCommand())
			{
				case LOGIN_NEW :
					COUT << ">>> NEW LOGIN =( serial n�"<<packet.getSerial()<<" )= --------------------------------------"<<endl;
					// We received a login authorization for a new account (no ship created)
					this->sendLoginAccept( clt, ipadr, 1);
					COUT << "<<< NEW LOGIN ----------------------------------------------------------------"<<endl;
				break;
				case LOGIN_ACCEPT :
					// Login is ok
					COUT<<">>> LOGIN ACCEPTED =( serial n�"<<packet.getSerial()<<" )= --------------------------------------"<<endl;
					sendLoginAccept( clt, ipadr, 0);
					COUT<<"<<< LOGIN ACCEPTED -----------------------------------------------------------"<<endl;
				break;
				case LOGIN_ERROR :
					COUT<<">>> LOGIN ERROR =( DENIED )= --------------------------------------"<<endl;
					// Login error -> disconnect
					this->sendLoginError( clt, ipadr);
					COUT<<"<<< LOGIN ERROR ---------------------------------------------------"<<endl;
				break;
				case LOGIN_ALREADY :
					COUT<<">>> LOGIN ALREADY =( ALREADY LOGGED IN -> serial n�"<<packet.getSerial()<<" )= --------------------------------------"<<endl;
					// Client already logged in -> disconnect
					this->sendLoginAlready( clt, ipadr);
					COUT<<"<<< LOGIN ALREADY --------------------------------------------------------------"<<endl;
				break;
				default:
					COUT<<">>> UNKNOWN COMMAND =( "<<hex<<cmd<<" )= --------------------------------------"<<endl;
			}
		}
		else
		{
			cerr<<"Connection to account server lost !!"<<endl;
			acct_sock.disconnect( __PRETTY_FUNCTION__, false );
			acct_con = 0;
		}
	}
}

/**************************************************************/
/**** Save the server state                                ****/
/**************************************************************/

// For now it only save units and player saves
void	NetServer::save()
{
	Packet pckt;
	Cockpit * cp;
	Unit * un;
	FILE * fp=NULL;
	string xmlstr, savestr;
	//unsigned int xmllen, savelen, nxmllen, nsavelen;
	NetBuffer netbuf;

	// Save the Dynamic Universe in the data dir for now
	string dynuniv_path = datadir+"dynaverse.dat";
	fp = fopen( dynuniv_path.c_str(), "w+b");
	if( !fp)
	{
		cerr<<"Error opening dynamic universe file"<<endl;
		cleanup();
	}
	else
	{
		string dyn_univ = globalsave->WriteDynamicUniverse();
		int wlen = fwrite( dyn_univ.c_str(), sizeof( char), dyn_univ.length(), fp);
		if( wlen != dyn_univ.length())
		{
			cerr<<"Error writing dynamic universe file"<<endl;
			cleanup();
		}
		fclose( fp);
		fp = NULL;
	}

	// Loop through all clients and write saves
	for( int i=0; i<_Universe->numPlayers(); i++)
	{
		cp = _Universe->AccessCockpit( i);
		SaveNetUtil::GetSaveStrings( i, savestr, xmlstr);
		// Write the save and xml unit
		FileUtil::WriteSaveFiles( savestr, xmlstr, datadir+"/serversaves", cp->savegame->GetCallsign());
		// SEND THE BUFFERS TO ACCOUNT SERVER
		if( acctserver && acct_con)
		{
			netbuf.Reset();
			bool found = false;
			// Loop through clients to find the one corresponding to the unit (we need its serial)
			ClientPtr clt = getClientFromSerial( cp->GetParent()->GetSerial());
			if( !clt )
			{
				cerr<<"Error client not found in save process !!!!"<<endl;
				exit(1);
			}
			netbuf.addString( savestr);
			netbuf.addString( xmlstr);
			//buffer = new char[savestr.length() + xmlstr.length() + 2*sizeof( unsigned int)];
			//SaveNetUtil::GetSaveBuffer( savestr, xmlstr, buffer);
			if( pckt.send( CMD_SAVEACCOUNTS, clt->game_unit.GetUnit()->GetSerial(), netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE, NULL, acct_sock, __FILE__, PSEUDO__LINE__(1678) ) < 0 )
				COUT<<"ERROR sending SAVE to account server"<<endl;
		}
	}
}

