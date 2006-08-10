#include "comm_ai.h"
#include "faction_generic.h"
#include "communication.h"
#include "cmd/collection.h"
#include "gfx/cockpit_generic.h"
#include "cmd/images.h"
#include "configxml.h"
#include "vs_globals.h"
#include "cmd/script/flightgroup.h"
#include "cmd/unit_util.h"
#include "vs_random.h"
#include "cmd/unit_find.h"
CommunicatingAI::CommunicatingAI (int ttype, int stype,  float rank, float mood, float anger,float appeas,  float moodswingyness, float randomresp) :Order (ttype,stype),anger(anger), appease(appeas), moodswingyness(moodswingyness),randomresponse (randomresp),mood(mood),rank(rank) {
  comm_face=NULL;
  if (rank>665&&rank<667) {
    static float ran = XMLSupport::parse_float(vs_config->getVariable ("AI","DefaultRank",".01"));
    this->rank = ran;
  }
  if (appease>665&&appease<667) {
    static float appeas = XMLSupport::parse_float(vs_config->getVariable ("AI","EaseToAppease",".5"));
    this->appease = appeas;    
  }
  if ((anger>665&&anger<667)||(anger >-667&&anger <-665)) {
    static float ang = XMLSupport::parse_float(vs_config->getVariable ("AI","EaseToAnger","-.5"));
    this->anger = ang;
  }
  if (moodswingyness>665&&moodswingyness<667) {
    static float ang1 = XMLSupport::parse_float(vs_config->getVariable ("AI","MoodSwingLevel",".2"));
    this->moodswingyness = ang1;
  }
  if (randomresp>665&&moodswingyness<667) {
    static float ang2 = XMLSupport::parse_float(vs_config->getVariable ("AI","RandomResponseRange",".8"));
    this->randomresponse = ang2;
  }
}
vector <Animation *> *CommunicatingAI::getCommFaces(unsigned char &sex) {
  sex = this->sex;
  return comm_face;
}

void CommunicatingAI::SetParent (Unit * par) {
  Order::SetParent(par);
  comm_face = FactionUtil::GetRandCommAnimation(par->faction,par,sex);
}
bool MatchingMood(const CommunicationMessage& c,float mood, float randomresponse, float relationship) {

  static float pos_limit =XMLSupport::parse_float(vs_config->getVariable ("AI",
                                                                          "LowestPositiveCommChoice",
                                                                          "0"));
  static float neg_limit =XMLSupport::parse_float(vs_config->getVariable ("AI",
                                                                          "LowestNegativeCommChoice",
                                                                          "-.00001"));
  const FSM::Node *n = c.curstate<c.fsm->nodes.size()?(&c.fsm->nodes[c.curstate]):(&c.fsm->nodes[c.fsm->getDefaultState(relationship)]);
  std::vector<unsigned int>::const_iterator iend= n->edges.end();
  for (std::vector<unsigned int>::const_iterator i=n->edges.begin();i!=iend;++i) {
    if (c.fsm->nodes[*i].messagedelta>=pos_limit&&relationship>=0)
      return true;
    if (c.fsm->nodes[*i].messagedelta<=neg_limit&&relationship<0)
      return true;
  }
  return false;
}
int CommunicatingAI::selectCommunicationMessageMood (CommunicationMessage &c, float mood) {

  Unit * targ = c.sender.GetUnit();
  float relationship=0;

  if (targ) {
    relationship= GetEffectiveRelationship(targ);
    if (targ==parent->Target()&&relationship>-1.0)
      relationship=-1.0;
    mood+=(1-randomresponse)*relationship;
  }
  //breaks stuff
  if ((c.curstate>=c.fsm->GetUnDockNode())||!MatchingMood(c,mood,randomresponse,relationship)) {
    c.curstate = c.fsm->getDefaultState(relationship);//hijack the current state
  }
  return c.fsm->getCommMessageMood (c.curstate,mood,randomresponse,relationship);

}
using std::pair;

float CommunicatingAI::getAnger(const Unit * target)const {
  relationmap::const_iterator i=effective_relationship.find(target);
  float rel=0;
  if (i!=effective_relationship.end()) {
    rel = (*i).second;
  }
  if (_Universe->isPlayerStarship(target)){
    if (FactionUtil::GetFactionName(parent->faction).find("pirates")!=std::string::npos) {
      static unsigned int cachedCargoNum=0;
      static bool good=true;
      if (cachedCargoNum!=target->numCargo()) {
        cachedCargoNum=target->numCargo();
        good=true;
        for (unsigned int i=0;i<cachedCargoNum;++i) {
          Cargo * c=&target->image->cargo[i];
          if (c->quantity!=0&&c->category.find("upgrades")==string::npos){
            good=false;
            break;
          }
        }
      }
      if (good) {
        static float goodness_for_nocargo=XMLSupport::parse_float(vs_config->getVariable("AI","pirate_bonus_for_empty_hold",".75"));
        rel+=goodness_for_nocargo;
      }    
    }
    {
      int fac=parent->faction;
      MapStringFloat::iterator mapiter=factions[fac]->ship_relation_modifier.find(target->name);
      if (mapiter!=factions[fac]->ship_relation_modifier.end()) {
        rel+=(*mapiter).second;
      }
    }
  }
  return rel;
}
float CommunicatingAI::GetEffectiveRelationship (const Unit * target)const {
  return Order::GetEffectiveRelationship (target)+getAnger(target);
}
void GetMadAt(Unit* un, Unit * parent, int numhits=0) {
  if (numhits==0) {
    static int snumhits = XMLSupport::parse_int(vs_config->getVariable("AI","ContrabandMadness","5"));
    numhits =snumhits;
  }
  CommunicationMessage hit (un,parent,NULL,0);
  hit.SetCurrentState(hit.fsm->GetHitNode(),NULL,0);
  for (         int i=0;i<numhits;i++) {
    parent->getAIState()->Communicate(hit);
  }

}

void AllUnitsCloseAndEngage(Unit * un, int faction) {
	Unit * ally;
	static float contraband_assist_range = XMLSupport::parse_float (vs_config->getVariable("physics","contraband_assist_range","50000"));
        float relation=0;
        static float minrel = XMLSupport::parse_float (vs_config->getVariable("AI","max_faction_contraband_relation","-.05"));
        static float adj = XMLSupport::parse_float (vs_config->getVariable("AI","faction_contraband_relation_adjust","-.025"));
        if ((relation=FactionUtil::GetIntRelation(faction,un->faction))>minrel) {
          FactionUtil::AdjustIntRelation(faction,un->faction,minrel-relation,1);
        }else {
          FactionUtil::AdjustIntRelation(faction,un->faction,adj,1);
        }
	for (un_iter i=_Universe->activeStarSystem()->getUnitList().createIterator();
		(ally = *i)!=NULL;
		++i) {
		//Vector loc;
          
		if (ally->faction==faction) {
			//if (ally->InRange (un,loc,true)) {
				if ((ally->Position()-un->Position()).Magnitude()<contraband_assist_range) {
                                        GetMadAt(un,ally);
					Flightgroup * fg = ally->getFlightgroup();
					if (fg) {
						if (fg->directive.empty()?true:toupper(*fg->directive.begin())!=*fg->directive.begin()) {
							ally->Target (un);
							ally->TargetTurret (un);
                                                        
						}else {
							ally->Target (un);
							ally->TargetTurret (un);

						}
					}
				}
			//}
		}
	}
}
void CommunicatingAI::TerminateContrabandSearch(bool contraband_detected) {
  //reports success or failure
  Unit * un;
  if ((un=contraband_searchee.GetUnit())) {
    CommunicationMessage c(parent,un,comm_face,sex);
    if (contraband_detected) {
      c.SetCurrentState(c.fsm->GetContrabandDetectedNode(),comm_face,sex);
      GetMadAt (un,0);
      AllUnitsCloseAndEngage(un,parent->faction);

    }else {
      c.SetCurrentState(c.fsm->GetContrabandUnDetectedNode(),comm_face,sex);      
    }
	Order * o = un->getAIState();
	if (o)
		o->Communicate(c);
  }
  contraband_searchee.SetUnit(NULL);

}
void CommunicatingAI::GetMadAt (Unit * un, int numHitsPerContrabandFail) {
  ::GetMadAt(un,parent,numHitsPerContrabandFail);
}
static int InList (std::string item, Unit * un) {
  float numcontr = 0;
  if (un) {
    for (unsigned int i=0;i<un->numCargo();i++) {
      if (item==un->GetCargo(i).content) {
        if (un->GetCargo(i).quantity>0)
          numcontr++;
      }
    }
  }
  return numcontr;
}

void CommunicatingAI::UpdateContrabandSearch () {
  Unit * u = contraband_searchee.GetUnit();
  if (u && (u->faction != parent->faction)) { // don't scan your buddies
    if (which_cargo_item<(int)u->numCargo()) {
      if (u->GetCargo(which_cargo_item).quantity>0) {
        int which_carg_item_bak=which_cargo_item;
        std::string item = u->GetManifest (which_cargo_item++,parent,SpeedAndCourse);
        static bool use_hidden_cargo_space=XMLSupport::parse_bool (vs_config->getVariable ("physics","use_hidden_cargo_space","true"));
        static float speed_course_change = XMLSupport::parse_float (vs_config->getVariable ("AI","PercentageSpeedChangeToStopSearch","1"));
        if (u->CourseDeviation(SpeedAndCourse,u->GetVelocity())>speed_course_change) {
          CommunicationMessage c(parent,u,comm_face,sex);
          c.SetCurrentState(c.fsm->GetContrabandWobblyNode(),comm_face,sex);
          Order * o;
          if ((o=u->getAIState()))
            o->Communicate (c);
          GetMadAt(u,1);
          SpeedAndCourse=u->GetVelocity();
        }
        float HiddenTotal = use_hidden_cargo_space?(u->getHiddenCargoVolume()):(0);
        //float HiddenUsed = u->getHiddenCargoVolume();
        Unit * contrabandlist=FactionUtil::GetContraband(parent->faction);
        if (InList (item,contrabandlist) > 0) { // inlist now returns an integer so that we can do this at all...
          if (HiddenTotal==0||u->GetCargo(which_carg_item_bak).quantity>HiddenTotal) {
            TerminateContrabandSearch(true); // BUCO this is where we want to check against free hidden cargo space.
          }else {
            unsigned int max=u->numCargo();
            unsigned int quantity=0;
            for (unsigned int i=0;i<max;++i) {
              if (InList(u->GetCargo(i).content,contrabandlist)>0) {
                quantity+=u->GetCargo(i).quantity;                              
                if (quantity>HiddenTotal) {
                  TerminateContrabandSearch(true);
                  break;
                }
              }
            }
          }
        }
      }
    }else {
      TerminateContrabandSearch(false);
      
    }
  }
}
static bool isDockedAtAll(Unit * un) {
  return (un->docked&(Unit::DOCKED_INSIDE|Unit::DOCKED))!=0;
}
void CommunicatingAI::Destroy(){    
	for ( int i=0;i<_Universe->numPlayers();++i) {
		Unit * target = _Universe->AccessCockpit(i)->GetParent();
		if (target) {
			FSM * fsm = FactionUtil::GetConversation(this->parent->faction,target->faction);	
			if (fsm->StopAllSounds(this->sex)) {
                          _Universe->AccessCockpit(i)->SetStaticAnimation ();
                          _Universe->AccessCockpit(i)->SetStaticAnimation ();
                          
                        }
		}
	}
    this->Order::Destroy();
}

void CommunicatingAI::InitiateContrabandSearch (float playaprob, float targprob) {
  Unit *u= GetRandomUnit (playaprob,targprob);
  if (u) {
    Unit * un =FactionUtil::GetContraband (parent->faction);
    if (un) {
    if (un->numCargo()>0&&UnitUtil::getUnitSystemFile(u)==UnitUtil::getUnitSystemFile(parent)&&!UnitUtil::isDockableUnit(parent)) {
    Unit * v;
    if ((v=contraband_searchee.GetUnit())) {
      if (v==u) {
	return;
      }
      TerminateContrabandSearch(false);
    }
    contraband_searchee.SetUnit (u);
    SpeedAndCourse = u->GetVelocity();
    CommunicationMessage c(parent,u,comm_face,sex);
    c.SetCurrentState(c.fsm->GetContrabandInitiateNode(),comm_face,sex);
	if (u->getAIState())
		u->getAIState()->Communicate (c);
    which_cargo_item = 0;
    }
    }
  }
}

void CommunicatingAI::AdjustRelationTo (Unit * un, float factor) {
  Order::AdjustRelationTo(un,factor);

  //now we do our magik  insert 0 if nothing's there... and add on our faction
  relationmap::iterator i = effective_relationship.insert (pair<const Unit*,float>(un,0)).first;
  bool abovezero=(*i).second+FactionUtil::GetIntRelation (parent->faction,un->faction)>=0;
  if (!abovezero) {
    static float slowrel=XMLSupport::parse_float (vs_config->getVariable ("AI","SlowDiplomacyForEnemies",".25"));
    factor *=slowrel;
  }
  FactionUtil::AdjustIntRelation (parent->faction,un->faction,factor,rank);  
  (*i).second+=factor;
  if ((*i).second<anger||(parent->Target()==NULL&&(*i).second+Order::GetEffectiveRelationship (un)<0)) {
	  if (parent->Target()==NULL||(parent->getFlightgroup()==NULL||parent->getFlightgroup()->directive.find(".")==string::npos)){
		  parent->Target(un);//he'll target you--even if he's friendly
		  parent->TargetTurret(un);//he'll target you--even if he's friendly
	  }
  } else if ((*i).second>appease) {
    if (parent->Target()==un) {
		if (parent->getFlightgroup()==NULL||parent->getFlightgroup()->directive.find(".")==string::npos) {
			parent->Target(NULL);
			parent->TargetTurret(NULL);//he'll target you--even if he's friendly
		}

    }
  }
  mood+=factor*moodswingyness;
}


//modified not to check player when hostiles are around--unless player IS the hostile
Unit * CommunicatingAI::GetRandomUnit (float playaprob, float targprob) {
  if (vsrandom.uniformInc(0,1)<playaprob) {
    Unit* playa = _Universe->AccessCockpit(rand()%_Universe->numPlayers())->GetParent();
    if (playa) {
      if ((playa->Position()-parent->Position()).Magnitude()-parent->rSize()-playa->rSize()) {
        return playa;
      }
    }
  }
  if (vsrandom.uniformInc(0,1)<targprob&&parent->Target()) {
    return parent->Target();
  }
  //FIXME FOR TESTING ONLY
  //return parent->Target();
  QVector where=parent->Position()+parent->GetComputerData().radar.maxrange*QVector(vsrandom.uniformInc(-1,1),
                                                                                   vsrandom.uniformInc(-1,1),
                                                                                   vsrandom.uniformInc(-1,1));
  Collidable wherewrapper(0,0,where);
  
  CollideMap* cm=_Universe->activeStarSystem()->collidemap[Unit::UNIT_ONLY];
  static float unitRad = XMLSupport::parse_float(vs_config->getVariable("graphics","hud","radar_search_extra_radius","1000"));
  
  NearestUnitLocator unitLocator;    
  
#ifdef VS_ENABLE_COLLIDE_KEY
  CollideMap::iterator iter=cm->lower_bound(wherewrapper);
  
  if (iter!=cm->end()&&(*iter)->radius>0) {
    if ((*iter)->ref.unit!=parent)
      return (*iter)->ref.unit;
  }
  findObjects(_Universe->activeStarSystem()->collidemap[Unit::UNIT_ONLY],iter,&unitLocator);

  Unit *target = unitLocator.retval.unit;

  if (target==NULL||target==parent) {
    target=parent->Target();
  }
#else
  Unit *target = parent->Target();
#endif
  return target;
}
void CommunicatingAI::RandomInitiateCommunication (float playaprob, float targprob) {
  Unit * target = GetRandomUnit(playaprob,targprob);
  if (target!=NULL) {
    if (UnitUtil::getUnitSystemFile(target)==UnitUtil::getUnitSystemFile(parent)&&UnitUtil::getFlightgroupName(parent)!="Base"&&!isDockedAtAll(target)&&UnitUtil::getDistance(parent,target)<=target->GetComputerData().radar.maxrange) {//warning--odd hack they can talk to you if you can sense them--it's like SETI@home
      for (std::list<CommunicationMessage *>::iterator i=messagequeue.begin();i!=messagequeue.end();i++) {   
        Unit * un=(*i)->sender.GetUnit();
        if (un==target) {
          return;
        }
      }
      //ok we're good to put a default msg in the queue as a fake message;
      FSM * fsm = FactionUtil::GetConversation(target->faction,this->parent->faction);
      int state =fsm->getDefaultState(parent->getRelation(target));
      messagequeue.push_back (new CommunicationMessage (target,this->parent,state,state,comm_face,sex));
    }
  }
}

int CommunicatingAI::selectCommunicationMessage (CommunicationMessage &c,Unit * un) {
  if (0&&mood==0) {
    FSM::Node * n = c.getCurrentState ();  
    if (n)
      return rand()%n->edges.size();
    else
      return 0;
  }else {
    static float moodmul = XMLSupport::parse_float(vs_config->getVariable ("AI","MoodAffectsRespose","0"));
    static float angermul = XMLSupport::parse_float(vs_config->getVariable ("AI","AngerAffectsRespose","1"));
    static float staticrelmul = XMLSupport::parse_float(vs_config->getVariable ("AI","StaticRelationshipAffectsResponse","1"));
    return selectCommunicationMessageMood (c,moodmul*mood+angermul*getAnger (un)+staticrelmul*Order::GetEffectiveRelationship(un));
  }
}

void CommunicatingAI::ProcessCommMessage (CommunicationMessage &c) {
  if (messagequeue.back()->curstate<messagequeue.back()->fsm->GetUnDockNode()) {

  Order::ProcessCommMessage(c);
  FSM *tmpfsm = c.fsm;
  Unit * targ = c.sender.GetUnit();
  if (targ&&UnitUtil::getUnitSystemFile(targ)==UnitUtil::getUnitSystemFile(parent)&&!isDockedAtAll(targ)) {
    c.fsm  =FactionUtil::GetConversation (parent->faction,targ->faction);
    FSM::Node * n = c.getCurrentState ();
    if (n) {
      if (n->edges.size()) {
        Unit * un = c.sender.GetUnit();
        if (un) {
          int b = selectCommunicationMessage (c,un);
          Order * o = un->getAIState();
          if (o)
            o->Communicate (CommunicationMessage (parent,un,c,b,comm_face,sex));
        }
      }
    }
    c.fsm=tmpfsm;    
  }
  }
}
CommunicatingAI::~CommunicatingAI() {

}
