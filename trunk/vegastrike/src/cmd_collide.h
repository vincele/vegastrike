#ifndef _CMD_COLLIDE_H_
#define _CMD_COLLIDE_H_
struct LineCollide {
  void * object;
  enum collidables {UNIT, BEAM,BALL,BOLT,PROJECTILE} type;
  Vector Mini;
  Vector Maxi;
  LineCollide (void * objec, enum collidables typ,const Vector &st, const Vector &en) {this->object=objec;this->type=typ;this->Mini=st;this->Maxi=en;}
  LineCollide (const LineCollide &l) {object=l.object; type=l.type; Mini=l.Mini;Maxi=l.Maxi;}      
};

void AddCollideQueue(const LineCollide &, bool );
void ClearCollideQueue();






#endif
