#include "building.h"
#include "cont_terrain.h"
#include "gfx/planetary_transform.h"
Building::Building (ContinuousTerrain * parent, bool vehicle, const char * filename, bool SubUnit, int faction, const string &modifications, Flightgroup * fg):Unit (filename,SubUnit,faction,modifications,fg) {
  this->vehicle = vehicle;
  continuous=true;
  this->parent.plane = parent;
}

Building::Building (Terrain * parent, bool vehicle, const char *filename, bool SubUnit, int faction, const string &modifications, Flightgroup * fg):Unit (filename,SubUnit,faction,modifications,fg) {
  this->vehicle = vehicle;
  continuous=false;
  this->parent.terrain = parent;
}



void Building::UpdatePhysics (const Transformation &trans, const Matrix transmat, const Vector & cum_vel,  bool lastframe, UnitCollection *uc){
  if (GetPlanetOrbit()) {
    SetPlanetOrbitData(GetPlanetOrbit());//makes it dirty
  }
  Unit::UpdatePhysics (trans,transmat,cum_vel,lastframe,uc);
  Vector tmp (LocalPosition());
  Vector p,q,r;
  GetOrientation (p,q,r);
  if (continuous) {
    if (!GetPlanetOrbit()) {
      tmp = parent.plane->GetGroundPos (tmp,p);
    }else {
      tmp = GetPlanetOrbit()->Transform(parent.plane->GetGroundPosIdentTrans (GetPlanetOrbit()->InvTransform(tmp),p));
    }
  } else {
    parent.terrain->GetGroundPos (tmp,p,(float)0,(float)0);

  }
  if (vehicle) {
    Normalize(p);
    //    float k = p.Dot (q);
    Vector tmp1;
#if 0
    if (k<=0) {
      tmp1 = Vector (0,0,1);
      if (k=tmp1.Magnitude()){
	tmp1*=800./k;
      }
    } else 
#endif
      {
      tmp1 = 200*q.Cross (p);
    }
    NetLocalTorque+=((tmp1-tmp1*(tmp1.Dot (GetAngularVelocity())/tmp1.Dot(tmp1))))*1./GetMass();
  }
  SetCurPosition (tmp);

}
