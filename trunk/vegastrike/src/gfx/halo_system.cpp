#include "vec.h"
#include "matrix.h"
#include "halo_system.h"
#include "universe.h"
#include <stdlib.h>
#include <stdio.h>
#include "vegastrike.h"
#include "mesh.h"
#include "vs_globals.h"
#include "xml_support.h"
#include "config_xml.h"
#include "gfx/particle.h"
#include "lin_time.h"
static void DoParticles (QVector pos, float percent, const Vector & velocity, float radial_size,int faction) {
  percent = 1-percent;
  int i=rand();
  static float scale = XMLSupport::parse_float (vs_config->getVariable("graphics",
								       "sparklescale",
								       
								       "8"));
  static float sspeed = XMLSupport::parse_float (vs_config->getVariable("graphics",
								       "sparklespeed",
								       
								       ".5"));
  static float flare = XMLSupport::parse_float (vs_config->getVariable("graphics",
								       "sparkleflare",
								       
								       ".5"));
  static float spread = XMLSupport::parse_float (vs_config->getVariable("graphics",
								       "sparklespread",
								       
								       "0"));
  if (i<(RAND_MAX*percent)*(GetElapsedTime()*scale)) {
      ParticlePoint pp;
      float r1 = rand()/((float)RAND_MAX*.5)-1;
      float r2 = rand()/((float)RAND_MAX*.5)-1;      
      QVector rand(r1,r2,0);
      pp.loc = pos+rand*radial_size*flare;
      const float * col = _Universe->GetSparkColor(faction);
      pp.col.i=col[0];
      pp.col.j=col[1];
      pp.col.k=col[2];
      particleTrail.AddParticle(pp,rand*velocity.Magnitude()*spread+velocity*sspeed);
    }
}
  




HaloSystem::HaloSystem() {
  VSCONSTRUCT2('h')
  mesh=NULL;
}

MyIndHalo::MyIndHalo(const QVector & loc, const Vector & size) {
    this->loc = loc;
    this->size=size;
}
unsigned int HaloSystem::AddHalo (const char * filename, const QVector & loc, const Vector &size, const GFXColor & col) {
  if (mesh==NULL) {
    mesh = new Mesh ((string (filename)+".xmesh").c_str(), 1,_Universe->GetFaction("neutral"),NULL);
  }
  static float engine_scale = XMLSupport::parse_float (vs_config->getVariable ("graphics","engine_radii_scale",".4"));
  static float engine_length = XMLSupport::parse_float (vs_config->getVariable ("graphics","engine_length_scale","1.25"));

  halo.push_back (MyIndHalo (loc, Vector (size.i*engine_scale,size.j*engine_scale,size.k)));
  return halo.size()-1;
}
using std::vector;
void HaloSystem::SetSize (unsigned int which, const Vector &size) {
  halo[which].size = size;
}
void HaloSystem::SetPosition (unsigned int which, const QVector &loc) {
  halo[which].loc = loc;
}
void HaloSystem::Draw(const Matrix & trans, const Vector &scale, short halo_alpha, float nebdist, float hullpercent, const Vector & velocity, int faction) {
  if (scale.k>0) {
    vector<MyIndHalo>::iterator i = halo.begin();
    for (;i!=halo.end();++i) {
      
      Matrix m = trans;
      ScaleMatrix (m,Vector (scale.i*i->size.i,scale.j*i->size.j,scale.k*i->size.k));
      m.p = Transform (trans,i->loc);
      mesh->Draw(50000000000000.0,m,1,halo_alpha,nebdist);    
      if (hullpercent<.99) {
	DoParticles(m.p,hullpercent,velocity,mesh->rSize()*scale.i,faction);
      }
    }
  }
}
HaloSystem::~HaloSystem() {
  VSDESTRUCT2
  if (mesh) {
    delete mesh;
  }
}
