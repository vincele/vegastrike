#ifndef _STAR_H_
#define _STAR_H_
#include "gfxlib.h"

#include "gfxlib_struct.h"
const int STARnumvlist = 27;
#include <string>

class StarVlist {
protected:
  float spread;
  Vector newcamr;
  Vector newcamq;
  Vector camr;
  Vector camq;
  double lasttime;
public:
  StarVlist(float spread);
  void UpdateGraphics();
  virtual ~StarVlist(){}
  virtual bool BeginDrawState(const QVector &center, const Vector & vel,const Vector & angular_vel,bool rotate, bool yawpitch, int whichTexture){return false;}
  virtual void Draw(bool,int whichtex){}
  virtual void EndDrawState(bool, int whichtex){}
  virtual int NumTextures() {return 1;}
  void DrawAll(const QVector &center, const Vector&vel, const Vector&ang_vel, bool rot,bool yawpitch) {
    int LC=0,LN=NumTextures();
    for (LC=0;LC<LN;++LC) {
      bool tmp=this->BeginDrawState(center,vel,ang_vel,rot,yawpitch,LC);
      this->Draw(tmp,LC);
      this->EndDrawState(tmp,LC);
    }
  }
};

class PointStarVlist:public StarVlist{
  GFXVertexList * vlist;
  GFXVertexList * nonstretchvlist;
public:
  PointStarVlist (int num, float spread,const std::string &our_system_name);
  ~PointStarVlist();
  bool BeginDrawState(const QVector &center, const Vector & vel,const Vector & angular_vel,bool rotate, bool yawpitch, int whichTexture);
  void Draw(bool, int whichTexture);
  void EndDrawState(bool,int whichTexture);
};

#define NUM_ACTIVE_ANIMATIONS 8
class SpriteStarVlist:public StarVlist{
  GFXVertexList * vlist[NUM_ACTIVE_ANIMATIONS];
  class Texture *decal[NUM_ACTIVE_ANIMATIONS];
public:
  SpriteStarVlist (int num, float spread, std::string our_system_name, std::string texturename,float size);
  ~SpriteStarVlist();
  int NumTextures();
  bool BeginDrawState(const QVector &center, const Vector & vel,const Vector & angular_vel,bool rotate, bool yawpitch, int whichTexture);
  void Draw(bool, int whichTexture);
  void EndDrawState(bool, int whichTexture);
};

class Stars {
private:
  StarVlist *vlist;
  QVector pos[STARnumvlist];
  float spread;
  bool blend;
  bool fade;
  void ResetPosition(const QVector & cent);
  void UpdatePosition(const QVector & cp);
public:
  Stars (int num, float spread);
  void SetBlend(bool blendit, bool fadeit);
  void Draw();
  ~Stars();
};
#endif
