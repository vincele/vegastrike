#ifndef _CMD_NAVIGATION_ORDERS_H_
#define _CMD_NAVIGATION_ORDERS_H_


#include "order.h"
#include "vegastrike.h"

namespace Orders {
  // This moveto always attempts to move in a straight line (unaware of strafing)
  const float bleed_threshold = 0.0001;
  const float THRESHOLD = 0.01;
  const unsigned char ABURN = 1;
class MoveTo : public Order {
  unsigned char afterburnAndSwitchbacks;//don't need the lowest order bit
  unsigned char terminatingX;
  unsigned char terminatingY;
  unsigned char terminatingZ;
  Vector last_velocity;
  bool OptimizeSpeed (float v, float &a);
  bool Done (const Vector &);
public:
  MoveTo(const Vector &target, bool aft, unsigned char numswitchbacks) : Order(LOCATION|MOVEMENT), afterburnAndSwitchbacks(aft+(numswitchbacks<<1)),terminatingX(0), terminatingY(0), terminatingZ(0), last_velocity(0,0,0) {
    targetlocation = target;
    done=false;
  }
  void SetDest (const Vector&);
  void Execute();
};

class ChangeHeading : public Order {
  unsigned char switchbacks;//don't need the lowest order bit
  unsigned char terminatingX;
  unsigned char terminatingY;
  Vector last_velocity;
  Vector final_heading;
  bool terminating;
  bool OptimizeAngSpeed(float limit, float v, float &a);
  bool Done (const Vector &);
  void TurnToward (float angle, float ang_vel, float &torque);
protected:
  void ResetDone () {done = false; terminatingX=terminatingY=0;}
 
 public:
   ChangeHeading(const Vector &final_heading, int switchback) : Order(FACING|LOCATION), switchbacks(switchback),terminatingX(0),terminatingY(0),last_velocity(0,0,0),final_heading(final_heading), terminating(false) {}
  void SetDest (const Vector&);
  void Execute();
};

class FaceTarget : public ChangeHeading {
  float finish;
public:
  FaceTarget (bool fini=false, int accuracy =3);
  void Execute ();
};

class FaceTargetITTS : public ChangeHeading {
  bool finish;
  float speed;
  float range;
public:
  FaceTargetITTS (bool fini=false, int accuracy = 3);
  void Execute();
};

}
#endif

