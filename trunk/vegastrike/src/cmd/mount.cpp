#include "unit_generic.h"
#include "beam.h"
#include "bolt.h"
#include "weapon_xml.h"
#include "audiolib.h"
#include "unit_factory.h"
#include "ai/order.h"
#include "ai/fireall.h"
#include "ai/script.h"
#include "ai/navigation.h"
#include "ai/flybywire.h"
#include "configxml.h"
#include "gfx/cockpit_generic.h"
#include "force_feedback.h"

void Mount::SwapMounts(Mount * other) {
	  short thisvol = volume;
	  short othervol = other->volume;
	  //short othersize = other->size;
	  short thissize = size;
	  Mount mnt = *this;
	  this->size=thissize;
	  *this=*other;
	  *other=mnt;
	  volume=thisvol;

	  other->volume=othervol;//volumes stay the same even if you swap out
	  Vector v =this->GetMountLocation();
	  Quaternion q = this->GetMountOrientation();
	  this->SetMountPosition(other->GetMountLocation());
	  this->SetMountOrientation(other->GetMountOrientation());
	  other->SetMountPosition (v);
	  other->SetMountOrientation (q);  
}
void Mount::ReplaceSound () {
  sound = AUDCreateSound (sound,type->type!=weapon_info::PROJECTILE);//copy constructor basically
}

void Mount::PhysicsAlignedUnfire() {
  //Stop Playing SOund?? No, that's done in the beam, must not be aligned
  if (processed==UNFIRED) {
  if (AUDIsPlaying (sound))
    AUDStopPlaying (sound);
    processed=PROCESSED;
  }
}

void Mount::UnFire () {
  processed = UNFIRED;
  if (status!=ACTIVE||ref.gun==NULL||type->type!=weapon_info::BEAM)
    return ;
  //  AUDStopPlaying (sound);
  ref.gun->Destabilize();
}

extern void AdjustMatrix (Matrix &mat, Unit * target, float speed, bool lead, float cone);
void AdjustMatrixToTrackTarget (Matrix &mat,Unit * target, float speed, bool lead, float cone) {
  AdjustMatrix (mat,target,speed,lead,cone);
}
bool Mount::PhysicsAlignedFire(const Transformation &Cumulative, const Matrix & m, const Vector & velocity, Unit * owner, Unit *target, signed char autotrack, float trackingcone) {
  if (time_to_lock>0) {
    target=NULL;
  }
  time_to_lock = type->LockTime;
  if (processed==FIRED) {
    processed = PROCESSED;
    Unit * temp;
    Transformation tmp (orient,pos.Cast());
    tmp.Compose (Cumulative,m);
    Matrix mat;
    tmp.to_matrix (mat);
    mat.p = Transform(mat,(type->offset+Vector(0,0,zscale)).Cast());
    if (autotrack&&NULL!=target) {
      AdjustMatrix (mat,target,type->Speed,autotrack>=2,trackingcone);
    }
    switch (type->type) {
    case weapon_info::BEAM:
      break;
    case weapon_info::BOLT:
      new Bolt (*type, mat, velocity, owner);//FIXME turrets! Velocity      
      break;
    case weapon_info::BALL:
      new Bolt (*type,mat, velocity,  owner);//FIXME:turrets won't work      
      break;
    case weapon_info::PROJECTILE:
      temp = UnitFactory::createMissile (type->file.c_str(),owner->faction,"",type->Damage,type->PhaseDamage,type->Range/type->Speed,type->Radius,type->RadialSpeed,type->PulseSpeed/*detonation_radius*/);
      if (target&&target!=owner) {
	temp->Target (target);
	temp->EnqueueAI (new AIScript ((type->file+".xai").c_str()));
	temp->EnqueueAI (new Orders::FireAllYouGot);
      } else {
	temp->EnqueueAI (new Orders::MatchLinearVelocity(Vector (0,0,100000),true,false));
	temp->EnqueueAI (new Orders::FireAllYouGot);
      }
      temp->SetOwner (owner);
      temp->Velocity = velocity;
      temp->curr_physical_state = temp->prev_physical_state= temp->cumulative_transformation = tmp;
      CopyMatrix (temp->cumulative_transformation_matrix,m);
      _Universe->activeStarSystem()->AddUnit(temp);
      break;
    }
    static bool use_separate_sound=XMLSupport::parse_bool (vs_config->getVariable ("audio","high_quality_weapon","true"));
    if ((((!use_separate_sound)||type->type==weapon_info::BEAM)||(!_Universe->isPlayerStarship(owner)))&&(type->type!=weapon_info::PROJECTILE)) {
    if (!AUDIsPlaying (sound)) {
      AUDPlay (sound,tmp.position,velocity,1);
    }else {
      AUDAdjustSound(sound,tmp.position,velocity);
    }
    }else {
      int snd =AUDCreateSound(sound,false);
      AUDAdjustSound(snd,tmp.position,velocity);
      AUDStartPlaying (snd);
      AUDDeleteSound(snd);
    }
    return true;
  }
  return false;
}

bool Mount::Fire (Unit * owner, bool Missile, bool listen_to_owner) {
  if (ammo==0) {
    processed=UNFIRED;
  }
  if (processed==FIRED||status!=ACTIVE||(Missile!=(type->type==weapon_info::PROJECTILE))||ammo==0)
    return false;
  if (type->type==weapon_info::BEAM) {
    if (ref.gun==NULL) {
      if (ammo>0)
	ammo--;//do we want beams to have amo
      processed=FIRED;
      ref.gun = new Beam (Transformation(orient,pos.Cast()),*type,owner,sound);
      ref.gun->ListenToOwner(listen_to_owner);
      return true;
    } else {
      if (ref.gun->Ready()) {
	if (ammo>0)
	  ammo--;//ditto about beams ahving ammo
	processed=FIRED;
	ref.gun->Init (Transformation(orient,pos.Cast()),*type,owner);
	return true;
      } else 
	return true;//can't fire an active beam
    }
  }else { 
    if (ref.refire>type->Refire) {
      ref.refire =0;
      if (ammo>0)
	ammo--;
      processed=FIRED;	

      if(owner==_Universe->AccessCockpit()->GetParent()){
	//printf("player has fired a bolt\n");
	forcefeedback->playLaser();
      };

      return true;
    }
  }
  return false;
}
Mount::Mount() {
	static weapon_info wi(weapon_info::BEAM);
	type=&wi; size=weapon_info::NOWEAP;
	ammo=-1;
	status= UNCHOSEN;
	processed=Mount::PROCESSED;
	sound=-1;
	static float xyscalestat=XMLSupport::parse_float (vs_config->getVariable ("graphics","weapon_xyscale","1"));

	static float zscalestat=XMLSupport::parse_float (vs_config->getVariable ("graphics","weapon_zscale","1"));
	xyscale=xyscalestat;	
	zscale=zscalestat;
}
Mount::Mount(const string& filename, short am,short vol, float xyscale, float zscale){
  static weapon_info wi(weapon_info::BEAM);
  size = weapon_info::NOWEAP;
  static float xyscalestat=XMLSupport::parse_float (vs_config->getVariable ("graphics","weapon_xyscale","1"));
  
  static float zscalestat=XMLSupport::parse_float (vs_config->getVariable ("graphics","weapon_zscale","1"));  
  if (xyscale==-1)
	  xyscale=xyscalestat;
  if (zscale==-1)
	  zscale=zscalestat;
  this->zscale=zscale;
    this->xyscale=xyscale;
  ammo = am;
  sound = -1;
  type = &wi;
  this->volume=vol;
  ref.gun = NULL;
  status=(UNCHOSEN);
  processed=Mount::PROCESSED;
  weapon_info * temp = getTemplate (filename);  
  if (temp==NULL) {
    status=UNCHOSEN;
    time_to_lock=0;
  }else {
    type = temp;
    status=ACTIVE;
    time_to_lock = temp->LockTime;
  }
}
