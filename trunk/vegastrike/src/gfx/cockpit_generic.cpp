#include "in.h"
#include "vsfilesystem.h"
#include "vs_globals.h"
#include "vegastrike.h"
#include "cockpit_generic.h"
#include "universe_generic.h"
#include "star_system_generic.h"
#include "cmd/unit_generic.h"
#include "cmd/unit_factory.h"
#include "cmd/unit_util.h"
#include "cmd/iterator.h"
#include "cmd/collection.h"
#include "lin_time.h"//for fps
#include "configxml.h"
#include "cmd/images.h"
#include "cmd/script/mission.h"
#include "cmd/script/msgcenter.h"
#include "cmd/ai/flyjoystick.h"
#include "cmd/ai/firekeyboard.h"
#include "cmd/ai/aggressive.h"
//#include "main_loop.h"
#include <assert.h>	// needed for assert() calls
#include "savegame.h"
//#include "animation.h"
//#include "mesh.h"
#include "universe_util.h"
#include "cmd/ai/fire.h"
//#include "in_mouse.h"
//#include "gui/glut_support.h"
//#include "networking/netclient.h"
extern float rand01();
#define SWITCH_CONST .9

vector <int> respawnunit;
vector <int> switchunit;
vector <int> turretcontrol;
vector <int> suicide;

void Cockpit::beginElement(void *userData, const XML_Char *name, const XML_Char **atts) {
  ((Cockpit*)userData)->beginElement(name, AttributeList(atts));
}

void Cockpit::endElement(void *userData, const XML_Char *name) {
  ((Cockpit*)userData)->endElement(name);
}

float Unit::computeLockingPercent() {
  float most=-1024;
  for (int i=0;i<GetNumMounts();i++) {
    if (mounts[i].type->type==weapon_info::PROJECTILE||(mounts[i].type->size&(weapon_info::SPECIALMISSILE|weapon_info::LIGHTMISSILE|weapon_info::MEDIUMMISSILE|weapon_info::HEAVYMISSILE|weapon_info::CAPSHIPLIGHTMISSILE|weapon_info::CAPSHIPHEAVYMISSILE|weapon_info::SPECIAL))) {
      if (mounts[i].status==Mount::ACTIVE&&mounts[i].type->LockTime>0) {
	float rat = mounts[i].time_to_lock/mounts[i].type->LockTime;
	if (rat<.99) {
	  if (rat>most) {
	    most = rat;
	  }
	}
      }
    }
  }
  return (most==-1024)?1:most;
}
void Cockpit::Eject() {
  ejecting=true;
}

using namespace VSFileSystem;
void Cockpit::Init (const char * file, bool isDisabled) {
  shakin=0;
  autopilot_time=0;
  bool f404=false;
  if (file == NULL || strlen(file)==0) {
	  if (isDisabled) {
		  file = "cockpit.cpt";
	  }else {
		  file="disabled-cockpit.cpt";
	  }
	  f404=true;
  }
  if (isDisabled==true) {
	  std::string disname = std::string("disabled-") + file;
    Init (disname.c_str());
    return;
  }
  VSFile f;
  VSError err = f.OpenReadOnly( file, CockpitFile);
  if (err>Ok)
  {
	  // File not found...
	  if (isDisabled==false && (string(file).find("disabled-")==string::npos)) {
	    Init(file, true);
	  } else {
		  if (!f404) {
			  Init(NULL);
		  }
	  }
	  return;
  }
  Delete();
//  string cpdir=vs_config->getVariable("data","cockpits","");
  LoadXML(f);
  f.Close();
} 
Unit *  Cockpit::GetSaveParent() {
  Unit * un = parentturret.GetUnit();
  if (!un) {
    un = parent.GetUnit();
  }
  return un;
}
void Cockpit::SetParent (Unit * unit, const char * filename, const char * unitmodname, const QVector & pos) {
  if (unit->getFlightgroup()!=NULL) {
    fg = unit->getFlightgroup();
  }
  if( !SERVER)
  	activeStarSystem= _Universe->activeStarSystem();//cannot switch to units in other star systems.
  parent.SetUnit (unit);
  savegame->SetPlayerLocation(pos);
  if (filename[0]!='\0') {
    if (this->unitfilename.empty()) this->unitfilename.push_back("");
    this->unitfilename[0]=std::string(filename);
    this->unitmodname=std::string(unitmodname);
  }
  if (unit) {
    this->unitfaction = unit->faction;
    unit->ArmorData (StartArmor);
    if (StartArmor[0]==0) StartArmor[0]=1;
    if (StartArmor[1]==0) StartArmor[1]=1;
    if (StartArmor[2]==0) StartArmor[2]=1;
    if (StartArmor[3]==0) StartArmor[3]=1;
	if (StartArmor[4]==0) StartArmor[4]=1;
    if (StartArmor[5]==0) StartArmor[5]=1;
    if (StartArmor[6]==0) StartArmor[6]=1;
    if (StartArmor[7]==0) StartArmor[7]=1;
    maxfuel = unit->FuelData();
    maxhull = unit->GetHull();
  }
}
void Cockpit::Delete () {
  //int i;
  viewport_offset=cockpit_offset=0;
}
void Cockpit::RestoreGodliness() {
  static float maxgodliness = XMLSupport::parse_float(vs_config->getVariable("physics","player_godliness","0"));
  godliness = maxgodliness;
  if (godliness>maxgodliness)
    godliness=maxgodliness;
}

void Cockpit::InitStatic () {  
  radar_time=0;
  cockpit_time=0;
}

Cockpit::Cockpit (const char * file, Unit * parent,const std::string &pilot_name): view(CP_FRONT),parent (parent), cockpit_offset(0), viewport_offset(0), zoomfactor (XMLSupport::parse_float(vs_config->getVariable("graphics","inital_zoom_factor","2.25"))),savegame (new SaveGame(pilot_name)) {
  //  static int headlag = XMLSupport::parse_int (vs_config->getVariable("graphics","head_lag","10"));
  //int i;
  fg=NULL;
  jumpok = 0;
  TimeOfLastCollision=-200;
  /*
  for (i=0;i<headlag;i++) {
    headtrans.push_back (Matrix());
    Identity(headtrans.back());
  }
  for (i=0;i<UnitImages::NUMGAUGES;i++) {
    gauges[i]=NULL;
  }
  */
  activeStarSystem=NULL;
  InitStatic();
  //mesh=NULL;
  ejecting=false;
  currentcamera = 0;	
  //Radar=Pit[0]=Pit[1]=Pit[2]=Pit[3]=NULL;
  RestoreGodliness();


  /*
  draw_all_boxes=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawAllTargetBoxes","false"));
  draw_line_to_target=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawLineToTarget","false"));
  draw_line_to_targets_target=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawLineToTargetsTarget","false"));
  draw_line_to_itts=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawLineToITTS","false"));
  always_itts=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawAlwaysITTS","false"));
  radar_type=vs_config->getVariable("graphics","hud","radarType","WC");

  friendly=GFXColor(-1,-1,-1,-1);
  enemy=GFXColor(-1,-1,-1,-1);
  neutral=GFXColor(-1,-1,-1,-1);
  targeted=GFXColor(-1,-1,-1,-1);
  targetting=GFXColor(-1,-1,-1,-1);
  planet=GFXColor(-1,-1,-1,-1);
  if (friendly.r==-1) {
    vs_config->getColor ("enemy",&enemy.r);
    vs_config->getColor ("friend",&friendly.r);
    vs_config->getColor ("neutral",&neutral.r);
    vs_config->getColor("target",&targeted.r);
    vs_config->getColor("targetting_ship",&targetting.r);
    vs_config->getColor("planet",&planet.r);
  }
  */

  Init (file);
}
/*
static vector <int> respawnunit;
static vector <int> switchunit;
static vector <int> turretcontrol;
static vector <int> suicide;
void RespawnNow (Cockpit * cp) {
  while (respawnunit.size()<=_Universe->numPlayers())
    respawnunit.push_back(0);
  for (unsigned int i=0;i<_Universe->numPlayers();i++) {
    if (_Universe->AccessCockpit(i)==cp) {
      respawnunit[i]=2;
    }
  }
}
void Cockpit::SwitchControl (int,KBSTATE k) {
  if (k==PRESS) {
    while (switchunit.size()<=_Universe->CurrentCockpit())
      switchunit.push_back(0);
    switchunit[_Universe->CurrentCockpit()]=1;
  }

}

void Cockpit::Respawn (int,KBSTATE k) {
  if (k==PRESS) {
    while (respawnunit.size()<=_Universe->CurrentCockpit())
      respawnunit.push_back(0);
    respawnunit[_Universe->CurrentCockpit()]=1;
  }
}
*/

static void FaceTarget (Unit * un) {
  Unit * targ = un->Target();
  if (targ) {
    QVector dir (targ->Position()-un->Position());
    dir.Normalize();
    Vector p,q,r;
    un->GetOrientation(p,q,r);
    QVector qq(q.Cast());
    qq = qq+QVector (.001,.001,.001);
    un->SetOrientation (qq,dir);
  }
}
int Cockpit::Autopilot (Unit * target) {
  int retauto = 0;
  if (target) {
    Unit * un=NULL;
    if ((un=GetParent())) {
      if ((retauto = un->AutoPilotTo(un,false))) {//can he even start to autopilot
	//SetView (CP_PAN);
	un->AutoPilotTo(target,false);
	static bool face_target_on_auto = XMLSupport::parse_bool (vs_config->getVariable ( "physics","face_on_auto", "false"));
	if (face_target_on_auto) {
	  FaceTarget(un);
	}	  
	static double averagetime = GetElapsedTime()/getTimeCompression();
	static double numave = 1.0;
	averagetime+=GetElapsedTime()/getTimeCompression();
	//	static float autospeed = XMLSupport::parse_float (vs_config->getVariable ("physics","autospeed",".020"));//10 seconds for auto to kick in;
	numave++;
	/*
	AccessCamera(CP_PAN)->myPhysics.SetAngularVelocity(Vector(0,0,0));
	AccessCamera(CP_PAN)->myPhysics.ApplyBalancedLocalTorque(_Universe->AccessCamera()->P,
							      _Universe->AccessCamera()->R,
							      averagetime*autospeed/(numave));
	*/
	static float initialzoom = XMLSupport::parse_float(vs_config->getVariable("graphics","inital_zoom_factor","2.25"));
	zoomfactor=initialzoom;
	static float autotime = XMLSupport::parse_float (vs_config->getVariable ("physics","autotime","10"));//10 seconds for auto to kick in;

	autopilot_time=autotime;
	autopilot_target.SetUnit (target);
      }
    }
  }
  return retauto;
}

extern void SwitchUnits2( Unit * nw);
void SwitchUnits (Unit * ol, Unit * nw) {
  bool pointingtool=false;
  bool pointingtonw=false;

  for (int i=0;i<_Universe->numPlayers();i++) {
    if (i!=(int)_Universe->CurrentCockpit()) {
      if (_Universe->AccessCockpit(i)->GetParent()==ol)
	pointingtool=true;
      if (_Universe->AccessCockpit(i)->GetParent()==nw)
	pointingtonw=true;
    }
  }

  if (ol&&(!pointingtool)) {
    Unit * oltarg = ol->Target();
    if (oltarg) {
      if (ol->getRelation (oltarg)>=0) {
	ol->Target(NULL);
      }
    }
    ol->PrimeOrders();
    ol->SetAI (new Orders::AggressiveAI ("default.agg.xml","default.int.xml"));
    ol->SetVisible (true);
  }
  SwitchUnits2( nw);
}
static void SwitchUnitsTurret (Unit *ol, Unit *nw) {
  static bool FlyStraightInTurret = XMLSupport::parse_bool (vs_config->getVariable("physics","ai_pilot_when_in_turret","true"));
  if (FlyStraightInTurret) {
    SwitchUnits (ol,nw);
  }else {
    ol->PrimeOrders();
    SwitchUnits (NULL,nw);
    
  }

}
extern void reset_time_compression(int i, KBSTATE a);

Unit * GetFinalTurret(Unit * baseTurret) {
  Unit * un = baseTurret;
  un_iter uj= un->getSubUnits();
  Unit * tur;
  while ((tur=uj.current())) {
    SwitchUnits (NULL,tur);
    un = GetFinalTurret (tur);
    ++uj;
  }
  return un;
}

void Cockpit::UpdAutoPilot()
{
	if (autopilot_time!=0) {
    autopilot_time-=SIMULATION_ATOM;
    if (autopilot_time<= 0) {
      autopilot_time=0;
      Unit * par = GetParent();
      if (par) {
	Unit * autoun = autopilot_target.GetUnit();
	autopilot_target.SetUnit(NULL);
	if (autoun) {
	  par->AutoPilotTo(autoun,false);
	}
      }
    }
  }
}

extern void DoCockpitKeys();
extern QVector DockToSavedBases (int playernum);
static float dockingdistance (Unit* port, Unit * un) {
	vector<DockingPorts>::const_iterator i =port->GetImageInformation().dockingports.begin();
	vector<DockingPorts>::const_iterator end =port->GetImageInformation().dockingports.end();
	QVector pos (InvTransform (port->cumulative_transformation_matrix,un->Position()));
	float mag=FLT_MAX;
	for (;i!=end;++i) {
		float tmag =(pos.Cast()-(*i).pos).Magnitude()-un->rSize()-(*i).radius;
		if (tmag < mag)
			mag=tmag;
	}
	if (mag==FLT_MAX)
		return UnitUtil::getDistance(port,un);
	return mag;
}
void Cockpit::Update () {
  if (jumpok) {
		jumpok++;
  }
  if (jumpok>5)
	  jumpok=0;
  
  UpdAutoPilot();
  Unit * par=GetParent();
  if (!par) {
	if (respawnunit.size()>_Universe->CurrentCockpit()){
	  if (respawnunit[_Universe->CurrentCockpit()]){
		parentturret.SetUnit(NULL);
		static float initialzoom = XMLSupport::parse_float(vs_config->getVariable("graphics","inital_zoom_factor","2.25"));
		zoomfactor=initialzoom;
		respawnunit[_Universe->CurrentCockpit()]=0;
		std::string savegamefile =mission->getVariable ("savegame","");
		int k;
		for ( k=0;k<_Universe->numPlayers();++k) {
		  if (_Universe->AccessCockpit(k)==this)
		  break;
		}
		if (k==_Universe->numPlayers()) k=0;
		string newsystem;QVector pos; bool setplayerXloc;
                savegame->SetStarSystem("");
		savegame->ParseSaveGame(savegamefile,newsystem,newsystem,pos,setplayerXloc,this->credits,unitfilename,k);
		newsystem= savegame->GetStarSystem()+".system";
		StarSystem * ss = _Universe->GenerateStarSystem (newsystem.c_str(),"",Vector(0,0,0));
		_Universe->getActiveStarSystem(0)->SwapOut();
		this->activeStarSystem=ss;
		_Universe->pushActiveStarSystem(ss);


		vector <StarSystem *> saved;
		while (_Universe->getNumActiveStarSystem()) {
		  saved.push_back (_Universe->activeStarSystem());
		  _Universe->popActiveStarSystem();
		}
		if (!saved.empty()) {
		  saved.back()=ss;
		}
		unsigned int mysize = saved.size();
		for (unsigned int i=0;i<mysize;i++) {
		  _Universe->pushActiveStarSystem (saved.back());
		  saved.pop_back();
		}
		ss->SwapIn();
		int fgsnumber = 0;
		if (fg) {
		  fgsnumber=fg->flightgroup_nr++;
		  fg->nr_ships++;
		  fg->nr_ships_left++;
		}
		Unit * un = UnitFactory::createUnit (GetUnitFileName().c_str(),false,this->unitfaction,unitmodname,fg,fgsnumber);
		un->SetCurPosition (UniverseUtil::SafeEntrancePoint (savegame->GetPlayerLocation()));
		ss->AddUnit (un);

		this->SetParent(un,GetUnitFileName().c_str(),unitmodname.c_str(),savegame->GetPlayerLocation());
		SwitchUnits (NULL,un);
		this->credits = savegame->GetSavedCredits();
		DoCockpitKeys();
		savegame->ReloadPickledData();
		_Universe->popActiveStarSystem();
		DockToSavedBases((int)(this - _Universe->AccessCockpit(0)));
      }
	} 
  }
  if (turretcontrol.size()>_Universe->CurrentCockpit())
  if (turretcontrol[_Universe->CurrentCockpit()]) {
    turretcontrol[_Universe->CurrentCockpit()]=0;
    Unit * par = GetParent();
    if (par) {
      static int index=0;
      int i=0;bool tmp=false;bool tmpgot=false;
      if (parentturret.GetUnit()==NULL) {
	tmpgot=true;
	un_iter ui= par->getSubUnits();
	Unit * un;
	while ((un=ui.current())) {
		if (_Universe->isPlayerStarship(un)){
			++ui;
			continue;
		}

	  if (i++==index) {
	    index++;
	    if (un->name.find ("accessory")==string::npos) {
	      tmp=true;
	      SwitchUnitsTurret(par,un);
	      parentturret.SetUnit(par);
	      Unit * finalunit = GetFinalTurret(un);
	      this->SetParent(finalunit,GetUnitFileName().c_str(),this->unitmodname.c_str(),savegame->GetPlayerLocation());
	      break;
	    }
	  }
	  ++ui;
	}
      }
      if (tmp==false) {
	if (tmpgot) index=0;
	Unit * un = parentturret.GetUnit();
	if (un&&(!_Universe->isPlayerStarship(un))) {
	  
	  SetParent (un,GetUnitFileName().c_str(),this->unitmodname.c_str(),savegame->GetPlayerLocation());
	  SwitchUnits (NULL,un);
	  parentturret.SetUnit(NULL);
	  un->SetTurretAI();
	  un->DisableTurretAI();
	}
      }
    }
  }
  static bool autoclear=XMLSupport::parse_bool(vs_config->getVariable("AI","autodock","false"));
  par=GetParent();
  if (autoclear&&par) {
    Unit *targ=par->Target();
	if (targ) {
		static float autopilot_term_distance = XMLSupport::parse_float (vs_config->getVariable ("physics","auto_pilot_termination_distance","6000"));
		float doubled=dockingdistance(targ,par);
    if (((targ->isUnit()!=PLANETPTR&&doubled<autopilot_term_distance)||(UnitUtil::getSignificantDistance(targ,par)<=0))&&(!(par->IsCleared(targ)||targ->IsCleared(par)||par->isDocked(targ)||targ->isDocked(par)))&&(par->getRelation(targ)>=0)&&(targ->getRelation(par)>=0)) {
		if (targ->isUnit()!=PLANETPTR||targ->GetDestinations().empty()) {
			RequestClearence(par,targ,0);//sex is always 0... don't know how to	 get it.
		}
    } else if (((par->IsCleared(targ)||targ->IsCleared(par)&&(!(par->isDocked(targ)||targ->isDocked(par)))))&&
			   ((targ->isUnit()==PLANETPTR&&UnitUtil::getSignificantDistance(par,targ)>0)||(targ->isUnit()!=PLANETPTR&&UnitUtil::getSignificantDistance(par,targ)>(targ->rSize()+par->rSize()))&&(doubled>=autopilot_term_distance))) {
		if (targ->isUnit()!=PLANETPTR||targ->GetDestinations().empty()) {
			par->EndRequestClearance(targ);
			targ->EndRequestClearance(par);
		}
	}
	}
  }
  if (switchunit.size()>_Universe->CurrentCockpit())
  if (switchunit[_Universe->CurrentCockpit()]) {
    parentturret.SetUnit(NULL);

	static float initialzoom = XMLSupport::parse_float(vs_config->getVariable("graphics","inital_zoom_factor","2.25"));
    zoomfactor=initialzoom;
    static int index=0;
    switchunit[_Universe->CurrentCockpit()]=0;
    un_iter ui= _Universe->activeStarSystem()->getUnitList().createIterator();
    Unit * un;
    bool found=false;
    int i=0;
    while ((un=ui.current())) {
      if (un->faction==this->unitfaction) {
	
	if ((i++)>=index&&(!_Universe->isPlayerStarship(un))) {
	  found=true;
	  index++;
	  Unit * k=GetParent(); 
	  SwitchUnits (k,un);
	  this->SetParent(un,GetUnitFileName().c_str(),this->unitmodname.c_str(),savegame->GetPlayerLocation());
	  //un->SetAI(new FireKeyboard ())
	  break;
	}
      }
      ++ui;
    }
    if (!found)
      index=0;
  }
  if (ejecting) {
    ejecting=false;
    Unit * un = GetParent();
    if (un) {
      un->EjectCargo((unsigned int)-1);
    }
  }else {
  if (suicide.size()>_Universe->CurrentCockpit()) {
    if (suicide[_Universe->CurrentCockpit()]) {
      Unit * un=NULL;
      if ((un = parent.GetUnit())) {
	float armor[8]; //short fix
	un->ArmorData(armor);
	un->DealDamageToHull(Vector(0,0,.1),un->GetHull()+2+armor[0]);
      }
      suicide[_Universe->CurrentCockpit()]=0;
    }

  }
  }

}

Cockpit::~Cockpit () {
  Delete();
  if( savegame!=NULL)
	  delete savegame;
}
