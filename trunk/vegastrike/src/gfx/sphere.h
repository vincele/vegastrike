#ifndef _GFX_SPHERE_H_
#define _GFX_SPHERE_H_

#include "mesh.h"
#include "quaternion.h"
#ifndef M_PI
#define M_PI 3.1415926536F
#endif

class SphereMesh : public Mesh {
  //no local vars allowed
 protected:
  virtual float GetT ( float rho, float rho_min, float rho_max);
  virtual float GetS (float theta,  float theta_min, float theta_max);
  virtual Mesh * AllocNewMeshesEachInSizeofMeshSpace (int num) {return new SphereMesh[num];}
  void InitSphere (float radius, int stacks, int slices, const char *texture, const char *alpha=NULL, bool insideout=false, const BLENDFUNC a=ONE, const BLENDFUNC b=ZERO, bool envMap=false, float rho_min=0.0, float rho_max=M_PI, float theta_min=0.0, float theta_max=2*M_PI, FILTER mipmap=MIPMAP);
 public:
  SphereMesh () :Mesh(){}
  SphereMesh(float radius, int stacks, int slices, const char *texture, const char *alpha=NULL, bool insideout=false, const BLENDFUNC a=ONE, const BLENDFUNC b=ZERO, bool envMap=false, float rho_min=0.0, float rho_max=M_PI, float theta_min=0.0, float theta_max=2*M_PI, FILTER mipmap=MIPMAP){
    InitSphere (radius,stacks,slices,texture,alpha,insideout,a,b,envMap,rho_min,rho_max,theta_min,theta_max,mipmap);
  }
  void Draw(float lod, bool centered =false, const Matrix &m=identity_matrix);
  virtual void ProcessDrawQueue(int which);
};
class CityLights : public SphereMesh {
  //no local vars allowed
  //these VARS BELOW ARE STATIC...change it and DIE
  static float wrapx;
  static float wrapy;
 protected:
  virtual float GetT ( float rho, float rho_min, float rho_max);
  virtual float GetS (float theta,  float theta_min, float theta_max);
  virtual Mesh * AllocNewMeshesEachInSizeofMeshSpace (int num ) {return new CityLights[num];}
 public:
  CityLights () : SphereMesh () {}
  CityLights (float radius, int stacks, int slices, const char *texture, const char *alpha=NULL, bool insideout=false, const BLENDFUNC a=ONE, const BLENDFUNC b=ZERO, bool envMap=false, float rho_min=0.0, float rho_max=M_PI, float theta_min=0.0, float theta_max=2*M_PI);
  virtual void ProcessDrawQueue(int which);

};
#endif

