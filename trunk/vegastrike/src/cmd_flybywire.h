#ifndef _CMD_FLYBYWIRE_H_
#define _CMD_FLYBYWIRE_H_
#include "cmd_order.h"

class MatchLinearVelocity : public Order {
 protected:
  Vector desired_velocity;//werld space... generally r*speed;
  bool LocalVelocity;//specified in Local or World coordinates
 public:
  MatchLinearVelocity (const Vector &desired, bool Local):desired_velocity(desired),LocalVelocity(Local) {type = LOCATION;done = false;}
  AI * Execute ();
  void SetDesiredVelocity (const Vector &desired, bool Local) {desired_velocity=desired;LocalVelocity=Local;}
};
class MatchAngularVelocity : public Order {
 protected:
  Vector desired_ang_velocity;//werld space... generally r*speed;
  bool LocalAng;//specified in Local or World coordinates
 public:
  MatchAngularVelocity (const Vector &desired, bool Local):desired_ang_velocity(desired),LocalAng(Local) {type = LOCATION;done = false;}
  AI * Execute ();
  void SetDesiredAngularVelocity (const Vector &desired, bool Local) {desired_ang_velocity=desired;LocalAng=Local;}
};

class MatchVelocity : public MatchLinearVelocity {
 protected:
  Vector desired_ang_velocity;//werld space... generally r*speed;
  bool LocalAng;
 public:
  MatchVelocity (const Vector &desired,const Vector &desired_ang, bool Local):MatchLinearVelocity (desired,Local),desired_ang_velocity(desired_ang), LocalAng(Local) {}
  AI * Execute ();
  void SetDesiredAngularVelocity (const Vector &desired, bool Local) {desired_ang_velocity=desired;LocalAng=Local;}
};

class FlyByWire : public MatchVelocity {
 protected:
 public:
  FlyByWire ();
  void Stop (float percentage);
  void Right (float percentage);//pass in the percentage of the turn they were turnin right.  -%age indicates left
  void Up (float percentage);//pass in the percentage of the turn they were turning up
  void RollRight (float percentage);
  void Afterburn (float percentage);
  void Accel (float percentage);//negative is decel... 0 = nothing
  AI * Execute();
};
#endif
