#ifndef _UNIVERSE_UTIL_H__
#define _UNIVERSE_UTIL_H__
#include "cmd/collection.h"
#include <string>

using std::string;

class Unit;

namespace UniverseUtil {
///this function sets the "current" system to be "name"  where name may be something like "sol_sector/sol"  or "vega_sector/vega"   this function may take some time if the system has not been loaded before

	void pushSystem (string name);

///this function restores the active system.... there must be an equal number of pushSystems ans popSystems or else Vega Strike may behave unpredictably
	void popSystem ();
///This function gets the current system's official name
	string getSystemFile();
///this function gets the current system's nickname (not useful)
	string getSystemName();
///this function gets an iterator into the units in the current system... do NOT keep an iterator across a frame--it may get deleted!
	un_iter getUnitList();
///This function gets a unit given a number (how many iterations to go down in the iterator)
	Unit *getUnit(int index);
///this function launches a wormhole or ajump point.
	Unit *launchJumppoint(string name_string,
			string faction_string,
			string type_string,
			string unittype_string,
			string ai_string,
			int nr_of_ships,
			int nr_of_waves, 
			QVector pos, 
			string squadlogo, 
			string destinations);
///this function launches a normal fighter  the name is the flightgroup name, the type is the ship type, the faction is who it belongs to, the unittype is usually "unit" unless you want to make asteroids or nebulae or jump points or planets.  the aistring is either a python filename or "default"  the nr of ships is the number of ships to be launched with this group, the number of waves is num reinforcements... the position is a tuple (x,y,z) where they appear and the squadlogo is a squadron image...you can leave this the empty string '' for the default squadron logo. 
	Unit* launch (string name_string,string type_string,string faction_string,string unittype, string ai_string,int nr_of_ships,int nr_of_waves, QVector pos, string sqadlogo);
///this gets a random cargo type (useful for cargo missions) from either any category if category is '' or else from a specific category  'Contraband'  comes to mind!
	Cargo getRandCargo(int quantity, string category);
///This gets the faction name of an index
	string GetFactionName(int index);
///this gets an index given a faction name
	int GetFactionIndex(string name);
///this gets a relationship between two factions
	float GetRelation(string myfaction,string theirfaction);
///this changes the relaationship based on the rank and how mad or happy they are (the relationship is between 0 and 1... so a adjustment of .01 with a rank of 1 is QUITE significant.
	void AdjustRelation(string myfaction,string theirfaction, float factor, float rank);
///this gets the number of factions in game
	int GetNumFactions ();
///this gets the current time in seconds
	float GetGameTime ();
///this sets the time compresison value to zero
	void SetTimeCompression ();
///this gets a string which has in it a space delimited list of neighmoring systems
	string GetAdjacentSystem (string str, int which);
///this gets a specific property of this system as found in universe/milky_way.xml
	string GetGalaxyProperty (string sys, string prop);
///this gets the number of systems adjacent to the sysname
	int GetNumAdjacentSystems (string sysname);
///this adds a playlist to the music and may be triggered with an int
	int musicAddList(string str);
///this plays a specific song
	void musicPlaySong(string str);
///this plays msuci from a given list (where the int is what was returned by musicAddList)
	void musicPlayList(int which);
///this gets the difficutly of the game... ranges between 0 and 1... many missions depend on it never going past .99 unless it's always at one.
	float GetDifficulty ();
///this sets the difficulty
	void SetDifficulty (float diff);
///this plays a sound at a location...if the sound has dual channels it will play in the center
	void playSound(string soundName, QVector loc, Vector speed);
///this plays an image (explosion or warp animation) at a location
	void playAnimation(string aniName, QVector loc, float size);
///this ends the mission with either success or failure
	void terminateMission(bool term);
///this gets the player belonging to this mission
	Unit *getPlayer();
///this gets a player number (if in splitscreen mode)
	Unit *getPlayerX(int which);
///this gets the number of active players
	int getNumPlayers ();
///this adds an objective for the cockpit to view ("go here and do this)
	int addObjective(string objective);
///this sets the objective's completeness (the int was returned by add objective)
	void setObjective(int which, string newobjective);
///this sets the completeness of a particular objective... chanigng the color onscreen
	void setCompleteness(int which, float completeNess);
///this gets that completeness
	float getCompleteness(int which);
///this sets the owner of a completeness
	void setOwner(int which,Unit *owner);
///this gets an owner of a completeness (NULL means all players can see this objective)
	Unit* getOwner(int which);
	//gets the owner of this mission
	int getMissionOwner();
	//sets the owner of this mission to be a particular cockpit
	void setMissionOwner(int);
	///returns number missions running to tweak difficulty
	int numActiveMissions();
///this sends an IO message... I'm not sure if delay currently works, but from, to and message do :-) ... if you want to send to the bar do "bar" as the to string... if you want to make news for the news room specify "news"
    void IOmessage(int delay,string from,string to,string message);
///this gets a unit with 1 of each cargo type in it
	Unit *GetMasterPartList ();
///this gets a unit with a faction's contraband list... may be null (check with isNull)
	Unit *GetContrabandList (string faction);
///this sets whether or not a player may autopilot.  Normally they are both 0 and the autopiloting is allowed based on if enemies are near... if you pass in 1 then autopilot will be allowed no matter who is near... if you set -1 then autopilot is never allowed.  global affects all players... player just affects the player who accepted the mission.
	void SetAutoStatus (int global_auto, int player_auto);
	void LoadMission (string missionname);


}

#undef activeSys

#endif
