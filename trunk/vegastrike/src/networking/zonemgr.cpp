//#include <netinet/in.h>
//#include "gfxlib.h"
//#include "cmd/unit.h"
#include "packet.h"
//#include "netserver.h"
#include "zonemgr.h"

ZoneMgr::ZoneMgr()
{
	nb_zones = atoi( (vs_config->getVariable( "server", "maxzones", "")).c_str());
	zone_list = new list<Client *>[nb_zones];
	zone_clients = new int[nb_zones];
	for( int i=0; i<nb_zones; i++)
		zone_clients[i]=0;
}

ZoneMgr::ZoneMgr( int nbzones)
{
	nb_zones = nbzones;
	zone_list = new list<Client *>[nbzones];
	zone_clients = new int[nbzones];
	for( int i=0; i<nbzones; i++)
		zone_clients[i]=0;
}

ZoneMgr::~ZoneMgr()
{
	delete zone_list;
	delete zone_clients;
}


// Return the client list that are in the zone n� serial
list<Client *>	* ZoneMgr::GetZone( int serial)
{
	return &zone_list[serial];
}

// Adds a client to the zone n� serial
void	ZoneMgr::addClient( Client * clt, int zone)
{
	zone_list[zone].push_back( clt);
	zone_clients[zone]++;
	clt->ingame = 1;
}

// Remove a client from its zone
void	ZoneMgr::removeClient( Client * clt)
{
	if( zone_list[clt->zone].empty())
	{
		cout<<"Trying to remove on an empty list !!"<<endl;
		exit( 1);
	}

	zone_list[clt->zone].remove( clt);
	zone_clients[clt->zone]--;
}

// Broadcast a packet to a client's zone clients
void	ZoneMgr::broadcast( Client * clt, Packet * pckt, NetUI *net)
{
	//cout<<"Sending update to "<<(zone_list[clt->zone].size()-1)<<" clients"<<endl;
	for( LI i=zone_list[clt->zone].begin(); i!=zone_list[clt->zone].end(); i++)
	{
		// Broadcast to other clients
		if( clt->serial!= (*i)->serial)
		{
			cout<<"Sending update to client n� "<<(*i)->serial;
			//cout<<" @ "<<net->getIPof( (*i)->cltadr);
			cout<<endl;
			net->sendbuf( (*i)->sock, (char *) pckt, pckt->getLength(), &(*i)->cltadr);
		}
	}
}

// Broadcast all snapshots
void	ZoneMgr::broadcastSnapshots( NetUI * net)
{
	char * buffer;
	int i=0, j=0, p=0;
	LI k, l;

	//cout<<"Sending snapshot for ";
	//int h_length = Packet::getHeaderLength();
	// Loop for all systems/zones
	for( i=0; i<nb_zones; i++)
	{
	// Check if system is non-empty
	if( zone_clients[i]>0)
	{
		//buffer = new char[zone_clients[i]*sizeof( ClientState)+h_length];

		/************* First method : build a snapshot buffer ***************/
		// It just look if positions or orientations have changed
		/*
		for( j=0, k=zone_list[i].begin(); k!=zone_list[i].end(); k++, j++)
		{
			// Check if position has changed
			memcpy( buffer+h_length+(j*sizeof( ClientState)), &(*k)->current_state, sizeof( ClientState));
		}
		// Then send all clients the snapshot
		Packet pckt;
		pckt.create( CMD_SNAPSHOT, zone_clients[i], buffer, zone_clients[j]*sizeof(ClientState), 0);
		for( j=0, k=zone_list[i].begin(); k!=zone_list[i].end(); k++, j++)
		{
			net->sendbuf( (*k)->sock, (char *) &pckt, pckt.getLength(), &(*k)->cltadr);
		}
		*/
		/************* Second method : send independently to each client an update ***************/
		// It allows to check (for a given client) if other clients are far away (so we can only
		// send position, not orientation and stuff) and if other clients are visible to the given
		// client.
		// -->> Reduce bandwidth usage but increase CPU usage
		// Loop for all the zone's clients
		int	offset = 0, nbclients = 0;
		float radius, distance, ratio;
		ObjSerial netserial, sertmp;
		Packet pckt;
		for( k=zone_list[i].begin(); k!=zone_list[i].end(); k++)
		{
			// If we don't want to send a client its own info set nbclients to zone_clients-1
			nbclients = zone_clients[i]-1;
			// buffer is bigger than needed in that way
			buffer = new char[nbclients*sizeof( ClientState)];
			Quaternion source_orient( (*k)->current_state.getOrientation());
			QVector source_pos( (*k)->current_state.getPosition());
			for( j=0, p=0, l=zone_list[i].begin(); l!=zone_list[i].end(); l++)
			{
				// Check if we are on the same client and that the client has moved !
				if( l!=k && !((*l)->current_state.getPosition()==(*l)->old_state.getPosition() && (*l)->current_state.getOrientation()==(*l)->old_state.getOrientation()))
				{
					//radius = (*l)->game_unit->rSize();
					QVector target_pos( (*l)->current_state.getPosition());
					// Client pointed by 'k' can see client pointed by 'l'
					// For now only check if the 'l' client is in front of the ship and not behind
					if( 1 /*(distance = this->isVisible( source_orient, source_pos, target_pos)) > 0*/)
					{
						// Test if client 'l' is far away from client 'k' = test radius/distance<=X
						// So we can send only position
						// Here distance should never be 0
						//ratio = radius/distance;
						if( 1 /* ratio > XX client not too far */)
						{
							// Mark as position+orientation+velocity update
							buffer[offset] = CMD_FULLUPDATE;
							offset += sizeof( char);
							// Prepare the state to be sent over network (convert byte order)
							//cout<<"Sending : ";
							//(*l)->current_state.display();
							(*l)->current_state.tosend();
							memcpy( buffer+offset, &((*l)->current_state), sizeof( ClientState));
							// Convert byte order back
							(*l)->current_state.received();
							offset += sizeof( ClientState);
							// Increment the number of clients we send full info about
							j++;
						}
						else if( 1 /* ratio>=1 far but still visible */)
						{
							// Mark as position update only
							buffer[offset] = CMD_POSUPDATE;
							offset += sizeof( char);
							sertmp = htons((*l)->serial);
							memcpy( buffer+offset, &(sertmp), sizeof( ObjSerial));
							offset += sizeof( ObjSerial);
							QVector qvtmp( (*l)->current_state.getPosition());
							memcpy( buffer+offset+sizeof( ObjSerial), &qvtmp, sizeof( QVector));
							offset += sizeof( QVector);
							// Increment the number of clients we send limited info about
							p++;
						}
					}
				}
				// Else : always send back to clients their own info or just ignore ?
				// Ignore for now
			}
			// Send snapshot to client k
			if( offset > 0)
			{
				//cout<<"\tsend update for "<<(p+j)<<" clients"<<endl;
				pckt.create( CMD_SNAPSHOT, nbclients, buffer, offset, 0);
				net->sendbuf( (*k)->sock, (char *) &pckt, pckt.getLength(), &(*k)->cltadr);
				//cout<<"SENT PACKET SIZE = "<<pckt.getLength()<<endl;
			}
			delete buffer;
			offset = 0;
		}
	}
	}
}

// Fills buffer with descriptions of clients in the same zone as our client
// Called after the client has been added in the zone so that it can get his
// own information/save from the server
int		ZoneMgr::getZoneClients( Client * clt, char * bufzone)
{
	LI k;
	int desc_size, state_size, nbsize;
	unsigned short nbt, nb;
	desc_size = sizeof( ClientDescription);
	state_size = sizeof( ClientState);
	nbsize = sizeof( unsigned short);
	nbt = zone_clients[clt->zone];
	int		offset = 0;

	//cout<<"Size of description : "<<desc_size<<endl;
	if( (desc_size+state_size)*nbt >= MAXBUFFER)
	{
		cout<<"Error : packet size too big, adjust MAXBUFFER in const.h"<<endl;
		cout<<"Data size = "<<((desc_size+state_size)*nbt)<<endl;
		cout<<"Nb clients = "<<nbt<<endl;
		exit( 1);
	}

	cout<<"ZONE "<<clt->zone<<" - "<<nbt<<" clients"<<endl;
	nb = htons( nbt);
	memcpy( bufzone, &nb, nbsize);
	for( k=zone_list[clt->zone].begin(); k!=zone_list[clt->zone].end(); k++)
	{
		cout<<"SENDING : ";
		(*k)->current_state.display();
		(*k)->current_state.tosend();
		memcpy( bufzone+nbsize+offset, &(*k)->current_state, state_size);
		offset += state_size;
		(*k)->current_state.received();
		memcpy( bufzone+nbsize+offset, &(*k)->current_desc, desc_size);
		offset += desc_size;
	}

	return (state_size+desc_size)*nbt;
}

double	ZoneMgr::isVisible( Quaternion orient, QVector src_pos, QVector tar_pos)
{
	double	dotp = 0;
	Matrix m;

	orient.to_matrix(m);
	QVector src_tar( m.getR());

	src_tar = tar_pos - src_pos;
	dotp = DotProduct( src_tar, (QVector) orient.v);

	return dotp;
}

/*** This is a copy of GFXSphereInFrustum from gl_matrix_hack.cpp avoiding
 * linking with a LOT of unecessary stuff
 */

/*
float	ZoneMgr::sphereInFrustum( const Vector &Cnt, float radius)
{
	float frust [6][4];
   int p;
   float d;
   for( p = 0; p < 5; p++ )//does not evaluate for yon
   {
      d = f[p][0] * Cnt.i + f[p][1] * Cnt.j + f[p][2] * Cnt.k + f[p][3];
      if( d <= -radius )
         return 0;
   }
   return d;
}
*/
