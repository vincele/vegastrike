#ifndef _STAR_H_
#define _STAR_H_
#include "gfxlib.h"

#include "gfxlib_struct.h"
const int STARnumvlist = 27;


class StarVlist {
	GFXVertexList * vlist;
	float spread;
	Vector newcamr;
	Vector newcamq;
    Vector camr;
	Vector camq;
	double lasttime;
public:
	StarVlist (int num, float spread,bool use_milky_way);
	~StarVlist();
	void BeginDrawState(const QVector &center, const Vector & vel,bool rotate, bool yawpitch);
	void Draw();
	void EndDrawState();
	void UpdateGraphics();
};
class Stars {
private:
  StarVlist vlist;
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
  ~Stars() {}
};
#endif
