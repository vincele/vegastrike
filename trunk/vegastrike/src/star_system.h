#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include <expat.h>
#include <string>
#include "xml_support.h"
using std::string;
#include "gfx/camera.h"
class Stars;
class Planet;
class UnitCollection;
class ClickList;
class Unit;
class TextPlane;
class InputDFA;
#define NUM_CAM		12
using XMLSupport::AttributeList;
struct Texture;
class Background;
class Terrain;
class ContinuousTerrain;
class Atmosphere;
/**
 * Star System
 * Scene management for a star system
 * Per-Frame Drawing & Physics simulation
 **/
class StarSystem {
 private:
  ///Physics is divided into 3 stages spread over 3 frames
  enum PHYSICS_STAGE {PHY_AI,TERRAIN_BOLT_COLLIDE,PHY_COLLIDE, PHY_TERRAIN, PHY_RESOLV,PHY_NUM} current_stage;
  
  ///Stars, planets, etc. Orbital mechanics precalculated 

  ///Starsystem XML Struct For use with XML loading
  struct StarXML {
    Terrain * parentterrain;
    ContinuousTerrain * ct;
    int unitlevel;
    std::vector <GFXLight> lights;
    std::vector <Planet *> moons;
    string backgroundname;
    Vector systemcentroid;
    Vector cursun;
    float timeofyear;
    float reflectivity;
    int numnearstars;
    int numstars;
    bool fade;
    float starsp;
  } *xml;
  std::vector <Terrain *> terrains;
  std::vector <ContinuousTerrain *>contterrains;
  /// Everything to be drawn. Folded missiles in here oneday
  UnitCollection *drawList; 
  /// Objects subject to global gravitron physics (disabled)   
  UnitCollection *units;    
  unsigned char no_collision_time;
  ///system name             
  char * name; 
  std::string filename;
  ///All avail cameras in this system
  Camera cam[NUM_CAM];
  int currentcamera;
  ///Process global clicks for input/output
  InputDFA * systemInputDFA;
  ///to track the next given physics frame
  double time;
  ///The background associated with this system
  Background * bg;
  ///The moving, fading stars
  Stars *stars;
  ///The Light Map corresponding for the BP for spheremapping
  Texture *LightMap[6]; 
  void AddStarsystemToUniverse(const string &filename);
  int lightcontext;
 public:
  class CollideTable *collidetable;
  class bolt_draw *bolts;
  Background* getBackground() {return bg;}
  ///activates the light map texture
  void activateLightMap();
  static void DrawJumpStars();
  Terrain * getTerrain (unsigned int which) {return terrains[which];}
  unsigned int numTerrain () {return terrains.size();}
  ContinuousTerrain * getContTerrain (unsigned int which) {return contterrains[which];}
  unsigned int numContTerrain () {return contterrains.size();}
  void LoadXML(const char*, const Vector & centroid, const float timeofyear);
  static void beginElement(void *userData, const XML_Char *name, const XML_Char **atts);
  static void endElement(void *userData, const XML_Char *name);
  string getFileName();
  string getName();
  void beginElement(const string &name, const AttributeList &attributes);
  void endElement(const string &name);
  ///Accesses the current camera
  Camera *AccessCamera() {return &cam[currentcamera];}
  ///Returns the passed in cam
  Camera *AccessCamera(int);
  ///Changes current camera to selected camera
  void SelectCamera(int);
  ///GFXLoadMatrix proper camera
  void SetViewport() {
    cam[currentcamera].UpdateGFX();
  }
  ///Loads the star system from an XML file
  StarSystem(const char * filename, const Vector & centroid=Vector (0,0,0), const float timeofyear=0);
  ~StarSystem();
  UnitCollection * getUnitList();
  /// returns xy sorted bounding spheres of all units in current view
  ClickList *getClickList(); 
  ///Adds to draw list
  void AddUnit(Unit *unit);
  ///Removes from draw list
  bool RemoveUnit(Unit *unit);
  bool JumpTo (Unit * unit, Unit * jumppoint, const std::string &system);
  ///Draws a frame of action, interpolating between physics frames
  void Draw(bool DrawCockpit=true);
  /// update a simulation atom
  void Update(); 
  ///re-enables the included lights and terrains
  void SwapIn ();
  ///Disables included lights and terrains
  void SwapOut ();
  static void ProcessPendingJumps ();

  friend class Atmosphere;
};

#endif

