#include "bolt.h"
#include "gfxlib.h"
#include "gfx/mesh.h"
#include "gfxlib_struct.h"
#include "gfx/aux_texture.h"
#include "gfx/decalqueue.h"
#include "gfx/animation.h"
#include <vector>
#include <string>
#include <algorithm>
#include "unit.h"
#include "audiolib.h"
using std::vector;
using std::string;
namespace bolt_draw {
  DecalQueue boltdecals;
  static GFXVertexList * boltmesh=NULL;
  static vector <string> animationname;
  static vector <Animation *> animations;
  static vector <vector <Bolt *> > bolts;
  static vector <vector <Bolt *> > balls;
  static void CreateBoltMesh () {
    GFXVertex vtx[12];
    #define V(ii,xx,yy,zz,ss,tt) vtx[ii].x=xx;vtx[ii].y=yy;vtx[ii].z=zz;vtx[ii].i=0;vtx[ii].j=0;vtx[ii].k=1;vtx[ii].s=ss;vtx[ii].t=tt;
    V(0,0,0,-.875,0,.5);
    V(1,0,-1,0,.875,1);
    V(2,0,0,.125,1,.5);
    V(3,0,1,0,.875,0);
    V(4,0,0,-.875,0,.5);
    V(5,-1,0,0,.875,1);
    V(6,0,0,.125,1,.5);
    V(7,1,0,0,.875,0);
    V(8,1,0,0,.1875,0);
    V(9,0,1,0,.375,.1875);
    V(10,-1,0,0,.1875,.375);
    V(11,0,-1,0,0,.1875);
    boltmesh = new GFXVertexList (GFXQUAD,12,vtx,12,false);//not mutable;
  }

}
extern double interpolation_blend_factor;
using namespace bolt_draw;   
inline void BlendTrans (float * drawmat, const Vector & cur_position, const Vector & prev_position) {
    drawmat[12] = prev_position.i*(1-interpolation_blend_factor) + cur_position.i*interpolation_blend_factor;
    drawmat[13] = prev_position.j*(1-interpolation_blend_factor) + cur_position.j*interpolation_blend_factor;
    drawmat[14] = prev_position.k*(1-interpolation_blend_factor) + cur_position.k*interpolation_blend_factor;
    
}
void Bolt::Cleanup() {
  unsigned int i;
  if (boltmesh)
    delete boltmesh;
  boltmesh = NULL;
  for (i=0;i<animations.size();i++) {
    delete animations[i];
  }
  for (i=0;i<balls.size();i++) {
    for (unsigned int j=0;j<balls[i].size();j++) {
      delete balls[i][j];
    }
  }
  for (i=0;i<bolts.size();i++) {
    for (unsigned int j=0;j<bolts[i].size();j++) {
      delete bolts[i][j];
    }
  }
}

void Bolt::Draw () {
  GFXDisable (LIGHTING);
  GFXDisable (CULLFACE);
  GFXBlendMode (ONE,ONE);
  //  GFXDisable(DEPTHTEST);
  GFXDisable(DEPTHWRITE);
  GFXDisable(TEXTURE1);
  GFXEnable (TEXTURE0);

  vector <vector <Bolt *> >::iterator i;
  vector <Bolt *>::iterator j;
  vector <Animation *>::iterator k = animations.begin();
  for (i=balls.begin();i!=balls.end();i++,k++) {
    Animation * cur= *k;
    //Matrix result;
    //FIXME::MuST USE DRAWNOTRANSFORMNOW    cur->CalculateOrientation (result);
    for (j=i->begin();j!=i->end();j++) {//don't update time more than once
      BlendTrans ((*j)->drawmat,(*j)->cur_position,(*j)->prev_position);
      //result[12]=(*j)->drawmat[12];
      //result[13]=(*j)->drawmat[13];
      //result[13]=(*j)->drawmat[14];
      //            cur->SetPosition (result[12],result[13],result[14]);
      cur->SetDimensions ((*j)->radius,(*j)->radius);
      //      cur->DrawNow(result);
      GFXLoadMatrix (MODEL,(*j)->drawmat);
#ifdef PERBOLTSOUND
#ifdef PERFRAMESOUND
      if ((*j)->sound!=-1)
	AUDAdjustSound ((*j)->sound,Vector ((*j)->drawmat[12],(*j)->drawmat[13],(*j)->drawmat[14]),(*j)->ShipSpeed+(*j)->speed*Vector ((*j)->drawmat[8],(*j)->drawmat[9],(*j)->drawmat[10]));
#endif
#endif
      GFXColorf ((*j)->col);
      cur->DrawNoTransform();
    }
    cur->UpdateTime (GetElapsedTime());//update the time of the animation;
  }
  if (boltmesh) {
    boltmesh->LoadDrawState();
    boltmesh->BeginDrawState();
    int decal=0;
    for (i=bolts.begin();i!=bolts.end();decal++,i++) {
      Texture * dec = boltdecals.GetTexture(decal);
      if (dec) {
	dec->MakeActive();
	for (j=i->begin();j!=i->end();j++) {
	  BlendTrans ((*j)->drawmat,(*j)->cur_position,(*j)->prev_position);
	  GFXLoadMatrix (MODEL,(*j)->drawmat);
	  GFXColorf ((*j)->col);
	  boltmesh->Draw();
	}
      }
    }
    boltmesh->EndDrawState();
  }
  GFXEnable  (LIGHTING);
  GFXEnable  (CULLFACE);
  GFXBlendMode (ONE,ZERO);
  GFXEnable (DEPTHTEST);
  GFXEnable(DEPTHWRITE);
  GFXEnable (TEXTURE0);
  GFXColor4f(1,1,1,1);
}

using namespace bolt_draw;
Bolt::Bolt (const weapon_info & typ, const Matrix orientationpos,  const Vector & shipspeed, Unit * owner): col (typ.r,typ.g,typ.b,typ.a), cur_position (orientationpos[12],orientationpos[13],orientationpos[14]), ShipSpeed (shipspeed) {
  prev_position= cur_position;
  this->owner = owner;
  type = typ.type;
  damage = typ.Damage;
  longrange = typ.Longrange;
  radius = typ.Radius;
  speed = typ.Speed/(type==weapon_info::BOLT?typ.Length:typ.Radius);//will scale it by length long!
  range = typ.Range/(type==weapon_info::BOLT?typ.Length:typ.Radius);
  curdist = 0;
  CopyMatrix (drawmat,orientationpos);
  
  if (type==weapon_info::BOLT) {
    ScaleMatrix (drawmat,Vector (typ.Radius,typ.Radius,typ.Length));
    if (boltmesh==NULL) {
      CreateBoltMesh();
    }
    decal = boltdecals.AddTexture (typ.file.c_str(),MIPMAP);
    if (decal>=(int)bolts.size())
      bolts.push_back (vector <Bolt *>());
    bolts[decal].push_back (this);
  } else {
    ScaleMatrix (drawmat,Vector (typ.Radius,typ.Radius,typ.Radius));
    decal=-1;
    for (unsigned int i=0;i<animationname.size();i++) {
      if (typ.file==animationname[i]) {
	decal=i;
      }
    }
    if (decal==-1) {
      decal = animations.size();
      animationname.push_back (typ.file);
      animations.push_back (new Animation (typ.file.c_str(), true,.1,MIPMAP,false));//balls have their own orientation
      animations.back()->SetPosition (cur_position);
      balls.push_back (vector <Bolt *> ());
    }
    balls[decal].push_back (this);
  }
#ifdef PERBOLTSOUND
  sound = AUDCreateSound (typ.sound,false);
  AUDAdjustSound (sound,cur_position,shipspeed+speed*Vector (drawmat[8],drawmat[9],drawmat[10]));
#endif
}


bool Bolt::Update () {
  curdist +=speed*SIMULATION_ATOM;
  prev_position = cur_position;
  cur_position.i+=(ShipSpeed.i+drawmat[8]*speed)*SIMULATION_ATOM;//the r vector I believe;
  cur_position.j+=(ShipSpeed.j+drawmat[9]*speed)*SIMULATION_ATOM;
  cur_position.k+=(ShipSpeed.k+drawmat[10]*speed)*SIMULATION_ATOM;
  if (curdist>range) {
    delete this;//risky
    return false;
  }
#ifdef PERBOLTSOUND
  if (!AUDIsPlaying(sound)) {
    AUDDeleteSound (sound);
    sound=-1;
  } else {
#ifndef PERFRAMESOUND
    if ((*j)->sound!=-1)
      AUDAdjustSound ((*j)->sound,cur_position,(*j)->ShipSpeed+(*j)->speed*Vector ((*j)->drawmat[8],(*j)->drawmat[9],(*j)->drawmat[10]));
#endif

  }
#endif
  return true;
}

void Bolt::UpdatePhysics () {
  vector <vector <Bolt *> > *tmp = &bolts;
  for (int l=0;l<2;l++) {    
    for (vector <vector <Bolt *> >::iterator i= tmp->begin();i!=tmp->end();i++) {
      for (unsigned int j=0;j<i->size();j++) {
	if (!(*i)[j]->Update()) {
	  j--;//deleted itself
	} else if ((*i)[j]->Collide()) {
	  j--;//deleted itself!
	}
      }
    }
    tmp = &balls;
  }
}

bool Bolt::Collide (Unit * target) {
  if (target==owner)
    return false;
  Vector normal;
  float distance;
  if ((distance =target->queryBSP (prev_position,cur_position,normal))) {//ignore return
    Vector tmp = (cur_position-prev_position).Normalize();
    tmp = tmp*distance;
    distance = curdist/range;
    GFXColor coltmp (col);
    /*    coltmp.r+=.5;
    coltmp.g+=.5;
    coltmp.b+=.5;
    if (coltmp.r>1)coltmp.r=1;
    if (coltmp.g>1)coltmp.g=1;
    if (coltmp.b>1)coltmp.b=1;*/
    target->ApplyDamage (prev_position+tmp,normal, damage* ((1-distance)+distance*longrange),coltmp);
    return true;
  }
  return false;
}


Bolt::~Bolt () {
  vector <vector <Bolt *> > *target;
  if (type==weapon_info::BOLT) { 
    target = &bolts;
    boltdecals.DelTexture (decal);
  } else {
    target = &balls;
  }
  vector <Bolt *>::iterator tmp= std::find ((*target)[decal].begin(),(*target)[decal].end(),this); 
  if (tmp!=(*target)[decal].end()) {
    (*target)[decal].erase(tmp);
  }
#ifdef PERBOLTSOUND
  if (sound!=-1) {
    AUDDeleteSound (sound);
  }
#endif
}
