#ifndef MISSILE_H_
#define MISSILE_H_

class MissileEffect {

  Vector pos;
  float damage;
  float phasedamage;
  float radius;
  float radialmultiplier;
 public:
  void ApplyDamage (Unit *);
  MissileEffect (const Vector & pos, float dam, float pdam, float radius, float radmult):pos(pos) {
    damage=dam;phasedamage=pdam;
    this->radius=radius;
    radialmultiplier=radmult;
  }
};
class Missile:public GameUnit {
 public:
 protected:
  virtual float ExplosionRadius();
  float time;
  float damage;
  float phasedamage;
  float radial_effect;
  float radial_multiplier;
  float detonation_radius;
  bool discharged;
  signed char retarget;
 public:
  void Discharge();
  virtual enum clsptr isUnit() {return MISSILEPTR;}

protected:
    /// constructor only to be called by UnitFactory
    Missile( const char * filename,
             int faction,
	     const string &modifications,
	     const float damage,
	     float phasedamage,
	     float time,
	     float radialeffect,
	     float radmult,
	     float detonation_radius)
        : GameUnit (filename,false,faction,modifications)
        , time(time)
        , damage(damage)
        , phasedamage(phasedamage)
        , radial_effect(radialeffect)
        , radial_multiplier (radmult)
        , detonation_radius(detonation_radius)
        , discharged(false)
        , retarget (-1)
    {maxhull*=10; }

  friend class GameUnitFactory;

public:
  virtual void Kill (bool eraseFromSave=true);
  virtual void reactToCollision (Unit * smaller, const QVector & biglocation, const Vector & bignormal, const QVector & smalllocation, const Vector & smallnormal, float dist);
  virtual void UpdatePhysics (const Transformation &trans, const Matrix &transmat, const Vector & CumulativeVelocity, bool ResolveLast, UnitCollection *uc=NULL);

private:
    /// default constructor forbidden
    Missile( );
    /// copy constructor forbidden
    Missile( const Missile& );
    /// assignment operator forbidden
    Missile& operator=( const Missile& );
};


#endif
