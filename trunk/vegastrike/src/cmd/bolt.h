#ifndef _CMD_BOLT_H_
#define _CMD_BOLT_H_
#include "gfxlib.h"
#include "gfxlib_struct.h"
#include "weapon_xml.h"
#include "gfx/matrix.h"
#include "gfx/quaternion.h"

class Animation;
class Unit;
class Bolt {
  GFXColor col;
  Matrix drawmat;
  Vector cur_position;
  Vector prev_position;//beams don't change heading.
  weapon_info::WEAPON_TYPE type;//beam or bolt;
  int decal;//which image it uses
  Unit *owner;
  float damage, curdist,longrange;
  float speed, range;
  bool Collide (Unit * target);
 public:
  Bolt(const weapon_info &type, const Matrix orientationpos, Unit *owner);//makes a bolt
  ~Bolt();
  static void Cleanup();
  static void Draw();
  static void UpdatePhysics();
  bool Update();///www.cachunkcachunk.com
  bool Collide();
};

#endif
