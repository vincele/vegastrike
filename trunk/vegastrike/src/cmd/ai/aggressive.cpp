#include "aggressive.h"
#include "event_xml.h"
#include "script.h"
#include <list>
#include <vector>
#include "vs_globals.h"
#include "config_xml.h"
#include "xml_support.h"
#include "cmd/unit_generic.h"
#include "communication.h"
#include "cmd/script/flightgroup.h"
#include "flybywire.h"
#include "hard_coded_scripts.h"
#include "cmd/script/mission.h"
#include "gfx/cockpit_generic.h"
#include "lin_time.h"
#include "faction_generic.h"
#include "cmd/role_bitmask.h"
#include "cmd/unit_util.h"
#include "warpto.h"
using namespace Orders;
using std::map;
const EnumMap::Pair element_names[] = {
  EnumMap::Pair ("AggressiveAI" , AggressiveAI::AGGAI),
  EnumMap::Pair ("UNKNOWN", AggressiveAI::UNKNOWN),
  EnumMap::Pair ("Distance", AggressiveAI::DISTANCE),
  EnumMap::Pair ("MeterDistance", AggressiveAI::METERDISTANCE),
  EnumMap::Pair ("Threat", AggressiveAI::THREAT),
  EnumMap::Pair ("FShield", AggressiveAI::FSHIELD),
  EnumMap::Pair ("LShield",AggressiveAI:: LSHIELD),
  EnumMap::Pair ("RShield", AggressiveAI::RSHIELD),
  EnumMap::Pair ("BShield", AggressiveAI::BSHIELD),
  EnumMap::Pair ("Hull", AggressiveAI::HULL),
  EnumMap::Pair ("Facing", AggressiveAI::FACING),
  EnumMap::Pair ("Movement", AggressiveAI::MOVEMENT),
  EnumMap::Pair ("Rand", AggressiveAI::RANDOMIZ)
};
const EnumMap AggressiveAIel_map(element_names, 12);
using std::pair;
std::map<string,AIEvents::ElemAttrMap *> logic;
std::map<string,AIEvents::ElemAttrMap *> interrupts;
AIEvents::ElemAttrMap* getLogicOrInterrupt (string name,int faction, std::map<string,AIEvents::ElemAttrMap *> &mymap, bool inter) {
  map<string,AIEvents::ElemAttrMap *>::iterator i = mymap.find (name+string("%")+tostring(faction));
  if (i==mymap.end()) {
    AIEvents::ElemAttrMap * attr = new AIEvents::ElemAttrMap(AggressiveAIel_map);
    string filename (name+(inter?string(".int.xml"):string(".agg.xml")));
    AIEvents::LoadAI (filename.c_str(),*attr,FactionUtil::GetFaction(faction));
    mymap.insert (pair<string,AIEvents::ElemAttrMap *> (name+string("%")+tostring(faction),attr));
    return attr;
  }
  return i->second;
}
AIEvents::ElemAttrMap* getProperLogicOrInterruptScript (string name,int faction, bool interrupt) {
  return getLogicOrInterrupt (name,faction,interrupt?interrupts:logic,interrupt);
}
AIEvents::ElemAttrMap * getProperScript(Unit * me, Unit * targ, bool interrupt) {
  if (!me||!targ) {
    int fac=0;
    if (me)
      fac=me->faction;
    return getProperLogicOrInterruptScript("default",fac,interrupt);
  }
  return getProperLogicOrInterruptScript (ROLES::getRoleEvents(me->combatRole(),targ->combatRole()),me->faction,interrupt);
}

inline std::string GetRelationshipColor (float rel) {
  if (rel>=1)
    return "#00FF00";
  if (rel<=-1)
    return "#FF0000";
  rel +=1.;
  rel/=2.;
  char str[20]; //Just in case all 8 digits of both #s end up inside the string for some reason.
  sprintf(str,"#%2X%2X00",(int)((1-rel)*256),(int)(rel*256));
  return str;
}

void DoSpeech (Unit * un, Unit *player_un, const FSM::Node &node) {
  const string &speech=node.message;
  string myname ("[Static]");
  if (un!=NULL) {
    myname= un->getFullname();
	Flightgroup * fg=un->getFlightgroup();
		if (fg&&fg->name!="base"&&fg->name!="Base") {
			myname = fg->name+" "+XMLSupport::tostring(un->getFgSubnumber())+", "+un->name;
		}else if (myname.length()==0)
			myname = un->name;
	if (player_un!=NULL) {
		if (player_un==un) {
			myname=std::string("#0033FF")+myname+"#000000";
		} else {
			float rel=un->getRelation(player_un);
			myname=GetRelationshipColor(rel)+myname+"#000000";
		}
	}
  }
  mission->msgcenter->add (myname,"all",GetRelationshipColor(node.messagedelta*10)+speech+"#000000"); //multiply by 2 so colors are easier to tell
}
void LeadMe (Unit * un, string directive, string speech) { 
  if (un!=NULL) {
    for (int i=0;i<_Universe->numPlayers();i++) {
      Unit * pun =_Universe->AccessCockpit(i)->GetParent();
      if (pun) {
	if (pun->getFlightgroup()==un->getFlightgroup()){
		DoSpeech (un, pun, FSM::Node(speech,.1));	
	}
      }
    }
    Flightgroup * fg = un->getFlightgroup();
    if (fg) {
      if (fg->leader.GetUnit()!=un) {
		  if ((!_Universe->isPlayerStarship(fg->leader.GetUnit()))||_Universe->isPlayerStarship(un)) {
			fg->leader.SetUnit (un);
		  }
      }
      fg->directive = directive;
    }
  }
}

static float aggressivity=2.01;
AggressiveAI::AggressiveAI (const char * filename, const char * interruptname, Unit * target):FireAt(), logic (getProperScript(NULL,NULL,false)), interrupts(getProperScript(NULL,NULL,true)) {
  last_jump_distance=FLT_MAX;
  interruptcurtime=0;
  jump_time_check=1;
  last_time_insys=true;
  logiccurtime=interrupts->maxtime;//set it to the time allotted
  obedient = true;
  if (aggressivity==2.01) {
    float defagg = XMLSupport::parse_float (vs_config->getVariable ("unit","aggressivity","2"));
    aggressivity = defagg;
  }
  if (target !=NULL) {
    AttachOrder (target);
  }
  last_directive = filename+string("|")+interruptname;
  //  AIEvents::LoadAI (filename,logic,"neutral");
  //  AIEvents::LoadAI (interruptname,interrupt,"neutral");
}
void AggressiveAI::SetParent (Unit * parent1) {
  FireAt::SetParent(parent1);
  unsigned int which = last_directive.find("|");
  string filename (string("default.agg.xml"));
  string interruptname (string("default.int.xml"));
  if (which!=string::npos) {
    filename = last_directive.substr (0,which);
    interruptname = last_directive.substr(which+1);
  }
  last_directive="b";//prevent escort race condition
}
void AggressiveAI::SignalChosenTarget () {
  if (parent) {
    logic =getProperScript(parent,parent->Target(),false);
    interrupts=getProperScript(parent,parent->Target(),true);
  }
  FireAt::SignalChosenTarget();
}

bool AggressiveAI::ExecuteLogicItem (const AIEvents::AIEvresult &item) {
  
  if (item.script.length()!=0) {
    Order * tmp = new AIScript (item.script.c_str());	
    //    parent->EnqueueAI (tmp);
    EnqueueOrder (tmp);
    return true;
  }else {
    return false;
  }
}


bool AggressiveAI::ProcessLogicItem (const AIEvents::AIEvresult &item) {
  float value;

  static float game_speed = XMLSupport::parse_float (vs_config->getVariable ("physics","game_speed","1"));
  static float game_accel = XMLSupport::parse_float (vs_config->getVariable ("physics","game_accel","1"));
  switch (abs(item.type)) {
  case DISTANCE:
    value = distance;
    break;
  case METERDISTANCE:
    {
      Unit * targ = parent->Target();
      if (targ) {
	Vector PosDifference=targ->Position().Cast()-parent->Position().Cast();
	float pdmag = PosDifference.Magnitude();
	value = (pdmag-parent->rSize()-targ->rSize());
	float myvel = PosDifference.Dot(parent->GetVelocity()-targ->GetVelocity())/pdmag;
	
	if (myvel>0)
	  value-=myvel*myvel/(2*(parent->Limits().retro/parent->GetMass()));
      }else {
	value = 10000; 
      }
      value/=game_speed;/*game_accel*/;
    }
    break;
  case THREAT:
    value = parent->GetComputerData().threatlevel;
    break;
  case FSHIELD:
    value = parent->FShieldData();
    break;
  case BSHIELD:
    value = parent->BShieldData();
    break;
  case HULL:
    value = parent->GetHull();
    break;
  case LSHIELD:
    value = parent->LShieldData();
    break;
  case RSHIELD:
    value = parent->RShieldData();
    break;
  case FACING:
    //    return parent->getAIState()->queryType (Order::FACING)==NULL;
    return queryType (Order::FACING)==NULL;
  case MOVEMENT:
    //    return parent->getAIState()->queryType (Order::MOVEMENT)==NULL;
    return queryType (Order::MOVEMENT)==NULL;
  case RANDOMIZ:
    value= ((float)rand())/RAND_MAX;
  default:
    return false;
  }
  return item.Eval(value);
}

bool AggressiveAI::ProcessLogic (AIEvents::ElemAttrMap & logi, bool inter) {
  //go through the logic. 
  bool retval=false;
  //  Unit * tmp = parent->Target();
  //  distance = tmp? (tmp->Position()-parent->Position()).Magnitude()-parent->rSize()-tmp->rSize() : FLT_MAX;
  std::vector <std::list <AIEvents::AIEvresult> >::iterator i = logi.result.begin();
  for (;i!=logi.result.end();i++) {
    std::list <AIEvents::AIEvresult>::iterator j;
    bool trueit=true;
    for (j= i->begin();j!=i->end();j++) {
      if (!ProcessLogicItem(*j)) {
	trueit=false;
	break;
      }
    }
    if (trueit&&j==i->end()) {
      //do it
      if (inter) {

	//parent->getAIState()->eraseType (Order::FACING);
	//parent->getAIState()->eraseType (Order::MOVEMENT);
	eraseType (Order::FACING);
	eraseType (Order::MOVEMENT);


      }
      j = i->begin();
      while (j!=i->end()) {
	if (ExecuteLogicItem (*j)) {
		logiccurtime = (*j).timetofinish;
		interruptcurtime = (*j).timetointerrupt;
	  AIEvents::AIEvresult tmp = *j;
	  i->erase(j);
	  retval=true;
	  i->push_back (tmp);
	  break; 
	}else {
	  j++;
	}
      }

    }
  }
  return retval;
}

Unit * GetThreat (Unit * parent, Unit * leader) {
  Unit * th=NULL;
  Unit * un=NULL;
  bool targetted=false;
  float mindist= FLT_MAX;
	  for (un_iter ui = _Universe->activeStarSystem()->getUnitList().createIterator();
	       (un = *ui);
	       ++ui) {
	    if (parent->getRelation (un)<0) {
	      float d = (un->Position()-leader->Position()).Magnitude();
	      bool thistargetted = (un->Target()==leader);
	      if (!th||(thistargetted&&!targetted)||((thistargetted||(!targetted))&&d<mindist)) {
		th = un;
		targetted=thistargetted;
		mindist = d;
	      }
	    }
	  }
	  return th;
}
bool AggressiveAI::ProcessCurrentFgDirective(Flightgroup * fg) {
  bool retval=false;
  if (fg !=NULL) {
    Unit * leader = fg->leader.GetUnit();
    if (last_directive.empty()) {
      last_directive = fg->directive;
    }
    if (fg->directive!=last_directive) {
      if (float(rand())/RAND_MAX<(obedient?(1-logic->obedience):logic->obedience)) {
	obedient = !obedient;
      }
      if (obedient) {
	eraseType (Order::FACING);
	eraseType (Order::MOVEMENT);
	Unit * targ = parent->Target();
	if (targ) {
	  if (!isJumpablePlanet (targ)) {
	    parent->Target(NULL);
	  }
	}
      }else {
	    CommunicationMessage c(parent,leader,NULL,0);
	    c.SetCurrentState (c.fsm->GetNoNode(),NULL,0);
		Order * lo = leader->getAIState();
		if (lo)
			lo->Communicate(c);
      }
    }
    if (obedient) {
      if (fg->directive==string("a")||fg->directive==string("A")) {
	Unit * targ = fg->leader.GetUnit();
	targ = targ!=NULL?targ->Target():NULL;
	if (targ) {
	  if (targ->InCorrectStarSystem(_Universe->activeStarSystem())) {
	    CommunicationMessage c(parent,leader,NULL,0);
	    if (parent->InRange (targ,true,false)) {
	      parent->Target (targ);
	      parent->TargetTurret(targ);
	      c.SetCurrentState (c.fsm->GetYesNode(),NULL,0);
	    }else {
	      c.SetCurrentState (c.fsm->GetNoNode(),NULL,0);
	    }
	    if (fg->directive!=last_directive) {
			Order * lo = leader->getAIState();
			if (lo)
				lo->Communicate(c);
	    }
	  }
	}
      }else if (fg->directive==string("f")||fg->directive==string("F")) {
	if (leader!=NULL) {
	  if (leader->InCorrectStarSystem(_Universe->activeStarSystem())) {
	    retval=true;
	    if (fg->directive!=last_directive||(!last_time_insys)) {
	      last_time_insys=true;
	      CommunicationMessage c(parent,leader,NULL,0);
	      c.SetCurrentState (c.fsm->GetYesNode(),NULL,0);
	    //}else {
	      //	  c.SetCurrentState (c.fsm->GetNoNode(),NULL,0);
	      //	}
		  Order * o = leader->getAIState();
		  if (o)
			  o->Communicate(c);
	      float left= parent->getFgSubnumber()%2?1:-1;
	      static float esc_percent= XMLSupport::parse_float(vs_config->getVariable ("AI",
											"Targetting",
											"EscortDistance",
											"10.0"));
	      static float turn_leader= XMLSupport::parse_float(vs_config->getVariable ("AI",
											"Targetting",
											"TurnLeaderDist",
											"5.0"));
	      
	      double dist=esc_percent*(1+parent->getFgSubnumber()/2)*left*(parent->rSize()+leader->rSize());
	      Order * ord = new Orders::FormUp(QVector(dist,0,-fabs(dist)));
	      ord->SetParent (parent);
	      ReplaceOrder (ord);
	      ord = new Orders::FaceDirection(dist*turn_leader);
	      ord->SetParent (parent);
	      ReplaceOrder (ord);
	    }
	  } else {
	    last_time_insys=false;
	  }
	  for (unsigned int i=0;i<suborders.size();i++) {
	    suborders[i]->AttachSelfOrder (leader);
	  }
	}
      }else if (fg->directive==string("h")||fg->directive==string("H")) {
	//	fprintf (stderr,"he wnats to help out");
	if (fg->directive!=last_directive&&leader) {
	  if (leader->InCorrectStarSystem(_Universe->activeStarSystem())) {
	    //fprintf (stderr,"%s he wnats to help out and hasn't died\n", parent->name.c_str());
	    Unit * th=NULL;
	    if ((th=leader->Threat())) {
	      //fprintf (stderr,"he wnats to help out and he has a threat\n");

	      CommunicationMessage c(parent,leader,NULL,0);
	      if (parent->InRange(th,true,false)) {
		parent->Target(th);
		parent->TargetTurret(th);
		c.SetCurrentState (c.fsm->GetYesNode(),NULL,0);
	      }else {
		c.SetCurrentState (c.fsm->GetNoNode(),NULL,0);
	      }
		  Order * oo = leader->getAIState();
		  if (oo) 
			  oo->Communicate(c);
	    }else {
	      //bool targetted=false;
	      //float mindist;
	      //Unit * un=NULL;
	      th= GetThreat(parent,leader);
	      CommunicationMessage c(parent,leader,NULL,0);
	      //fprintf (stderr,"he wnats to help out against threat %d",th);
	      if (th) {
		if (parent->InRange (th,true,false)) {
		  c.SetCurrentState (c.fsm->GetYesNode(),NULL,0);
		  parent->Target (th);
		  parent->TargetTurret (th);
		}else {
		  c.SetCurrentState(c.fsm->GetNoNode(),NULL,0);
		}
		//fprintf (stderr,"Helping out kill: %s",th->name.c_str());
	      }else {
		c.SetCurrentState (c.fsm->GetNoNode(),NULL,0);
	      }
		  Order * loo = leader->getAIState();
		  if (loo)
			  loo->Communicate(c);
	    }
	  }
	}
      }
    } 
    last_directive=fg->directive;
  }
  return retval;
}
static bool overridable (const std::string &s) {
  if (s.empty())
    return true;
  return (*s.begin())!=toupper(*s.begin());
}
extern void LeadMe (Unit * un, string directive, string speech);
void AggressiveAI::ReCommandWing(Flightgroup * fg) {
  static float time_to_recommand_wing = XMLSupport::parse_float(vs_config->getVariable ("AI",
											"Targetting",
											"TargetCommandierTime",
											"100"));
  if (fg!=NULL) {
    Unit* lead;
    if (overridable (fg->directive)) {//computer won't override capital orders
      if (NULL!=(lead=fg->leader.GetUnit())) {
	  if (float(rand())/RAND_MAX<SIMULATION_ATOM/time_to_recommand_wing) {
	    if (parent->Threat()&&(parent->FShieldData()<.2||parent->RShieldData()<.2)){
	      fg->directive = string("h");
	      LeadMe (parent,"h","I need help here!");
	      fprintf (stderr,"he needs help %s",parent->name.c_str());
	    }else {
	      if (lead->getFgSubnumber()>=parent->getFgSubnumber()) {	
		fg->directive = string("b");
		LeadMe (parent,"b","I'm taking over this wing. Break and attack");
	      }
	    }
	  }
      }
    }
  }
}
void AggressiveAI::AfterburnerJumpTurnTowards (Unit * target) {
  AfterburnTurnTowards(this,parent);
  if (jump_time_check==0) {
    float dist = (target->Position()- parent->Position()).MagnitudeSquared();
    if (last_jump_distance<dist) {
      //force jump
      if (target->GetDestinations().size()) {
	string dest= target->GetDestinations()[0];
	UnitUtil::JumpTo(parent,dest);
      }
    }else {
      last_jump_distance = dist;
    }
  }
  
}
void AggressiveAI::Execute () {  
  jump_time_check++;//just so we get a nicely often wrapping var;
  Flightgroup * fg=parent->getFlightgroup();
  //ReCommandWing(fg);
  FireAt::Execute();
  if (!ProcessCurrentFgDirective (fg)) {
  Unit * target = parent->Target();
  bool isjumpable = target?(!target->GetDestinations().empty()):false;
  if (isjumpable) {
  if (parent->GetJumpStatus().drive<0) {
    parent->ActivateJumpDrive(0);
    if (parent->GetJumpStatus().drive==-2) {
      static bool AIjumpCheat=XMLSupport::parse_bool (vs_config->getVariable ("AI","always_have_jumpdrive_cheat","false"));
      if (AIjumpCheat) {
		  static int i=0;
		  if (!i) {
			  fprintf (stderr,"FIXME: warning ship not equipped to jump");
			  i=1;
		  }
		  parent->jump.drive=-1;
      }else {
	//	fprintf (stderr,"warning ship not equipped to jump");
	parent->Target(NULL);
      }
    }else if (parent->GetJumpStatus().drive<0){
      static bool AIjumpCheat=XMLSupport::parse_bool (vs_config->getVariable ("AI","jump_cheat","true"));
      if (AIjumpCheat) {
	parent->jump.drive=0;
      }
    }
  }
  }
  if ((!isjumpable) &&interruptcurtime<=0) {
//	  fprintf (stderr,"i");
	  ProcessLogic (*interrupts, true);
  }
  //  if (parent->getAIState()->queryType (Order::FACING)==NULL&&parent->getAIState()->queryType (Order::MOVEMENT)==NULL) { 
  


  if (queryAny (Order::FACING|Order::MOVEMENT)==NULL) { 
    if (isjumpable) {
      AfterburnerJumpTurnTowards (target);
    }else {
      last_jump_distance=FLT_MAX;
      if (target) {
		  ProcessLogic(*logic,false);
      }else {
	FlyStraight(this,parent);
	/*
	Order * ord = new Orders::MatchLinearVelocity (parent->ClampVelocity(Vector (0,0,10000),false),true,true,false);
	ord->SetParent(parent);
	EnqueueOrder (ord);
	*/
      }
    }
  } else {
    if (target) {
    WarpToP(parent,target);
    logiccurtime-=SIMULATION_ATOM;
    interruptcurtime-=SIMULATION_ATOM;	
    if (logiccurtime<=0) {
      //parent->getAIState()->eraseType (Order::FACING);
      //parent->getAIState()->eraseType (Order::MOVEMENT);
      eraseType (Order::FACING);
      eraseType (Order::MOVEMENT);
      if (isjumpable ) {
		  AfterburnerJumpTurnTowards (target);
	      logiccurtime = logic->maxtime;      
      }else {
		  last_jump_distance=FLT_MAX;
		  ProcessLogic (*logic,false);
      }

    }
    }
  }
  }
#ifdef AGGDEBUG
  fprintf (stderr,"endagg");
  fflush (stderr);
#endif    
  if (getTimeCompression()>3) {
	float mag = parent->GetVelocity().Magnitude();
	if (mag>.01)
		mag = 1/mag;
    parent->SetVelocity(parent->GetVelocity()*(mag*parent->GetComputerData().max_speed()/getTimeCompression()));
	parent->NetLocalForce=parent->NetForce=Vector(0,0,0);
  }
}  


