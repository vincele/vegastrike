/*
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
 *
 * http://vegastrike.sourceforge.net/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "gfx/mesh.h"
#include "unit.h"
#include "lin_time.h"
//#include "physics.h"
#include "beam.h"
#include "planet.h"
#include "audiolib.h"
#include "config_xml.h"
#include "vs_globals.h"
//#ifdef WIN32
#ifdef FIX_TERRAIN
#include "gfx/planetary_transform.h"
#endif
#include "gfx/cockpit.h"
#include "unit_util.h"
#include "universe_util.h"
#include "cmd/script/mission.h"
#include "networking/netclient.h"
//#endif
extern float copysign (float x, float y);

// the rotation should be applied in world coordinates
/** MISNOMER...not really clamping... more like renomalizing  slow too
Vector Unit::ClampTorque(const Vector &amt1) {
  Vector norm = amt1;
  norm.Normalize();
  Vector max = MaxTorque(norm);

  if(max.Magnitude() > amt1.Magnitude())
    return amt1;
  else 
    return max;
}
*/
//FIXME 062201

extern unsigned short apply_float_to_short (float tmp);
//    float max_speed;
//    float max_ab_speed;
//    float max_yaw;
//    float max_pitch;
//    float max_roll;

void GameUnit::Thrust(const Vector &amt1,bool afterburn){
  Vector amt = ClampThrust(amt1,afterburn);
  ApplyLocalForce(amt);  
 if (_Universe->AccessCockpit(0)->GetParent()==this)
  if (afterburn!=AUDIsPlaying (sound->engine)) {
    if (afterburn)
      AUDPlay (sound->engine,cumulative_transformation.position,cumulative_velocity,1);
    else
      //    if (Velocity.Magnitude()<computer.max_speed)
      AUDStopPlaying (sound->engine);
  }
}

Cockpit * GameUnit::GetVelocityDifficultyMult(float &difficulty) const{
  difficulty=1;
  Cockpit * player_cockpit=_Universe->isPlayerStarship(this);
  if ((player_cockpit)==NULL) {
    static float exp = XMLSupport::parse_float (vs_config->getVariable ("physics","difficulty_speed_exponent",".2"));
    difficulty = pow(g_game.difficulty,exp);
  }
  return player_cockpit;
}

void GameUnit::UpdatePhysics (const Transformation &trans, const Matrix &transmat, const Vector & cum_vel,  bool lastframe, UnitCollection *uc) {
  static float VELOCITY_MAX=XMLSupport::parse_float(vs_config->getVariable ("physics","velocity_max","10000"));

	Transformation old_physical_state = curr_physical_state;

	Vector accel( this->ResolveForces( trans, transmat));

  if (docked&DOCKING_UNITS) {

    PerformDockingOperations();

  }

  Repair();

  if (fuel<0)

    fuel=0;

  if (cloaking>=cloakmin) {

    if (image->cloakenergy*SIMULATION_ATOM>energy) {

      Cloak(false);//Decloak

    } else {

      if (image->cloakrate>0||cloaking==cloakmin) {

	energy-=apply_float_to_short(SIMULATION_ATOM*image->cloakenergy);

      }

      if (cloaking>cloakmin) {

	AUDAdjustSound (sound->cloak, cumulative_transformation.position,cumulative_velocity);

	if ((cloaking==32767&&image->cloakrate>0)||(cloaking==cloakmin+1&&image->cloakrate<0)) {

	  AUDStartPlaying (sound->cloak);

	}

	cloaking-=image->cloakrate*SIMULATION_ATOM;

	if (cloaking<=cloakmin&&image->cloakrate>0) {

	  //AUDStopPlaying (sound->cloak);

	  cloaking=cloakmin;

	}

	if (cloaking<0&&image->cloakrate<0) {

	  //AUDStopPlaying (sound->cloak);

	  cloaking=(short)32768;//wraps

	}

      }

    }

  }



  RegenShields();

  if (lastframe) {

    if (!(docked&(DOCKED|DOCKED_INSIDE))) 

      prev_physical_state = curr_physical_state;//the AIscript should take care
#ifdef FIX_TERRAIN
    if (planet) {

      if (!planet->dirty) {

	SetPlanetOrbitData (NULL);

      }else {

	planet->pps = planet->cps;

      }
    }
#endif
  }

  if (isUnit()==PLANETPTR) {

    ((Planet *)this)->gravitate (uc);

  } else {

    if (resolveforces) {

      ResolveForces (trans,transmat);//clamp velocity

      if (fabs (Velocity.i)>VELOCITY_MAX) {

	Velocity.i = copysign (VELOCITY_MAX,Velocity.i);

      }

      if (fabs (Velocity.j)>VELOCITY_MAX) {

	Velocity.j = copysign (VELOCITY_MAX,Velocity.j);

      }

      if (fabs (Velocity.k)>VELOCITY_MAX) {

	Velocity.k = copysign (VELOCITY_MAX,Velocity.k);

      }

    }

  } 

  if(AngularVelocity.i||AngularVelocity.j||AngularVelocity.k) {

    Rotate (SIMULATION_ATOM*(AngularVelocity));

  }

  float difficulty;

  Cockpit * player_cockpit=GetVelocityDifficultyMult (difficulty);



  // Here send (new position + direction = curr_physical_state.position and .orientation)

  // + speed to server (which velocity is to consider ?)

  // + maybe Angular velocity to anticipate rotations in the other network clients

  if( Network!=NULL && Network->isTime())

  {

	  //cout<<"SEND UPDATE"<<endl;

	  this->networked=1;

		// Check if this is a player, because in network mode we should only send updates of our moves

	  if( _Universe->isPlayerStarship( this) /* && this->networked */ )

	  {

		  // (NetForce + Transform (ship_matrix,NetLocalForce) )/mass = GLOBAL ACCELERATION

		  curr_physical_state.position = curr_physical_state.position +  (Velocity*SIMULATION_ATOM*difficulty).Cast();

		  // If we want to inter(extra)polate sent position, DO IT HERE

		  if( !(old_physical_state.position == curr_physical_state.position && old_physical_state.orientation == curr_physical_state.orientation))

				// We moved so update

				 Network->sendPosition( ClientState( Network->getSerial(), curr_physical_state, Velocity, accel, 0));

		    else

			  // Say we are still alive

			  Network->sendAlive();

	  }

	  else

		  // Not the player so update the unit's position and stuff with the last received snapshot from the server

			;

  }

  else

  {

	  //this->networked++;

 	 curr_physical_state.position = curr_physical_state.position +  (Velocity*SIMULATION_ATOM*difficulty).Cast();

  }

#ifdef DEPRECATEDPLANETSTUFF

  if (planet) {

    Matrix basis;

    curr_physical_state.to_matrix (cumulative_transformation_matrix);

    Vector p,q,r,c;

    MatrixToVectors (cumulative_transformation_matrix,p,q,r,c);

    planet->trans->InvTransformBasis (cumulative_transformation_matrix,p,q,r,c);

    planet->cps=Transformation::from_matrix (cumulative_transformation_matrix);

  }

#endif

  cumulative_transformation = curr_physical_state;

  cumulative_transformation.Compose (trans,transmat);

  cumulative_transformation.to_matrix (cumulative_transformation_matrix);

  cumulative_velocity = TransformNormal (transmat,Velocity)+cum_vel;





  Transformation * ct;

  Matrix * ctm=NULL;

  SetPlanetHackTransformation (ct,ctm);

  int i;

  if (lastframe) {

    char tmp=0;

    for (i=0;i<meshdata.size();i++) {

      if (!meshdata[i])

	continue;

      tmp |=meshdata[i]->HasBeenDrawn();

      if (!meshdata[i]->HasBeenDrawn()) {

	meshdata[i]->UpdateFX(SIMULATION_ATOM);

      }

      meshdata[i]->UnDraw();

    }

    if (!tmp&&hull<0) {

      Explode(false,SIMULATION_ATOM);

	

    }

  }      

  Unit * target = Unit::Target();

  bool increase_locking=false;

  if (target&&cloaking<0/*-1 or -32768*/) {

    if (target->isUnit()!=PLANETPTR) {

      Vector TargetPos (ToLocalCoordinates ((target->Position()-Position()).Cast())); 

      TargetPos.Normalize(); 

      if (TargetPos.Dot(Vector(0,0,1))>computer.radar.lockcone) {

	increase_locking=true;

      }

    }

  }

  static string LockingSoundName = vs_config->getVariable ("unitaudio","locking","locking.wav");

  static int LockingSound = AUDCreateSoundWAV (LockingSoundName,true);

  bool locking=false;

  bool touched=false;

  for (i=0;i<GetNumMounts();i++) {

//    if (increase_locking&&cloaking<0) {

//      mounts[i]->time_to_lock-=SIMULATION_ATOM;

//    }

    if (mounts[i]->status==Mount::ACTIVE&&cloaking<0&&mounts[i]->ammo!=0) {

      if (player_cockpit) {

	  touched=true;

      }

      if (increase_locking) {

	mounts[i]->time_to_lock-=SIMULATION_ATOM;

	static bool ai_lock_cheat=XMLSupport::parse_bool(vs_config->getVariable ("physics","ai_lock_cheat","true"));	

	if (!player_cockpit) {

	  if (ai_lock_cheat) {

	    mounts[i]->time_to_lock=-1;

	  }

	}else {



	  if (mounts[i]->type->LockTime>0) {

	    static string LockedSoundName= vs_config->getVariable ("unitaudio","locked","locked.wav");

	    static int LockedSound = AUDCreateSoundWAV (LockedSoundName,false);



	    if (mounts[i]->time_to_lock>-SIMULATION_ATOM&&mounts[i]->time_to_lock<=0) {

	      if (!AUDIsPlaying(LockedSound)) {

		AUDStartPlaying(LockedSound);

		AUDStopPlaying(LockingSound);	      

	      }

	      AUDAdjustSound (LockedSound,Position(),GetVelocity()); 

	    }else if (mounts[i]->time_to_lock>0)  {

	      locking=true;

	      if (!AUDIsPlaying(LockingSound)) {



		AUDStartPlaying(LockingSound);	      

		



	      }

	      AUDAdjustSound (LockingSound,Position(),GetVelocity());



	    }

	  }



	}

      

      }else {

        if (mounts[i]->ammo!=0) {

	  mounts[i]->time_to_lock=mounts[i]->type->LockTime;

        }

      }

    } else {

      if (mounts[i]->ammo!=0) {

        mounts[i]->time_to_lock=mounts[i]->type->LockTime;

      }

    }

    if (mounts[i]->type->type==weapon_info::BEAM) {

      if (mounts[i]->ref.gun) {

	mounts[i]->ref.gun->UpdatePhysics (cumulative_transformation, cumulative_transformation_matrix);

      }

    } else {

      mounts[i]->ref.refire+=SIMULATION_ATOM;

    }

    if (mounts[i]->processed==Mount::FIRED) {

      Transformation t1;

      Matrix m1;

      t1=prev_physical_state;//a hack that will not work on turrets

      t1.Compose (trans,transmat);

      t1.to_matrix (m1);

      int autotrack=0;

      if ((0!=(mounts[i]->size&weapon_info::AUTOTRACKING))) {

	autotrack = computer.itts?2:1;

      }

      mounts[i]->PhysicsAlignedFire (t1,m1,cumulative_velocity,(!SubUnit||owner==NULL)?this:owner,target,autotrack, computer.radar.trackingcone);

      if (mounts[i]->ammo==0&&mounts[i]->type->type==weapon_info::PROJECTILE) {

	ToggleWeapon (true);

      }

    }else if (mounts[i]->processed==Mount::UNFIRED) {

      mounts[i]->PhysicsAlignedUnfire();

    }

  }

  if (locking==false&&touched==true) {

    if (AUDIsPlaying(LockingSound)) {

      AUDStopPlaying(LockingSound);	

    }      

  }

  bool dead=true;



  if (!SubUnits.empty()) {

    Unit * su;

    UnitCollection::UnitIterator iter=getSubUnits();

    while ((su=iter.current())) {

      su->UpdatePhysics(cumulative_transformation,cumulative_transformation_matrix,cumulative_velocity,lastframe,uc); 

      su->cloaking = (short unsigned int) cloaking;

      if (hull<0) {

	UnFire();//don't want to go off shooting while your body's splitting everywhere

	su->hull-=SIMULATION_ATOM;

      }

      iter.advance();

      //    dead &=(subunits[i]->hull<0);

    }

  }

  if (hull<0) {

    dead&= (image->explosion==NULL);    

    if (dead)

      Kill();

  }

  if ((!SubUnit)&&(!killed)&&(!(docked&DOCKED_INSIDE))) {

    UpdateCollideQueue();

  }



}

void GameUnit::SetPlanetOrbitData (PlanetaryTransform *t) {
#ifdef FIX_TERRAIN
  if (isUnit()!=BUILDINGPTR)
        return;
  if (!planet)
    planet = (PlanetaryOrbitData *)malloc (sizeof (PlanetaryOrbitData));
  else if (!t) {
    free (planet);
    planet=NULL;
  }
  if (t) {
    planet->trans = t;
    planet->dirty=true;
  }
#endif
}

PlanetaryTransform * GameUnit::GetPlanetOrbit () const {

#ifdef FIX_TERRAIN
  if (planet==NULL)
    return NULL;
  return planet->trans;
#else
  return NULL;
#endif
}

bool GameUnit::jumpReactToCollision (Unit * smalle) {
  if (!GetDestinations().empty()) {//only allow big with small
    if ((smalle->GetJumpStatus().drive>=0||image->forcejump)) {
      smalle->DeactivateJumpDrive();
      GameUnit * jumppoint = this;
      _Universe->activeStarSystem()->JumpTo (smalle, jumppoint, std::string(GetDestinations()[smalle->GetJumpStatus().drive%GetDestinations().size()]));
      return true;
    }
    return true;
  }
  if (!smalle->GetDestinations().empty()) {
    if ((GetJumpStatus().drive>=0||smalle->image->forcejump)) {
      DeactivateJumpDrive();
      Unit * jumppoint = smalle;
      _Universe->activeStarSystem()->JumpTo (this, jumppoint, std::string(smalle->GetDestinations()[GetJumpStatus().drive%smalle->GetDestinations().size()]));
      return true;
    }
    return true;
  }
  return false;
}

void GameUnit::reactToCollision(Unit * smalle, const QVector & biglocation, const Vector & bignormal, const QVector & smalllocation, const Vector & smallnormal,  float dist) {
  clsptr smltyp = smalle->isUnit();
  if (smltyp==ENHANCEMENTPTR||smltyp==MISSILEPTR) {
    if (isUnit()!=ENHANCEMENTPTR&&isUnit()!=MISSILEPTR) {
      smalle->reactToCollision (this,smalllocation,smallnormal,biglocation,bignormal,dist);
      return;
    }
  }	       
  //don't bounce if you can Juuuuuuuuuuuuuump
  if (!jumpReactToCollision(smalle)) {
#ifdef NOBOUNCECOLLISION
#else
    static float bouncepercent = XMLSupport::parse_float (vs_config->getVariable ("physics","BouncePercent",".1"));
    smalle->ApplyForce (bignormal*.4*bouncepercent*smalle->GetMass()*fabs(bignormal.Dot (((smalle->GetVelocity()-this->GetVelocity())/SIMULATION_ATOM))+fabs (dist)/(SIMULATION_ATOM*SIMULATION_ATOM)));
    this->ApplyForce (smallnormal*.4*bouncepercent*GetMass()*fabs(smallnormal.Dot ((smalle->GetVelocity()-this->GetVelocity()/SIMULATION_ATOM))+fabs (dist)/(SIMULATION_ATOM*SIMULATION_ATOM)));
    float m1=smalle->GetMass(),m2=GetMass();
    Vector Elastic_dvl = (m1-m2)/(m1+m2)*smalle->GetVelocity() + smalle->GetVelocity()*2*m2/(m1+m2);
    Vector Elastic_dvs = (m2-m1)/(m1+m2)*smalle->GetVelocity() + smalle->GetVelocity()*2*m1/(m1+m2);
    Vector Inelastic_vf = (m1/(m1+m2))*smalle->GetVelocity() + (m2/(m1+m2))*GetVelocity();
    float LargeKE = (0.5)*m2*GetVelocity().MagnitudeSquared();
    float SmallKE = (0.5)*m1*smalle->GetVelocity().MagnitudeSquared();
    float FinalInelasticKE = Inelastic_vf.MagnitudeSquared()*(0.5)*(m1+m2);
	float InelasticDeltaKE = LargeKE +SmallKE - FinalInelasticKE;
    static float kilojoules_per_damage = XMLSupport::parse_float (vs_config->getVariable ("physics","kilojoules_per_unit_damage","5400"));
    static float inelastic_scale = XMLSupport::parse_float (vs_config->getVariable ("physics","inelastic_scale","1"));
	float large_damage=inelastic_scale*(InelasticDeltaKE *(1.0/4.0 + (0.5*m2/(m1+m2))) )/kilojoules_per_damage;
    float small_damage=inelastic_scale*(InelasticDeltaKE *(1.0/4.0 + (0.5*m1/(m1+m2))) )/kilojoules_per_damage;
    smalle->ApplyDamage (biglocation.Cast(),bignormal,small_damage,smalle,GFXColor(1,1,1,2),NULL);
    this->ApplyDamage (smalllocation.Cast(),smallnormal,large_damage,this,GFXColor(1,1,1,2),NULL);

    //OLDE METHODE
    //    smalle->ApplyDamage (biglocation.Cast(),bignormal,.33*g_game.difficulty*(  .5*fabs((smalle->GetVelocity()-this->GetVelocity()).MagnitudeSquared())*this->mass*SIMULATION_ATOM),smalle,GFXColor(1,1,1,2),NULL);
    //    this->ApplyDamage (smalllocation.Cast(),smallnormal, .33*g_game.difficulty*(.5*fabs((smalle->GetVelocity()-this->GetVelocity()).MagnitudeSquared())*smalle->mass*SIMULATION_ATOM),this,GFXColor(1,1,1,2),NULL);

#endif
  //each mesh with each mesh? naw that should be in one way collide
  }
}

Vector GameUnit::ResolveForces (const Transformation &trans, const Matrix &transmat) {
#ifndef PERFRAMESOUND
  AUDAdjustSound (sound->engine,cumulative_transformation.position, cumulative_velocity); 
#endif
	return Unit::ResolveForces( trans, transmat);
}

static signed char  ComputeAutoGuarantee (GameUnit * un) {
  Cockpit * cp;
  int cpnum=-1;
  if ((cp =_Universe->isPlayerStarship (un))) {
    cpnum = cp-_Universe->AccessCockpit(0);
  }else {
    return Mission::AUTO_ON;
  }
  unsigned int i;
  for (i=0;i<active_missions.size();i++) {
    if(active_missions[i]->player_num==cpnum&&active_missions[i]->player_autopilot!=Mission::AUTO_NORMAL) {
      return active_missions[i]->player_autopilot;
    }
  }
  for (i=0;i<active_missions.size();i++) {
    if(active_missions[i]->global_autopilot!=Mission::AUTO_NORMAL) {
      return active_missions[i]->global_autopilot;
    }
  }
  return Mission::AUTO_NORMAL;
}

static float getAutoRSize (Unit * orig,Unit * un, bool ignore_friend=false) {
  static float friendly_autodist =  XMLSupport::parse_float (vs_config->getVariable ("physics","friendly_auto_radius","100"));
  static float neutral_autodist =  XMLSupport::parse_float (vs_config->getVariable ("physics","neutral_auto_radius","1000"));
  static float hostile_autodist =  XMLSupport::parse_float (vs_config->getVariable ("physics","hostile_auto_radius","8000"));
  static int upgradefaction = FactionUtil::GetFaction("upgrades");
  static int neutral = FactionUtil::GetFaction("neutral");

  if (un->isUnit()==PLANETPTR||(un->getFlightgroup()==orig->getFlightgroup()&&orig->getFlightgroup())) {
    //same flihgtgroup
    return orig->rSize();
  }
  if (un->faction==upgradefaction) {
    return ignore_friend?-FLT_MAX:(-orig->rSize()-un->rSize());
  }
  float rel=un->getRelation(orig);
  if (orig == un->Target())
	rel-=1.5;
  if (rel>.1||un->faction==neutral) {
	  return ignore_friend?-FLT_MAX:friendly_autodist;//min distance apart
  }else if (rel<-.1) {
    return hostile_autodist;
  }else {
	  return ignore_friend?-FLT_MAX:neutral_autodist;
  }
}

bool GameUnit::AutoPilotTo (Unit * target, bool ignore_friendlies) {
  signed char Guaranteed = ComputeAutoGuarantee (this);
  if (Guaranteed==Mission::AUTO_OFF) {
    return false;
  }
  static float autopilot_term_distance = XMLSupport::parse_float (vs_config->getVariable ("physics","auto_pilot_termination_distance","6000"));
//  static float autopilot_p_term_distance = XMLSupport::parse_float (vs_config->getVariable ("physics","auto_pilot_planet_termination_distance","60000"));
  if (SubUnit) {
    return false;//we can't auto here;
  }
  StarSystem * ss = activeStarSystem;
  if (ss==NULL) {
    ss = _Universe->activeStarSystem();
  }

  Unit * un=NULL;
  QVector start (Position());
  QVector end (target->LocalPosition());
  float totallength = (start-end).Magnitude();
  if (totallength>1) {
    //    float apt = (target->isUnit()==PLANETPTR&&target->GetDestinations().empty())?autopilot_p_term_distance:autopilot_term_distance;
	  float apt = (target->isUnit()==PLANETPTR)?(autopilot_term_distance+target->rSize()*UnitUtil::getPlanetRadiusPercent()):autopilot_term_distance;
    float percent = (getAutoRSize(this,this)+rSize()+target->rSize()+apt)/totallength;
    if (percent>1) {
      end=start;
    }else {
      end = start*percent+end*(1-percent);
    }
  }
  bool ok=true;
  if (Guaranteed==Mission::AUTO_NORMAL&&CloakVisible()>.5) {
    for (un_iter i=ss->getUnitList().createIterator(); (un=*i)!=NULL; ++i) {
      if (un->isUnit()!=NEBULAPTR) {
		if (un!=this&&un!=target) {
    	  if ((start-un->Position()).Magnitude()-getAutoRSize (this,this,ignore_friendlies)-rSize()-un->rSize()-getAutoRSize(this,un,ignore_friendlies)<=0) {
	    return false;
	  }
	  float intersection = un->querySphere (start,end,getAutoRSize (this,un,ignore_friendlies));
	  if (intersection>0) {
	    end = start+ (end-start)*intersection;
	    ok=false;
	  }
	 }
    }
   }
  }

  if (this!=target) {
    SetCurPosition(end);
    if (_Universe->isPlayerStarship (this)&&getFlightgroup()!=NULL) {
      Unit * other=NULL;
      for (un_iter ui=ss->getUnitList().createIterator(); NULL!=(other = *ui); ++ui) {
    	Flightgroup * ff = other->getFlightgroup();
		bool leadah=(ff==getFlightgroup());
		if (ff) {
			if (ff->leader.GetUnit()==this) {
				leadah=true;
		}
	}
	if (leadah) {
	  if (NULL==_Universe->isPlayerStarship (other)) {
	    //other->AutoPilotTo(this);
	    other->SetPosition(UniverseUtil::SafeEntrancePoint (LocalPosition(),other->rSize()*1.5));
	   }
	}
      }
    }
  }
  return ok;
}

