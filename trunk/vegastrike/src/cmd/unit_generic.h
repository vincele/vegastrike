/*
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
 *
 * http://vegastrike.sourceforge.net/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/***** Unit is the Unit class without GFX/Sound nor AI *****/

#ifndef _UNIT_H_
#define _UNIT_H_
#define CONTAINER_DEBUG
#ifdef CONTAINER_DEBUG
#include "hashtable.h"
class Unit;
void CheckUnit(class Unit *);
void UncheckUnit (class Unit * un);
//extern Hashtable <long, Unit, char[2095]> deletedUn;
#endif
#include "vegastrike.h"
#include "vs_globals.h"

#include <string>
#include "gfx/matrix.h"
#include "gfx/quaternion.h"
#include "weapon_xml.h"
#include "linecollide.h"
//#include "gfx/vdu.h"
#include "xml_support.h"
#include "container.h"
#include "collection.h"
#include "script/flightgroup.h"
#include "faction.h"
using std::string;

extern char * GetUnitDir (const char * filename);
extern float capship_size;
using namespace XMLSupport;

class PlanetaryOrbit;
class UnitCollection;

class Order;
class Beam;
class Animation;
class Nebula;
class Animation;
class Sprite;
class Box;
class StarSystem;
struct colTrees;

#include "images.h"

/**
 * Currently the only inheriting function is planet
 * Needed by star system to determine whether current unit
 * is orbitable
 */
enum clsptr {
	UNITPTR,
	PLANETPTR,
	BUILDINGPTR,
	NEBULAPTR,
	ASTEROIDPTR,
	ENHANCEMENTPTR,
	MISSILEPTR
};

class VDU;
struct UnitImages;
struct UnitSounds;
struct Cargo;
class Mesh;

/// used to scan the system - faster than c_alike code

struct Scanner {
  Unit *nearest_enemy;
  Unit *nearest_friend;
  Unit *nearest_ship;
  Unit *leader;

  float nearest_enemy_dist,nearest_friend_dist,nearest_ship_dist;

  double last_scantime;
};

/**
 * Unit contains any physical object that may collide with something
 * And may be physically affected by forces.
 * Units are assumed to have various damage and explode when they are dead.
 * Units may have any number of weapons which, themselves may be units
 * the aistate indicates how the unit will behave in the upcoming phys frame
 */
class PlanetaryTransform;
struct PlanetaryOrbitData;
class Unit
{
protected:
  UnitSounds * sound;
  ///How many lists are referencing us
  int ucref;

public:
  ///The name (type) of this unit shouldn't be public
  std::string name;

/***************************************************************************************/
/**** CONSTRUCTORS / DESCTRUCTOR                                                    ****/
/***************************************************************************************/

protected:
  /// forbidden
  Unit( const Unit& ); 
  
  /// forbidden
  Unit& operator=( const Unit& );
  
public:
  /** default constructor
   */
  Unit();
    
  /** Default constructor. This is just to figure out where default
   *  constructors are used. The useless argument will be removed
   *  again later.
   */
  Unit( int dummy );
  
  /** Constructor that creates aa mesh with meshes as submeshes (number
   *  of them) as either as subunit with faction faction
   */
  Unit (std::vector <string> &meshes  , bool Subunit, int faction);

  /** Constructor that creates a mesh from an XML file If it is a
   *  customizedUnit, it will check in that directory in the home dir for
   *  the unit.
   */
// Uses a lot of stuff that does not belong to here
  Unit( const char *filename,
        bool        SubUnit,
       int         faction,
       std::string customizedUnit=string(""),
       Flightgroup *flightgroup=NULL,
       int         fg_subnumber=0 );
  friend class UnitFactory;

public:    
  virtual ~Unit();

/***************************************************************************************/
/**** NETWORKING STUFF                                                              ****/
/***************************************************************************************/

protected:
  // Tell if networked unit
  char networked;
  bool player;
public:
  void SetNetworkMode( bool mode) {networked = true;}
  void SetPlayer( bool p=true) { player = p;}
  bool isPlayer() { return player;}

/***************************************************************************************/
/**** UPGRADE/CUSTOMIZE STUFF                                                       ****/
/***************************************************************************************/

// Uses mmm... stuff not desired here ?
  virtual bool UpgradeSubUnits (Unit * up, int subunitoffset, bool touchme, bool downgrade, int &numave, double &percentage);
  bool UpgradeMounts (const Unit * up, int subunitoffset, bool touchme, bool downgrade, int &numave, Unit * templ, double &percentage);
  virtual bool Dock (Unit * unitToDockWith){return false;};
  /// the turrets and spinning parts fun fun stuff
  UnitCollection SubUnits; 

  /** 
   * Contains information about a particular Mount on a unit.
   * And the weapons it has, where it is, where it's aimed, 
   * The ammo and the weapon type. As well as the possible weapons it may fit
   * Warning: type has a string inside... cannot be memcpy'd
   */
public:
  class Mount;
  un_iter getSubUnits();
  un_kiter viewSubUnits() const;
protected:
  vector <Mount *> mounts;
protected:
  ///Mount may access unit
  friend class Unit::Mount;
  ///no collision table presence.
  bool SubUnit;
public:
  bool UpAndDownGrade (Unit * up, Unit * templ, int mountoffset, int subunitoffset, bool touchme, bool downgrade, int additive, bool forcetransaction, double &percentage);
  void ImportPartList (const std::string& category, float price, float pricedev,  float quantity, float quantdev);

  int GetNumMounts ()const  {return mounts.size();}

  ///Loads a user interface for the user to upgrade his ship
// Uses base stuff -> only in Unit
  virtual void UpgradeInterface (Unit * base) {}
  ///

  bool canUpgrade (Unit * upgrador, int mountoffset,  int subunitoffset, int additive, bool force,  double & percentage, Unit * templ=NULL);
  bool Upgrade (Unit * upgrador, int mountoffset,  int subunitoffset, int additive, bool force,  double & percentage, Unit * templ=NULL);
  double Upgrade (const std::string &file,int mountoffset, int subunitoffset, bool force, bool loop_through_mounts) { return 1;}
  bool canDowngrade (Unit *downgradeor, int mountoffset, int subunitoffset, double & percentage);
  bool Downgrade (Unit * downgradeor, int mountoffset, int subunitoffset,  double & percentage);

/***************************************************************************************/
/**** GFX/PLANET STUFF                                                              ****/
/***************************************************************************************/

protected:
  Nebula * nebula;
  PlanetaryOrbitData * planet;
  ///The orbit needs to have access to the velocity directly to disobey physics laws to precalculate orbits
  friend class PlanetaryOrbit;
  friend class ContinuousTerrain;
  ///VDU needs mount data to draw weapon displays
  friend class VDU;
  friend class UpgradingInfo;//needed to actually upgrade unit through interface
 public:
// Uses a static member of Cockpit class -- TO CHECK
//  void DamageRandSys (float dam,const Vector &vec);
  void SetNebula (Nebula *neb) {
    nebula = neb;
    if (!SubUnits.empty()) {
      un_fiter iter =SubUnits.fastIterator();
      Unit * un;
      while ((un = iter.current())) {
	un->SetNebula (neb);
	iter.advance();
      }
    } 
  }
  inline Nebula * GetNebula () const{return nebula;}
  ///Should draw selection box?
  bool selected;  
  ///Process all meshes to be deleted
  static void ProcessDeleteQueue();
  ///Returns the cockpit name so that the controller may load a new cockpit
  std::string getCockpit ()const;

  // Shouldn't do anything here - but needed by Python
  virtual class Cockpit * GetVelocityDifficultyMult(float &) const { return NULL;}

// Make it a string in class AcctUnit and in class NetUnit and a StarSystem * in Unit class
  StarSystem * activeStarSystem;//the star system I'm in
  ///Takes out of the collide table for this system.
// Uses StarSystem and other stuff : only in NetUnit and Unit
  virtual void RemoveFromSystem (){}
// Uses starsystem stuff so only in Unit class and maybe in
  virtual bool InCorrectStarSystem (StarSystem *active) {return false;}
// Make it an array of string in AcctUnit and NetUnit and a Mesh * in Unit
 std::vector <string> meshdata;
//Use that only in sub classes not in GenericUnit
  virtual int nummesh()const {return 0;}
//void FixGauges();
// Uses planet stuff to put in NetUnit
  virtual void SetPlanetOrbitData (PlanetaryTransform *trans) {}
  virtual PlanetaryTransform *GetPlanetOrbit () const {return NULL;}
  ///Updates the collide Queue with any possible change in sectors
///Split this mesh with into 2^level submeshes at arbitrary planes
// Uses Mesh so only in Unit and maybe in NetUnit
  virtual void Split (int level){}
  //  void SwapOutHalos();
  //  void SwapInHalos();

// Uses Mesh -> in NetUnit and Unit only
  virtual vector <Mesh *> StealMeshes() { vector <Mesh *> v; return v;}
  ///Begin and continue explosion
// Uses GFX so only in Unit class
  virtual bool Explode(bool draw, float timeit) {return false;}
  ///explodes then deletes
  virtual void Destroy(){}

// Uses GFX so only in Unit class
  virtual void Draw(const Transformation & quat = identity_transformation, const Matrix &m = identity_matrix) {}
  virtual void DrawNow(const Matrix &m = identity_matrix, float lod=1000000000) {}
  ///Deprecated
  //virtual void ProcessDrawQueue() {}
  ///Gets the minimum distance away from the point in 3space

  ///Sets the camera to be within this unit.
// Uses Universe so not needed here -> only in Unit class
  virtual void UpdateHudMatrix(int whichcam) {}
  ///What's the HudImage of this unit
// Uses GFX stuff so only in Unit class
  virtual Sprite * getHudImage ()const {return NULL;}
// Not needed just in Unit class

/***************************************************************************************/
/**** NAVIGATION STUFF                                                              ****/
/***************************************************************************************/

public:
  const std::vector <char *> &GetDestinations () const;
  void AddDestination (const char *);
  /**
   * The computer holds all data in the navigation computer of the current unit
   * It is outside modifyable with GetComputerData() and holds only volatile
   * Information inside containers so that destruction of containers will not
   * result in segfaults.
   * Maximum speeds and turning restrictions are merely facts of the computer
   * and have nothing to do with the limitations of the physical nature
   * of space combat
   */
  struct Computer {
    struct RADARLIM {
      ///the max range the radar can handle
      float maxrange;
      ///the dot with (0,0,1) indicating the farthest to the side the radar can handle.
      float maxcone;
      float lockcone;
      float trackingcone;
      ///The minimum radius of the target
      float mintargetsize;
      ///does this radar support IFF?
      bool color;
      bool locked;
    } radar;
    ///The nav point the unit may be heading for
    Vector NavPoint;
    ///The target that the unit has in computer
    UnitContainer target;
    ///Any target that may be attacking and has set this threat
    UnitContainer threat;
    ///Unit that it should match velocity with (not speed) if null, matches velocity with universe frame (star)
    UnitContainer velocity_ref;
    ///The threat level that was calculated from attacking unit's threat
    float threatlevel;
    ///The speed the flybywire system attempts to maintain
    float set_speed;
    ///Computers limitation of speed
    float max_speed; float max_ab_speed;
    ///Computer's restrictions of YPR to limit space combat maneuvers
    float max_yaw; float max_pitch; float max_roll;
    ///Whether or not an 'lead' indicator appears in front of target
    unsigned char slide_start;
    unsigned char slide_end;
    bool itts;
  };
  Computer computer;
// Only in Unit because of StarSystem
  virtual bool TransferUnitToSystem (StarSystem *NewSystem) {return false;}
  virtual void TransferUnitToSystem (unsigned int whichJumpQueue, class StarSystem *&previouslyActiveStarSystem, bool DoSightAndSound) {}
  virtual StarSystem * getStarSystem() {return NULL;}
    struct UnitJump {
    short energy;
    signed char drive;
    unsigned char delay;
    unsigned char damage;
    //negative means fuel
  } jump;
 
  const UnitJump &GetJumpStatus() const {return jump;}
  float CourseDeviation (const Vector &OriginalCourse, const Vector &FinalCourse)const ;
  Computer & GetComputerData () {return computer;}
  const Computer & ViewComputerData () const {return computer;}

  // for scanning purposes
 protected:
  struct Scanner scanner;
 public:
  virtual void scanSystem() {}
  struct Scanner *getScanner() { return &scanner; };
  virtual void ActivateJumpDrive (int destination=0){}
  virtual void DeactivateJumpDrive (){}

/***************************************************************************************/
/**** XML STUFF                                                                     ****/
/***************************************************************************************/

 protected:
  ///Unit XML Load information
  struct XMLstring;
  ///Loading information
  XMLstring *xml_str;

  static void beginElement(void *userData, const XML_Char *name, const XML_Char **atts);
  static void endElement(void *userData, const XML_Char *name);

  void beginElement(const std::string &name, const AttributeList &attributes);
  void endElement(const std::string &name);

 protected:
	 static std::string massSerializer(const struct XMLType &input, void*mythis);
	 static std::string cargoSerializer(const struct XMLType &input, void*mythis);
	 static std::string mountSerializer(const struct XMLType &input, void*mythis);
	 static std::string shieldSerializer(const struct XMLType &input, void*mythis);
	 static std::string subunitSerializer(const struct XMLType &input, void*mythis);

public:
  ///tries to warp as close to un as possible abiding by the distances of various enemy ships...it might not make it all the way
  virtual void WriteUnit(const char * modificationname="") {}
  ///Loads a unit from an xml file into a complete datastructure
  virtual void LoadXML(const char *filename, const char * unitModifications="", char * xmlbuffer=0, int buflength=0) {};

/***************************************************************************************/
/**** PHYSICS STUFF                                                                 ****/
/***************************************************************************************/

private:
  void RechargeEnergy();
protected:
  virtual float ExplosionRadius();
public:
  ///The owner of this unit. This may not collide with owner or units owned by owner. Do not dereference (may be dead pointer)
  Unit *owner;
  ///The previous state in last physics frame to interpolate within
  Transformation prev_physical_state;
  ///The state of the current physics frame to interpolate within
  Transformation curr_physical_state;
  ///number of meshes (each with separate texture) this unit has
  ///The cumulative (incl subunits parents' transformation)
  Matrix cumulative_transformation_matrix;
  ///The cumulative (incl subunits parents' transformation)
  Transformation cumulative_transformation;
  ///The velocity this unit has in World Space
  Vector cumulative_velocity;
  ///The force applied from outside accrued over the whole physics frame
  Vector NetForce;
  ///The force applied by internal objects (thrusters)
  Vector NetLocalForce;
  ///The torque applied from outside objects
  Vector NetTorque;
  ///The torque applied from internal objects
  Vector NetLocalTorque;
  ///the current velocities in LOCAL space (not world space)
  Vector AngularVelocity;  Vector Velocity;  ///The image that will appear on those screens of units targetting this unit
  UnitImages *image;
  ///mass of this unit (may change with cargo)
  float mass;
protected:
  ///are shields tight to the hull.  zero means bubble
  float shieldtight;
  ///fuel of this unit
  float fuel;
  unsigned short afterburnenergy;
  ///-1 means it is off. -2 means it doesn't exist. otherwise it's engaged to destination (positive number)
 ///Moment of intertia of this unit
  float MomentOfInertia;
  struct Limits {
    ///max ypr--both pos/neg are symmetrical
    float yaw; float pitch; float roll;
    ///side-side engine thrust max
    float lateral;
    ///vertical engine thrust max
    float vertical;
    ///forward engine thrust max
    float forward;
    ///reverse engine thrust max
    float retro;
    ///after burner acceleration max
    float afterburn;
    ///the vector denoting the "front" of the turret cone!
    Vector structurelimits;
    ///the minimum dot that the current heading can have with the structurelimit
    float limitmin;
  } limits;

public:
  ///-1 is not available... ranges between 0 32767 for "how invisible" unit currently is (32768... -32768) being visible)
  short cloaking;
  ///How big is this unit
  float radial_size;
protected:
  ///the minimum cloaking value...
  short cloakmin;
  ///Is dead already?
  bool killed;
  ///Should not be drawn
  bool invisible;
  /// corners of object  

public:
  Vector corner_min, corner_max;
  Vector LocalCoordinates (Unit * un) const {
    return ToLocalCoordinates ((un->Position()-Position()).Cast());
  }
  ///how visible the ship is from 0 to 1
  float CloakVisible() const {
    if (cloaking<0)
      return 1;
    return ((float)cloaking)/32767;
  }

  ///cloaks or decloaks the starship depending on the bool
  void Cloak (bool cloak);
  ///deletes
  virtual void Kill(bool eraseFromSave=true) {}
  ///Is dead yet?
  inline bool Killed() const {return killed;}
  ///returns the current ammt of armor left
  float FuelData() const;
  ///Returns the current ammt of energy left
  float EnergyData() const;
  ///Should we resolve forces on this unit (is it free to fly or in orbit)
  bool resolveforces;
  ///What's the size of this unit
  float rSize () const {return radial_size;}

  ///Returns the current world space position
  QVector Position() const{return cumulative_transformation.position;};
  const Matrix &  GetTransformation () const {return cumulative_transformation_matrix;}
  ///Returns the unit-space position
  QVector LocalPosition() const {return curr_physical_state.position;};
  ///Sets the unit-space position
  void SetPosition(const QVector &pos) {prev_physical_state.position = curr_physical_state.position = pos;}
  ///Sets the cumulative transformation matrix's position...for setting up to be out in the middle of nowhere
  void SetCurPosition (const QVector & pos) {curr_physical_state.position=pos;}
  void SetPosAndCumPos (const QVector &pos) {SetPosition (pos);cumulative_transformation_matrix.p=pos;cumulative_transformation.position=pos;}
  ///Sets the state of drawing
  void SetVisible(bool isvis);
  ///Rotates about the axis
  void Rotate(const Vector &axis);
  /**
   * Fire engine takes a unit vector for direction
   * and how fast the fuel speed and mass coming out are
   */
  void FireEngines (const Vector &Direction, /*unit vector... might default to "r"*/
					float FuelSpeed,
					float FMass);
  ///applies a force for the whole gameturn upon the center of mass
  void ApplyForce(const Vector &Vforce); 
  ///applies a force for the whole gameturn upon the center of mass, using local coordinates
  void ApplyLocalForce(const Vector &Vforce); 
  /// applies a force that is multipled by the mass of the ship
  void Accelerate(const Vector &Vforce); 
  ///Apply a torque in world level coords
  void ApplyTorque (const Vector &Vforce, const QVector &Location);
  ///Applies a torque in local level coordinates
  void ApplyLocalTorque (const Vector &Vforce, const Vector &Location);
  ///usually from thrusters remember if I have 2 balanced thrusters I should multiply their effect by 2 :)
  void ApplyBalancedLocalTorque (const Vector &Vforce, const Vector &Location); 

  ///convenient shortcut to applying torques with vector and position
  void ApplyLocalTorque(const Vector &torque); 
  ///Applies damage to the local area given by pnt
  virtual void ApplyLocalDamage (const Vector &pnt, const Vector & normal, float amt, Unit * affectedSubUnit, const GFXColor &, float phasedamage=0) {}
  ///Applies damage to the pre-transformed area of the ship
  virtual void ApplyDamage (const Vector & pnt, const Vector & normal, float amt, Unit * affectedSubUnit, const GFXColor &,  Unit *ownerDoNotDereference, float phasedamage=0 ) {}
  ///Deals remaining damage to the hull at point and applies lighting effects
  virtual float DealDamageToHull (const Vector &pnt, float Damage) {return 1;}

  ///Clamps thrust to the limits struct
  Vector ClampThrust(const Vector &thrust, bool afterburn);
  ///Takes a unit vector for direction of thrust and scales to limits
  Vector MaxThrust(const Vector &thrust);
  ///Thrusts by ammt and clamps accordingly (afterburn or not)
  virtual void Thrust(const Vector &amt,bool afterburn = false);
  ///Applies lateral thrust
  void LateralThrust(float amt);
  ///Applies vertical thrust
  void VerticalThrust(float amt);
  ///Applies forward thrust
  void LongitudinalThrust(float amt);
  ///Clamps desired velocity to computer set limits
  Vector ClampVelocity (const Vector & velocity, const bool afterburn);
  ///Clamps desired angular velocity to computer set limits
  Vector ClampAngVel (const Vector & vel);
  ///Clamps desired torque to computer set limits of thrusters
  Vector ClampTorque(const Vector &torque);
  ///scales unit size torque to limits in that direction
  Vector MaxTorque(const Vector &torque);
  ///Applies a yaw of amt
  void YawTorque(float amt);
  ///Applies a pitch of amt
  void PitchTorque(float amt);
  ///Applies a roll of amt
  void RollTorque(float amt);
  ///executes a repair if the repair bot is up to it
  virtual void Repair() {};
  ///Updates physics given unit space transformations and if this is the last physics frame in the current gfx frame
// Not needed here, so only in NetUnit and Unit classes
  virtual void UpdatePhysics (const Transformation &trans, const Matrix &transmat, const Vector & CumulativeVelocity, bool ResolveLast, UnitCollection *uc=NULL) {}
  ///Resolves forces of given unit on a physics frame
  virtual Vector ResolveForces (const Transformation &, const Matrix&);
  ///Returns the pqr oritnattion of the unit in world space
  void SetOrientation (QVector q, QVector r);
  void SetOrientation (Quaternion Q);
  void GetOrientation(Vector &p, Vector &q, Vector &r) const;
  ///Transforms a orientation vector Up a coordinate level. Does not take position into account
  Vector UpCoordinateLevel(const Vector &v) const;
  ///Transforms a orientation vector Down a coordinate level. Does not take position into account
  Vector DownCoordinateLevel(const Vector &v) const;
  ///Transforms a orientation vector from world space to local space. Does not take position into account 
  Vector ToLocalCoordinates(const Vector &v) const;
  ///Transforms a orientation vector to world space. Does not take position into account
  Vector ToWorldCoordinates(const Vector &v) const;
  ///Returns unit-space ang velocity
  const Vector &GetAngularVelocity() const { return AngularVelocity; }
  ///Return unit-space velocity
  const Vector &GetVelocity() const { return cumulative_velocity; }

  void SetVelocity (const Vector & v) {Velocity = v;}
  void SetAngularVelocity (const Vector & v) {AngularVelocity = v;}
  float GetMoment() const { return MomentOfInertia; }
  float GetMass() const { return mass; }
  ///returns the ammt of elasticity of collisions with this unit
  float GetElasticity ();
  ///returns given limits (should not be necessary with clamping functions)
  const Limits &Limits() const { return limits; }
  ///Sets if forces should resolve on this unit or not
  void SetResolveForces(bool);

  /*
    //FIXME Deprecated turret restrictions (may bring back)
  enum restr {YRESTR=1, PRESTR=2, RRESTR=4};

  char yprrestricted;

  float ymin, ymax, ycur;
  float pmin, pmax, pcur;
  float rmin, rmax, rcur;
  */
  /// thrusting limits (acceleration-wise

/***************************************************************************************/
/**** WEAPONS/SHIELD STUFF                                                          ****/
/***************************************************************************************/

protected:
  ///Activates all guns of that size
  void ActivateGuns (const weapon_info *, bool Missile);
  ///Armor values: how much damage armor can withhold before internal damage accrues
  struct {
    unsigned short front, back, right, left;
  } armor;
  ///Shielding Struct holding values of current shields
  struct {
    ///How much the shield recharges per second
    float recharge; 
    ///A union containing the different shield values and max values depending on number
    union {
      ///if shield is 2 big, 2 floats make this shield up, and 2 floats for max {front,back,frontmax,backmax}
      float fb[4];
      ///If the shield if 4 big, 4 floats make the shield up, and 4 keep track of max recharge value
      struct {
	unsigned short front, back, right, left;
	unsigned short frontmax, backmax, rightmax, leftmax;
      }fbrl;
      ///If the shield is 6 sided, 6 floats make it up, 2 indicating the max value of various sides, and 6 being the actual vals
      struct {
	unsigned short v[6];
	unsigned short fbmax,rltbmax;
      }fbrltb;
    };
    ///the number of shields in the current shielding struct
    signed char number;
    ///What percentage leaks (divide by 100%)
    char leak; 
  } shield;
  float MaxShieldVal() const;
  ///regenerates all 2,4, or 6 shields for 1 SIMULATION_ATOM
  void RegenShields();
public:
  ///The structual integ of the current unit
  float hull;
protected:
  ///Original hull
  float maxhull;
  ///The radar limits (range, cone range, etc) 
  ///the current order
  ///how much the energy recharges per second
  float recharge;
  ///maximum energy
  unsigned short maxenergy;
  ///current energy
  unsigned short energy;
  ///applies damage from the given pnt to the shield, and returns % damage applied and applies lighitn
  virtual float DealDamageToShield (const Vector & pnt, float &Damage);
  ///If the shields are up from this position
  bool ShieldUp (const Vector &) const;

public:
  int LockMissile() const;//-1 is no lock necessary 1 is locked
  void LockTarget(bool myboo){computer.radar.locked=myboo;}
  bool TargetLocked()const {return computer.radar.locked;}
  float TrackingGuns(bool &missileLock);
  ///Changes currently selected weapon
  void ToggleWeapon (bool Missile);
  ///Selects all weapons
  void SelectAllWeapon (bool Missile);
  ///Gets the average gun speed of the unit::caution SLOW
  void getAverageGunSpeed (float & speed, float & range) const;
  ///Finds the position from the local position if guns are aimed at it with speed
  QVector PositionITTS (const QVector & local_posit, float speed) const;
  ///returns percentage of course deviation for contraband searches.  .5 causes error and 1 causes them to get mad 
  float FShieldData() const;  float RShieldData() const;  float LShieldData() const;  float BShieldData() const;
  void ArmorData(unsigned short armor[4])const;
  ///Gets the current status of the hull
  float GetHull() const{return hull;}
  float GetHullPercent() const{return maxhull!=0?hull/maxhull:hull;}
  ///Fires all active guns that are or arent Missiles
  virtual void Fire(bool Missile) {}
  ///Stops all active guns from firing
  void UnFire();
  ///reduces shields to X percentage and reduces shield recharge to Y percentage
  void leach (float XshieldPercent, float YrechargePercent, float ZenergyPercent);


/***************************************************************************************/
/**** TARGETTING STUFF                                                              ****/
/***************************************************************************************/

protected:
  ///not used yet
  string target_fgid[3];
public:
  bool InRange (Unit *target, bool cone=true, bool cap=true) const{
    double mm;
    return InRange( target,mm,cone,cap,true);
  }
  bool InRange (Unit *target, double & mm, bool cone, bool cap, bool lock) const{
    if (this==target||target->CloakVisible()<.8)
      return false;
    if (cone&&computer.radar.maxcone>-.98){
      QVector delta( target->Position()-Position());
      mm = delta.Magnitude();
      if ((!lock)||(!(TargetLocked()&&computer.target==target))) {
	double tempmm =mm-target->rSize();
	if (tempmm>0.0001) {
	  if ((ToLocalCoordinates (Vector(delta.i,delta.j,delta.k)).k/tempmm)<computer.radar.maxcone&&cone) {
	    return false;
	  }
	}
      }
    }else {
      mm = (target->Position()-Position()).Magnitude();
    }
    if (((mm-rSize()-target->rSize())>computer.radar.maxrange)||target->rSize()<computer.radar.mintargetsize) {//owner==target?!
      Flightgroup *fg = target->getFlightgroup();
      if ((target->rSize()<capship_size||(!cap))&&(fg==NULL?true:fg->name!="Base")) 
        return target->isUnit()==PLANETPTR;
    }
    return true;
  }
  Unit *Target() {return computer.target.GetUnit(); }
  Unit *VelocityReference() {return computer.velocity_ref.GetUnit(); }
  Unit *Threat() {return computer.threat.GetUnit(); }
// Uses Universe stuff so only in Unit class
  void VelocityReference (Unit *targ);
  virtual void TargetTurret (Unit * targ){}
  ///Threatens this unit with "targ" as aggressor. Danger should be cos angle to target
  void Threaten (Unit * targ, float danger);
  ///Rekeys the threat level to zero for another turn of impending danger
  void ResetThreatLevel() {computer.threatlevel=0;}
  ///The cosine of the angle to the target given passed in speed and range
  float cosAngleTo (Unit * target, float & distance, float speed= 0.001, float range=0.001) const;
  ///Highest cosine from given mounts to target. Returns distance and cosine
  float cosAngleFromMountTo (Unit * target, float & distance) const;
  float computeLockingPercent();//how locked are we
  ///Turns on selection box
  void Select();
  ///Turns off selection box
  void Deselect();

  // Shouldn't do anything here - but needed by Python
  virtual void Target (Unit * targ) {}

  ///not used yet
  virtual void setTargetFg(string primary,string secondary=string(),string tertiary=string()) {}
  ///not used yet
  virtual void ReTargetFg(int which_target=0) {}
  ///not used yet

/***************************************************************************************/
/**** CARGO STUFF                                                                   ****/
/***************************************************************************************/

protected:
  void SortCargo();
public:
  bool CanAddCargo (const Cargo &carg) const;
  void AddCargo (const Cargo &carg,bool sort=true);
  int RemoveCargo (unsigned int i, int quantity, bool eraseZero=true);
  float PriceCargo (const std::string &s);
  Cargo & GetCargo (unsigned int i);
  void GetCargoCat (const std::string &category, vector <Cargo> &cat);
  ///below function returns NULL if not found
  Cargo * GetCargo (const std::string &s, unsigned int &i);
  unsigned int numCargo ()const;
  std::string GetManifest (unsigned int i, Unit * scanningUnit, const Vector & original_velocity) const;
  bool SellCargo (unsigned int i, int quantity, float &creds, Cargo & carg, Unit *buyer);
  bool SellCargo (const std::string &s, int quantity, float & creds, Cargo &carg, Unit *buyer);
  bool BuyCargo (const Cargo &carg, float & creds);
  bool BuyCargo (unsigned int i, unsigned int quantity, Unit * buyer, float &creds);
  bool BuyCargo (const std::string &cargo,unsigned int quantity, Unit * buyer, float & creds);
// Uses Universe stuff
  virtual void EjectCargo (unsigned int index) {}

/***************************************************************************************/
/**** AI STUFF                                                                      ****/
/***************************************************************************************/

public:
  // Because accessing in daughter classes member function from Unit * instances
  Order *aistate;
  Order *getAIState() const{return aistate;}
  ///Sets up a null queue for orders
// Uses AI so only in NetUnit and Unit classes
  virtual void PrimeOrders() {}
//  void PrimeOrders(Order * newAI);
  ///Sets the AI to be a specific order
  virtual void SetAI(Order *newAI) {}
  ///Enqueues an order to the unit's order queue
  virtual void EnqueueAI(Order *newAI) {}
  ///EnqueuesAI first
  virtual void EnqueueAIFirst (Order * newAI) {}
  ///num subunits
  virtual void LoadAIScript (const std::string &aiscript) {}
  virtual bool LoadLastPythonAIScript () {return false;}
  virtual bool EnqueueLastPythonAIScript () {return false;}
// Uses Order class but just a poiner so ok
// Uses AI so only in NetUnit and Unit classes
  virtual double getMinDis(const QVector &pnt){ return 1;}//for clicklist
// Uses AI stuff so only in NetUnit and Unit classes
  virtual void SetTurretAI () {}
  virtual void DisableTurretAI () {}
// AI so only in NetUnit and Unit classes
  virtual string getFullAIDescription() { return string("");}
   ///Erases all orders that bitwise OR with that type
// Uses AI so only in NetUnit and Unit classes
//  void eraseOrderType (unsigned int type);
  ///Executes 1 frame of physics-based AI
// Uses AI so only in NetUnit and Unit classes
  virtual void ExecuteAI() {}

/*************** T O   C H E C K ! ! ! *******************/
// Uses UnitUtil stuff but needed in NetUnit and Unit classes -- TO CHECK
  virtual bool AutoPilotTo(Unit * un, bool ignore_friendlies=false) {return false;}

/***************************************************************************************/
/**** COLLISION STUFF                                                               ****/
/***************************************************************************************/

public:
  ///The information about the minimum and maximum ranges of this unit. Collide Tables point to this bit of information.
  LineCollide CollideInfo;
  struct collideTrees * colTrees;
  ///Sets the parent to be this unit. Unit never dereferenced for this operation
  void SetCollisionParent (Unit *name);
  ///won't collide with owner
  void SetOwner(Unit *target);
  void SetRecursiveOwner(Unit *target);

  // Shouldn't do anything here - but needed by Python
  ///Queries the BSP tree with a world space st and end point. Returns the normal and distance on the line of the intersection
  virtual Unit * queryBSP (const QVector &st, const QVector & end, Vector & normal, float &distance, bool ShieldBSP=true) {return NULL;}
  ///queries the BSP with a world space pnt, radius err.  Returns the normal and distance of the plane to the shield. If Unit returned not NULL, that subunit hit
  virtual Unit * queryBSP (const QVector &pnt, float err, Vector & normal, float &dist,  bool ShieldBSP) {return NULL;}
  // Using collision stuff -> NetUnit if possible
  virtual Unit * BeamInsideCollideTree(const QVector &start, const QVector &end, QVector & pos, Vector & norm, double & distance) {return NULL;}

// Uses BSP...
// struct collideTrees * colTrees;
  ///fils in corner_min,corner_max and radial_size
// Uses Box stuff -> only in NetUnit and Unit
  virtual void calculate_extent(bool update_collide_queue) {}

// To let only in Unit class
///Builds a BSP tree from either the hull or else the current meshdata[] array
// void BuildBSPTree (const char *filename, bool vplane=false, Mesh * hull=NULL);//if hull==NULL, then use meshdata **
// Uses mesh stuff (only rSize()) : I have to find something to do
  virtual bool Inside (const QVector &position, const float radius, Vector & normal, float &dist) {return false;}
// Uses collide and Universe stuff -> put in NetUnit
//  void UpdateCollideQueue();
// Uses LineCollide stuff so only in NetUnit and Unit
  const LineCollide &GetCollideInfo () {return CollideInfo;}
// Uses collision stuff so only in NetUnit and Unit classes
//  virtual bool querySphere (const QVector &pnt, float err) const {return false;}
  ///queries the sphere for beams (world space start,end)  size is added to by my_unit_radius
  virtual float querySphere (const QVector &start, const QVector & end, float my_unit_radius=0) const { return 1;}
//  float querySphereNoRecurse (const QVector &start, const QVector &end, float my_unit_radius=0) const ;
  ///queries the ship with a directed ray
  virtual float querySphereClickList (const QVector &st, const QVector &dir, float err) const {return 1;}//for click list
  ///Queries if this unit is within a given frustum
// Uses GFX -> defined only Unit class
  virtual bool queryFrustum (float frustum[6][4]) const {return false;}

  /**
   *Queries bounding box with a point, radius err
   */
// Uses GFX :(
// Try to use in NetUnit thought
  /**
   *Queries the bounding box with a ray.  1 if ray hits in front... -1 if ray
   * hits behind.
   * 0 if ray misses 
   */
  virtual bool queryBoundingBox (const QVector &pnt, float err) {return false;}
  virtual int queryBoundingBox(const QVector &origin,const Vector &direction, float err) { return 0;}
  /**Queries the bounding sphere with a duo of mouse coordinates that project
   * to the center of a ship and compare with a sphere...pretty fast*/
  ///queries the sphere for weapons (world space point)
// Only in Unit class
  virtual bool querySphereClickList (int,int, float err, Camera *activeCam) {return false;}


  virtual bool InsideCollideTree (Unit * smaller, QVector & bigpos, Vector & bigNormal, QVector & smallpos, Vector & smallNormal) { return false;}
  virtual void reactToCollision(Unit * smaller, const QVector & biglocation, const Vector & bignormal, const QVector & smalllocation, const Vector & smallnormal, float dist) {}
  ///returns true if jump possible even if not taken
// Uses Universe thing
//  bool jumpReactToCollision (Unit *smaller);
  ///Does a collision between this and another unit
  virtual bool Collide(Unit * target) {return false;}
  ///checks for collisions with all beams and other units roughly and then more carefully
  virtual void CollideAll() {}

/***************************************************************************************/
/**** DOCKING STUFF                                                                 ****/
/***************************************************************************************/

public:
  unsigned char docked;
  ///returns -1 if unit cannot dock, otherwise returns which dock it can dock at
  enum DOCKENUM {NOT_DOCKED=0x0, DOCKED_INSIDE=0x1, DOCKED=0x2, DOCKING_UNITS=0x4};
  int CanDockWithMe (Unit * dockingunit) ;
  virtual void PerformDockingOperations() {}
  virtual void FreeDockingPort(unsigned int whichport) {}
  virtual const vector <struct DockingPorts> &DockingPortLocations() { return image->dockingports;}
  char DockedOrDocking()const {return docked;}
  bool IsCleared (Unit * dockignunit);
  bool isDocked ( Unit *dockingUnit);
  bool UnDock (Unit * unitToDockWith);
// Use AI
  virtual bool RequestClearance (Unit * dockingunit) {return false;}
// Uses Cockpit stuff
//  bool Dock (Unit * unitToDockWith);

/***************************************************************************************/
/**** FACTION/FLIGHTGROUP STUFF                                                     ****/
/***************************************************************************************/

public:
  void SetFg (Flightgroup * fg, int fg_snumber);
  ///The faction of this unit
  int faction;
  void SetFaction (int faction);
protected:
  ///the flightgroup this ship is in
  Flightgroup *flightgroup;
  ///the flightgroup subnumber
  int flightgroup_subnumber;
public:
  ///get the flightgroup description
  Flightgroup *getFlightgroup() const { return flightgroup; };
  ///get the subnumber
  int getFgSubnumber() const { return flightgroup_subnumber; };
  ///get the full flightgroup ID (i.e 'green-4')
  const string getFgID();


// Uses Universe stuff
  virtual vector <struct Cargo>& FilterDowngradeList (vector <struct Cargo> & mylist) { return mylist;}
  virtual vector <struct Cargo>& FilterUpgradeList (vector <struct Cargo> & mylist) { return mylist;}

/***************************************************************************************/
/**** MISC STUFF                                                                    ****/
/***************************************************************************************/

protected:
  ///if the unit is a planet, this contains the long-name 'mars-station'
  string fullname;
public:
  void setFullname(string name)  { fullname=name; };
  string getFullname() const { return fullname; };

  ///Initialize many of the defaults inherant to the constructor
  void Init();
  ///Is this class a unit
  virtual enum clsptr isUnit() {return UNITPTR;}
  inline void Ref() {
#ifdef CONTAINER_DEBUG
    CheckUnit(this);
#endif
    ucref++;
  }
  ///Low level list function to reference the unit as being the target of a UnitContainer or Colleciton
  ///Releases the unit from this reference of UnitContainer or Collection
  void UnRef();

  //0 in additive is reaplce  1 is add 2 is mult
  // Put that in NetUnit & AcctUnit with string and with Unit
  UnitImages &GetImageInformation();

  /// sets the full name/fgid for planets
  bool isStarShip(){ if(isUnit()==UNITPTR){ return true;} return false; };
  bool isPlanet(){ if(isUnit()==PLANETPTR){ return true;} return false; };
  bool isJumppoint(){ if(GetDestinations().size()!=0){ return true; } return false; }

// Uses Universe stuff -> maybe only needed in Unit class
  bool isEnemy(Unit *other){ if(FactionUtil::GetIntRelation(this->faction,other->faction)<0.0){ return true; } return false; };
  bool isFriend(Unit *other){ if(FactionUtil::GetIntRelation(this->faction,other->faction)>0.0){ return true; } return false; };
  bool isNeutral(Unit *other){ if(FactionUtil::GetIntRelation(this->faction,other->faction)==0.0){ return true; } return false; };
  // Uses AI
  virtual float getRelation(Unit *other){return false;}
};

///Holds temporary values for inter-function XML communication Saves deprecated restr info
struct Unit::XMLstring {
  //  vector<Halo*> halos;
  vector<Unit::Mount *> mountz;
  vector<string> meshes;
  string shieldmesh;
  string bspmesh;
  string rapidmesh;
  void * data;
  vector<Unit*> units;
  int unitlevel;
  bool hasBSP;
  bool hasColTree;
  enum restr {YRESTR=1, PRESTR=2, RRESTR=4};
  const char * unitModifications;
  char yprrestricted;
  float unitscale;
  float ymin, ymax, ycur;
  float pmin, pmax, pcur;
  float rmin, rmax, rcur;
  std::string cargo_category;
  std::string hudimage;
  int damageiterator;
};

class Unit::Mount {
  protected:
    ///Where is it
    Transformation LocalPosition;
  public:
    void SwapMounts (Mount * othermount);
    virtual void ReplaceMounts (const Mount * othermount);
    double Percentage (const Mount * oldmount) const;
// Gotta look at that, if we can make Beam a string in AcctUnit and a Beam elsewhere
    union REF{
      ///only beams are actually coming out of the gun at all times...bolts, balls, etc aren't
      Beam *gun;
      ///Other weapons must track refire times
      float refire;
    } ref;
    ///the size that this mount can hold. May be any bitwise combination of weapon_info::MOUNT_SIZE

    short size;
    ///-1 is infinite
    short ammo;
    short volume;//-1 is infinite
    ///The data behind this weapon. May be accordingly damaged as time goes on
    enum MOUNTSTATUS{PROCESSED,UNFIRED,FIRED} processed;
    ///Status of the selection of this weapon. Does it fire when we hit space
    enum {ACTIVE, INACTIVE, DESTROYED, UNCHOSEN} status;
    ///The sound this mount makes when fired
    const weapon_info *type;
    int sound;
    float time_to_lock;
    Mount();
// Requires weapon_xml.cpp stuff so Beam stuff so GFX and AUD stuff
    Mount(const std::string& name, short int ammo=-1, short int volume=-1);
    ///Sets this gun to active, unless unchosen or destroyed
    void Activate (bool Missile);
    ///Sets this gun to inactive, unless unchosen or destroyed
    void DeActive (bool Missile);
    ///Sets this gun's position on the mesh
    void SetMountPosition (const Transformation &t) {LocalPosition = t;}
    ///Gets the mount's position and transform
    Transformation &GetMountLocation () {return LocalPosition;}
    ///Turns off a firing beam (upon key release for example)
	virtual void UnFire() {}
    /**
     *  Fires a beam when the firing unit is at the Cumulative location/transformation 
     * owner (won't crash into)  as owner and target as missile target. bool Missile indicates if it is a missile
     * should it fire
     */ 
	// Uses Sound Forcefeedback and other stuff
	virtual void PhysicsAlignedUnfire() {}
	virtual bool PhysicsAlignedFire (const Transformation &Cumulative, const Matrix & mat, const Vector & Velocity, Unit *owner,  Unit *target, signed char autotrack, float trackingcone) { return false;}//0 is no track...1 is target 2 is target + lead
	virtual bool Fire (Unit *owner, bool Missile=false) {return false;}
};

inline void UnitCollection::UnitIterator::GetNextValidUnit () {
  while (pos->next->unit?pos->next->unit->Killed():false) {
    remove();
  }
}

inline Unit * UnitContainer::GetUnit() {
  if (unit==NULL)
    return NULL;
#ifdef CONTAINER_DEBUG
  CheckUnit(unit);
#endif
  if (unit->Killed()) {
    unit->UnRef();
    unit = NULL;
    return NULL;
  }

  return unit;
}


#endif
