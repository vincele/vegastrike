
#include "firekeyboard.h"
#include "flybywire.h"
#include "navigation.h"
#include "in_joystick.h"
#include "cmd/unit_generic.h"
#include "communication.h"
#include "gfx/cockpit_generic.h"
#include "gfx/animation.h"
#include "audiolib.h"
#include "config_xml.h"
#include "cmd/images.h"
#include "cmd/planet.h"
#include "cmd/script/flightgroup.h"
#include "cmd/script/mission.h"
#include "vs_globals.h"
#include "gfx/car_assist.h"
#include "cmd/unit_util.h"
#include <algorithm>


FireKeyboard::FireKeyboard (unsigned int whichplayer, unsigned int whichjoystick): Order (WEAPON,0){
  this->cloaktoggle=true;
  this->whichjoystick = whichjoystick;
  this->whichplayer=whichplayer;
  gunspeed = gunrange = .0001;
  refresh_target=true;
  sex = XMLSupport::parse_int( vs_config->getVariable ("player","sex","0"));
}
const unsigned int NUMCOMMKEYS=10;
struct FIREKEYBOARDTYPE {
  FIREKEYBOARDTYPE() {
    lockkey=ECMkey=commKeys[0]=commKeys[1]=commKeys[2]=commKeys[3]=commKeys[4]=commKeys[5]=commKeys[6]=commKeys[7]=commKeys[8]=commKeys[9]=turretaikey = UP;
    eject=ejectcargo=firekey=missilekey=jfirekey=jtargetkey=jmissilekey=weapk=misk=cloakkey=
		neartargetkey=targetskey=targetukey=threattargetkey=picktargetkey=subtargetkey=targetkey=
		rneartargetkey=rtargetskey=rtargetukey=rthreattargetkey=rpicktargetkey=rtargetkey=
		nearturrettargetkey =threatturrettargetkey= pickturrettargetkey=turrettargetkey=UP;
#ifdef CAR_SIM
    blinkleftkey=blinkrightkey=headlightkey=sirenkey=UP;
#endif
    doc=und=req=0;
  }
 KBSTATE firekey;
 bool doc;
 bool und;
 bool req;
#ifdef CAR_SIM
  KBSTATE blinkleftkey;
  KBSTATE blinkrightkey;
  KBSTATE headlightkey;
  KBSTATE sirenkey;
#endif
 KBSTATE rneartargetkey;
 KBSTATE rthreattargetkey;
 KBSTATE rpicktargetkey;
 KBSTATE rtargetkey;
 KBSTATE rtargetskey;
 KBSTATE rtargetukey;
 KBSTATE missilekey;
 KBSTATE jfirekey;
 KBSTATE jtargetkey;
 KBSTATE jmissilekey;
 KBSTATE weapk;
 KBSTATE misk;
 KBSTATE eject;
 KBSTATE lockkey;
 KBSTATE ejectcargo;
 KBSTATE cloakkey;
 KBSTATE ECMkey;
 KBSTATE neartargetkey;
 KBSTATE threattargetkey;
 KBSTATE picktargetkey;
 KBSTATE subtargetkey;
 KBSTATE targetkey;
 KBSTATE targetskey;
 KBSTATE targetukey;
 KBSTATE turretaikey;

  KBSTATE commKeys[NUMCOMMKEYS];
 KBSTATE nearturrettargetkey;
 KBSTATE threatturrettargetkey;
 KBSTATE pickturrettargetkey;
 KBSTATE turrettargetkey;
};
static std::vector <FIREKEYBOARDTYPE> vectorOfKeyboardInput;
static FIREKEYBOARDTYPE &g() {
  while (vectorOfKeyboardInput.size()<=(unsigned int)_Universe->CurrentCockpit()||vectorOfKeyboardInput.size()<=(unsigned int)MAX_JOYSTICKS) {
    vectorOfKeyboardInput.push_back(FIREKEYBOARDTYPE());
  }
  return vectorOfKeyboardInput [_Universe->CurrentCockpit()];
}
FIREKEYBOARDTYPE &FireKeyboard::f() {
  return vectorOfKeyboardInput[whichplayer];
}
FIREKEYBOARDTYPE &FireKeyboard::j() {
  return vectorOfKeyboardInput[whichjoystick];
}

void FireKeyboard::PressComm1Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[0]=PRESS;
  }
}
void FireKeyboard::PressComm2Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[1]=PRESS;
  }
}
void FireKeyboard::PressComm3Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[2]=PRESS;
  }
}
void FireKeyboard::PressComm4Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[3]=PRESS;
  }
}
void FireKeyboard::PressComm5Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[4]=PRESS;
  }
}
void FireKeyboard::PressComm6Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[5]=PRESS;
  }
}
void FireKeyboard::PressComm7Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[6]=PRESS;
  }
}
void FireKeyboard::PressComm8Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[7]=PRESS;
  }
}
void FireKeyboard::PressComm9Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[8]=PRESS;
  }
}
void FireKeyboard::PressComm10Key (int, KBSTATE k) {
  if (k==PRESS) {
    g().commKeys[9]=PRESS;
  }
}


void FireKeyboard::RequestClearenceKey(int, KBSTATE k) {

    if (k==PRESS) {
      g().req=true;      
    }
    if (k==RELEASE) {
      g().req=false;      
    }
}
void FireKeyboard::DockKey(int, KBSTATE k) {

    if (k==PRESS) {
      g().doc=true;      
    }
    if (k==RELEASE) {
      g().doc=false;      
    }
}
void FireKeyboard::UnDockKey(int, KBSTATE k) {
    if (k==PRESS) {
      g().und=true;      
    }
    if (k==RELEASE) {
      g().und=false;      
    }
}
void FireKeyboard::EjectKey (int, KBSTATE k) {
  if (k==PRESS) {
    g().eject= k;
  }
}


void FireKeyboard::TurretAI (int, KBSTATE k) {
  if (k==PRESS) {
    if (g().turretaikey==UP) {

      g().turretaikey=PRESS;
    }else {

      g().turretaikey=RELEASE;
    }
  }
}


void FireKeyboard::EjectCargoKey (int, KBSTATE k) {
    if (k==PRESS) {
      g().ejectcargo = k;      
    }
  
}
void FireKeyboard::CloakKey(int, KBSTATE k) {

    if (k==PRESS) {
      g().cloakkey = k;      
    }
}
void FireKeyboard::LockKey(int, KBSTATE k) {

    if (k==PRESS) {
      g().lockkey = k;      
    }
}
void FireKeyboard::ECMKey(int, KBSTATE k) {

    if (k==PRESS) {
      g().ECMkey = k;      
    }
    if (k==RELEASE) {
      g().ECMkey = k;
    }
}
void FireKeyboard::FireKey(int key, KBSTATE k) {
  if(g().firekey==DOWN && k==UP){
    return;
  }
  if (k==UP&&g().firekey==RELEASE) {

  } else {

    g().firekey = k;
    //    printf("firekey %d %d\n",k,key);
  }
}

void FireKeyboard::TargetKey(int, KBSTATE k) {
  if (g().targetkey!=PRESS)
    g().targetkey = k;
  if (k==RESET) {
    g().targetkey=PRESS;
  }
}
void FireKeyboard::PickTargetKey(int, KBSTATE k) {
  if (g().picktargetkey!=PRESS)
    g().picktargetkey = k;
  if (k==RESET) {
    g().picktargetkey=PRESS;
  }
}

void FireKeyboard::NearestTargetKey(int, KBSTATE k) {
  if (g().neartargetkey!=PRESS)
    g().neartargetkey = k;

}
void FireKeyboard::SubUnitTargetKey(int, KBSTATE k) {
  if (g().subtargetkey!=PRESS)
    g().subtargetkey = k;

}
void FireKeyboard::ThreatTargetKey(int, KBSTATE k) {
  if (g().threattargetkey!=PRESS)
    g().threattargetkey = k;
}

void FireKeyboard::UnitTargetKey(int, KBSTATE k) {
  if (g().targetukey!=PRESS)
    g().targetukey = k;
}

void FireKeyboard::SigTargetKey(int, KBSTATE k) {
  if (g().targetskey!=PRESS)
    g().targetskey = k;
}

void FireKeyboard::ReverseTargetKey(int, KBSTATE k) {
  if (g().rtargetkey!=PRESS)
    g().rtargetkey = k;
  if (k==RESET) {
    g().rtargetkey=PRESS;
  }
}
void FireKeyboard::ReversePickTargetKey(int, KBSTATE k) {
  if (g().rpicktargetkey!=PRESS)
    g().rpicktargetkey = k;
  if (k==RESET) {
    g().rpicktargetkey=PRESS;
  }
}
void FireKeyboard::ReverseNearestTargetKey(int, KBSTATE k) {
  if (g().rneartargetkey!=PRESS)
    g().rneartargetkey = k;

}
void FireKeyboard::ReverseThreatTargetKey(int, KBSTATE k) {
  if (g().rthreattargetkey!=PRESS)
    g().rthreattargetkey = k;
}

void FireKeyboard::ReverseUnitTargetKey(int, KBSTATE k) {
  if (g().rtargetukey!=PRESS)
    g().rtargetukey = k;
}

void FireKeyboard::ReverseSigTargetKey(int, KBSTATE k) {
  if (g().rtargetskey!=PRESS)
    g().rtargetskey = k;
}


#ifdef CAR_SIM
void FireKeyboard::BlinkLeftKey(int, KBSTATE k) {
    if (k==PRESS) {
      g().blinkleftkey = k;      
    }
    if (k==RELEASE) {
      g().blinkleftkey = k;
    }

}
void FireKeyboard::BlinkRightKey(int, KBSTATE k) {
    if (k==PRESS) {
      g().blinkrightkey = k;      
    }
    if (k==RELEASE) {
      g().blinkrightkey = k;
    }

}
void FireKeyboard::SirenKey(int, KBSTATE k) {
    if (k==PRESS) {
      g().sirenkey = k;      
    }
    if (k==RELEASE) {
      g().sirenkey = k;
    }
}
void FireKeyboard::HeadlightKey(int, KBSTATE k) {
    if (k==PRESS) {
      g().headlightkey = k;      
    }
    if (k==RELEASE) {
      g().headlightkey = k;
    }
}
#endif
extern void DoSpeech (Unit * un, const string &speech);
extern void LeadMe (Unit * un, string directive, string speech);

static void LeadMe (string directive, string speech) {
  Unit * un= _Universe->AccessCockpit()->GetParent();
  LeadMe (un,directive,speech);
  
}
extern Unit * GetThreat (Unit * par, Unit * leader);
void HelpOut (bool crit, std::string conv) {
  Unit * un = _Universe->AccessCockpit()->GetParent();
  if (un) {
    Unit * par=NULL;
    DoSpeech(  un,conv);
    for (un_iter ui = _Universe->activeStarSystem()->getUnitList().createIterator();
	 (par = (*ui));
	 ++ui) {
      if ((crit&&FactionUtil::GetIntRelation(par->faction,un->faction)>0)||par->faction==un->faction) {
	Unit * threat = GetThreat (par,un);
	CommunicationMessage c(par,un,NULL,0);
	if (threat) {
	  par->Target(threat);
	  c.SetCurrentState (c.fsm->GetYesNode(),NULL,0);
	} else {
	  c.SetCurrentState (c.fsm->GetNoNode(),NULL,0);
	}
	un->getAIState()->Communicate (c);
      }
    }
  }
}
void FireKeyboard::JoinFg (int, KBSTATE k) {
  if (k==PRESS) {
    Unit * un = _Universe->AccessCockpit()->GetParent();
    if (un) {
      Unit * targ = un->Target();
      if (targ) {
	if (targ->faction==un->faction) {
	  Flightgroup * fg = targ->getFlightgroup();
	  if (fg) {
	    if (fg!=un->getFlightgroup()) {
	      if (un->getFlightgroup()) {
		un->getFlightgroup()->Decrement(un);
	      }
	      fg->nr_ships_left++;
	      fg->nr_ships++;
	      un->SetFg(fg,fg->nr_ships_left-1);
	    }
	  }
	}
      }

    }
  }

}

void FireKeyboard::AttackTarget (int, KBSTATE k) {
  if (k==PRESS) {
    LeadMe ("a","Attack my target!");
  }
}
void FireKeyboard::HelpMeOut (int, KBSTATE k) {
  if (k==PRESS) {
    LeadMe ("h","Help me out!");
  }
}
void FireKeyboard::HelpMeOutFaction (int, KBSTATE k) {
  if (k==PRESS) {
    HelpOut (false,"Help me out! I need critical assistance!");
  }
}
void FireKeyboard::HelpMeOutCrit (int, KBSTATE k) {
  if (k==PRESS) {
    HelpOut (true,"Help me out! Systems going critical!");
  }
}

void FireKeyboard::FormUp (int, KBSTATE k) {
  if (k==PRESS) {
    LeadMe ("f","Form on my wing.");
  }
}
void FireKeyboard::BreakFormation(int, KBSTATE k) {
  if (k==PRESS) {
    LeadMe ("b","Break formation and open fire!");
  }
}


void FireKeyboard::TargetTurretKey(int, KBSTATE k) {
  if (g().turrettargetkey!=PRESS)
    g().turrettargetkey = k;
  if (k==RESET) {
    g().turrettargetkey=PRESS;
  }
}
void FireKeyboard::PickTargetTurretKey(int, KBSTATE k) {
  if (g().pickturrettargetkey!=PRESS)
    g().pickturrettargetkey = k;
  if (k==RESET) {
    g().pickturrettargetkey=PRESS;
  }
}

void FireKeyboard::NearestTargetTurretKey(int, KBSTATE k) {
  if (g().nearturrettargetkey!=PRESS)
    g().nearturrettargetkey = k;

}
void FireKeyboard::ThreatTargetTurretKey(int, KBSTATE k) {
  if (g().threatturrettargetkey!=PRESS)
    g().threatturrettargetkey = k;
}



void FireKeyboard::WeapSelKey(int, KBSTATE k) {
  if (g().weapk!=PRESS)
    g().weapk = k;
}
void FireKeyboard::MisSelKey(int, KBSTATE k) {
  if (g().misk!=PRESS)
    g().misk = k;
} 

void FireKeyboard::MissileKey(int, KBSTATE k) {
  if (g().missilekey!=PRESS)
   g().missilekey = k;
}
#if 0
void FireKeyboard::ChooseNearTargets(bool turret,bool reverse) {
  UnitCollection::UnitIterator iter = _Universe->activeStarSystem()->getUnitList().createIterator();
  Unit * un;
  float range=FLT_MAX;
  while ((un = iter.current())) {
    Vector t;
    bool tmp = parent->InRange (un);
    t = parent->LocalCoordinates (un);
    if (tmp&&t.Dot(t)<range&&t.k>0&&FactionUtil::GetIntRelation(parent->faction,un->faction)<0) {
      range = t.Dot(t);
      if (turret)
	parent->TargetTurret(un);
      parent->Target (un);
    }
    iter.advance();
  }
#ifdef ORDERDEBUG
  fprintf (stderr,"i4%x",iter);
  fflush (stderr);
#endif

#ifdef ORDERDEBUG
  fprintf (stderr,"i4\n");
  fflush (stderr);
#endif

}
void FireKeyboard::ChooseThreatTargets(bool turret,bool reverse) {
	static int index=0;
	UnitCollection * drawlist = &_Universe->activeStarSystem()->getUnitList();
	un_iter iter = drawlist->createIterator();
	Unit *target;
	std::vector <Unit *> vec; //not saving across frames
	Unit *threat=parent->Threat();
	if (target) {
		vec.push_back(threat);
	}
	while ((target = iter.current())!=NULL) {
		if (target->Target()==parent&&target!=threat) {
			vec.push_back(target);
		}
		iter.advance();
	}
	if (!vec.empty()) {
		if (reverse) {
			index--;
			if (index<0) {
				index=vec.size()-index;
				if (index<0) {
					index=0;
				}
			}
		} else {
			index++;
		}
		target=vec[index%vec.size()];
		parent->Target(target);
		if (turret)
			parent->TargetTurret(target);
	}
}

void FireKeyboard::PickTargets(bool Turrets,bool reverse){
  UnitCollection::UnitIterator uiter = _Universe->activeStarSystem()->getUnitList().createIterator();

  float smallest_angle=PI;

  Unit *other=uiter.current();
  Unit *found_unit=NULL;

  while(other){
    if(other!=parent && parent->InRange(other)){
      Vector p,q,r;
      QVector vectothem=QVector(other->Position() - parent->Position()).Normalize();
      parent->GetOrientation(p,q,r);
      double angle=acos( vectothem.Dot(r.Cast()) );
      
      if(angle<smallest_angle){
	found_unit=other;
	smallest_angle=angle;
      }
    }
    uiter.advance();
    other=uiter.current();
  }
  if (Turrets)
    parent->TargetTurret(found_unit);
  parent->Target(found_unit);

}
void FireKeyboard::ChooseRTargets (bool turret,bool significant) {
  Unit * targ =parent->Target();
  Unit * oldun=NULL;
  Unit * un=NULL;
  un_iter i;
  for (i=_Universe->activeStarSystem()->getUnitList().createIterator();
       (un=*i)!=NULL;
       ++i) {
    if (un==targ) {
      break;
    }else {
      if (un!=parent&&parent->InRange(un,true,significant)) {
	oldun=un;
      }
    }
  }
  if (oldun==NULL) {//get us the last
    for (i=_Universe->activeStarSystem()->getUnitList().createIterator();
	 (un=*i)!=NULL;
	 ++i) {
      if (un!=parent) {
	oldun=un;
      }
    }
  }
  parent->Target (oldun);
  if (turret) {
	parent->TargetTurret (un);
  }
}

void FireKeyboard::ChooseTargets (bool turret,bool significant,bool reverse) {
  if (reverse) {
    ChooseRTargets(turret,significant);
    return;
  }
  UnitCollection::UnitIterator iter = _Universe->activeStarSystem()->getUnitList().createIterator();
  Unit * un ;
  bool found=false;
  bool find=false;
  while ((un = iter.current())) {
    //how to choose a target?? "if looks particularly juicy... :-) tmp.prepend (un);
    //Vector t;

    if (un==parent->Target()) {
      iter.advance();
      found=true;
      continue;
    }
    if (!parent->InRange(un,true,significant)) {
      iter.advance();
      continue;
    }
    iter.advance();
    if (found) {
      find=true;
      if (turret)
	parent->TargetTurret (un);
      parent->Target (un);
      break;
    }
  }
  //  if ((un = iter->current())) {


    //  }
#ifdef ORDERDEBUG
  fprintf (stderr,"i5%x",iter);
  fflush (stderr);
#endif

  
#ifdef ORDERDEBUG
  fprintf (stderr,"i5\n");
  fflush (stderr);
#endif
  if (!find) {
    iter = _Universe->activeStarSystem()->getUnitList().createIterator();
    while ((un = iter.current())) {
      //how to choose a target?? "if looks particularly juicy... :-) tmp.prepend (un);
      //Vector t;
      if (un==parent->Target()){
	iter.advance();
	continue;
      }
      if (!parent->InRange(un)) {
	iter.advance();
	continue;
      }
      if (turret)
	parent->TargetTurret(un);
      parent->Target(un);
      break;
    }
  }
}
#endif

bool TargUn (Unit *me,Unit *target) {
	return me->InRange(target,true,false);
}
bool TargSig (Unit *me,Unit *target) {
	return me->InRange(target,true,true);
}
bool TargAll (Unit *me,Unit *target) {
	return TargUn(me,target)||TargSig(me,target);
}
bool TargNear (Unit *me,Unit *target) {
	return me->getRelation(target)<0&&TargAll(me,target);
}
bool TargFront (Unit *me,Unit *target) {
	/*
	float dist;
	if (me->cosAngleTo(target,dist)>.6) {
		return true;
	}
	return false;
	*/
	QVector delta( target->Position()-me->Position());
	double mm = delta.Magnitude();
	double tempmm =mm-target->rSize();
	if (tempmm>0.0001) {
		if ((me->ToLocalCoordinates (Vector(delta.i,delta.j,delta.k)).k/tempmm)>.85) {
			return true;
		}
	}
	return false;
}
bool TargThreat (Unit *me,Unit *target) {
	if (target->Target()==me) {
		return true;
	}
	if (me->Threat()==target) {
		return true;
	}
	return false;
}

void ChooseTargets(Unit * me, bool (*typeofunit)(Unit *,Unit *), bool reverse) {
	UnitCollection * drawlist = &_Universe->activeStarSystem()->getUnitList();
	un_iter iter = drawlist->createIterator();
	vector <Unit *> vec;
	Unit *target;
	while ((target = iter.current())!=NULL) {
		vec.push_back(target);
		iter.advance();
	}
	if (reverse) {
		std::reverse (vec.begin(),vec.end());
	}
	std::vector <Unit *>::const_iterator veciter=std::find(vec.begin(),vec.end(),me->Target());
	if (veciter!=vec.end())
		veciter++;
	int cur=0;
	while (1) {
		while (veciter!=vec.end()) {
			if (((*veciter)!=me)&&typeofunit(me,(*veciter))) {
				me->Target(*veciter);
				return;
			}
			veciter++;
		}
		cur++;
		if (cur>=2)
			break;
		veciter=vec.begin();
	}
}

void ChooseSubTargets(Unit * me) {
	Unit *parent=UnitUtil::owner(me->Target());
	if (!parent) {
		return;
	}
	un_iter uniter=parent->getSubUnits();
	if (parent==me->Target()) {
		if (!uniter.current()) {
			return;
		}
		me->Target(uniter.current());
		return;
	}
	while (uniter.current()) {
		if (uniter.current()==me->Target()) {
			uniter.advance();
			if (uniter.current()) {
				me->Target(uniter.current());
			} else {
				me->Target(parent);
			}
			return;
		}
		uniter.advance();
	}
}


FireKeyboard::~FireKeyboard () {
#ifdef ORDERDEBUG
  fprintf (stderr,"fkb%x",this);
  fflush (stderr);
#endif

}
bool FireKeyboard::ShouldFire(Unit * targ) {
  float dist=FLT_MAX;
  if (gunspeed==.0001) {
    parent->getAverageGunSpeed (gunspeed,gunrange);
  }
  float angle = parent->cosAngleTo (targ, dist,gunspeed,gunrange);
  targ->Threaten (parent,angle/(dist<.8?.8:dist));
  if (targ==parent->Target()) {
    distance = dist;
  }
  return (dist<.8&&angle>1);
}
static bool UnDockNow (Unit* me, Unit * targ) {
  bool ret=false;
  Unit * un;
  for (un_iter i=_Universe->activeStarSystem()->getUnitList().createIterator();
       (un = *i)!=NULL;
       ++i) {
    if (un->isDocked (me)) {
      if (me->UnDock (un)) {
	ret=true;
      }
    }
  }
  return ret;
}
static Unit * getAtmospheric (Unit * targ) {
  if (targ) {
    Unit * un;
    for (un_iter i=_Universe->activeStarSystem()->getUnitList().createIterator();
	 (un=*i)!=NULL;
	 ++i) {
      if (un->isUnit()==PLANETPTR) {
	if ((targ->Position()-un->Position()).Magnitude()<targ->rSize()*.5) {
	  if (!(((GamePlanet *)un)->isAtmospheric())) {
	    return un;
	  }
	}
      }
    }
  }
  return NULL;
  
}
static void DoDockingOps (Unit * parent, Unit * targ,unsigned char playa, unsigned char sex) {
  static bool nodockwithclear = XMLSupport::parse_bool (vs_config->getVariable ("physics","dock_with_clear_planets","true"));
    if (vectorOfKeyboardInput[playa].doc) {
      if (targ->isUnit()==PLANETPTR) {
      if (((GamePlanet * )targ)->isAtmospheric()&&nodockwithclear) {
	targ = getAtmospheric (targ);
	if (!targ) {
	  mission->msgcenter->add("game","all","[Computer] Cannot dock with insubstantial object, target another object and retry.");
	  return;
	}
	parent->Target(targ);
      }
      }
      CommunicationMessage c(targ,parent,NULL,0);

      bool hasDock = parent->Dock(targ);
      if (hasDock) {
	  c.SetCurrentState (c.fsm->GetDockNode(),NULL,0);
	  vectorOfKeyboardInput[playa].req=true;
      }else {
        if (UnDockNow(parent,targ)) {
	  c.SetCurrentState (c.fsm->GetUnDockNode(),NULL,0);
        }else {
          //docking is unsuccess
	  c.SetCurrentState (c.fsm->GetFailDockNode(),NULL,0);
        }
      }
      parent->getAIState()->Communicate (c);
      vectorOfKeyboardInput[playa].doc=false;

    }
    if (vectorOfKeyboardInput[playa].req) {
      if (targ->isUnit()==PLANETPTR) {

	if (((GamePlanet * )targ)->isAtmospheric()&&nodockwithclear) {
	  targ = getAtmospheric (targ);
	  if (!targ) {
	    mission->msgcenter->add("game","all","[Computer] Cannot dock with insubstantial object, target another object and retry.");
	    return;
	  }
	  parent->Target(targ);
	}
      }
      //      fprintf (stderr,"request %d", targ->RequestClearance (parent));
      CommunicationMessage c(parent,targ,NULL,sex);
      c.SetCurrentState(c.fsm->GetRequestLandNode(),NULL,sex);
      targ->getAIState()->Communicate (c);
      vectorOfKeyboardInput[playa].req=false;
    }

    if (vectorOfKeyboardInput[playa].und) {
      CommunicationMessage c(targ,parent,NULL,0);
      if (UnDockNow(parent,targ)) {
	c.SetCurrentState (c.fsm->GetUnDockNode(),NULL,0);
      }else {
	c.SetCurrentState (c.fsm->GetFailDockNode(),NULL,0);
      }
      parent->getAIState()->Communicate (c);
      vectorOfKeyboardInput[playa].und=0;
    }
}
using std::list;

void FireKeyboard::ProcessCommMessage (class CommunicationMessage&c){

  Unit * un = c.sender.GetUnit();
  if (!AUDIsPlaying (c.getCurrentState()->GetSound(c.sex))) {
    AUDStartPlaying(c.getCurrentState()->GetSound(c.sex));
  }
  if (un) {
    for (list<CommunicationMessage>::iterator i=resp.begin();i!=resp.end();i++) {
      if ((*i).sender.GetUnit()==un) {
	i = resp.erase (i);
      }
    }
    resp.push_back(c);
    AdjustRelationTo(un,c.getCurrentState()->messagedelta);
    DoSpeech (un,c.getCurrentState()->message);
    if (parent==_Universe->AccessCockpit()->GetParent()) {
      _Universe->AccessCockpit()->SetCommAnimation (c.ani);
    }
    refresh_target=true;
  }else {
    DoSpeech (NULL,c.getCurrentState()->message);
    if (parent==_Universe->AccessCockpit()->GetParent()) {
      static Animation Statuc ("static.ani");
      _Universe->AccessCockpit()->SetCommAnimation (&Statuc);
    }

  }
}
using std::list;

static CommunicationMessage * GetTargetMessageQueue (Unit * targ,std::list <CommunicationMessage> &messagequeue) {
      CommunicationMessage * mymsg=NULL;
      for (list<CommunicationMessage>::iterator i=messagequeue.begin();i!=messagequeue.end();i++) {
	if ((*i).sender.GetUnit()==targ) {
	  mymsg = &(*i);
	  break;
	}
      }
      return mymsg;
}


void FireKeyboard::Execute () {
  while (vectorOfKeyboardInput.size()<=whichplayer||vectorOfKeyboardInput.size()<=whichjoystick) {
    vectorOfKeyboardInput.push_back(FIREKEYBOARDTYPE());
  }
  ProcessCommunicationMessages(SIMULATION_ATOM,true);
  Unit * targ;
  if ((targ = parent->Target())) {
    ShouldFire (targ);
    DoDockingOps(parent,targ,whichplayer,sex);
  } else {
    ChooseTargets(parent,TargAll,false);
    refresh_target=true;
  }
  if (f().firekey==PRESS||f().jfirekey==PRESS||j().firekey==DOWN||j().jfirekey==DOWN) 
    parent->Fire(false);
  if (f().missilekey==DOWN||f().missilekey==PRESS||j().jmissilekey==PRESS||j().jmissilekey==DOWN) {
    parent->Fire(true);
    if (f().missilekey==PRESS)
      f().missilekey = DOWN;
    if (j().jmissilekey==PRESS)
      j().jmissilekey=DOWN;
  }
  else if (f().firekey==RELEASE||j().jfirekey==RELEASE) {
    f().firekey=UP;
    j().jfirekey=UP;
    parent->UnFire();
  }
  if (f().cloakkey==PRESS) {

    f().cloakkey=DOWN;
    parent->Cloak(cloaktoggle);
    cloaktoggle=!cloaktoggle;
  }
  if (f().lockkey==PRESS) {
    f().lockkey=DOWN;
    parent->LockTarget(!parent->TargetLocked());
  }
  if (f().ECMkey==PRESS) {
    parent->GetImageInformation().ecm=-parent->GetImageInformation().ecm;
    
  }
#ifdef CAR_SIM
  int origecm=UnitUtil::getECM(parent);
  if (origecm>=CAR::ON_NO_BLINKEN)
    origecm=CAR::FORWARD_BLINKEN;
  if (origecm<0)
    origecm=0;
  if (f().blinkleftkey==PRESS) {
    f().blinkleftkey=DOWN;
    if ((origecm&CAR::LEFT_BLINKEN)) {
      UnitUtil::setECM (parent,(origecm& (~CAR::LEFT_BLINKEN)));
    }else {
      UnitUtil::setECM (parent,origecm|CAR::LEFT_BLINKEN);
    }
  }
  if (f().blinkrightkey==PRESS) {
    f().blinkrightkey=DOWN;
    if ((origecm&CAR::RIGHT_BLINKEN)) {
      UnitUtil::setECM (parent,(origecm& (~CAR::RIGHT_BLINKEN)));
    }else {
      UnitUtil::setECM (parent,origecm|CAR::RIGHT_BLINKEN);
    }
  }
  if (f().sirenkey==PRESS) {
    f().sirenkey=DOWN;
    if ((origecm&CAR::SIREN_BLINKEN)) {
      UnitUtil::setECM (parent,(origecm& (~CAR::SIREN_BLINKEN)));
    }else {
      UnitUtil::setECM (parent,origecm|CAR::SIREN_BLINKEN);
    }
  }
  if (f().headlightkey==PRESS) {
    f().headlightkey=DOWN;
    if ((origecm&CAR::FORWARD_BLINKEN)) {
      UnitUtil::setECM (parent,(origecm& (~CAR::FORWARD_BLINKEN)));
    }else {
      UnitUtil::setECM (parent,origecm|CAR::FORWARD_BLINKEN);
    }
  }
#endif
  if (f().targetkey==PRESS||j().jtargetkey==PRESS) {
    f().targetkey=DOWN;
    j().jtargetkey=DOWN;
    ChooseTargets(parent,TargAll,false);
    refresh_target=true;
  }
  if (f().rtargetkey==PRESS) {
    f().rtargetkey=DOWN;
    ChooseTargets(parent,TargAll,true);
    refresh_target=true;
  }
  if (f().targetskey==PRESS) {
    f().targetskey=DOWN;
    ChooseTargets(parent,TargSig,false);
    refresh_target=true;
  }
  if (f().targetukey==PRESS) {
    f().targetukey=DOWN;
    ChooseTargets(parent,TargUn,false);
    refresh_target=true;
  }
  if(f().picktargetkey==PRESS){
    f().picktargetkey=DOWN;
    ChooseTargets(parent,TargFront,false);
    refresh_target=true;
  }
  if (f().neartargetkey==PRESS) {
    ChooseTargets(parent,TargNear,false);
    f().neartargetkey=DOWN;
    refresh_target=true;
  }
  if (f().threattargetkey==PRESS) {
    ChooseTargets(parent,TargThreat,false);
    f().threattargetkey=DOWN;
    refresh_target=true;
  }
  if (f().subtargetkey==PRESS) {
    ChooseSubTargets(parent);
    f().subtargetkey=DOWN;
    refresh_target=true;
  }
  if(f().rpicktargetkey==PRESS){
    f().rpicktargetkey=DOWN;
    ChooseTargets(parent,TargFront,true);
    refresh_target=true;
  }
  if (f().rneartargetkey==PRESS) {
    ChooseTargets(parent,TargNear,true);
    f().rneartargetkey=DOWN;
    refresh_target=true;
  }
  if (f().rthreattargetkey==PRESS) {
    ChooseTargets(parent,TargThreat,true);
    f().rthreattargetkey=DOWN;
    refresh_target=true;
  }
  if (f().rtargetskey==PRESS) {
    f().rtargetskey=DOWN;
    ChooseTargets(parent,TargSig,true);
    refresh_target=true;
  }
  if (f().rtargetukey==PRESS) {
    f().rtargetukey=DOWN;
    ChooseTargets(parent,TargUn,true);
    refresh_target=true;
  }

  if (f().turretaikey == PRESS) {
      parent->DisableTurretAI();
      f().turretaikey = DOWN;
  }
  if (f().turretaikey==RELEASE) {
    f().turretaikey = UP;
    parent->SetTurretAI();
    parent->TargetTurret(parent->Target());
  }
  if (f().turrettargetkey==PRESS) {
    f().turrettargetkey=DOWN;
    //    ChooseTargets(true);
    parent->TargetTurret(parent->Target());
    refresh_target=true;
  }
  if(f().pickturrettargetkey==PRESS){
    f().pickturrettargetkey=DOWN;
    parent->TargetTurret(parent->Target());
    refresh_target=true;
  }
  if (f().nearturrettargetkey==PRESS) {
    parent->TargetTurret(parent->Target());
    f().nearturrettargetkey=DOWN;
    refresh_target=true;
  }
  if (f().threatturrettargetkey==PRESS) {
    parent->TargetTurret(parent->Target());
    f().threatturrettargetkey=DOWN;
    refresh_target=true;
  }



  if (f().weapk==PRESS) {
    f().weapk=DOWN;
    parent->ToggleWeapon (false);
  }
  if (f().misk==PRESS) {
    f().misk=DOWN;
    parent->ToggleWeapon(true);
  }
  for (unsigned int i=0;i<NUMCOMMKEYS;i++) {
    if (f().commKeys[i]==PRESS) {
      f().commKeys[i]=RELEASE;
      Unit * targ=parent->Target();
      if (targ) {
	CommunicationMessage * mymsg = GetTargetMessageQueue(targ,resp);       

	if (mymsg==NULL) {
	  CommunicationMessage c(parent,targ,i,NULL,sex);
	  DoSpeech (parent,c.getCurrentState()->message);
	  if (!AUDIsPlaying (c.getCurrentState()->GetSound(c.sex))) {
	    AUDStartPlaying(c.getCurrentState()->GetSound(c.sex));
	  }
	  targ->getAIState ()->Communicate (c);
	}else {
	  FSM::Node * n = mymsg->getCurrentState();
	  if (i<n->edges.size()) {
	    CommunicationMessage c(parent,targ,*mymsg,i,NULL,sex);
	    DoSpeech (parent,c.getCurrentState()->message);
	    if (!AUDIsPlaying (c.getCurrentState()->GetSound(c.sex))) {
	      AUDStartPlaying(c.getCurrentState()->GetSound(c.sex));
	    }
	    targ->getAIState ()->Communicate (c);
	  }
	}
      }
    }
  }
  if (refresh_target) {
    Unit * targ;
    if ((targ =parent->Target())) {
      CommunicationMessage *mymsg = GetTargetMessageQueue(targ,resp);
      if (mymsg==NULL) {
	FSM *fsm =FactionUtil::GetConversation (parent->faction,targ->faction);
	_Universe->AccessCockpit()->communication_choices=fsm->GetEdgesString(fsm->getDefaultState(parent->getRelation(targ)));
      }else {
       _Universe->AccessCockpit()->communication_choices=mymsg->fsm->GetEdgesString(mymsg->curstate);
      }
    } else {
      _Universe->AccessCockpit()->communication_choices="\nNo Communication\nLink\nEstablished";
    }
  }
  if (f().ejectcargo==PRESS) {
    int offset = _Universe->AccessCockpit()->getScrollOffset (VDU::MANIFEST);
    if (offset<3) {
      offset=0;
    }else {
      offset-=3;
    }
    parent->EjectCargo(offset);
    f().ejectcargo=DOWN;
  }
  if (f().eject==PRESS) {
    f().eject=DOWN;
    Cockpit * cp;
    if ((cp=_Universe->isPlayerStarship (parent))) {
      cp->Eject();
    }
  }
}
