#include "unit.h"
#include "unit_factory.h"
#include "ai/order.h"
#include "gfx/animation.h"
#include "gfx/mesh.h"
#include "gfx/halo.h"
#include "gfx/bsp.h"
#include "vegastrike.h"
#include "unit_collide.h"
#include <float.h>
#include "audiolib.h"
#include "images.h"
#include "beam.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "xml_support.h"
#include "savegame.h"
#include "gfx/cockpit.h"
#include "cmd/script/mission.h"
#include "missile.h"
#include "cmd/ai/communication.h"
#include "cmd/script/flightgroup.h"
#include "music.h"
//#define DESTRUCTDEBUG
#include "base.h"
extern unsigned short apply_float_to_short (float tmp);

static list<Unit*> Unitdeletequeue;
static std::vector <Mesh *> MakeMesh(unsigned int mysize) {
  std::vector <Mesh *> temp;
  for (unsigned int i=0;i<mysize;i++) {
    temp.push_back(NULL);
  }
  return temp;
}
void GameUnit::Split (int level) {
  int i;
  int nm = nummesh();
  if (nm<=0) {
    return;
  }
  Vector PlaneNorm;
  std::vector <Mesh *> old = meshdata;

  for (int split=0;split<level;split++) {
    std::vector<Mesh *> nw= MakeMesh(nm*2+1);
    nw[nm*2]=old[nm];//copy shield
    for (i=0;i<nm;i++) {
      PlaneNorm.Set (rand()-RAND_MAX/2,rand()-RAND_MAX/2,rand()-RAND_MAX/2+.5);
      PlaneNorm.Normalize();  
      old[i]->Fork (nw[i*2], nw[i*2+1],PlaneNorm.i,PlaneNorm.j,PlaneNorm.k,-PlaneNorm.Dot(old[i]->Position()));//splits somehow right down the middle.
      if (nw[i*2]&&nw[i*2+1]) {
	delete old[i];
	old[i]=NULL;
      }else {
	nw[i*2+1]= NULL;
	nw[i*2]=old[i];
      }
    }
    nm*=2;
    for (i=0;i<nm;i++) {
      if (nw[i]==NULL) {
	for (int j=i+1;j<nm;j++) {
	  nw[j-1]=nw[j];
	}
	nm--;
	nw[nm]=NULL;
      }
    }
    old = nw;
  }
  if (old[nm])
    delete old[nm];
  old[nm]=NULL;
  for (i=0;i<nm;i++) {
    Unit * splitsub;
    std::vector<Mesh *> tempmeshes;
    tempmeshes.push_back (old[i]);
    SubUnits.prepend(splitsub = UnitFactory::createUnit (tempmeshes,true,faction));
    splitsub->mass = mass/level;
    splitsub->image->timeexplode=.1;
    if (splitsub->meshdata[0]) {
      Vector loc = splitsub->meshdata[0]->Position();
      splitsub->ApplyForce(splitsub->meshdata[0]->rSize()*10*mass*loc/loc.Magnitude());
      loc.Set (rand(),rand(),rand());
      loc.Normalize();
      static float explosion_torque = XMLSupport::parse_float (vs_config->getVariable ("graphics","explosiontorque",".002"));//10 seconds for auto to kick in;
      splitsub->ApplyLocalTorque(loc*mass*explosion_torque*rSize()*(1+rand()%(int)(1+rSize())));
    }
  }
  old.clear();
  meshdata.clear();
  meshdata.push_back(NULL);//the shield
  //FIXME...how the heck can they go spinning out of control!
}

extern Music *muzak;

void GameUnit::Kill(bool erasefromsave) {

  if (colTrees)
    colTrees->Dec();//might delete
  colTrees=NULL;
  //if (erasefromsave)
  //  _Universe->AccessCockpit()->savegame->RemoveUnitFromSave((long)this);
  
  if (docked&(DOCKING_UNITS)) {
    vector <Unit *> dockedun;
    unsigned int i;
    for (i=0;i<image->dockedunits.size();i++) {
      Unit * un;
      if (NULL!=(un=image->dockedunits[i]->uc.GetUnit())) 
	dockedun.push_back (un);
    }
    while (!dockedun.empty()) {
      dockedun.back()->UnDock(this);
      dockedun.pop_back();
    }
  }
  for (int beamcount=0;beamcount<GetNumMounts();beamcount++) {
    AUDStopPlaying(mounts[beamcount]->sound);
    AUDDeleteSound(mounts[beamcount]->sound);
    if (mounts[beamcount]->ref.gun&&mounts[beamcount]->type->type==weapon_info::BEAM)
      delete mounts[beamcount]->ref.gun;//hope we're not killin' em twice...they don't go in gunqueue
  }
	for( vector<Mount *>::iterator jj=mounts.begin(); jj!=mounts.end(); jj++)
	{
		// Free all mounts elements
		if( (*jj)!=NULL)
			delete (*jj);
	}
    mounts.clear();
  //eraticate everything. naturally (see previous line) we won't erraticate beams erraticated above
  if (!SubUnit) 
    RemoveFromSystem();
  killed = true;
  computer.target.SetUnit (NULL);

  //God I can't believe this next line cost me 1 GIG of memory until I added it
  computer.threat.SetUnit (NULL);
  computer.velocity_ref.SetUnit(NULL);
  if(aistate) {
    aistate->ClearMessages();
    aistate->Destroy();
  }
  aistate=NULL;
  UnitCollection::UnitIterator iter = getSubUnits();
  Unit *un;
  while ((un=iter.current())) {
    un->Kill();
    iter.advance();
  }
  if (ucref==0) {
    Unitdeletequeue.push_back(this);
  if (flightgroup) {
    if (flightgroup->leader.GetUnit()==this) {
      flightgroup->leader.SetUnit(NULL);
    }
  }

#ifdef DESTRUCTDEBUG
    fprintf (stderr,"%s 0x%x - %d\n",name.c_str(),this,Unitdeletequeue.size());
#endif
  }
}

extern float rand01 ();

float GameUnit::DealDamageToHullReturnArmor (const Vector & pnt, float damage, unsigned short * &t ) {
  float percent;
  unsigned short *targ=NULL;
  percent = Unit::DealDamageToHullReturnArmor( pnt, damage, targ);
  if( percent == -1)
	  return -1;
  if (damage<*targ) {
    if (!AUDIsPlaying (sound->armor))
      AUDPlay (sound->armor,ToWorldCoordinates(pnt).Cast()+cumulative_transformation.position,Velocity,1);
    else
      AUDAdjustSound (sound->armor,ToWorldCoordinates(pnt).Cast()+cumulative_transformation.position,Velocity);
    *targ -= apply_float_to_short (damage);
  }else {
    if (!AUDIsPlaying (sound->hull))
      AUDPlay (sound->hull,ToWorldCoordinates(pnt).Cast()+cumulative_transformation.position,Velocity,1);
    else
      AUDAdjustSound (sound->hull,ToWorldCoordinates(pnt).Cast()+cumulative_transformation.position,Velocity);
    damage -= ((float)*targ);
    *targ= 0;
    if (_Universe->AccessCockpit()->GetParent()!=this||_Universe->AccessCockpit()->godliness<=0||hull>damage) {
      static float system_failure=XMLSupport::parse_float(vs_config->getVariable ("physics","indiscriminate_system_destruction",".25"));
      DamageRandSys(system_failure*rand01()+(1-system_failure)*(1-(damage/hull)),pnt);
      hull -=damage;
    }else {
      _Universe->AccessCockpit()->godliness-=damage;
      DamageRandSys(rand01()*.5+.2,pnt);//get system damage...but live!
    }

  }
  ////////////////// MOVE IN UNIT::DEALDAMAGETOHULL //////////////////////
  if (hull <0) {
      static float hulldamtoeject = XMLSupport::parse_float(vs_config->getVariable ("physics","hull_damage_to_eject","100"));
    if (!SubUnit&&hull>-hulldamtoeject) {
      static float autoejectpercent = XMLSupport::parse_float(vs_config->getVariable ("physics","autoeject_percent",".5"));

      static float cargoejectpercent = XMLSupport::parse_float(vs_config->getVariable ("physics","eject_cargo_percent",".25"));
      if (rand()<(RAND_MAX*autoejectpercent)&&isUnit()==UNITPTR) {
	EjectCargo ((unsigned int)-1);
      }
      for (unsigned int i=0;i<numCargo();i++) {
	if (rand()<(RAND_MAX*cargoejectpercent)) {
	  EjectCargo(i);
	}
      }
    }
#ifdef ISUCK
    Destroy();
#endif
    PrimeOrders();
    maxenergy=energy=0;

    Split (rand()%3+1);
#ifndef ISUCK
    Destroy();
    return -1;
#endif
  }
  /////////////////////////////
  if (!FINITE (percent))
    percent = 0;
  return percent;
}
float GameUnit::DealDamageToShield (const Vector &pnt, float &damage) {
  float percent = Unit::DealDamageToShield( pnt, damage);
  if (percent&&!AUDIsPlaying (sound->shield))
	AUDPlay (sound->shield,ToWorldCoordinates(pnt).Cast()+cumulative_transformation.position,Velocity,1);
  else
	AUDAdjustSound (sound->shield,ToWorldCoordinates(pnt).Cast()+cumulative_transformation.position,Velocity);

  return percent;
}

float GameUnit::ApplyLocalDamage (const Vector & pnt, const Vector & normal, float amt, Unit * affectedUnit,const GFXColor &color, float phasedamage) {
  static float nebshields=XMLSupport::parse_float(vs_config->getVariable ("physics","nebula_shield_recharge",".5"));
  //  #ifdef REALLY_EASY
  Cockpit * cpt;
  if ((cpt=_Universe->isPlayerStarship(this))!=NULL) {
    if (color.a!=2) {
      //    ApplyDamage (amt);
      phasedamage*= (g_game.difficulty);
      amt*=(g_game.difficulty);
      cpt->Shake (amt);
    }
  }
  //  #endif
  float percentage=0;
  percentage = Unit::ApplyLocalDamage( pnt, normal, amt, affectedUnit, color, phasedamage);
  float leakamt = phasedamage+amt*.01*shield.leak;
  if( percentage==-1)
	return -1;
  if (GetNebula()==NULL||(nebshields>0)) {
    percentage = DealDamageToShield (pnt,amt);
    if (meshdata.back()&&percentage>0&&amt==0) {//shields are up
      /*      meshdata[nummesh]->LocalFX.push_back (GFXLight (true,
	      GFXColor(pnt.i+normal.i,pnt.j+normal.j,pnt.k+normal.k),
	      GFXColor (.3,.3,.3), GFXColor (0,0,0,1), 
	      GFXColor (.5,.5,.5),GFXColor (1,0,.01)));*/
      //calculate percentage
      if (GetNebula()==NULL) 
	meshdata.back()->AddDamageFX(pnt,shieldtight?shieldtight*normal:Vector(0,0,0),percentage,color);
    }
  }
  if (shield.leak>0||!meshdata.back()||percentage==0||amt>0||phasedamage) {
    percentage = DealDamageToHull (pnt, leakamt+amt);
    if (percentage!=-1) {//returns -1 on death--could delete
      for (int i=0;i<nummesh();i++) {
	if (percentage)
	  meshdata[i]->AddDamageFX(pnt,shieldtight?shieldtight*normal:Vector (0,0,0),percentage,color);
      }
    }
  }
  return 1;
}

//un scored a faction kill
void ScoreKill (Cockpit * cp, Unit * un, int faction) {
  static float KILL_FACTOR=-XMLSupport::parse_float(vs_config->getVariable("AI","kill_factor",".2"));
  FactionUtil::AdjustIntRelation(faction,un->faction,KILL_FACTOR,1);
  static float FRIEND_FACTOR=-XMLSupport::parse_float(vs_config->getVariable("AI","friend_factor",".1"));
  for (unsigned int i=0;i<FactionUtil::GetNumFactions();i++) {
    float relation;
    if (faction!=i&&un->faction!=i) {
      relation=FactionUtil::GetIntRelation(i,faction);
      if (relation)
        FactionUtil::AdjustIntRelation(i,un->faction,FRIEND_FACTOR*relation,1);
    }
  }
  olist_t * killlist = &cp->savegame->getMissionData (string("kills"));
  while (killlist->size()<=FactionUtil::GetNumFactions()) {
    killlist->push_back (new varInst (VI_IN_OBJECT));
    killlist->back()->type=VAR_FLOAT;
    killlist->back()->float_val=0;
  }
  if (killlist->size()>faction) {
    (*killlist)[faction]->float_val++;
  }
  killlist->back()->float_val++;
}


void GameUnit::ApplyDamage (const Vector & pnt, const Vector & normal, float amt, Unit * affectedUnit, const GFXColor & color, Unit * ownerDoNotDereference, float phasedamage) {
  Cockpit * cp = _Universe->isPlayerStarship (ownerDoNotDereference);

  if (cp) {
      //now we can dereference it because we checked it against the parent
      CommunicationMessage c(ownerDoNotDereference,this,NULL,0);
      c.SetCurrentState(c.fsm->GetHitNode(),NULL,0);
      this->getAIState()->Communicate (c);      
      Threaten (ownerDoNotDereference,10);//the dark danger is real!
  }
  bool mykilled = hull<0;
  Vector localpnt (InvTransform(cumulative_transformation_matrix,pnt));
  Vector localnorm (ToLocalCoordinates (normal));
  ApplyLocalDamage(localpnt, localnorm, amt,affectedUnit,color,phasedamage);
  if (hull<0&&(!mykilled)) {
    if (cp) {
      ScoreKill (cp,ownerDoNotDereference,faction);
      
    }
  }
}
extern Animation * GetVolatileAni (unsigned int);
extern unsigned int AddAnimation (const QVector &, const float, bool, const std::string &, float percentgrow);


extern Animation * getRandomCachedAni() ;
extern std::string getRandomCachedAniString() ;
bool GameUnit::Explode (bool drawit, float timeit) {

  if (image->explosion==NULL&&image->timeexplode==0) {	//no explosion in unit data file && explosions haven't started yet

  // notify the director that a ship got destroyed
  mission->DirectorShipDestroyed(this);

    image->timeexplode=0;
    static std::string expani = vs_config->getVariable ("graphics","explosion_animation","explosion_orange.ani");

    string bleh=image->explosion_type;
    if (bleh.empty()) {
      GameFactionUtil::getRandAnimation(faction,bleh);
    }
    if (bleh.empty()) {
      static Animation cache(expani.c_str(),false,.1,BILINEAR,false);
      bleh = getRandomCachedAniString();
      if (bleh.size()==0) {
	bleh = expani;
      }
    }
    image->explosion= new Animation (bleh.c_str(),false,.1,BILINEAR,false);
    image->explosion->SetDimensions(ExplosionRadius(),ExplosionRadius());
    if (isUnit()!=MISSILEPTR) {
      static float expdamagecenter=XMLSupport::parse_float(vs_config->getVariable ("physics","explosion_damage_center","1"));
      static float damageedge=XMLSupport::parse_float(vs_config->getVariable ("graphics","explosion_damage_edge",".125"));
      _Universe->activeStarSystem()->AddMissileToQueue (new MissileEffect (Position().Cast(),MaxShieldVal(),0,ExplosionRadius()*expdamagecenter,ExplosionRadius()*expdamagecenter*damageedge));
    }
	if (!SubUnit){
		QVector exploc = cumulative_transformation.position;
		Unit * un;
		if (NULL!=(un=_Universe->AccessCockpit(0)->GetParent())) {
			exploc = un->Position();						
		}
	    AUDPlay (sound->explode,exploc,Velocity,1);

	  un=_Universe->AccessCockpit()->GetParent();
	  if (isUnit()==UNITPTR) {
	    static float percentage_shock=XMLSupport::parse_float(vs_config->getVariable ("graphics","percent_shockwave",".5"));
	    if (rand () < RAND_MAX*percentage_shock&&(!SubUnit)) {
	      static float shockwavegrowth=XMLSupport::parse_float(vs_config->getVariable ("graphics","shockwave_growth","1.05"));
	      static string shockani (vs_config->getVariable ("graphics","shockwave_animation","explosion_wave.ani"));
	      
	      static Animation * __shock__ani = new Animation (shockani.c_str(),true,.1,MIPMAP,false);
	      __shock__ani->SetFaceCam(false);
	      unsigned int which = AddAnimation (Position(),ExplosionRadius(),true,shockani,shockwavegrowth);
	      Animation * ani = GetVolatileAni (which);
	      if (ani) {
		ani->SetFaceCam(false);
		Vector p,q,r;
		GetOrientation(p,q,r);
		int tmp = rand();
		if (tmp < RAND_MAX/3) {
		  ani->SetOrientation (Vector(1,0,0),Vector(0,1,0),Vector(0,0,1));
		}else if (tmp< 2*(RAND_MAX/3)) {
		  ani->SetOrientation (Vector(0,1,0),Vector(0,0,1),Vector(1,0,0));
		}else {
		  ani->SetOrientation (Vector(0,0,1),Vector(1,0,0),Vector(0,1,0));
		}
	      }
	    }
		  if (un ) {
			static float badrel=XMLSupport::parse_float(vs_config->getVariable("sound","loss_relationship","-.1"));
			static float goodrel=XMLSupport::parse_float(vs_config->getVariable("sound","victory_relationship",".5"));
			float rel=un->getRelation(this);
			if (!Base::CurrentBase)
			  if (rel>goodrel) {
			    muzak->SkipRandSong(Music::LOSSLIST);
			  } else if (rel < badrel) {
			    muzak->SkipRandSong(Music::VICTORYLIST);
			  }
		  } else {
			muzak->SkipRandSong(Music::LOSSLIST);
		  }
	  }
	}
  }
  if (image->explosion) {
      image->timeexplode+=timeit;
      //Translate (tmp,meshdata[i]->Position());
      //MultMatrix (tmp2,cumulative_transformation_matrix,tmp);
      image->explosion->SetPosition(Position());
      Vector p,q,r;
      GetOrientation (p,q,r);
      image->explosion->SetOrientation(p,q,r);
      if (image->explosion->Done()) {
	delete image->explosion;	
	image->explosion=NULL;
      }
      if (drawit&&image->explosion) { 
	image->explosion->Draw();//puts on draw queue... please don't delete
      }
      
  }
  bool alldone = image->explosion?!image->explosion->Done():false;
  if (!SubUnits.empty()) {
    UnitCollection::UnitIterator ui = getSubUnits();
    Unit * su;
    while ((su=ui.current())) {
      alldone |=su->Explode(drawit,timeit);
      ui.advance();
    }
  }
  return alldone;
}
