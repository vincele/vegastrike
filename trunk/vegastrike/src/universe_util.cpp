#include "cmd/script/mission.h"
#include "universe.h"
#include "cmd/unit.h"
#include "cmd/unit_interface.h"
#include "cmd/unit_factory.h" //for UnitFactory::getMasterPartList()
#include "cmd/collection.h"
#include "star_system.h"
#include <string>
#include "cmd/music.h"
#include "audiolib.h"
#include "lin_time.h"
extern class Music *muzak;
extern unsigned int AddAnimation (const QVector & pos, const float size, bool mvolatile, const std::string &name );

extern Universe *_Universe;

using std::string;

#define activeSys _Universe->activeStarSystem() //less to write

namespace UniverseUtil {
	void pushSystem (string name) {
		StarSystem * ss = _Universe->GenerateStarSystem (name.c_str(),"",Vector(0,0,0));
		_Universe->pushActiveStarSystem(ss);
	}
	void popSystem () {
		_Universe->popActiveStarSystem();
	}
	string getSystemFile() {
		return activeSys->getFileName();
	}
	string getSystemName() {
		return activeSys->getName();
	}
	un_iter getUnitList() {
		return activeSys->getUnitList().createIterator();
	}
	Unit *getUnit(int index) {
		un_iter iter=activeSys->getUnitList().createIterator();
		for(int i=0;iter.current()&&i<index;i++) {
			iter.advance();
		}
		return iter.current();
	}
	Unit *launchJumppoint(string name_string,
			string faction_string,
			string type_string,
			string unittype_string,
			string ai_string,
			int nr_of_ships,
			int nr_of_waves, 
			QVector pos, 
			string squadlogo, 
			string destinations){
		int clstype=UNITPTR;
		if (unittype_string=="planet") {
			clstype =PLANETPTR;			
		}else if (unittype_string=="asteroid") {
			clstype = ASTEROIDPTR;
		}else if (unittype_string=="nebula") {
			clstype = NEBULAPTR;
		}
		CreateFlightgroup cf;
		cf.fg = Flightgroup::newFlightgroup (name_string,type_string,faction_string,ai_string,nr_of_ships,nr_of_waves,squadlogo,"",mission);
		cf.unittype=CreateFlightgroup::UNIT;
		cf.terrain_nr=-1;
		cf.waves=nr_of_waves;
		cf.nr_ships=nr_of_ships;
		cf.fg->pos=pos;
		for(int i=0;i<3;i++){
			cf.rot[i]=0.0;
		}
		Unit *tmp= mission->call_unit_launch(&cf,clstype,destinations);
		mission->number_of_ships+=nr_of_ships;
		return tmp;
	}
	Unit* launch (string name_string,string type_string,string faction_string,string unittype, string ai_string,int nr_of_ships,int nr_of_waves, QVector pos, string sqadlogo){
		return launchJumppoint(name_string,faction_string,type_string,type_string,ai_string,nr_of_ships,nr_of_waves,pos,sqadlogo,"");
	}
	Cargo getRandCargo(int quantity, string category) {
	  Cargo *ret=NULL;
	  Unit *mpl = &GetUnitMasterPartList();
	  unsigned int max=mpl->numCargo();
	  if (!category.empty()) {
	    vector <Cargo> cat;
	    mpl->GetCargoCat (category,cat);
	    if (!cat.empty()) {
	      unsigned int i;
	      ret = mpl->GetCargo(cat[rand()%cat.size()].content,i);
	    }
	  }else {
	    if (mpl->numCargo()) {
	      for (unsigned int i=0;i<500;i++) {
		ret = &mpl->GetCargo(rand()%max);  
		if (ret->content.find("mission")==string::npos) {
		  break;
		}
	      }
	    }		  
	  }
	  if (ret) {
		Cargo tempret = *ret;
		tempret.quantity=quantity;
	    return tempret;//uses copy
	  }else {
	    Cargo newret;
	    newret.quantity=0;
	    return newret;
	  }
	}
	string GetFactionName(int index) {
		return _Universe->GetFaction(index);
	}
	int GetFactionIndex(string name) {
		return _Universe->GetFaction(name.c_str());
	}
	float GetRelation(string myfaction,string theirfaction) {
		return _Universe->GetRelation(GetFactionIndex(myfaction),GetFactionIndex(theirfaction));
	}
	void AdjustRelation(string myfaction,string theirfaction, float factor, float rank) {
		_Universe->AdjustRelation(GetFactionIndex(myfaction),GetFactionIndex(theirfaction), factor, rank);
	}
	int GetNumFactions () {
		return _Universe->GetNumFactions();
	}
	//NOTEXPORTEDYET
	float GetGameTime () {
		return mission->gametime;
	}
	void SetTimeCompression () {
		setTimeCompression(1.0);
	}
	string GetAdjacentSystem (string str, int which) {
		return _Universe->getAdjacentStarSystems(str)[which];
	}
	string GetGalaxyProperty (string sys, string prop) {
		return _Universe->getGalaxyProperty(sys,prop);
	}
	int GetNumAdjacentSystems (string sysname) {
		return _Universe->getAdjacentStarSystems(sysname).size();
	}
	int musicAddList(string str) {
		return muzak->Addlist(str.c_str());
	}
	void musicPlaySong(string str) {
		muzak->GotoSong(str);
	}
	void musicPlayList(int which) {
		muzak->SkipRandSong(which);
	}
	float GetDifficulty () {
		return g_game.difficulty;
	}
	void SetDifficulty (float diff) {
		g_game.difficulty=diff;
	}
	void playSound(string soundName, QVector loc, Vector speed) {
		int sound = AUDCreateSoundWAV (soundName,false);
		AUDAdjustSound (sound,loc,speed);
		AUDStartPlaying (sound);
		AUDDeleteSound(sound);
	}
	void playAnimation(string aniName, QVector loc, float size) {
		AddAnimation(loc,size,true,aniName);
	}
	void terminateMission(bool Win){
		mission->terminateMission();
	}
	Unit *getPlayer(){
		return _Universe->AccessCockpit()->GetParent();;
	}
	int getNumPlayers () {
		return _Universe->numPlayers();
	}
	Unit *getPlayerX(int which){
		if (which>=getNumPlayers()) {
			return NULL;
		}
		return _Universe->AccessCockpit(which)->GetParent();
	}
	int addObjective(string objective) {
		mission->objectives.push_back(Mission::Objective(0,objective));
		return mission->objectives.size()-1;
	}
	void setObjective(int which, string newobjective) {
		if (which<mission->objectives.size()) {
			mission->objectives[which].objective=newobjective;
		}
	}
	void setCompleteness(int which, float completeNess) {
		if (which<mission->objectives.size()) {
			mission->objectives[which].completeness=completeNess;
		}
	}
	float getCompleteness(int which) {
		if (which<mission->objectives.size()) {
			return mission->objectives[which].completeness;
		} else {
			return 0;
		}
	}
	void setOwner(int which,Unit *owner) {
		if (which<mission->objectives.size()) {
			mission->objectives[which].owner=owner;
		}
	}
	Unit* getOwner(int which) {
		if (which<mission->objectives.size()) {
			return mission->objectives[which].owner.GetUnit();
		} else {
			return 0;
		}
	}
    void IOmessage(int delay,string from,string to,string message){
		mission->msgcenter->add(from,to,message,delay);
	}
	Unit *GetMasterPartList () {
		return UnitFactory::getMasterPartList();
	}
	Unit *GetContrabandList (string faction) {
		return _Universe->GetContraband(_Universe->GetFaction(faction.c_str()));
	}
}

#undef activeSys
