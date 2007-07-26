#include "networking/netserver.h"
#include "networking/lowlevel/vsnet_debug.h"
#include "networking/lowlevel/netbuffer.h"
#include "networking/lowlevel/vsnet_debug.h"
#include "networking/lowlevel/vsnet_sockethttp.h"
#include "lin_time.h"

extern StarSystem * GetLoadedStarSystem( const char * system);

// WEAPON STUFF

void	NetServer::BroadcastTarget( ObjSerial serial, ObjSerial target, unsigned short zone)
{
	Packet p;
	NetBuffer netbuf;

	netbuf.addSerial( target);

	//p.send( CMD_UNFIREREQUEST, serial, netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE, NULL, this->clt_sock, __FILE__, __LINE__);
	p.bc_create( CMD_TARGET, serial,
                 netbuf.getData(), netbuf.getDataLength(),
                 SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(26) );
	zonemgr->broadcast( zone, serial, &p, true ); // NETFIXME: Should unfire be TCP?
}

void	NetServer::BroadcastUnfire( ObjSerial serial, const vector<int> &weapon_indicies, unsigned short zone)
{
	Packet p;
	NetBuffer netbuf;

	//netbuf.addSerial( serial);
	netbuf.addInt32( weapon_indicies.size());
	for (unsigned int i=0;i<weapon_indicies.size();i++) {
		netbuf.addInt32( weapon_indicies[i] );
	}

	//p.send( CMD_UNFIREREQUEST, serial, netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE, NULL, this->clt_sock, __FILE__, __LINE__);
	p.bc_create( CMD_UNFIREREQUEST, serial,
                 netbuf.getData(), netbuf.getDataLength(),
                 SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(1695) );
	zonemgr->broadcast( zone, serial, &p, true ); // NETFIXME: Should unfire be TCP?
}

// In BroadcastFire we must use the provided serial because it may not be the client's serial
// but may be a turret serial
void	NetServer::BroadcastFire( ObjSerial serial, const vector<int> &weapon_indicies, ObjSerial missile_serial, float player_energy, unsigned short zone)
{
	Packet p;
	NetBuffer netbuf;
	// bool found = false;

	netbuf.addFloat( player_energy );
	netbuf.addSerial( missile_serial);
	netbuf.addInt32( weapon_indicies.size());
	for (unsigned int i=0;i<weapon_indicies.size();i++) {
		netbuf.addInt32( weapon_indicies[i] );
	}

	p.bc_create( CMD_FIREREQUEST, serial,
                 netbuf.getData(), netbuf.getDataLength(),
                 SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(1710) );
	// WARNING : WE WILL SEND THE INFO BACK TO THE CLIENT THAT HAS FIRED -> SHOULD USE broadcastNoSelf instead if we dont want that
	zonemgr->broadcast( zone, serial, &p, true ); // NETFIXME: Should unfire be TCP?
}

void NetServer::BroadcastCargoUpgrade(ObjSerial sender, ObjSerial buyer, ObjSerial seller, const std::string &cargo,
				float price, float mass, float volume, bool mission, unsigned int quantity,
				int mountOffset, int subunitOffset, unsigned short zone)
{
	Packet p;
	NetBuffer netbuf;

	netbuf.addSerial( buyer ); // If the buyer is the player, it is buying cargo.
	netbuf.addSerial( seller ); // If seller is the player, it is selling cargo.
	netbuf.addInt32( quantity );
	netbuf.addString( cargo );
	netbuf.addFloat( price );
	netbuf.addFloat( mass );
	netbuf.addFloat( volume );
	if (mission) {
		netbuf.addInt32(1);
		netbuf.addInt32(1);
	} else {
		netbuf.addInt32( (unsigned int)(mountOffset) );
		netbuf.addInt32 ((unsigned int)(subunitOffset) );
	}
	
	p.bc_create( CMD_CARGOUPGRADE, sender,
                 netbuf.getData(), netbuf.getDataLength(),
                 SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(87) );
	zonemgr->broadcast( zone, sender, &p, true ); // NETFIXME: Should unfire be TCP?
}

void NetServer::sendCredits(ObjSerial serial, float creds) {
	Packet p;
	NetBuffer netbuf;
	netbuf.addFloat(creds);
	ClientPtr clt = this->getClientFromSerial(serial);
	if (!clt) return;
	p.send( CMD_CREDITS, serial,
                 netbuf.getData(), netbuf.getDataLength(),
                 SENDRELIABLE,NULL,clt->tcp_sock,
                 __FILE__, PSEUDO__LINE__(97) );
}

void	NetServer::sendCargoSnapshot( ObjSerial cltser, const UnitCollection &list) {
	ClientPtr clt = this->getClientFromSerial(cltser);
	if (!clt) return;
	NetBuffer netbuf;
	Packet p2;
	const Unit *un;
	for (un_kiter iter = list.constIterator(); (un = *iter); ++iter) {
		if (!un->GetSerial() || un->GetSerial()==cltser) continue;
		unsigned int numCargo = un->numCargo();
		if (!numCargo) continue;
		netbuf.addSerial(un->GetSerial());
		netbuf.addInt32(numCargo);
		for (unsigned int i=0;i<numCargo;i++) {
			const Cargo &carg = un->GetCargo(i);
			netbuf.addInt32(carg.GetQuantity());
			netbuf.addString(carg.GetContent());
			netbuf.addFloat(carg.GetPrice());
			netbuf.addFloat(carg.GetMass());
			netbuf.addFloat(carg.GetVolume());
		}
	}
	netbuf.addSerial(0);
	p2.send( CMD_SNAPCARGO, 0, netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE,
			 NULL, clt->tcp_sock, __FILE__, PSEUDO__LINE__(196) );
}

void	NetServer::sendDamages( ObjSerial serial, unsigned short zone, Shield shield, Armor armor, float ppercentage, float spercentage, float amt, Vector & pnt, Vector & normal, GFXColor & color)
{
  static ObjSerial lastserial;
  static float timestamp=getNewTime();
  float curtime=getNewTime();
  static float mintime=XMLSupport::parse_float(vs_config->getVariable( "network", "min_time_btw_damage_updates", ".25" ));

  if (lastserial!=serial||curtime-timestamp>mintime) {
    timestamp=curtime;
    lastserial=serial;
	Packet p;
	NetBuffer netbuf;

	netbuf.addFloat( amt);
	netbuf.addFloat( ppercentage);
	netbuf.addFloat( spercentage);
	netbuf.addVector( pnt);
	netbuf.addVector( normal);
	netbuf.addColor( color);
	netbuf.addShield( shield);
	netbuf.addArmor( armor);

	p.bc_create( CMD_DAMAGE, serial,
                 netbuf.getData(), netbuf.getDataLength(),
                 SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(1729) );
	// WARNING : WE WILL SEND THE INFO BACK TO THE CLIENT THAT HAS FIRED
	zonemgr->broadcast( zone, serial, &p, false ); // NETFIXME: Should damages be TCP? NO..we have alternate method to deal with it
  }
}

void	NetServer::sendKill( ObjSerial serial, unsigned short zone)
{
	Packet p;
	Unit * un;

	cerr<<"SENDING A KILL for serial "<<serial<<" in zone "<<zone<<endl;
	// Find the client in the udp & tcp client lists in order to set it out of the game (not delete it yet)
	ClientPtr clt = this->getClientFromSerial( serial);

	p.bc_create( CMD_KILL, serial,
                 NULL, 0, SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(1771) );
	// WARNING : WE WILL SEND THE INFO BACK TO THE CLIENT THAT HAS FIRED
	zonemgr->broadcast( zone, serial, &p, true );

	if( !clt )
	{
		COUT<<"Killed a non client Unit = "<<serial<<endl;
		//un = zonemgr->getUnit( serial, zone);
		//zonemgr->removeUnit( un, zone);
	}
	else
	{
		COUT<<"Killed client serial = "<<serial<<endl;
		zonemgr->removeClient( clt );
	}
}

void	NetServer::sendJump(Unit * un, Unit * dst,std::string dststr)
{
  ClientPtr clt = this->getClientFromSerial(un->GetSerial());
  Cockpit * cp =NULL;
  if (clt&&(cp=_Universe->isPlayerStarship(un))!=NULL&&un!=NULL) {
    Packet p2;
    std::string netbuf;
    std::string fn=dststr;
    vector<StringPool::Reference>*dat=&cp->savegame->getMissionStringData("jump_from");
    if (dat->empty()) {
	  dat->push_back(StringPool::Reference(_Universe->activeStarSystem()->getFileName()));
    }else {
      (*dat)[0]=_Universe->activeStarSystem()->getFileName();
    }
    std::string savestr= cp->savegame->WriteSaveGame (fn.c_str(),un->LocalPosition(),cp->credits,cp->unitfilename,-1,FactionUtil::GetFactionName(un->faction), false);
    std::string::size_type where=savestr.find("^");
    addSimpleChar(netbuf,ACCT_SAVE);
    
    if (where!=std::string::npos) {
      if (dststr.substr(0,where)!=savestr) {
        savestr=dststr+savestr.substr(where);
      }
    }
    addSimpleString(netbuf, clt->callsign );
    addSimpleString(netbuf, clt->passwd );
    addSimpleString(netbuf,savestr);
    addSimpleString(netbuf, un->WriteUnitString());
    p2.send(CMD_JUMP,0,NULL,0,SENDANDFORGET,NULL,clt->tcp_sock,__FILE__,148);
    clt->jumpok=1;    if (acct_sock)
      acct_sock->sendstr(netbuf);

    if (clt&&0/*dont discon until client requests it*/)
      logoutList.push_back(clt);

  }else {
    // do something intelligent for NPCs
    
  }
  
  /*
  if (0) {
	Packet p2;
	NetBuffer netbuf;
	string file_content;
	ClientPtr clt = this->getClientFromSerial( serial);
        if (!clt) {
          cerr<<"Client "<< serial<<" Trying to jump but cannot retrieve client struct \n";
        }
	// Send a CMD_JUMP to tell the client if the jump is allowed
	netbuf.addString( clt->jumpfile );
	netbuf.addSerial( serial);
	netbuf.addSerial( jumpserial);

	// DO NOT DO THAT : the client itself will ask to be removed and also to be added when it is allowed to do so
	// And remove the player from its old starsystem and set it out of game
	//this->removeClient( clt );
	// Have to set new starsystem here
	Cockpit * cp = _Universe->isPlayerStarship( clt->game_unit.GetUnit());
	cp->savegame->SetStarSystem( clt->jumpfile);
	StarSystem* sts = GetLoadedStarSystem( clt->jumpfile.c_str());
	unsigned short zoneid = _Universe->StarSystemIndex( sts);
	netbuf.addShort( zoneid);

	// Test whether the jump was accepted or not by server
	if( ok)
	{
		// If jumpfile is empty the hash was correct
		if( clt->jumpokOLD==1 )
			p2.bc_create( CMD_JUMP, serial,
                          netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE,
                          __FILE__, PSEUDO__LINE__(1164) );
		// New system file HASH is wrong tell the client with serial != player serial so he can ask for a new download
		else if( clt->jumpokOLD==2)
			p2.bc_create( CMD_JUMP, serial+1,
                          netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE,
                          __FILE__, PSEUDO__LINE__(1164) );
		zonemgr->broadcast( clt, &p2, true );
	}
*/

	// Should broadcast JUMP so other client display jump anim too ?
}

// NETFIXME: AI Units won't be removed when docking... this may be desired but may be confusing for clients.

void	NetServer::sendDockAuthorize( ObjSerial serial, ObjSerial utdw_serial, int docknum, unsigned short zone)
{
	// Set client not ingame while docked
	ClientPtr clt = this->getClientFromSerial( serial);
	if (!clt) {
		cerr << "Client " << serial << " for dock autorization is NULL." << endl;
		return;
	}
	Unit * un=clt->game_unit.GetUnit();
	if (!un)
		return;
//	StarSystem * currentsys = un->getStarSystem();
	// Remove the unit from the system list
//	currentsys->RemoveUnit( un );

	NetBuffer netbuf;
	Packet p;
	// Send a CMD_DOCK with serial, an ObjSerial = unit_to_dock_with_serial and an int = docking port num
	netbuf.addSerial( utdw_serial);
	netbuf.addInt32( docknum);
	p.bc_create( CMD_DOCK, serial,
                 netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(118) );
	zonemgr->broadcast( zone, serial, &p, true );

	// Whoops... Make sure to de-ingame the client *AFTER* the CMD_DOCK has been broadcast!
	clt->ingame = false;
	// Set timestamps to 0 so we won't have prediction problem when undocking
	clt->clearLatestTimestamp();
}

void	NetServer::sendDockDeny( ObjSerial serial, unsigned short zone)
{
	// In fact do nothing
}

void	NetServer::sendUnDock( ObjSerial serial, ObjSerial utdwserial, unsigned short zone)
{
	// Set client ingame
	ClientPtr clt = this->getClientFromSerial( serial);
	if (!clt) {
		cerr << "Client " << serial << " for dock autorization is NULL." << endl;
		return;
	}
	Unit *un = clt->game_unit.GetUnit();
	if (!un)
		return;
	clt->ingame = true;
	// Add the unit back into the system list
//	StarSystem * currentsys = un->getStarSystem();
//	currentsys->AddUnit( un );
	
	// SEND A CMD_UNDOCK TO OTHER CLIENTS IN THE ZONE with utdw serial
	NetBuffer netbuf;
	Packet p;
	netbuf.addSerial( utdwserial);
	p.bc_create( CMD_UNDOCK, serial,
                 netbuf.getData(), netbuf.getDataLength(), SENDRELIABLE,
                 __FILE__, PSEUDO__LINE__(134) );
	zonemgr->broadcastNoSelf( zone, serial, &p, true );
}

