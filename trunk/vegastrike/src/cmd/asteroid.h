#ifndef _ASTEROID_H_
#define _ASTEROID_H_
#include "asteroid_generic.h"
#include "cmd/unit.h"
class GameAsteroid: public GameUnit<Asteroid> {
  public:
	//void Init( float difficulty);
	//virtual enum clsptr isUnit() {return ASTEROIDPTR;}
	//virtual void reactToCollision(Unit * smaller, const QVector & biglocation, const Vector & bignormal, const QVector & smalllocation, const Vector & smallnormal, float dist);
  
	virtual void UpdatePhysics (const Transformation &trans, const Matrix &transmat, const Vector & CumulativeVelocity, bool ResolveLast, UnitCollection *uc=NULL);
protected:
    /** Constructor that can only be called by the UnitFactory.
     */
    GameAsteroid(const char * filename, int faction, Flightgroup* fg=NULL, int fg_snumber=0, float difficulty=.01);

    friend class UnitFactory;

private:
    /// default constructor forbidden
    GameAsteroid( );

    /// copy constructor forbidden
    GameAsteroid( const Asteroid& );

    /// assignment operator forbidden
    GameAsteroid& operator=( const Asteroid& );
};

#endif
