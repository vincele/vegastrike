#include "configxml.h"
#include "cmd/script/mission.h"
#include "cmd/script/pythonmission.h"
#include "vs_globals.h"
//#include "gfxlib.h"
#include "star_system_generic.h"
#include "vs_globals.h"
#include "cmd/unit_generic.h"
#include "cmd/unit_factory.h"
#include "gfx/cockpit_generic.h"
#include "cmd/ai/aggressive.h"
#include "cmd/ai/script.h"
#include "cmd/ai/missionscript.h"
#include "cmd/script/flightgroup.h"
#include "python/python_class.h"
#include "savegame.h"
#include "load_mission.h"
std::string PickledDataSansMissionName (std::string pickled) {
  unsigned int newline = pickled.find ("\n");
  if (newline!=string::npos)
    return pickled.substr(newline+1,pickled.length()-(newline+1));
  else
    return pickled;
}
std::string PickledDataOnlyMissionName (std::string pickled) {
  unsigned int newline = pickled.find ("\n");
  return pickled.substr (0,newline);  
}
int ReadIntSpace (std::string &str) {
  std::string myint;
  bool toggle=false;
  int i=0;
  while (i<(int)str.length()) {
    char c[2]={0,0};
    c[0]=str[i++];
    if (c[0]!=' ') {
      toggle=true;
      myint+=c;
    }else {
      if (toggle) {
	break;
      }
    }
  }
  str =str.substr (i,str.length()-i);
  return XMLSupport::parse_int (myint);

}
struct delayed_mission {
  std::string str;
  unsigned int player;
  delayed_mission (std::string str) {
    this->str= str;
    player = _Universe->CurrentCockpit();
  }
};
vector <delayed_mission> delayed_missions;

void processDelayedMissions() {
  while (!delayed_missions.empty()) {
    if (delayed_missions.back().player<(unsigned int)_Universe->numPlayers()) {
      int i = _Universe->CurrentCockpit();
      _Universe->SetActiveCockpit(delayed_missions.back().player);
      StarSystem * ss =_Universe->AccessCockpit()->activeStarSystem;
      if (ss==NULL) {
	ss = _Universe->activeStarSystem();
      }
      _Universe->pushActiveStarSystem (ss);
      LoadMission (delayed_missions.back().str.c_str(),false);
      delayed_missions.pop_back();
      _Universe->popActiveStarSystem();
      _Universe->SetActiveCockpit(i);
    }
  }
}
void delayLoadMission (std::string str) {
  delayed_missions.push_back (delayed_mission(str));
}
void SaveGame::ReloadPickledData () {
  std::string lpd = last_pickled_data;
  if (_Universe->numPlayers()==1) {
    if (!lpd.empty()) {
      unsigned int nummis= ReadIntSpace(lpd);
      for (unsigned int i=0;i<nummis;i++) {
	int len = ReadIntSpace (lpd);
	std::string pickled = lpd.substr (0,len);
	lpd = lpd.substr (len,lpd.length()-len);
	if (i<active_missions.size()) {
	  active_missions[i]->UnPickle (PickledDataSansMissionName(pickled));
	}else {
	  UnpickleMission(lpd);
	}
      }
    }
  }
}
void UnpickleMission (std::string pickled) {

  std::string file= PickledDataOnlyMissionName(pickled);
  pickled = PickledDataSansMissionName(pickled);
  if (pickled.length()) {
    active_missions.push_back (new Mission(file.c_str()));
    active_missions.back()->initMission();
    active_missions.back()->SetUnpickleData(pickled);
  }
}
std::string lengthify (std::string tp) {
    int len = tp.length();
    tp = XMLSupport::tostring(len)+" "+tp;
    return tp;
}
std::string PickleAllMissions () {
  std::string res;
  int count =0;
  for (unsigned int i=0;i<active_missions.size();i++) {
    string tmp = active_missions[i]->Pickle();
    if (tmp.length()||i==0) {
      count++;
      res+= lengthify(tmp);
    }
  }
  return XMLSupport::tostring(count)+" "+res;
}
int ReadIntSpace (FILE * fp) {
  std::string myint;
  bool toggle=false;
  while (!feof(fp)) {
    char c[2]={0,0};
    if (1==fread (&c[0],sizeof(char),1,fp)) {
      if (c[0]!=' ') {
	toggle=true;
	myint+=c;
      }else {
	if (toggle) {
	  break;
	}
      }
    }
  }
  return XMLSupport::parse_int (myint);
}
std::string UnpickleAllMissions (FILE * fp) {
  std::string retval;
  unsigned int nummissions = ReadIntSpace(fp);
  retval+=XMLSupport::tostring((int)nummissions)+" ";
  for (unsigned int i=0;i<nummissions;i++) {
    unsigned int picklelength = ReadIntSpace(fp);
    retval+=XMLSupport::tostring((int)picklelength)+" ";
    char * temp = (char *)malloc (sizeof(char)*(1+picklelength));
    temp[0]=0;
    temp[picklelength]=0;
    fread (temp,picklelength,1,fp);
    retval += temp;
    if (i<active_missions.size()) {
      active_missions[i]->SetUnpickleData (PickledDataSansMissionName(temp));
    }else {
      UnpickleMission(temp);
    }
    free(temp);
  }
  return retval;
}
void LoadMission (const char * mission_name, bool loadFirstUnit) {
  FILE * fp = fopen (mission_name,"r");
  if (!fp) {
    return;
  }
  fclose (fp);
  char * tmp = strdup (mission_name);
  active_missions.push_back (new Mission(tmp));
  mission = active_missions.back();
  active_missions.back()->initMission();
  free (tmp);

  char fightername[1024];
  vector<Flightgroup *>::const_iterator siter;
  vector<Flightgroup *> fg=active_missions.back()->flightgroups;
  Unit * fighter;
  for(siter= fg.begin() ; siter!=fg.end() ; siter++){
    Flightgroup *fg=*siter;
    string fg_name=fg->name;
    string fullname=fg->type;
    //    int fg_terrain = fg->terrain_nr;
    //    bool isvehicle = (fg->unittype==Flightgroup::VEHICLE);
    strcpy(fightername,fullname.c_str());
    int a=0;
    int tmptarget=0;
    string ainame=fg->ainame;
	float fg_radius=0.0;
	for(int s=0;s < fg->nr_ships;s++){

	  
	  QVector pox;

	  pox.i=fg->pos.i+s*fg_radius*3;
	  pox.j=fg->pos.i+s*fg_radius*3;
	  pox.k=fg->pos.i+s*fg_radius*3;
	  if (pox.i==pox.j&&pox.j==pox.k&&pox.k==0) {
	    pox.i=rand()*10000./RAND_MAX-5000;
	    pox.j=rand()*10000./RAND_MAX-5000;
	    pox.k=rand()*10000./RAND_MAX-5000;

	  }
	  if (_Universe->AccessCockpit()->GetParent()) {
	    QVector fposs =_Universe->AccessCockpit()->GetParent()->Position();
	    pox = pox+fposs;//adds our own position onto this
	  }
	  tmptarget=FactionUtil::GetFaction(fg->faction.c_str()); // that should not be in xml?
	  string modifications ("");
	  if (a!=0||loadFirstUnit) {
	    fighter = UnitFactory::createUnit(fightername, false,tmptarget,modifications,fg,s);
	  }else {
	    continue;
	  }
	  fighter->SetPosAndCumPos (pox);
	  
	  fg_radius=fighter->rSize();


	  if (benchmark>0.0  || a!=0) {
	    if(ainame[0]!='_'){
	      string ai_agg=ainame+".agg.xml";
	      string ai_int=ainame+".int.xml";

	      char ai_agg_c[1024];
	      char ai_int_c[1024];
	      strcpy(ai_agg_c,ai_agg.c_str());
	      strcpy(ai_int_c,ai_int.c_str());
	      //      printf("1 - %s  2 - %s\n",ai_agg_c,ai_int_c);

	      fighter->EnqueueAI( new Orders::AggressiveAI (ai_agg_c, ai_int_c));
	    }
	    else{
	      string modulename=ainame.substr(1);
	      
	      fighter->EnqueueAI( new AImissionScript(modulename));
	      //fighters[a]->SetAI( new AImissionScript(modulename));
	    }
	    fighter->SetTurretAI ();
	  }
	  _Universe->activeStarSystem()->AddUnit(fighter);
	  a++;
	} // for nr_ships
  } // end of for flightgroups
  
  active_missions.back()->DirectorInitgame();
  mission=active_missions[0];
  //  return true;
}
