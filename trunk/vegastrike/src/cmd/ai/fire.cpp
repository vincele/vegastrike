#include "fire.h"
#include "flybywire.h"
#include "navigation.h"
#include "cmd/planet_generic.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "cmd/unit_util.h"
#include "cmd/script/flightgroup.h"
#include "cmd/role_bitmask.h"
#include "cmd/ai/communication.h"
#include "universe_util.h"
#include <algorithm>
#include "cmd/unit_find.h"
#include "vs_random.h"

static bool NoDockWithClear() {
	static bool nodockwithclear = XMLSupport::parse_bool (vs_config->getVariable ("physics","dock_with_clear_planets","true"));
	return nodockwithclear;
}

VSRandom targrand(time(NULL));

Unit * getAtmospheric (Unit * targ) {
  if (targ) {
    Unit * un;
    for (un_iter i=_Universe->activeStarSystem()->getUnitList().createIterator();
	 (un=*i)!=NULL;
	 ++i) {
      if (un->isUnit()==PLANETPTR) {
	if ((targ->Position()-un->Position()).Magnitude()<targ->rSize()*.5) {
	  if (!(((Planet *)un)->isAtmospheric())) {
	    return un;
	  }
	}
      }
    }
  }
  return NULL;
  
}

bool RequestClearence(Unit *parent, Unit *targ, unsigned char sex) {
	if (!targ->DockingPortLocations().size())
		return false;
	if (targ->isUnit()==PLANETPTR) {
		if (((Planet * )targ)->isAtmospheric()&&NoDockWithClear()) {
			targ = getAtmospheric (targ);
			if (!targ) {
				return false;
			}
			parent->Target(targ);
		}
	}
	CommunicationMessage c(parent,targ,NULL,sex);
	c.SetCurrentState(c.fsm->GetRequestLandNode(),NULL,sex);
	Order * o=targ->getAIState();
	if (o)
		o->Communicate (c);
	return true;
}

using Orders::FireAt;
bool FireAt::PursueTarget (Unit * un, bool leader) {
  if (leader)
    return true;
  if (un==parent->Target())
    return rand()<.9*RAND_MAX;
  if (parent->getRelation(un)<0)
    return rand()<.2*RAND_MAX;
  return false;
}

bool CanFaceTarget (Unit * su, Unit *targ,const Matrix & matrix) {
	return true;
	float limitmin = su->Limits().limitmin;
	if (limitmin>-.99) {
		QVector pos = (targ->Position()- su ->Position()).Normalize();
		QVector pnorm = pos.Cast();
		Vector structurelimits = su->Limits().structurelimits;
		Vector worldlimit = TransformNormal(matrix,structurelimits);
		if (pnorm.Dot (worldlimit)<limitmin) {
			return false;
		}
		
	}
	Unit * ssu;
	for (un_iter i = su->getSubUnits();
		 (ssu = *i)!=NULL;
		 ++i) {
		if (!CanFaceTarget (ssu,targ,su->cumulative_transformation_matrix)) {
			return false;
		}
	}
	return true;
}


void FireAt::ReInit (float reaction_time, float aggressivitylevel) {
  static float missileprob = XMLSupport::parse_float (vs_config->getVariable ("AI","Firing","MissileProbability",".01"));
  static float mintimetoswitch = XMLSupport::parse_float(vs_config->getVariable ("AI","Targetting","MinTimeToSwitchTargets","3"));
  lastmissiletime=UniverseUtil::GetGameTime()-65536.;
  missileprobability = missileprob;  
  gunspeed=float(.0001);
  gunrange=float(.0001);
  missilerange=float(.0001);
  delay=0;
  agg = aggressivitylevel;
  rxntime = reaction_time;
  distance=1;
  //JS --- spreading target switch times
  lastchangedtarg=0.0-targrand.uniformInc(0,1)*mintimetoswitch;
  had_target=false;

}
FireAt::FireAt (float reaction_time, float aggressivitylevel): CommunicatingAI (WEAPON,STARGET){
  ReInit (reaction_time,aggressivitylevel);
}
FireAt::FireAt (): CommunicatingAI (WEAPON,STARGET) {
  static float reaction = XMLSupport::parse_float (vs_config->getVariable ("AI","Firing","ReactionTime",".2"));
  static float aggr = XMLSupport::parse_float (vs_config->getVariable ("AI","Firing","Aggressivity","15"));
  ReInit (reaction,aggr);
}

void FireAt::SignalChosenTarget () {
}
//temporary way of choosing
struct TargetAndRange {
  Unit * t;
  float range;
  float relation;
  TargetAndRange (Unit * tt, float r,float rel) {
    t =tt;range=r;this->relation = rel;
  }
};
struct RangeSortedTurrets {
  Unit * tur;
  float gunrange;
  RangeSortedTurrets (Unit * t, float r) {tur = t; gunrange = r;}
  bool operator < (const RangeSortedTurrets &o) const{
    return gunrange<o.gunrange;
  }
};
struct TurretBin{
  float maxrange;
  vector <RangeSortedTurrets> turret;
  vector <TargetAndRange> listOfTargets[2];//we have the over (and eq) 16 crowd and the under 16  
  TurretBin () {
    maxrange =0;
  }
  bool operator < (const TurretBin & o) const{
    return (maxrange>o.maxrange);
  }
  void AssignTargets(const TargetAndRange &finalChoice, const Matrix & pos) {
    //go through modularly assigning as you go;
    int count=0;
    const unsigned int lotsize[2]={listOfTargets[0].size(),listOfTargets[1].size()};
    for (vector<RangeSortedTurrets>::iterator uniter=turret.begin();uniter!=turret.end();++uniter) {
      bool foundfinal=false;
      uniter->tur->Target(NULL);
      uniter->tur->TargetTurret(NULL);
      if (finalChoice.t) {
		/*  FIX ME FIXME missiles not accounted for yet
		  if(uniter->gunrange<0){
		    uniter->gunrange=FLT_MAX; // IS MISSILE TURRET (we hope)
		  }
		  */
	    if (finalChoice.range<uniter->gunrange&&ROLES::getPriority (uniter->tur->combatRole())[finalChoice.t->combatRole()]<31) {
			if (CanFaceTarget(uniter->tur,finalChoice.t,pos)) {
				uniter->tur->Target(finalChoice.t);
				uniter->tur->TargetTurret(finalChoice.t);
				foundfinal=true;
			}
		}
      }
      if (!foundfinal) {
	for (unsigned int f=0;f<2&&!foundfinal;f++) {
	for (unsigned int i=0;i<lotsize[f];i++) {
	  const int index =(count+i)%lotsize[f];
	  if (listOfTargets[f][index].range<uniter->gunrange) {
		  if (CanFaceTarget(uniter->tur,finalChoice.t,pos)) {
			  uniter->tur->Target(listOfTargets[f][index].t);
			  uniter->tur->TargetTurret(listOfTargets[f][index].t);
			  count++;
			  foundfinal=true;
			  break;
		  }
	  }
	}
	}
      }
    }
  }
};
void FireAt::getAverageGunSpeed (float & speed, float & range, float &mmrange) const{
  speed =gunspeed;
  range= gunrange;
  mmrange=missilerange;
}
void AssignTBin (Unit * su, vector <TurretBin> & tbin) {
    unsigned int bnum=0;
    for (;bnum<tbin.size();bnum++) {
      if (su->combatRole()==tbin[bnum].turret[0].tur->combatRole())
	break;
    }
    if (bnum>=tbin.size()) {
      tbin.push_back (TurretBin());
    }
    float gspeed, grange, mrange;
    grange=FLT_MAX;
	Order * o = su->getAIState();
	if (o) {
          o->getAverageGunSpeed (gspeed,grange,mrange);
        }
        {
          float ggspeed,ggrange,mmrange;
          Unit * ssu;
          for (un_iter i=su->getSubUnits();(ssu=*i)!=NULL;++i) {
            if (ssu->getAIState()) {
              ssu->getAIState()->getAverageGunSpeed(ggspeed,ggrange,mmrange);
              if (ggspeed>gspeed)gspeed=ggspeed;
              if (ggrange>grange)grange=ggrange;
              if (mmrange>mrange)mrange=mmrange;
            }
          }
        }
    if (tbin [bnum].maxrange<grange) {
      tbin [bnum].maxrange=grange;
    }
    tbin[bnum].turret.push_back (RangeSortedTurrets (su,grange));
}
float Priority (Unit * me, Unit * targ, float gunrange,float rangetotarget, float relationship, char *rolepriority) {
  if(relationship>=0)
    return -1;
  if (targ->GetHull()<0)
    return -1;
  *rolepriority = ROLES::getPriority (me->combatRole())[targ->combatRole()];//number btw 0 and 31 higher better
  char invrolepriority=31-*rolepriority;
  if(invrolepriority<=0)
    return -1;
  if(rangetotarget <1 && rangetotarget >-1000){
	rangetotarget=1;
  } else {
	  rangetotarget=fabs(rangetotarget);
  }
  if(rangetotarget<.5*gunrange)
    rangetotarget=.5*gunrange;
  if(gunrange <=0){
	  static float mountless_gunrange = XMLSupport::parse_float (vs_config->getVariable("AI","Targetting","MountlessGunRange","300000000"));
	  gunrange= mountless_gunrange;
	  //rangetotarget;
  }//probably a mountless capship. 50000 is chosen arbitrarily
  float inertial_priority=0;
  {
    static float mass_inertial_priority_cutoff =XMLSupport::parse_float (vs_config->getVariable ("AI","Targetting","MassInertialPriorityCutoff","5000"));
    if (me->GetMass()>mass_inertial_priority_cutoff) {
      static float mass_inertial_priority_scale =XMLSupport::parse_float (vs_config->getVariable ("AI","Targetting","MassInertialPriorityScale",".0000001"));
      Vector normv (me->GetVelocity());
      float Speed = me->GetVelocity().Magnitude();
      normv*=1/Speed;
      Vector ourToThem = targ->Position()-me->Position();
      ourToThem.Normalize();
      inertial_priority = mass_inertial_priority_scale*(.5 + .5 * (normv.Dot(ourToThem)))*me->GetMass()*Speed;
    }
    
  }
  static float threat_weight = XMLSupport::parse_float (vs_config->getVariable ("AI","Targetting","ThreatWeight",".5"));
  float threat_priority = (me->Threat()==targ)?threat_weight:0;
  threat_priority+= (targ->Target()==me)?threat_weight:0;
  float role_priority01 = ((float)*rolepriority)/31.;
  float range_priority01 =.5*gunrange/rangetotarget;//number between 0 and 1 for most ships 1 is best
  return range_priority01*role_priority01+inertial_priority+threat_priority;
}
float Priority (Unit * me, Unit * targ, float gunrange,float rangetotarget, float relationship) {
  char rolepriority=0;
  return Priority(me,targ,gunrange,rangetotarget,relationship,&rolepriority);
}
template <class T, size_t n> class StaticTuple{
public:
  T vec[n];  
  size_t size(){
    return n;
  }
  T& operator[](size_t index) {return vec[index];}
  const T&operator[](size_t index)const{return vec[index];}
};
template <size_t numTuple> class ChooseTargetClass{
  Unit * parent;
  Unit * parentparent;
  vector<TurretBin>*tbin;
  StaticTuple<float,numTuple> maxinnerrange;
  float priority;
  char rolepriority;
  char maxrolepriority;
  bool reached;
  FireAt* fireat;
  float gunrange;
  int numtargets;
  int maxtargets;
public:
  Unit * mytarg;
  ChooseTargetClass(){}
  void init (FireAt* fireat, Unit*un, float gunrange, vector<TurretBin>*tbin, const StaticTuple<float,numTuple> &innermaxrange, char maxrolepriority, int maxtargets) {
    this->fireat=fireat;
    this->tbin=tbin;
    this->parent=un;
    this->parentparent=un->owner?UniverseUtil::getUnitByPtr(un->owner,un,false):0;
    mytarg=NULL;
    this->maxinnerrange=innermaxrange;
    this->maxrolepriority=maxrolepriority;// max priority that will allow gun range to be ok
    reached=false;
    this->priority=-1;
    this->rolepriority=31;
    this->gunrange=gunrange;
    this->numtargets=0;
    this->maxtargets=maxtargets;
  }
  bool acquire(Unit*un, float distance) {
    
    if (distance>maxinnerrange[0]&&!reached) {
      reached=true;          
      if (mytarg&&rolepriority<maxrolepriority) {
        return false;
      }else {
        for (size_t i=1;i<numTuple;++i) {
          if (distance<maxinnerrange[i]){
            maxinnerrange[0]=maxinnerrange[i];
            reached=false;
          }
        }
      }
    }
    
    if (un->CloakVisible()>.8) {
      float rangetotarget = distance;
      float rel[] = {
          fireat->GetEffectiveRelationship (un),
          un->getRelation(this->parent),
          (parentparent?parentparent->getRelation(un):rel[0]),
          (parentparent?un->getRelation(parentparent):rel[0]) };
      float relationship = rel[0];
      for (int i=1; i<sizeof(rel)/sizeof(*rel); i++) 
          if (rel[i]<relationship) 
              relationship=rel[i];
      char rp=31;
      float tmp=Priority (parent,un, gunrange,rangetotarget, relationship,&rp);
      if (tmp>priority) {
        mytarg = un;
        priority=tmp;
        rolepriority=rp;
      }
      for (vector <TurretBin>::iterator k=tbin->begin();k!=tbin->end();++k) {
	if (rangetotarget>k->maxrange) {
	  break;
	}
	const char tprior=ROLES::getPriority (k->turret[0].tur->combatRole())[un->combatRole()];
	if (relationship<0) {
	  if (tprior<16){
	    k->listOfTargets[0].push_back (TargetAndRange (un,rangetotarget,relationship));
        numtargets++;
	  }else if (tprior<31){
	    k->listOfTargets[1].push_back (TargetAndRange (un,rangetotarget,relationship));
        numtargets++;
	  }
	}
      }
    }
    return (maxtargets==0)||(numtargets<maxtargets);
  }
};

static float targettimer =UniverseUtil::GetGameTime(); // timer used to determine passage of physics frames
static int numpolled=0; // number of units that searched for a target
static int prevpollindex=10000; // previous number of units touched (doesn't need to be precise)
static int pollindex=1; // current count of number of units touched (doesn't need to be precise)  -- used for "fairness" heuristic
void FireAt::ChooseTargets (int numtargs, bool force) {
  static float mintimetoswitch = XMLSupport::parse_float(vs_config->getVariable ("AI","Targetting","MinTimeToSwitchTargets","3"));
  static float targetswitchtime = XMLSupport::parse_float (vs_config->getVariable ("AI","Targetting","TimeUntilSwitch","20"));	
  static int numpollers = XMLSupport::parse_float(vs_config->getVariable ("AI","Targetting","Numberofpollersperframe","49")); // maximum number of vessels allowed to search for a target in a given physics frame
  if (lastchangedtarg+mintimetoswitch>0) 
    return;//don't switch if switching too soon
  
  // Following code exists to limit the number of craft polling for a target in a given frame - this is an expensive operation, and needs to be spread out, or there will be pauses.
  if((UniverseUtil::GetGameTime())-targettimer>SIMULATION_ATOM){ // Check if at least one physics frame has passed
    numpolled=0; // reset counters
	prevpollindex=pollindex;
	pollindex=0;
    targettimer=UniverseUtil::GetGameTime();
  }
  pollindex++; // count number of craft touched - will use in the next physics frame to spread out the vessels actually chosen to be processed among all of the vessels being touched
  if(numpolled>numpollers){ // over quota, wait until next physics frame
	return;
  }
  if(!(pollindex%((prevpollindex/numpollers)+1))){ // spread out, in modulo fashion, the possibility of changing one's target. Use previous physics frame count of craft to estimate current number of craft
	  if(parent->Target()==NULL){ // if they didn't have a target, they probably won't have one this time either - we're likely wasting our time, so 
		  if(rand()%2){ // half the time we see an unlikely candidate, we punt until the next time we see them
		    lastchangedtarg+=mintimetoswitch; // which we make sure isn't for a while.
		    return;
		  }
	  }
	numpolled++; // if a more likely candidate, we're going to search for a target.
  }else{
	return; // skipped to achieve better fairness - see comment on modulo distribution above
  }
  Unit * curtarg=NULL;
  if ((curtarg=parent->Target())) 
    if (isJumpablePlanet (curtarg))
      return;
  Flightgroup * fg = parent->getFlightgroup();;
  parent->getAverageGunSpeed (gunspeed,gunrange,missilerange);  
  lastchangedtarg=0+targrand.uniformInc(0,1)*mintimetoswitch; // spread out next valid time to switch targets - helps to ease per-frame loads.
  if (fg) {
    if (!fg->directive.empty()) {
      if (curtarg!=NULL&&(*fg->directive.begin())==toupper (*fg->directive.begin())) {
		  return;//not 	allowed to switch targets
      }
    }
  }
  

  Unit * un=NULL;
  vector <TurretBin> tbin;;
  
  float priority=0;
  Unit * mytarg=NULL;
  Unit * su=NULL;
  un_iter subun = parent->getSubUnits();
  for (;(su = *subun)!=NULL;++subun) {
	  static int inert = ROLES::getRole ("INERT");
	  static int pointdef = ROLES::getRole("POINTDEF");
	  static bool assignpointdef = XMLSupport::parse_bool(vs_config->getVariable("AI","Targetting","AssignPointDef","true"));
	  if ((su->combatRole()!=pointdef)||assignpointdef) {
		if (su->combatRole()!=inert) {
		  AssignTBin (su,tbin);
		}else {
			Unit * ssu=NULL;
			for (un_iter subturret = su->getSubUnits();(ssu =(*subturret));++subturret) {
			  AssignTBin(ssu ,tbin);
			}
		}
	  }
	  
  }
  std::sort (tbin.begin(),tbin.end());
  float efrel = 0;
  float mytargrange = FLT_MAX;
  static float unitRad = XMLSupport::parse_float(vs_config->getVariable("AI","Targetting","search_extra_radius","1000")); // Maximum target radius that is guaranteed to be detected
  static char maxrolepriority = XMLSupport::parse_int(vs_config->getVariable("AI","Targetting","search_max_role_priority","16")); 
  static int maxtargets = XMLSupport::parse_int(vs_config->getVariable("AI","Targetting","search_max_candidates","64")); // Cutoff candidate count (if that many hostiles found, stop search - performance/quality tradeoff, 0=no cutoff)
  UnitWithinRangeLocator<ChooseTargetClass<2> > unitLocator(parent->GetComputerData().radar.maxrange,unitRad);
  StaticTuple<float,2> maxranges;
  maxranges[0]=gunrange;
  maxranges[1]=missilerange;
  if (tbin.size())
    maxranges[0]=(tbin[0].maxrange>gunrange?tbin[0].maxrange:gunrange);
  unitLocator.action.init(this,parent,gunrange,&tbin,maxranges,maxrolepriority,maxtargets);
  findObjects(_Universe->activeStarSystem(),parent->location,&unitLocator);
  mytarg=unitLocator.action.mytarg;
  if (mytarg) {
    efrel=GetEffectiveRelationship (mytarg);
    mytargrange = UnitUtil::getDistance (parent,mytarg);
  }
  TargetAndRange my_target (mytarg,mytargrange,efrel);
  for (vector <TurretBin>::iterator k =  tbin.begin();k!=tbin.end();++k) {
    k->AssignTargets(my_target,parent->cumulative_transformation_matrix);
  } 
  parent->LockTarget(false);
  parent->Target (mytarg);
  parent->LockTarget(true);
  SignalChosenTarget();
}
/* Proper choosing of targets
void FireAt::ChooseTargets (int num) {
  UnitCollection tmp;
  UnitCollection::UnitIterator *iter = _Universe->activeStarSystem()->getUnitList().createIterator();
  Unit * un ;
  while ((un = iter->current())) {
    //how to choose a target?? "if looks particularly juicy... :-) tmp.prepend (un);
    iter->advance();
  }
  delete iter;
  AttachOrder (&tmp);
}

*/
bool FireAt::ShouldFire(Unit * targ, bool &missilelock) {
  float dist;
    if (!targ) {
      return false;
        static int test=0;
        if (test++%1000==1)
            VSFileSystem::vs_fprintf (stderr,"lost target");
    }
  float angle = parent->cosAngleTo (targ, dist,parent->GetComputerData().itts?gunspeed:FLT_MAX,gunrange);
  missilelock=false;
  targ->Threaten (parent,angle/(dist<.8?.8:dist));
  if (targ==parent->Target()) {
    distance = dist;
  }
  static float firewhen = XMLSupport::parse_float (vs_config->getVariable ("AI","Firing","InWeaponRange","1.2"));
  static float fireangle_minagg = (float)cos(XMLSupport::parse_float (vs_config->getVariable ("AI","Firing","MaximumFiringAngle.minagg","0.35"))); //Roughly 20 degrees
  static float fireangle_maxagg = (float)cos(XMLSupport::parse_float (vs_config->getVariable ("AI","Firing","MaximumFiringAngle.maxagg","0.785"))); //Roughly 45 degrees
  bool temp=parent->TrackingGuns(missilelock);
  bool isjumppoint=targ->isUnit()==PLANETPTR&&((Planet*)targ)->GetDestinations().empty()==false;
  float fangle = (fireangle_minagg+fireangle_maxagg*agg)/(1.0f+agg);
  return ((dist<firewhen&&angle>fangle)||(temp&&dist<firewhen&&angle>0))&&!isjumppoint;
}

FireAt::~FireAt() {
#ifdef ORDERDEBUG
  VSFileSystem::vs_fprintf (stderr,"fire%x\n",this);
  fflush (stderr);
#endif

}
unsigned int FireBitmask (Unit * parent,bool shouldfire, bool firemissile) {
   unsigned int firebitm = ROLES::EVERYTHING_ELSE;
    Unit * un=parent->Target();
    if (un) {
      firebitm = (1 << un->combatRole());

      static bool AlwaysFireAutotrackers = XMLSupport::parse_bool(vs_config->getVariable("AI","AlwaysFireAutotrackers","true"));
      if (shouldfire)
        firebitm|= ROLES::FIRE_GUNS;

      if (AlwaysFireAutotrackers&&!shouldfire) {
        firebitm |= ROLES::FIRE_GUNS;
	firebitm |= ROLES::FIRE_ONLY_AUTOTRACKERS;
      }
      if (firemissile) 
	firebitm = ROLES::FIRE_MISSILES;// stops guns
    }
    return firebitm;
}
void FireAt::FireWeapons(bool shouldfire, bool lockmissile) {
  static float missiledelay =XMLSupport::parse_float(vs_config->getVariable("AI","MissileGunDelay","4"));
  static float missiledelayprob =XMLSupport::parse_float(vs_config->getVariable("AI","MissileGunDelayProbability",".25"));
  bool fire_missile=lockmissile&&rand()<RAND_MAX*missileprobability*SIMULATION_ATOM;
  delay+=SIMULATION_ATOM;
  if (shouldfire&&delay<rxntime) {

    return;
  }else if (!shouldfire) {
    delay=0;
  }
  if (fire_missile) {
    lastmissiletime=UniverseUtil::GetGameTime();
  }else if (UniverseUtil::GetGameTime()-lastmissiletime<missiledelay&&!fire_missile) {
    return;
  }
  parent->Fire(FireBitmask(parent,shouldfire,fire_missile),true);
}

bool FireAt::isJumpablePlanet(Unit * targ) {
    bool istargetjumpableplanet = targ->isUnit()==PLANETPTR;
    if (istargetjumpableplanet) {
      istargetjumpableplanet=(!((Planet*)targ)->GetDestinations().empty())&&(parent->GetJumpStatus().drive>=0);
      if (!istargetjumpableplanet) {
	//	ChooseTarget(); //WTF this will cause endless loopdiloop
      }
    }
    return istargetjumpableplanet;
}
using std::string;
void FireAt::PossiblySwitchTarget(bool unused) {
//  static float targetswitchprobability = XMLSupport::parse_float (vs_config->getVariable ("AI","Targetting","TargetSwitchProbability",".01"));
  static float targettime = XMLSupport::parse_float (vs_config->getVariable ("AI","Targetting","TimeUntilSwitch","20"));
  if (lastchangedtarg+targettime<0) {
	  bool ct= true;
	  Flightgroup * fg;	  
	  if ((fg = parent->getFlightgroup())) {
		  if (fg->directive.find(".")!=string::npos) {
			  ct=(parent->Target()==NULL);
		  }
	  }
	  if (ct)
		  ChooseTarget();
  }
}
void FireAt::Execute () {
  lastchangedtarg-=SIMULATION_ATOM;
  
  bool missilelock=false;
  bool tmp = done;
  Order::Execute();	
  if (gunspeed==float(.0001)) {
    ChooseTarget();//starting condition
  }
  done = tmp;
  Unit * targ;
  if (parent->isUnit()==UNITPTR) {
    static float cont_update_time = XMLSupport::parse_float (vs_config->getVariable ("AI","ContrabandUpdateTime","1"));
    if (rand()<RAND_MAX*SIMULATION_ATOM/cont_update_time) {
      UpdateContrabandSearch();
    }
    static float cont_initiate_time = XMLSupport::parse_float (vs_config->getVariable ("AI","CommInitiateTime","300"));
    if ((float)rand()<((float)RAND_MAX*(SIMULATION_ATOM/cont_initiate_time))) {
      static float contraband_initiate_time = XMLSupport::parse_float (vs_config->getVariable ("AI","ContrabandInitiateTime","3000"));
	  static float comm_to_player=XMLSupport::parse_float(vs_config->getVariable("AI","CommToPlayerPercent",".05"));
	  static float comm_to_target=XMLSupport::parse_float(vs_config->getVariable("AI","CommToTargetPercent",".25"));
	  static float contraband_to_player=XMLSupport::parse_float(vs_config->getVariable("AI","ContrabandToPlayerPercent",".98"));
	  static float contraband_to_target=XMLSupport::parse_float(vs_config->getVariable("AI","ContrabandToTargetPercent","0.001"));

      unsigned int modulo = ((unsigned int)(contraband_initiate_time/cont_initiate_time));
      if (modulo<1)
	modulo=1;
      if (rand()%modulo) {
	RandomInitiateCommunication(comm_to_player,comm_to_target);
      }else {
	InitiateContrabandSearch (contraband_to_player,contraband_to_target);
      }
    }
  }

  bool shouldfire=false;
  //  if (targets) 
  //    shouldfire |=DealWithMultipleTargets();
  bool istargetjumpableplanet=false;
  if ((targ = parent->Target())) {
    istargetjumpableplanet = isJumpablePlanet (targ);
    if (targ->CloakVisible()>.8&&targ->GetHull()>=0) {
      had_target=true;
      if (parent->GetNumMounts()>0) {
	if (!istargetjumpableplanet)
	  shouldfire |= ShouldFire (targ,missilelock);
      }
    }else {
      if (had_target) {
	had_target=false;
	lastchangedtarg=-100000;
      }
      ChooseTarget();
    }
  }else {
    if (had_target) {
      had_target=false;
      lastchangedtarg=-100000;
    }
  }
  PossiblySwitchTarget(istargetjumpableplanet);

  if ((!istargetjumpableplanet)&&parent->GetNumMounts ()>0) {
    FireWeapons (shouldfire,missilelock);
  }
}
