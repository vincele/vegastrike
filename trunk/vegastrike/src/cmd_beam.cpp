#include "physics.h"
#include <vector>
#include "cmd_beam.h"
#include "cmd_unit.h"
#include "cmd_collide.h"
//using std::vector;


static vector <Texture *> BeamDecal;
static vector <int> DecalRef;
static vector <vector <DrawContext> > beamdrawqueue;
extern double interpolation_blend_factor;
Beam::Beam (const Transformation & trans, const weapon_info & clne, void * own) :vlist(NULL), Col(clne.r,clne.g,clne.b,clne.a){
  string texname (clne.file);
  Texture * tmpDecal = Texture::Exists(texname);
  unsigned int i=0;
  int nullio=-1;
  if (tmpDecal) {
    for (;i<BeamDecal.size();i++) {
      if (BeamDecal[i]==tmpDecal) {
	decal = i;
	DecalRef[i]++;
	break;
      }
      if (!BeamDecal[i]&&DecalRef[i]==0)
	nullio=i;
    }
  }
  if (!tmpDecal||i==BeamDecal.size()) { //make sure we have our own to delete upon refcount =0
    if (nullio!=-1) {
      decal = nullio;
      BeamDecal[nullio]=new Texture (texname.c_str(),0,TRILINEAR);
      DecalRef[nullio]=1;
    } else{
      decal = BeamDecal.size();
      BeamDecal.push_back(new Texture (texname.c_str(),0,TRILINEAR));
      beamdrawqueue.push_back (vector<DrawContext>());
      DecalRef.push_back (1);
    }
  }
  Init(trans,clne,own);
}

void Beam::SetPosition (float x,float y, float z) {
  local_transformation.position = Vector (x,y,z);
}
void Beam::SetPosition (const Vector &k) {
  local_transformation.position = k;
}
void Beam::SetOrientation(const Vector &p, const Vector &q, const Vector &r)
{	
  local_transformation.orientation = Quaternion::from_vectors(p,q,r);
}

Vector &Beam::Position()
{
	return local_transformation.position;
}


void Beam::Init (const Transformation & trans, const weapon_info &cln , void * own)  {
  //Matrix m;
  if (vlist)
    delete vlist;
  local_transformation = trans;//location on ship
  //  cumalative_transformation =trans; 
  //  trans.to_matrix (cumalative_transformation_matrix);
  speed = cln.Speed;
  texturespeed = cln.PulseSpeed;
  range = cln.Range;
  radialspeed = cln.RadialSpeed;
  thickness = cln.Radius;
  stability = cln.Stability;
  rangepenalty=cln.Longrange;
  damagerate = cln.Damage;
  refiretime = 0;
  refire = cln.Refire;
  Col.r = cln.r;
  Col.g = cln.g;
  Col.b = cln.b;
  Col.a=cln.a;
  impact= ALIVE;
  owner = own;
  numframes=0;

  lastlength=0;
  curlength = SIMULATION_ATOM*speed;
  lastthick=0;
  curthick = SIMULATION_ATOM*radialspeed;
  GFXVertex beam[32];

  GFXColor * calah = (GFXColor *)malloc (sizeof(GFXColor)*32);

  calah[0].r=0;calah[0].g=0;calah[0].b=0;calah[0].a=0;
  calah[1].r=calah[1].g=calah[1].b=calah[1].a=0;
  memcpy (&calah[2],&Col,sizeof (GFXColor));
  memcpy (&calah[3],&Col,sizeof (GFXColor));
  memcpy (&calah[4],&Col,sizeof (GFXColor));
  memcpy (&calah[5],&Col,sizeof (GFXColor));
  calah[6].r=calah[6].g=calah[6].b=calah[6].a=0;
  calah[7].r=calah[7].g=calah[7].b=calah[7].a=0;


  calah[8].r=calah[8].g=calah[8].b=calah[8].a=0;
  calah[9].r=calah[9].g=calah[9].b=calah[9].a=0;

  calah[10].r=calah[10].g=calah[10].b=calah[10].a=0;
  calah[11].r=Col.r;calah[11].g=Col.g;calah[11].b=Col.b;calah[11].a=Col.a;

  calah[12].r=calah[12].g=calah[12].b=calah[12].a=0;
  calah[13].r=calah[13].g=calah[13].b=calah[13].a=0;
  calah[14].r=calah[14].g=calah[14].b=calah[14].a=0;

  calah[15].r=Col.r;calah[15].g=Col.g;calah[15].b=Col.b;calah[15].a=Col.a;
  //since mode is ONE,ONE
  calah[2].r*=Col.a;calah[2].g*=Col.a;calah[2].b*=Col.a;
  calah[3].r*=Col.a;calah[3].g*=Col.a;calah[3].b*=Col.a;
  calah[4].r*=Col.a;calah[4].g*=Col.a;calah[4].b*=Col.a;
  calah[5].r*=Col.a;calah[5].g*=Col.a;calah[5].b*=Col.a;
  calah[11].r*=Col.a;calah[11].g*=Col.a;calah[11].b*=Col.a;
  calah[15].r*=Col.a;calah[15].g*=Col.a;calah[15].b*=Col.a;


  memcpy (&calah[16],&calah[0],sizeof(GFXColor)*16);    
  vlist = new GFXVertexList (GFXQUAD,32,beam,calah,true);//mutable color contained list
  free (calah);
}

Beam::~Beam () {
  delete vlist;
  DecalRef[decal]--;
  if (DecalRef[decal]<=0) {
    delete BeamDecal[decal];
    BeamDecal[decal]=NULL;
  }

}
void Beam::RecalculateVertices() {
  GFXVertex beam[32];
  
  float leftex = -texturespeed*(numframes*SIMULATION_ATOM+interpolation_blend_factor*SIMULATION_ATOM);
  float righttex = leftex+curlength/curthick;//how long compared to how wide!
  float len = (impact==ALIVE)?(curlength!=range?curlength - speed*SIMULATION_ATOM*(1-interpolation_blend_factor):range):curlength+thickness;
  float fadelen = (impact==ALIVE)?len*.85:(range*.85>curlength?curlength:range*.85);
  float fadetex = leftex + (righttex-leftex)*.85;
  float thick = curthick!=thickness?curthick-radialspeed*SIMULATION_ATOM*(1-interpolation_blend_factor):thickness;
  int a=0;
#define V(xx,yy,zz,ss,tt) { beam[a].x = xx; beam[a].y = yy; beam[a].z = zz; beam[a].s=ss; beam[a].t=tt;a++; }

  V(0,thick,0,leftex,1);
  V(0,thick,fadelen,fadetex,1);
  V(0,0,fadelen,fadetex,.5);
  V(0,0,0,leftex,.5);
  V(0,0,0,leftex,.5);
  V(0,0,fadelen,fadetex,.5);
  V(0,-thick,fadelen,fadetex,0);
  V(0,-thick,0,leftex,0);

  V(0,thick,fadelen,fadetex,1);
  V(0,thick,len,righttex,1);
  V(0,0,len,righttex,.5);
  V(0,0,fadelen,fadetex,.5);

  V(0,-thick,fadelen,fadetex,0);
  V(0,-thick,len,righttex,0);
  V(0,0,len,righttex,.5);
  V(0,0,fadelen,fadetex,.5);




#undef V//reverse the notation for the rest of the identical vertices
#define QV(yy,xx,zz,ss,tt) { beam[a].x = xx; beam[a].y = yy; beam[a].z = zz; beam[a].s=ss; beam[a].t=tt;a++; }
  QV(0,thick,0,leftex,1);
  QV(0,thick,fadelen,fadetex,1);
  QV(0,0,fadelen,fadetex,.5);
  QV(0,0,0,leftex,.5);
  QV(0,0,0,leftex,.5);
  QV(0,0,fadelen,fadetex,.5);
  QV(0,-thick,fadelen,fadetex,0);
  QV(0,-thick,0,leftex,0);

  QV(0,thick,fadelen,fadetex,1);
  QV(0,thick,len,righttex,1);
  QV(0,0,len,righttex,.5);
  QV(0,0,fadelen,fadetex,.5);
  QV(0,-thick,fadelen,fadetex,0);
  QV(0,-thick,len,righttex,0);
  QV(0,0,len,righttex,.5);
  QV(0,0,fadelen,fadetex,.5);



#undef QV
  vlist->Mutate(0,beam,32);
}


void Beam::Draw (const Transformation &trans, const float* m) {//hope that the correct transformation is on teh stack
  if (curthick==0)
    return;
  Matrix cumulative_transformation_matrix;
  local_transformation.to_matrix(cumulative_transformation_matrix);
  Transformation cumulative_transformation = local_transformation;
  cumulative_transformation.Compose(trans, m);
  cumulative_transformation.to_matrix(cumulative_transformation_matrix);
  RecalculateVertices();
  beamdrawqueue[decal].push_back(DrawContext (cumulative_transformation_matrix,vlist));
}

void Beam::ProcessDrawQueue() {
    GFXDisable (LIGHTING);
    GFXDisable (CULLFACE);//don't want lighting on this baby
    GFXDisable (DEPTHWRITE);
    GFXPushBlendMode();
    GFXBlendMode(ONE,ONE);

  GFXEnable (TEXTURE0);
  GFXDisable (TEXTURE1);
  DrawContext c;
  for (unsigned int decal = 0;decal < beamdrawqueue.size();decal++) {	
    BeamDecal[decal]->MakeActive();
    if (beamdrawqueue[decal].size()) {
      beamdrawqueue[decal].back().vlist->LoadDrawState();//loads clarity+color
    }
    while (beamdrawqueue[decal].size()) {
      c= beamdrawqueue[decal].back();
      beamdrawqueue[decal].pop_back();
      GFXLoadMatrix (MODEL, c.m);
      c.vlist->Draw();
    }
  }
  //  GFXEnable (TEXTURE1);
  GFXEnable (DEPTHWRITE);
  GFXEnable (CULLFACE);
  GFXDisable (LIGHTING);
  GFXPopBlendMode();
}

void Beam::UpdatePhysics(const Transformation &trans, const Matrix m) {
  curlength += SIMULATION_ATOM*speed;
  if (curlength<0)
    curlength=0;
  if (curlength > range)
    curlength=range;
  if (curthick ==0) {
    refiretime +=SIMULATION_ATOM;
    return;
  }
  numframes++;
  Matrix cumulative_transformation_matrix;
  Transformation cumulative_transformation = local_transformation;
  cumulative_transformation.Compose(trans, m);
  cumulative_transformation.to_matrix(cumulative_transformation_matrix);
  //to help check for crashing.
  
  if (stability&&numframes*SIMULATION_ATOM>stability)
    impact|=UNSTABLE;
  
  curthick+=(impact&UNSTABLE)?-radialspeed*SIMULATION_ATOM:radialspeed*SIMULATION_ATOM;
  if (curthick > thickness)
    curthick = thickness;
  if (curthick <0)
    curthick =0;//die die die
  center = cumulative_transformation.position;
  direction = TransformNormal (cumulative_transformation_matrix,Vector(0,0,1));
  Vector tmpvec = center + direction*curlength;

  AddCollideQueue (LineCollide (this,LineCollide::BEAM,center.Min(tmpvec),center.Max(tmpvec)),false);
				       
  //Check if collide...that'll change max beam length REAL quick
}
