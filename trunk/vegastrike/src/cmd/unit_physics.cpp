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
#include "images.h"
#include "config_xml.h"
#include "vs_globals.h"
//#ifdef WIN32
#include "gfx/planetary_transform.h"
#include "gfx/cockpit.h"
float copysign (float x, float y) {
	if (y>0)
			return x;
	else
			return -x;
}
//#endif

// the rotation should be applied in world coordinates
void Unit:: Rotate (const Vector &axis)
{
	float theta = axis.Magnitude();
	float ootheta = 1/theta;
	float s = cos (theta * .5);
	Quaternion rot = Quaternion(s, axis * (sinf (theta*.5)*ootheta));
	if(theta < 0.0001) {
	  rot = identity_quaternion;
	}
	curr_physical_state.orientation *= rot;
	if (limits.limitmin>-1) {
	  Matrix mat;
	  curr_physical_state.orientation.to_matrix (mat);
	  if (limits.structurelimits.Dot (Vector (mat[8],mat[9],mat[10]))<limits.limitmin) {
	    curr_physical_state.orientation=prev_physical_state.orientation;
	  }
	}
}

void Unit:: FireEngines (const Vector &Direction/*unit vector... might default to "r"*/,
					float FuelSpeed,
					float FMass)
{
	mass -= FMass; //fuel is sent out
	fuel -= FMass;
	if (fuel <0)
	{
		
		FMass +=fuel;
		mass -= fuel;
		fuel = 0; //ha ha!
	}
	NetForce += Direction *(FuelSpeed *FMass/GetElapsedTime());
}
void Unit::ApplyForce(const Vector &Vforce) //applies a force for the whole gameturn upon the center of mass
{
	NetForce += Vforce;
}
void Unit::ApplyLocalForce(const Vector &Vforce) //applies a force for the whole gameturn upon the center of mass
{
	NetLocalForce += Vforce;
}
void Unit::Accelerate(const Vector &Vforce)
{
  NetForce += Vforce * mass;
}

void Unit::ApplyTorque (const Vector &Vforce, const Vector &Location)
{
  //Not completely correct
	NetForce += Vforce;
	NetTorque += Vforce.Cross (Location-curr_physical_state.position);
}
void Unit::ApplyLocalTorque (const Vector &Vforce, const Vector &Location)
{
	NetForce += Vforce;
	NetTorque += Vforce.Cross (Location);
}
void Unit::ApplyBalancedLocalTorque (const Vector &Vforce, const Vector &Location) //usually from thrusters remember if I have 2 balanced thrusters I should multiply their effect by 2 :)
{
	NetTorque += Vforce.Cross (Location);
	
}

void Unit::ApplyLocalTorque(const Vector &torque) {
  /*  Vector p,q,r;
  Vector tmp(ClampTorque(torque));
  GetOrientation (p,q,r);
  fprintf (stderr,"P: %f,%f,%f Q: %f,%f,%f",p.i,p.j,p.k,q.i,q.j,q.k);
  NetTorque+=tmp.i*p+tmp.j*q+tmp.k*r; 
  */
  NetLocalTorque+= ClampTorque(torque); 
}

Vector Unit::MaxTorque(const Vector &torque) {
  // torque is a normal
  return torque * (Vector(copysign(limits.pitch, torque.i), 
			  copysign(limits.yaw, torque.j),
			  copysign(limits.roll, torque.k)) * torque);
}

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
Vector Unit::ClampTorque (const Vector &amt1) {
  Vector Res=amt1;
  static float staticfuelclamp = XMLSupport::parse_float (vs_config->getVariable ("physics","NoFuelThrust",".9"));
  float fuelclamp=(fuel<=0)?staticfuelclamp:1;
  if (fabs(amt1.i)>fuelclamp*limits.pitch)
    Res.i=copysign(fuelclamp*limits.pitch,amt1.i);
  if (fabs(amt1.j)>fuelclamp*limits.yaw)
    Res.j=copysign(fuelclamp*limits.yaw,amt1.j);
  if (fabs(amt1.k)>fuelclamp*limits.roll)
    Res.k=copysign(fuelclamp*limits.roll,amt1.k);
  fuel-=Res.Magnitude()*SIMULATION_ATOM;
  return Res;
}
//    float max_speed;
//    float max_ab_speed;
//    float max_yaw;
//    float max_pitch;
//    float max_roll;

Vector Unit::ClampVelocity (const Vector & velocity, const bool afterburn) {
  static float staticfuelclamp = XMLSupport::parse_float (vs_config->getVariable ("physics","NoFuelThrust",".9"));
  static float staticabfuelclamp = XMLSupport::parse_float (vs_config->getVariable ("physics","NoFuelAfterburn",".1"));
  float fuelclamp=(fuel<=0)?staticfuelclamp:1;
  float abfuelclamp= (fuel<=0)?staticabfuelclamp:1;
  float limit = afterburn?(abfuelclamp*(computer.max_ab_speed-computer.max_speed)+(fuelclamp*computer.max_speed)):fuelclamp*computer.max_speed;
  float tmp = velocity.Magnitude();
  if (tmp>fabs(limit)) {
    return velocity * (limit/tmp);
  }
  return velocity;
}


Vector Unit::ClampAngVel (const Vector & velocity) {
  Vector res (velocity);
  if (fabs (res.i)>computer.max_pitch) {
    res.i = copysign (computer.max_pitch,res.i);
  }
  if (fabs (res.j)>computer.max_yaw) {
    res.j = copysign (computer.max_yaw,res.j);
  }
  if (fabs (res.k)>computer.max_roll) {
    res.k = copysign (computer.max_roll,res.k);
  }
  return res;
}


Vector Unit::MaxThrust(const Vector &amt1) {
  // amt1 is a normal
  return amt1 * (Vector(copysign(limits.lateral, amt1.i), 
	       copysign(limits.vertical, amt1.j),
	       amt1.k>0?limits.forward:-limits.retro) * amt1);
}
/* misnomer..this doesn't get the max value of each axis
Vector Unit::ClampThrust(const Vector &amt1){ 
  // Yes, this can be a lot faster with LUT
  Vector norm = amt1;
  norm.Normalize();
  Vector max = MaxThrust(norm);

  if(max.Magnitude() > amt1.Magnitude())
    return amt1;
  else 
    return max;
}
*/
//CMD_FLYBYWIRE depends on new version of Clampthrust... don't change without resolving it

Vector Unit::ClampThrust (const Vector &amt1, bool afterburn) {
  Vector Res=amt1;
  if (energy<1+afterburnenergy*SIMULATION_ATOM) {
    afterburn=false;
  }
  if (afterburn) {
    energy -=apply_float_to_short( afterburnenergy*SIMULATION_ATOM);
  }

  static float staticfuelclamp = XMLSupport::parse_float (vs_config->getVariable ("physics","NoFuelThrust",".4"));
  static float staticabfuelclamp = XMLSupport::parse_float (vs_config->getVariable ("physics","NoFuelAfterburn","0"));
  static float abfuelusage = XMLSupport::parse_float (vs_config->getVariable ("physics","AfterburnerFuelUsage","4"));
  float fuelclamp=(fuel<=0)?staticfuelclamp:1;
  float abfuelclamp= (fuel<=0)?staticabfuelclamp:1;
  if (fabs(amt1.i)>fabs(fuelclamp*limits.lateral))
    Res.i=copysign(fuelclamp*limits.lateral,amt1.i);
  if (fabs(amt1.j)>fabs(fuelclamp*limits.vertical))
    Res.j=copysign(fuelclamp*limits.vertical,amt1.j);
  float ablimit =       
    afterburn
    ?((limits.afterburn-limits.forward)*abfuelclamp+limits.forward*fuelclamp)
    :limits.forward;

  if (amt1.k>ablimit)
    Res.k=ablimit;
  if (amt1.k<-limits.retro)
    Res.k =-limits.retro;
  fuel-=(afterburn?abfuelusage:1)*Res.Magnitude();
  return Res;
}


void Unit::Thrust(const Vector &amt1,bool afterburn){
  Vector amt = ClampThrust(amt1,afterburn);
  ApplyLocalForce(amt);
  if (afterburn!=AUDIsPlaying (sound->engine)) {
    if (afterburn)
      AUDPlay (sound->engine,cumulative_transformation.position,cumulative_velocity,1);
    else
      //    if (Velocity.Magnitude()<computer.max_speed)
      AUDStopPlaying (sound->engine);
    
  }
}

void Unit::LateralThrust(float amt) {
  if(amt>1.0) amt = 1.0;
  if(amt<-1.0) amt = -1.0;
  ApplyLocalForce(amt*limits.lateral * Vector(1,0,0));
}

void Unit::VerticalThrust(float amt) {
  if(amt>1.0) amt = 1.0;
  if(amt<-1.0) amt = -1.0;
  ApplyLocalForce(amt*limits.vertical * Vector(0,1,0));
}

void Unit::LongitudinalThrust(float amt) {
  if(amt>1.0) amt = 1.0;
  if(amt<-1.0) amt = -1.0;
  ApplyLocalForce(amt*limits.forward * Vector(0,0,1));
}

void Unit::YawTorque(float amt) {
  if(amt>limits.yaw) amt = limits.yaw;
  else if(amt<-limits.yaw) amt = -limits.yaw;
  ApplyLocalTorque(amt * Vector(0,1,0));
}

void Unit::PitchTorque(float amt) {
  if(amt>limits.pitch) amt = limits.pitch;
  else if(amt<-limits.pitch) amt = -limits.pitch;
  ApplyLocalTorque(amt * Vector(1,0,0));
}

void Unit::RollTorque(float amt) {
  if(amt>limits.roll) amt = limits.roll;
  else if(amt<-limits.roll) amt = -limits.roll;
  ApplyLocalTorque(amt * Vector(0,0,1));
}

const float VELOCITY_MAX=1000;
void Unit::UpdatePhysics (const Transformation &trans, const Matrix transmat, const Vector & cum_vel,  bool lastframe, UnitCollection *uc) {
  if (docked&DOCKING_UNITS) {
    PerformDockingOperations();
  }

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
    if (planet) {
      if (!planet->dirty) {
	SetPlanetOrbitData (NULL);
      }else {
	planet->pps = planet->cps;
      }
    }
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
  curr_physical_state.position = curr_physical_state.position + QVector (Velocity*SIMULATION_ATOM);
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
  float * ctm;
  SetPlanetHackTransformation (ct,ctm);
  int i;
  if (lastframe) {
    char tmp=0;
    for (i=0;i<=nummesh;i++) {
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
  for (i=0;i<nummounts;i++) {
    if (mounts[i].type.type==weapon_info::BEAM) {
      if (mounts[i].ref.gun) {
	mounts[i].ref.gun->UpdatePhysics (cumulative_transformation, cumulative_transformation_matrix);
      }
    } else {
      mounts[i].ref.refire+=SIMULATION_ATOM;
    }
    if (mounts[i].processed==Mount::FIRED) {
      Transformation t1;
      Matrix m1;
      t1=prev_physical_state;//a hack that will not work on turrets
      t1.Compose (trans,transmat);
      t1.to_matrix (m1);
      mounts[i].PhysicsAlignedFire (t1,m1,cumulative_velocity,owner==NULL?this:owner,Target());
    }else if (mounts[i].processed==Mount::UNFIRED) {
      mounts[i].PhysicsAlignedUnfire();
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
void Unit::SetPlanetOrbitData (PlanetaryTransform *t) {
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
}
PlanetaryTransform * Unit::GetPlanetOrbit () const {
  if (planet==NULL)
    return NULL;
  return planet->trans;
}

bool Unit::jumpReactToCollision (Unit * smalle) {
  if (!GetDestinations().empty()) {//only allow big with small
    if ((smalle->GetJumpStatus().drive>=0||image->forcejump)) {
      smalle->DeactivateJumpDrive();
      Unit * jumppoint = this;
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
void Unit::reactToCollision(Unit * smalle, const Vector & biglocation, const Vector & bignormal, const Vector & smalllocation, const Vector & smallnormal,  float dist) {
  clsptr smltyp = smalle->isUnit();
  if (smltyp==ENHANCEMENTPTR||smltyp==MISSILEPTR) {
    smalle->reactToCollision (this,smalllocation,smallnormal,biglocation,bignormal,dist);
    return;
  }	       
  //don't bounce if you can Juuuuuuuuuuuuuump
  if (!jumpReactToCollision(smalle)) {
#ifdef NOBOUNCECOLLISION
#else
    smalle->ApplyForce (bignormal*.4*smalle->GetMass()*fabs(bignormal.Dot (((smalle->GetVelocity()-this->GetVelocity())/SIMULATION_ATOM))+fabs (dist)/(SIMULATION_ATOM*SIMULATION_ATOM)));
    this->ApplyForce (smallnormal*.4*(smalle->GetMass()*smalle->GetMass()/this->GetMass())*fabs(smallnormal.Dot ((smalle->GetVelocity()-this->GetVelocity()/SIMULATION_ATOM))+fabs (dist)/(SIMULATION_ATOM*SIMULATION_ATOM)));
    
    smalle->ApplyDamage (biglocation,bignormal,  .5*fabs(bignormal.Dot(smalle->GetVelocity()-this->GetVelocity()))*this->mass*SIMULATION_ATOM,smalle,GFXColor(1,1,1,1),NULL);
    this->ApplyDamage (smalllocation,smallnormal, .5*fabs(smallnormal.Dot(smalle->GetVelocity()-this->GetVelocity()))*smalle->mass*SIMULATION_ATOM,this,GFXColor(1,1,1,1),NULL);
#endif
    
  //each mesh with each mesh? naw that should be in one way collide
  }
}



void Unit::ResolveForces (const Transformation &trans, const Matrix transmat) {
  Vector p, q, r;
  GetOrientation(p,q,r);
  Vector temp = (InvTransformNormal(transmat,NetTorque)+NetLocalTorque.i*p+NetLocalTorque.j*q+NetLocalTorque.k *r)*SIMULATION_ATOM*(1.0/MomentOfInertia);
  if (FINITE(temp.i)&&FINITE (temp.j)&&FINITE(temp.k)) {
    AngularVelocity += temp;
  }
  temp = ((InvTransformNormal(transmat,NetForce) + NetLocalForce.i*p + NetLocalForce.j*q + NetLocalForce.k*r ) * SIMULATION_ATOM)/mass; //acceleration
  if (FINITE(temp.i)&&FINITE (temp.j)&&FINITE(temp.k)) {	//FIXME
    Velocity += temp;
  } 
#ifndef PERFRAMESOUND
  AUDAdjustSound (sound->engine,cumulative_transformation.position, cumulative_velocity); 
#endif
  NetForce = NetLocalForce = NetTorque = NetLocalTorque = Vector(0,0,0);

  /*
    if (fabs (Velocity.i)+fabs(Velocity.j)+fabs(Velocity.k)> co10) {
    float magvel = Velocity.Magnitude(); float y = (1-magvel*magvel*oocc);
    temp = temp * powf (y,1.5);
    }*/
}

void Unit::GetOrientation(Vector &p, Vector &q, Vector &r) const {
  Matrix m;
  curr_physical_state.to_matrix(m);
  p.i = m[0];
  p.j = m[1];
  p.k = m[2];

  q.i = m[4];
  q.j = m[5];
  q.k = m[6];

  r.i = m[8];
  r.j = m[9];
  r.k = m[10];
}

Vector Unit::UpCoordinateLevel (const Vector &v) const {
  float m[16];
  curr_physical_state.to_matrix(m);
#define M(A,B) m[B*4+A]
  return Vector(v.i*M(0,0)+v.j*M(1,0)+v.k*M(2,0),
		v.i*M(0,1)+v.j*M(1,1)+v.k*M(2,1),
		v.i*M(0,2)+v.j*M(1,2)+v.k*M(2,2));
#undef M
}

Vector Unit::ToLocalCoordinates(const Vector &v) const {
  //Matrix m;
  //062201: not a cumulative transformation...in prev unit space  curr_physical_state.to_matrix(m);
  
#define M(A,B) cumulative_transformation_matrix[B*4+A]
  return Vector(v.i*M(0,0)+v.j*M(1,0)+v.k*M(2,0),
		v.i*M(0,1)+v.j*M(1,1)+v.k*M(2,1),
		v.i*M(0,2)+v.j*M(1,2)+v.k*M(2,2));
#undef M
}

Vector Unit::ToWorldCoordinates(const Vector &v) const {
  return TransformNormal(cumulative_transformation_matrix,v); 
#undef M

}


