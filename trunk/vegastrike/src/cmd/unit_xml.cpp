#include "unit.h"

#include "xml_support.h"
#include "gfx/halo.h"
//#include <iostream.h>
#include <fstream>
#include <expat.h>
//#include <values.h>
#include <float.h>
#include "gfx/mesh.h"
#include "gfx/sphere.h"
#include "gfx/bsp.h"
#include "gfx/sprite.h"
#include "audiolib.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "vegastrike.h"
#include <assert.h>
#include "images.h"
#include "xml_serializer.h"
#include "collide/rapcol.h"
#include "unit_collide.h"
#include "vs_path.h"
#define VS_PI 3.1415926536
void Unit::beginElement(void *userData, const XML_Char *name, const XML_Char **atts) {
  ((Unit*)userData)->beginElement(name, AttributeList(atts));
}

void Unit::endElement(void *userData, const XML_Char *name) {
  ((Unit*)userData)->endElement(name);
}


namespace UnitXML {
    enum Names {
      UNKNOWN,
      UNIT,
      SUBUNIT,
      MESHFILE,
      SHIELDMESH,
      BSPMESH,
      MOUNT,
      MESHLIGHT,
      DOCK,
      XFILE,
      X,
      Y,
      Z,
      RI,
      RJ,
      RK,
      QI,
      QJ,
      QK,
      RED,
      GREEN,
      BLUE,
      ALPHA,
      MOUNTSIZE,
      WEAPON,
      DEFENSE,
      ARMOR,
      FORWARD,
      RETRO,
      FRONT,
      BACK,
      LEFT,
      RIGHT,
      TOP,
      BOTTOM,
      SHIELDS,
      RECHARGE,
      LEAK,
      HULL,
      STRENGTH,
      STATS,
      MASS,
      MOMENTOFINERTIA,
      FUEL,
      THRUST,
      MANEUVER,
      YAW,
      ROLL,
      PITCH,
      ENGINE,
      COMPUTER,
      AACCEL,
      ENERGY,
      REACTOR,
      LIMIT,
      RESTRICTED,
      MAX,
      MIN,
      MAXSPEED,
      AFTERBURNER,
      SHIELDTIGHT,
      ITTS,
      AMMO,
      HUDIMAGE,
      SOUND,
      MINTARGETSIZE,
      MAXCONE,
      RANGE,
      ISCOLOR,
      RADAR,
      CLOAK,
      CLOAKRATE,
      CLOAKMIN,
      CLOAKENERGY,
      CLOAKGLASS,
      CLOAKWAV,
      CLOAKMP3,
      ENGINEWAV,
      ENGINEMP3,
      HULLWAV,
      HULLMP3,
      ARMORWAV,
      ARMORMP3,
      SHIELDWAV,
      SHIELDMP3,
      EXPLODEWAV,
      EXPLODEMP3,
      COCKPIT,
      JUMP,
      DELAY,
      JUMPENERGY,
      JUMPWAV,
      DOCKINTERNAL,
      WORMHOLE,
      RAPID,
      USEBSP,
      AFTERBURNENERGY,
      MISSING,
      UNITSCALE,
      PRICE,
      VOLUME,
      QUANTITY,
      CARGO,
      HOLD,
      CATEGORY,
      IMPORT
    };

  const EnumMap::Pair element_names[] = {
    EnumMap::Pair ("UNKNOWN", UNKNOWN),
    EnumMap::Pair ("Unit", UNIT),
    EnumMap::Pair ("SubUnit", SUBUNIT),
    EnumMap::Pair ("Sound", SOUND),
    EnumMap::Pair ("MeshFile", MESHFILE),
    EnumMap::Pair ("ShieldMesh",SHIELDMESH),
    EnumMap::Pair ("BspMesh",BSPMESH),
    EnumMap::Pair ("Light",MESHLIGHT),
    EnumMap::Pair ("Defense", DEFENSE),
    EnumMap::Pair ("Armor", ARMOR),
    EnumMap::Pair ("Shields", SHIELDS),
    EnumMap::Pair ("Hull", HULL),
    EnumMap::Pair ("Stats", STATS),
    EnumMap::Pair ("Thrust", THRUST),
    EnumMap::Pair ("Maneuver", MANEUVER),
    EnumMap::Pair ("Engine", ENGINE),
    EnumMap::Pair ("Computer",COMPUTER),
    EnumMap::Pair ("Cloak", CLOAK),
    EnumMap::Pair ("Energy", ENERGY),
    EnumMap::Pair ("Reactor", REACTOR),
    EnumMap::Pair ("Restricted", RESTRICTED),
    EnumMap::Pair ("Yaw", YAW),
    EnumMap::Pair ("Pitch", PITCH),
    EnumMap::Pair ("Roll", ROLL),
    EnumMap::Pair ("Mount", MOUNT),
    EnumMap::Pair ("Radar", RADAR),
    EnumMap::Pair ("Cockpit", COCKPIT),
    EnumMap::Pair ("Jump", JUMP),
    EnumMap::Pair ("Dock", DOCK),
    EnumMap::Pair ("Hold",HOLD),
    EnumMap::Pair ("Cargo",CARGO),
    EnumMap::Pair ("Category",CATEGORY),
    EnumMap::Pair ("Import",IMPORT)

  };
  const EnumMap::Pair attribute_names[] = {
    EnumMap::Pair ("UNKNOWN", UNKNOWN),
    EnumMap::Pair ("missing",MISSING),
    EnumMap::Pair ("file", XFILE), 
    EnumMap::Pair ("x", X), 
    EnumMap::Pair ("y", Y), 
    EnumMap::Pair ("z", Z), 
    EnumMap::Pair ("ri", RI), 
    EnumMap::Pair ("rj", RJ), 
    EnumMap::Pair ("rk", RK), 
    EnumMap::Pair ("qi", QI),     
    EnumMap::Pair ("qj", QJ),     
    EnumMap::Pair ("qk", QK),
    EnumMap::Pair ("red",RED),
    EnumMap::Pair ("green",GREEN),
    EnumMap::Pair ("blue",BLUE),    
    EnumMap::Pair ("alpha",ALPHA),
    EnumMap::Pair ("size", MOUNTSIZE),
    EnumMap::Pair ("forward",FORWARD),
    EnumMap::Pair ("retro", RETRO),    
    EnumMap::Pair ("front", FRONT),
    EnumMap::Pair ("back", BACK),
    EnumMap::Pair ("left", LEFT),
    EnumMap::Pair ("right", RIGHT),
    EnumMap::Pair ("top", TOP),
    EnumMap::Pair ("bottom", BOTTOM),
    EnumMap::Pair ("recharge", RECHARGE),
    EnumMap::Pair ("leak", LEAK),
    EnumMap::Pair ("strength", STRENGTH),
    EnumMap::Pair ("mass", MASS),
    EnumMap::Pair ("momentofinertia", MOMENTOFINERTIA),
    EnumMap::Pair ("fuel", FUEL),
    EnumMap::Pair ("yaw", YAW),
    EnumMap::Pair ("pitch", PITCH),
    EnumMap::Pair ("roll", ROLL),
    EnumMap::Pair ("accel", AACCEL),
    EnumMap::Pair ("limit", LIMIT),
    EnumMap::Pair ("max", MAX),
    EnumMap::Pair ("min", MIN),
    EnumMap::Pair ("weapon", WEAPON),
    EnumMap::Pair ("maxspeed", MAXSPEED),
    EnumMap::Pair ("afterburner", AFTERBURNER),
    EnumMap::Pair ("tightness",SHIELDTIGHT),
    EnumMap::Pair ("itts",ITTS),
    EnumMap::Pair ("ammo", AMMO),
    EnumMap::Pair ("HudImage",HUDIMAGE),
    EnumMap::Pair ("MaxCone",MAXCONE),
    EnumMap::Pair ("MinTargetSize",MINTARGETSIZE),
    EnumMap::Pair ("Range",RANGE),
    EnumMap::Pair ("EngineMp3",ENGINEMP3),
    EnumMap::Pair ("EngineWav",ENGINEWAV),
    EnumMap::Pair ("HullMp3",HULLMP3),
    EnumMap::Pair ("HullWav",HULLWAV),
    EnumMap::Pair ("ArmorMp3",ARMORMP3),
    EnumMap::Pair ("ArmorWav",ARMORWAV),
    EnumMap::Pair ("ShieldMp3",SHIELDMP3),
    EnumMap::Pair ("ShieldWav",SHIELDWAV),
    EnumMap::Pair ("ExplodeMp3",EXPLODEMP3),
    EnumMap::Pair ("ExplodeWav",EXPLODEWAV),
    EnumMap::Pair ("CloakRate",CLOAKRATE),
    EnumMap::Pair ("CloakGlass",CLOAKGLASS),
    EnumMap::Pair ("CloakEnergy",CLOAKENERGY),
    EnumMap::Pair ("CloakMin",CLOAKMIN),
    EnumMap::Pair ("CloakMp3",CLOAKMP3),
    EnumMap::Pair ("CloakWav",CLOAKWAV),
    EnumMap::Pair ("Color",ISCOLOR),
    EnumMap::Pair ("Restricted", RESTRICTED),
    EnumMap::Pair ("Delay", DELAY),
    EnumMap::Pair ("AfterburnEnergy", AFTERBURNENERGY),
    EnumMap::Pair ("JumpEnergy", JUMPENERGY),
    EnumMap::Pair ("JumpWav", JUMPWAV),
    EnumMap::Pair ("DockInternal", DOCKINTERNAL),
    EnumMap::Pair ("RAPID", RAPID),
    EnumMap::Pair ("BSP", USEBSP),
    EnumMap::Pair ("Wormhole", WORMHOLE),
    EnumMap::Pair ("Scale", UNITSCALE),
    EnumMap::Pair ("Price",PRICE),
    EnumMap::Pair ("Volume",VOLUME),
    EnumMap::Pair ("Quantity",QUANTITY)
};

  const EnumMap element_map(element_names, 33);
  const EnumMap attribute_map(attribute_names, 78);
}

using XMLSupport::EnumMap;
using XMLSupport::Attribute;
using XMLSupport::AttributeList;
using namespace UnitXML;

int parseMountSizes (const char * str) {
  char tmp[13][50];
  int ans = weapon_info::NOWEAP;
  int num= sscanf (str,"%s %s %s %s %s %s %s %s %s %s %s %s %s",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],tmp[12]);
  for (int i=0;i<num;i++) {
    ans |= lookupMountSize (tmp[i]);
  }
  return ans;
}

static short CLAMP_SHORT(float x) {return (short)(((x)>65536)?65536:((x)<0?0:(x)));}  
void Unit::beginElement(const string &name, const AttributeList &attributes) {
  Cargo carg;
  string filename;
  Vector P;
  int indx;
  Vector Q;
  Vector R;
  Vector pos;
  bool tempbool;
  float fbrltb[6];
  AttributeList::const_iterator iter;
  float halocolor[4];
  short ammo=-1;
  int mntsiz=weapon_info::NOWEAP;
  Names elem = (Names)element_map.lookup(name);
#define ADDTAGNAME(a) image->unitwriter->AddTag (a)
#define ADDTAG  image->unitwriter->AddTag (name)
#define ADDELEMNAME(a,b,c) image->unitwriter->AddElement(a,b,c)
#define ADDELEM(b,c) image->unitwriter->AddElement((*iter).name,b,c)
#define ADDDEFAULT image->unitwriter->AddElement((*iter).name,stringHandler,XMLType((*iter).value))
#define ADDELEMI(b) ADDELEM(intStarHandler,XMLType(&b))
#define ADDELEMF(b) ADDELEM(floatStarHandler,XMLType(&b))
  switch(elem) {
  case SHIELDMESH:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case XFILE:
	ADDELEM(stringHandler,(*iter).value);
	xml->shieldmesh =(new Mesh((*iter).value.c_str(), xml->unitscale, faction));
	break;
      case SHIELDTIGHT: 
	ADDDEFAULT;
	shieldtight = parse_float ((*iter).value);
	break;
      }
    }
    break;
  case BSPMESH:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    xml->hasBSP = false;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case XFILE:
	ADDDEFAULT;
	xml->bspmesh =(new Mesh((*iter).value.c_str(), xml->unitscale, faction));
	xml->hasBSP = true;	
	break;
      case RAPID:
	ADDDEFAULT;
	xml->hasColTree=parse_bool ((*iter).value);
	break;
      case USEBSP:
	ADDDEFAULT;
	xml->hasBSP=parse_bool ((*iter).value);
	break;
      }
    }
    break;
  case HOLD:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case VOLUME:
	ADDDEFAULT;
	image->cargo_volume=parse_float ((*iter).value);
	break;
      }
    }
    image->unitwriter->AddTag ("Category");
    image->unitwriter->AddElement ("file",Unit::cargoSerializer,XMLType ((int)0));
    image->unitwriter->EndTag ("Category");
    break;
  case IMPORT:
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case QUANTITY:
	carg.quantity=parse_int ((*iter).value);
	//import cargo from ze maztah liztz
	break;
      case PRICE:
	carg.price = parse_float ((*iter).value);
	break;
      }
    }
    ImportPartList (xml->cargo_category, carg.price, 0,carg.quantity,0);
    break;
  case CATEGORY:
    //this is autogenerated by the handler
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case XFILE:
	xml->cargo_category = (*iter).value;
	break;
      }
    }
    break;
  case CARGO:
    ///handling taken care of above;
    assert (xml->unitlevel>=2);
    xml->unitlevel++;
    carg.category = xml->cargo_category;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case QUANTITY:
	carg.quantity=parse_int ((*iter).value);
	break;
      case MASS:
	carg.mass=parse_float ((*iter).value);
	break;
      case VOLUME:
	carg.volume=parse_float ((*iter).value);
	break;
      case PRICE:
	carg.price=parse_float ((*iter).value);
	break;
      case XFILE:
	carg.content = ((*iter).value);
	break;
      }
    }
    if (carg.mass!=0)
      AddCargo (carg);
    break;
  case MESHFILE:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case XFILE:
	ADDDEFAULT;
	xml->meshes.push_back(new Mesh((*iter).value.c_str(), xml->unitscale, faction));
	break;
      }
    }
    break;
  case DOCK:
    ADDTAG;
    tempbool=false;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    pos=Vector(0,0,0);
    P=Vector (1,1,1);
    Q=Vector (FLT_MAX,FLT_MAX,FLT_MAX);
    R=Vector (FLT_MAX,FLT_MAX,FLT_MAX);

    for (iter = attributes.begin();iter!=attributes.end();iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case DOCKINTERNAL:
	ADDDEFAULT;
	tempbool=parse_bool ((*iter).value);
	break;
      case X:
	ADDDEFAULT;
	pos.i=xml->unitscale*parse_float((*iter).value);
	break;
      case Y:
	ADDDEFAULT;
	pos.j=xml->unitscale*parse_float((*iter).value);
	break;
      case Z:
	ADDDEFAULT;
	pos.k=xml->unitscale*parse_float((*iter).value);
	break;
      case TOP:
	ADDDEFAULT;
	R.j=xml->unitscale*parse_float((*iter).value);
	break;
      case BOTTOM:
	ADDDEFAULT;
	Q.j=xml->unitscale*parse_float((*iter).value);
	break;
      case LEFT:
	ADDDEFAULT;
	Q.i=xml->unitscale*parse_float((*iter).value);
	break;
      case RIGHT:
	ADDDEFAULT;
	R.i=parse_float((*iter).value);
	break;
      case BACK:
	ADDDEFAULT;
	Q.k=xml->unitscale*parse_float((*iter).value);
	break;
      case FRONT:
	ADDDEFAULT;
	R.k=xml->unitscale*parse_float((*iter).value);
	break;
      case MOUNTSIZE:
	ADDDEFAULT;
	P.i=xml->unitscale*parse_float((*iter).value);
	P.j=xml->unitscale*parse_float((*iter).value);
	break;
      }
    }
    if (Q.i==FLT_MAX||Q.j==FLT_MAX||Q.k==FLT_MAX||R.i==FLT_MAX||R.j==FLT_MAX||R.k==FLT_MAX) {
      image->dockingports.push_back (DockingPorts(pos,P.i,tempbool));
    }else {
      Vector tQ = Q.Min (R);
      Vector tR = R.Max (Q);
      image->dockingports.push_back (DockingPorts (tQ,tR,tempbool));
    }
    break;
  case MESHLIGHT:
    ADDTAG;
    vs_config->gethColor ("unit","engine",halocolor,0xffffffff);
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    P=Vector (1,1,1);
    Q=Vector (1,1,1);
    pos=Vector(0,0,0);
    for (iter = attributes.begin();iter!=attributes.end();iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case X:
	ADDDEFAULT;
	pos.i=xml->unitscale*parse_float((*iter).value);
	break;
      case Y:
	ADDDEFAULT;
	pos.j=xml->unitscale*parse_float((*iter).value);
	break;
      case Z:
	ADDDEFAULT;
	pos.k=xml->unitscale*parse_float((*iter).value);
	break;
      case RED:
	ADDDEFAULT;
	halocolor[0]=parse_float((*iter).value);
	break;
      case GREEN:
	ADDDEFAULT;
       	halocolor[1]=parse_float((*iter).value);
	break;
      case BLUE:
	ADDDEFAULT;
	halocolor[2]=parse_float((*iter).value);
	break;
      case ALPHA:
	ADDDEFAULT;
	halocolor[3]=parse_float((*iter).value);
	break;
      case XFILE:
	ADDDEFAULT;
	filename = (*iter).value;
	break;
      case MOUNTSIZE:
	ADDDEFAULT;
	P.i=xml->unitscale*parse_float((*iter).value);
	P.j=xml->unitscale*parse_float((*iter).value);
	break;
      }
    }
   xml->halos.push_back(new Halo(filename.c_str(),GFXColor(halocolor[0],halocolor[1],halocolor[2],halocolor[3]),pos,P.i,P.j));
    break;
  case MOUNT:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    Q = Vector (0,1,0);
    R = Vector (0,0,1);
    pos = Vector (0,0,0);
    tempbool=false;
    ADDELEMNAME("size",Unit::mountSerializer,XMLType(XMLSupport::tostring(xml->unitscale),(int)xml->mountz.size()));
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case WEAPON:
	filename = (*iter).value;
	break;
      case AMMO:
	ammo = parse_int ((*iter).value);
	break;
      case MOUNTSIZE:
	tempbool=true;
	mntsiz=parseMountSizes((*iter).value.c_str());
	break;
      case X:
	pos.i=xml->unitscale*parse_float((*iter).value);
	break;
      case Y:
	pos.j=xml->unitscale*parse_float((*iter).value);
	break;
      case Z:
	pos.k=xml->unitscale*parse_float((*iter).value);
	break;
      case RI:
	R.i=parse_float((*iter).value);
	break;
      case RJ:
	R.j=parse_float((*iter).value);
	break;
      case RK:
	R.k=parse_float((*iter).value);
	break;
      case QI:
	Q.i=parse_float((*iter).value);
	break;
      case QJ:
	Q.j=parse_float((*iter).value);
	break;
      case QK:
	Q.k=parse_float((*iter).value);
	break;
      }

    }
    Q.Normalize();
    if (fabs(Q.i)==fabs(R.i)&&fabs(Q.j)==fabs(R.j)&&fabs(Q.k)==fabs(R.k)){
      Q.i=-1;
      Q.j=0;
      Q.k=0;
    }
    R.Normalize();
    
    CrossProduct (Q,R,P);
    CrossProduct (R,P,Q);
    Q.Normalize();
    //Transformation(Quaternion (from_vectors (P,Q,R),pos);
    indx = xml->mountz.size();
    xml->mountz.push_back(new Mount (filename.c_str(), ammo));
    xml->mountz[indx]->SetMountPosition(Transformation(Quaternion::from_vectors(P,Q,R),pos));
    //xml->mountz[indx]->Activate();
    if (tempbool)
      xml->mountz[indx]->size=mntsiz;
    else
      xml->mountz[indx]->size = xml->mountz[indx]->type.size;
    //->curr_physical_state=xml->units[indx]->prev_physical_state;

    break;

  case SUBUNIT:
    ADDTAG;
    assert (xml->unitlevel==1);
    ADDELEMNAME("file",Unit::subunitSerializer,XMLType((int)xml->units.size()));
    xml->unitlevel++;
    Q = Vector (0,1,0);
    R = Vector (0,0,1);
    pos = Vector (0,0,0);
    fbrltb[0] =-1;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case XFILE:
	filename = (*iter).value;
	break;
      case X:
	ADDDEFAULT;
	pos.i=xml->unitscale*parse_float((*iter).value);
	break;
      case Y:
	ADDDEFAULT;
	pos.j=xml->unitscale*parse_float((*iter).value);
	break;
      case Z:
	ADDDEFAULT;
	pos.k=xml->unitscale*parse_float((*iter).value);
	break;
      case RI:
	ADDDEFAULT;
	R.i=parse_float((*iter).value);
	break;
      case RJ:
	ADDDEFAULT;
	R.j=parse_float((*iter).value);
	break;
      case RK:
	ADDDEFAULT;
	R.k=parse_float((*iter).value);
	break;
      case QI:
	ADDDEFAULT;
	Q.i=parse_float((*iter).value);
	break;
      case QJ:
	ADDDEFAULT;
	Q.j=parse_float((*iter).value);
	break;
      case QK:
	ADDDEFAULT;
	Q.k=parse_float((*iter).value);
	break;
      case RESTRICTED:
	ADDDEFAULT;
	fbrltb[0]=parse_float ((*iter).value);//minimum dot turret can have with "fore" vector 
	break;
      }

    }
    Q.Normalize();
    R.Normalize();
    
    CrossProduct (Q,R,P);
    indx = xml->units.size();
    xml->units.push_back(new Unit (filename.c_str(),true,faction,xml->unitModifications,NULL)); // I set here the fg arg to NULL
    xml->units.back()->SetRecursiveOwner (this);
    xml->units[indx]->prev_physical_state= Transformation(Quaternion::from_vectors(P,Q,R),pos);
    xml->units[indx]->curr_physical_state=xml->units[indx]->prev_physical_state;
    xml->units[indx]->limits.structurelimits=R;
    xml->units[indx]->limits.limitmin=fbrltb[0];
    
    break;
  case JUMP:
    //serialization covered in LoadXML
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    jump.drive = -1;//activate the jump unit
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case MISSING:
	//serialization covered in LoadXML
	if (parse_bool((*iter).value))
	  jump.drive=-2;
	break;
      case JUMPENERGY:
	//serialization covered in LoadXML
	jump.energy = CLAMP_SHORT(parse_float((*iter).value));
	break;
      case DELAY:
	//serialization covered in LoadXML
	jump.delay = parse_int ((*iter).value);
	break;
      case FUEL:
	//serialization covered in LoadXML
	jump.energy = -CLAMP_SHORT (parse_float((*iter).value));
	break;
      case WORMHOLE:
	//serialization covered in LoadXML
	image->forcejump=parse_bool ((*iter).value);
	if (image->forcejump)
	  jump.drive=-2;
	break;
      }
    }
    break;
  case SOUND:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case CLOAKWAV:
	ADDDEFAULT;
	sound->cloak = AUDCreateSoundWAV ((*iter).value,false);
	break;
      case JUMPWAV:
	ADDDEFAULT;
	sound->jump = AUDCreateSoundWAV ((*iter).value,false);
	break;
      case CLOAKMP3:
	ADDDEFAULT;
	sound->cloak = AUDCreateSoundMP3 ((*iter).value,false);
	break;
      case ENGINEWAV:
	ADDDEFAULT;
	sound->engine = AUDCreateSoundWAV ((*iter).value,true);
	break;
      case ENGINEMP3:
	ADDDEFAULT;
	sound->engine = AUDCreateSoundMP3((*iter).value,true); 
	break;
      case SHIELDMP3:
	ADDDEFAULT;
	sound->shield = AUDCreateSoundMP3((*iter).value,false); 
	break;
      case SHIELDWAV:
	ADDDEFAULT;
	sound->shield = AUDCreateSoundWAV((*iter).value,false); 
	break;
      case EXPLODEMP3:
	ADDDEFAULT;
	sound->explode = AUDCreateSoundMP3((*iter).value,false); 
	break;
      case EXPLODEWAV:
	ADDDEFAULT;
	sound->explode = AUDCreateSoundWAV((*iter).value,false); 
	break;
      case ARMORMP3:
	ADDDEFAULT;
	sound->armor = AUDCreateSoundMP3((*iter).value,false); 
	break;
      case ARMORWAV:
	ADDDEFAULT;
	sound->armor = AUDCreateSoundWAV((*iter).value,false); 
	break;
      case HULLWAV:
	ADDDEFAULT;
	sound->hull = AUDCreateSoundWAV((*iter).value,false); 
	break;
      case HULLMP3:
	ADDDEFAULT;
	sound->hull = AUDCreateSoundMP3((*iter).value,false); 
	break;
      }
    }
    if (sound->cloak==-1) {
      sound->cloak=AUDCreateSound(vs_config->getVariable ("unitaudio","cloak", "sfx43.wav"),false);
    }
    if (sound->engine==-1) {
      sound->engine=AUDCreateSound (vs_config->getVariable ("unitaudio","afterburner","sfx10.wav"),true);
    }
    if (sound->shield==-1) {
      sound->shield=AUDCreateSound (vs_config->getVariable ("unitaudio","shield","sfx09.wav"),false);
    }
    if (sound->armor==-1) {
      sound->armor=AUDCreateSound (vs_config->getVariable ("unitaudio","armor","sfx08.wav"),false);
    }
    if (sound->hull==-1) {
      sound->hull=AUDCreateSound (vs_config->getVariable ("unitaudio","armor","sfx08.wav"),false);
    }
    if (sound->explode==-1) {
      sound->explode=AUDCreateSound (vs_config->getVariable ("unitaudio","explode","sfx03.wav"),false);
    }
      
    break;    
  case CLOAK:
    //serialization covered elsewhere
    assert (xml->unitlevel==2);
    xml->unitlevel++;
    image->cloakrate=(short int)(.2*32767);
    cloakmin=1;
    image->cloakenergy=0;
    cloaking = (short) 32768;//lowest negative number
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case MISSING:
	//serialization covered in LoadXML
	if (parse_bool((*iter).value))
	  cloaking=(short)-1;
	break;
      case CLOAKMIN:
	//serialization covered in LoadXML
	cloakmin = (short int)(32767*parse_float ((*iter).value));
	break;
      case CLOAKGLASS:
	//serialization covered in LoadXML
	image->cloakglass=parse_bool ((*iter).value);
	break;
      case CLOAKRATE:
	//serialization covered in LoadXML
	image->cloakrate = (short int)(32767*parse_float ((*iter).value));
	break;
      case CLOAKENERGY:
	//serialization covered in LoadXML
	image->cloakenergy = parse_float ((*iter).value);
	break;
      }
    }
    if ((cloakmin&0x1)&&!image->cloakglass) {
      cloakmin-=1;
    }
    if ((cloakmin&0x1)==0&&image->cloakglass) {
      cloakmin+=1;
    }
    break;
  case ARMOR:
	assert (xml->unitlevel==2);
	xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case FRONT:
	//serialization covered in LoadXML
	armor.front = CLAMP_SHORT(parse_float((*iter).value));
	break;
      case BACK:
	//serialization covered in LoadXML
	armor.back= CLAMP_SHORT(parse_float((*iter).value));
	break;
      case LEFT:
	//serialization covered in LoadXML
	armor.left= CLAMP_SHORT(parse_float((*iter).value));
	break;
      case RIGHT:
	//serialization covered in LoadXML
	armor.right= CLAMP_SHORT(parse_float((*iter).value));
	break;
      }
    }

 
    break;
  case SHIELDS:
	//serialization covered in LoadXML
    assert (xml->unitlevel==2);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case FRONT:
	//serialization covered in LoadXML
	fbrltb[0] = parse_float((*iter).value);
	shield.number++;
	break;
      case BACK:
	//serialization covered in LoadXML
	fbrltb[1]=parse_float((*iter).value);
	shield.number++;
	break;
      case LEFT:
	//serialization covered in LoadXML
	fbrltb[3]=parse_float((*iter).value);
	shield.number++;
	break;
      case RIGHT:
	//serialization covered in LoadXML
	fbrltb[2]=parse_float((*iter).value);
	shield.number++;
	break;
      case TOP:
	//serialization covered in LoadXML
	fbrltb[4]=parse_float((*iter).value);
	shield.number++;
	break;
      case BOTTOM:
	//serialization covered in LoadXML
	fbrltb[5]=parse_float((*iter).value);
	shield.number++;
	break;
      case RECHARGE:
	//serialization covered in LoadXML
	shield.recharge=parse_float((*iter).value);
	break;
      case LEAK:
	//serialization covered in LoadXML
	shield.leak = parse_int ((*iter).value);
	break;
      }
    }
    if (fbrltb[0]>65535||fbrltb[1]>65535)
      shield.number=2;

    switch (shield.number) {
    case 2:
      shield.fb[2]=shield.fb[0]=fbrltb[0];
      shield.fb[3]=shield.fb[1]=fbrltb[1];
      break;
    case 6:
      shield.fbrltb.v[0]=CLAMP_SHORT(fbrltb[0]);
      shield.fbrltb.v[1]=CLAMP_SHORT(fbrltb[1]);
      shield.fbrltb.v[2]=CLAMP_SHORT(fbrltb[2]);
      shield.fbrltb.v[3]=CLAMP_SHORT(fbrltb[3]);
      shield.fbrltb.v[4]=CLAMP_SHORT(fbrltb[4]);
      shield.fbrltb.v[5]=CLAMP_SHORT(fbrltb[5]);
      shield.fbrltb.fbmax= CLAMP_SHORT((fbrltb[0]+fbrltb[1])*.5);
      shield.fbrltb.rltbmax= CLAMP_SHORT((fbrltb[2]+fbrltb[3]+fbrltb[4]+fbrltb[5])*.25);
      
      break;
    case 4:
    default:
      shield.fbrl.frontmax = shield.fbrl.front = CLAMP_SHORT(fbrltb[0]);
      shield.fbrl.backmax = shield.fbrl.back = CLAMP_SHORT(fbrltb[1]);
      shield.fbrl.rightmax = shield.fbrl.right = CLAMP_SHORT(fbrltb[2]);
      shield.fbrl.leftmax = shield.fbrl.left = CLAMP_SHORT(fbrltb[3]);
    }

    break;
  case HULL:

	assert (xml->unitlevel==2);
	xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case STRENGTH:
	hull = parse_float((*iter).value);
	break;
      }
    }
   
    break;
  case STATS:
	assert (xml->unitlevel==1);
	xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case MASS:
	mass = parse_float((*iter).value);
	break;
      case MOMENTOFINERTIA:
	MomentOfInertia=parse_float((*iter).value);
	break;
      case FUEL:
	fuel=parse_float((*iter).value);
	break;
      }
    }
	break;
  case MANEUVER:
	assert (xml->unitlevel==2);
	xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case YAW:
	limits.yaw = parse_float((*iter).value)*(VS_PI/180);
	break;
      case PITCH:
	limits.pitch=parse_float((*iter).value)*(VS_PI/180);
	break;
      case ROLL:
	limits.roll=parse_float((*iter).value)*(VS_PI/180);
	break;
      }
    }

    
    break;

  case ENGINE:
	  
	assert (xml->unitlevel==2);
	xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case AACCEL:
	//accel=parse_float((*iter).value);
	break;
      case FORWARD:
	limits.forward=parse_float((*iter).value);
	break;
      case RETRO:
	limits.retro=parse_float((*iter).value);
	break;
      case AFTERBURNER:
	limits.afterburn=parse_float ((*iter).value);
	break;
      case LEFT:
	limits.lateral=parse_float((*iter).value);
	break;
      case RIGHT:
	limits.lateral=parse_float((*iter).value);
	break;
      case TOP:
	limits.vertical=parse_float((*iter).value);
	break;
      case BOTTOM:
	limits.vertical=parse_float((*iter).value);
	break;
    }
    }

    break;

  case COMPUTER:  
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case MAXSPEED:
	computer.max_speed=parse_float((*iter).value);
	ADDELEMF (computer.max_speed);
	break;
      case AFTERBURNER:
	computer.max_ab_speed=parse_float((*iter).value);
	ADDELEMF (computer.max_ab_speed);
	break;
      case YAW:
	computer.max_yaw=parse_float((*iter).value)*(VS_PI/180);
	ADDELEM (angleStarHandler, XMLType(&computer.max_yaw));
	break;
      case PITCH:
	computer.max_pitch=parse_float((*iter).value)*(VS_PI/180);
	ADDELEM (angleStarHandler,XMLType(&computer.max_pitch));
	break;
      case ROLL:
	computer.max_roll=parse_float((*iter).value)*(VS_PI/180);
	ADDELEM (angleStarHandler,XMLType(&computer.max_roll));
	break;
      }
    }
    image->unitwriter->AddTag ("Radar");    
    ADDELEMNAME("itts",charStarHandler,XMLType(&computer.itts));    
   ADDELEMNAME("color",charStarHandler,XMLType(&computer.radar.color));    
    ADDELEMNAME("mintargetsize",charStarHandler,XMLType(&computer.radar.mintargetsize));    
    ADDELEMNAME("range",floatStarHandler,XMLType(&computer.radar.maxrange));    
    ADDELEMNAME("maxcone",floatStarHandler,XMLType(&computer.radar.maxcone));    
    image->unitwriter->EndTag ("Radar");    
    break;
  case RADAR:
    //handled above
    assert (xml->unitlevel==2);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case ITTS:
	computer.itts=parse_bool ((*iter).value);
	break;
      case MINTARGETSIZE:
	computer.radar.mintargetsize=parse_float ((*iter).value);
	break;
      case MAXCONE:
	computer.radar.maxcone = parse_float ((*iter).value);
	break;
      case RANGE:
	computer.radar.maxrange = parse_float ((*iter).value);
	break;
      case ISCOLOR:
	computer.radar.color=parse_bool ((*iter).value);
	break;
      }
    }
    break;
  case REACTOR:
    ADDTAG;
    assert (xml->unitlevel==2);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case RECHARGE:
	ADDELEMF(recharge);
	recharge=parse_float((*iter).value);
	break;
      case LIMIT:
	ADDELEM(ushortStarHandler,&maxenergy);
	maxenergy=energy=CLAMP_SHORT(parse_float((*iter).value));
	break;
    }
    }
    break;

  case YAW:
    ADDTAG;
    xml->yprrestricted+=Unit::XML::YRESTR;
    assert (xml->unitlevel==2);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case MAX:
	ADDDEFAULT;
	xml->ymax=parse_float((*iter).value)*(VS_PI/180);
	break;
      case MIN:
	ADDDEFAULT;
	xml->ymin=parse_float((*iter).value)*(VS_PI/180);
	break;
    }
    }
    break;

  case PITCH:
    ADDTAG;
    xml->yprrestricted+=Unit::XML::PRESTR;
    assert (xml->unitlevel==2);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case MAX:
	ADDDEFAULT;
	xml->pmax=parse_float((*iter).value)*(VS_PI/180);
	break;
      case MIN:
	ADDDEFAULT;
	xml->pmin=parse_float((*iter).value)*(VS_PI/180);
	break;
      }
    }
    break;

  case ROLL:
    ADDTAG;
    xml->yprrestricted+=Unit::XML::RRESTR;
    assert (xml->unitlevel==2);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case MAX:
	ADDDEFAULT;
	xml->rmax=parse_float((*iter).value)*(VS_PI/180);
	break;
      case MIN:
	ADDDEFAULT;
	xml->rmin=parse_float((*iter).value)*(VS_PI/180);
	break;
      }
    }
    break;

  case UNIT:

    assert (xml->unitlevel==0);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      default:
	break;
      case UNITSCALE:
	xml->unitscale=parse_float((*iter).value);
	break;
      case COCKPIT:
	fprintf (stderr,"Cockpit attrib deprecated use tag");
	break;
      }
    }
    break;
  case COCKPIT:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;

    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case XFILE:
	image->cockpitImage = (*iter).value;
	ADDELEM(stringStarHandler,XMLType (&image->cockpitImage));
	break;
      case X:
	image->CockpitCenter.i =xml->unitscale*parse_float ((*iter).value);
	ADDELEM(scaledFloatStarHandler,XMLType(tostring(xml->unitscale),&image->CockpitCenter.i));
	break;
      case Y:
	image->CockpitCenter.j =xml->unitscale*parse_float ((*iter).value);
	ADDELEM(scaledFloatStarHandler,XMLType(tostring(xml->unitscale),&image->CockpitCenter.j));
	break;
      case Z:
	image->CockpitCenter.k =xml->unitscale*parse_float ((*iter).value);
	ADDELEM(scaledFloatStarHandler,XMLType(tostring(xml->unitscale),&image->CockpitCenter.k));
	break;
      }
    }
    break;
  case DEFENSE:
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case HUDIMAGE:
	if ((*iter).value.length()){
	  image->hudImage = new Sprite ((*iter).value.c_str());
	  xml->hudimage=(*iter).value;
	}
	break;
      default:
	break;
      }
    }

    break;

  case THRUST:
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    break;

  case ENERGY:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case AFTERBURNENERGY:
	afterburnenergy =CLAMP_SHORT(parse_float((*iter).value)); 
	ADDELEM(ushortStarHandler,&afterburnenergy);
	break;
      default:
	break;
      }
    }

    break;

  case RESTRICTED:
    ADDTAG;
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    break;
	
  case UNKNOWN:
    ADDTAG;
  default:
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      ADDDEFAULT;
    }
    xml->unitlevel++;
    break;
  }
#undef ADDELEM
}

void Unit::endElement(const string &name) {
  image->unitwriter->EndTag (name);
  Names elem = (Names)element_map.lookup(name);

  switch(elem) {
  case UNKNOWN:

	  xml->unitlevel--;
//    cerr << "Unknown element end tag '" << name << "' detected " << endl;
    break;
  default:
	  xml->unitlevel--;
    break;
  }
}

std::string Unit::shieldSerializer (const XMLType &input, void * mythis) {
  Unit * un=(Unit *)mythis;
  switch (un->shield.number) {
  case 2:
    return tostring(un->shield.fb[2])+string("\" back=\"")+tostring(un->shield.fb[3]);
  case 6:
    return tostring(un->shield.fbrltb.fbmax)+string("\" back=\"")+tostring(un->shield.fbrltb.fbmax)+string("\" left=\"")+tostring(un->shield.fbrltb.rltbmax)+string("\" right=\"")+tostring(un->shield.fbrltb.rltbmax)+string("\" top=\"")+tostring(un->shield.fbrltb.rltbmax)+string("\" bottom=\"")+tostring(un->shield.fbrltb.rltbmax);
  case 4:
  default:
    return tostring(un->shield.fbrl.frontmax)+string("\" back=\"")+tostring(un->shield.fbrl.backmax)+string("\" left=\"")+tostring(un->shield.fbrl.leftmax)+string("\" right=\"")+tostring(un->shield.fbrl.rightmax);
  }
  return string("");
}

std::string Unit::mountSerializer (const XMLType &input, void * mythis) {
  Unit * un=(Unit *)mythis;
  int i=input.w.hardint;
  if (un->nummounts>i) {
    string result(lookupMountSize(un->mounts[i].size));
    if (un->mounts[i].status==Mount::INACTIVE||un->mounts[i].status==Mount::ACTIVE)
      result+=string("\" weapon=\"")+(un->mounts[i].type.weapon_name);
    if (un->mounts[i].ammo!=-1)
      result+=string("\" ammo=\"")+XMLSupport::tostring(un->mounts[i].ammo);
    
    Matrix m;
    un->mounts[i].GetMountLocation().to_matrix(m);
    result+=string ("\" x=\"")+tostring((float)(m[12]/parse_float(input.str)));
    result+=string ("\" y=\"")+tostring((float)(m[13]/parse_float(input.str)));
    result+=string ("\" z=\"")+tostring((float)(m[14]/parse_float(input.str)));

    result+=string ("\" qi=\"")+tostring(m[4]);
    result+=string ("\" qj=\"")+tostring(m[5]);
    result+=string ("\" qk=\"")+tostring(m[6]);
     
    result+=string ("\" ri=\"")+tostring(m[8]);    
    result+=string ("\" rj=\"")+tostring(m[9]);    
    result+=string ("\" rk=\"")+tostring(m[10]);    
    return result;
  }else {
    return string("");
  }
}
std::string Unit::subunitSerializer (const XMLType &input, void * mythis) {
  Unit * un=(Unit *)mythis;
  int index=input.w.hardint;
  Unit *su;
  int i=0;
  for (un_iter ui=un->getSubUnits();NULL!= (su=ui.current());++ui,++i) {
    if (i==index) {
      if (su->image->unitwriter) {
	return su->image->unitwriter->getName();
      }
      return su->name;
    }
  }
  return string("destroyed_turret");
}

void Unit::WriteUnit (const char * modifications) {
  if (image->unitwriter)
    image->unitwriter->Write(modifications);
  for (un_iter ui= getSubUnits();(*ui)!=NULL;++ui) {
    (*ui)->WriteUnit(modifications);
  } 
}

void Unit::LoadXML(const char *filename, const char * modifications)
{
  shield.number=0;
  const int chunk_size = 16384;
 // rrestricted=yrestricted=prestricted=false;
  FILE * inFile=NULL;
  std::string collideTreeHash = GetHashName(string(modifications)+"#"+string(filename));
  if (modifications) {
    if (strlen(modifications)!=0) {
      changehome();
      static std::string savedunitpath=vs_config->getVariable ("data","serialized_xml","serialized_xml");
      vschdir (savedunitpath.c_str());
      vschdir (modifications);
      inFile = fopen (filename,"r");
      vscdup();
      vscdup();
      returnfromhome();
    }
  }
  if (inFile==NULL)
    inFile = fopen (filename, "r");
  if(!inFile) {
    cout << "Unit file " << filename << " not found" << endl;
    assert(0);
    return;
  }
  image->unitwriter=new XMLSerializer (filename,modifications,this);
  image->unitwriter->AddTag ("Unit");
  string * myhudim = new string("");
  float * myscale=new float;//memory leak!
  image->unitwriter->AddElement("scale",floatStarHandler,XMLType(myscale));
  {
    image->unitwriter->AddTag ("Jump");
    image->unitwriter->AddElement("missing",lessNeg1Handler,XMLType(&jump.drive));
    image->unitwriter->AddElement("jumpenergy",shortStarHandler,XMLType(&jump.energy));
    image->unitwriter->AddElement("delay",ucharStarHandler,XMLType(&jump.delay));
    image->unitwriter->AddElement("damage",ucharStarHandler,XMLType(&jump.damage));
    image->unitwriter->AddElement("wormhole",ucharStarHandler,XMLType(&image->forcejump));
    image->unitwriter->EndTag ("Jump");
  }
  {
    image->unitwriter->AddTag("Defense");
    image->unitwriter->AddElement("HudImage",stringStarHandler,XMLType (myhudim));
    
    {
      image->unitwriter->AddTag ("Cloak");
      image->unitwriter->AddElement("missing",cloakHandler,XMLType(&cloaking));
      image->unitwriter->AddElement("cloakmin",shortToFloatHandler,XMLType(&cloakmin));
      image->unitwriter->AddElement("cloakglass",ucharStarHandler,XMLType(&image->cloakglass));
      image->unitwriter->AddElement("cloakrate",shortToFloatHandler,XMLType(&image->cloakrate));
      image->unitwriter->AddElement("cloakenergy",floatStarHandler,XMLType(&image->cloakenergy));
      image->unitwriter->EndTag ("Cloak");
    }
    {
      image->unitwriter->AddTag ("Armor");
      image->unitwriter->AddElement("front",ushortStarHandler,XMLType(&armor.front));
      image->unitwriter->AddElement("back",ushortStarHandler,XMLType(&armor.back));
      image->unitwriter->AddElement("left",ushortStarHandler,XMLType(&armor.left));
      image->unitwriter->AddElement("right",ushortStarHandler,XMLType(&armor.right));
      image->unitwriter->EndTag ("Armor");
    }    
    {
      image->unitwriter->AddTag ("Shields");
      image->unitwriter->AddElement("front",shieldSerializer,XMLType((void *)&shield));
      image->unitwriter->AddElement("recharge",floatStarHandler,XMLType(&shield.recharge));
      image->unitwriter->AddElement("leak",charStarHandler,XMLType(&shield.leak));
      image->unitwriter->EndTag ("Shields");      
    }
    {
      image->unitwriter->AddTag ("Hull");
      image->unitwriter->AddElement("strength",floatStarHandler,XMLType(&hull));
      image->unitwriter->EndTag ("Hull");
    }

    image->unitwriter->EndTag("Defense");
  }
  {
    image->unitwriter->AddTag ("Stats");    
    image->unitwriter->AddElement("mass",massSerializer,XMLType(&mass));
    image->unitwriter->AddElement("momentofinertia",floatStarHandler,XMLType(&MomentOfInertia));
    image->unitwriter->AddElement("fuel",floatStarHandler,XMLType(&fuel));
    image->unitwriter->EndTag ("Stats");    
    image->unitwriter->AddTag ("Thrust");    
    {
      image->unitwriter->AddTag ("Maneuver");    
      image->unitwriter->AddElement("yaw",angleStarHandler,XMLType(&limits.yaw));      
      image->unitwriter->AddElement("pitch",angleStarHandler,XMLType(&limits.pitch));      
      image->unitwriter->AddElement("roll",angleStarHandler,XMLType(&limits.roll));      
      image->unitwriter->EndTag ("Maneuver");    
    }
    {
      image->unitwriter->AddTag ("Engine");    
      image->unitwriter->AddElement("forward",floatStarHandler,XMLType(&limits.forward));      
      image->unitwriter->AddElement("retro",floatStarHandler,XMLType(&limits.retro));      
      image->unitwriter->AddElement("left",floatStarHandler,XMLType(&limits.lateral));     
      image->unitwriter->AddElement("right",floatStarHandler,XMLType(&limits.lateral));      
      image->unitwriter->AddElement("top",floatStarHandler,XMLType(&limits.vertical));      
      image->unitwriter->AddElement("bottom",floatStarHandler,XMLType(&limits.vertical));      
      image->unitwriter->AddElement("afterburner",floatStarHandler,XMLType(&limits.afterburn));      
      image->unitwriter->EndTag ("Engine");    
    }
    image->unitwriter->EndTag ("Thrust");    

  }
  image->CockpitCenter.Set (0,0,0);
  xml = new XML;
  xml->unitModifications = modifications;
  xml->shieldmesh = NULL;
  xml->bspmesh = NULL;
  xml->hasBSP = true;
  xml->hasColTree=true;
  xml->unitlevel=0;
  xml->unitscale=1;
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, &Unit::beginElement, &Unit::endElement);
  
  do {
#ifdef BIDBG
    char *buf = (XML_Char*)XML_GetBuffer(parser, chunk_size);
#else
    char buf[chunk_size];
#endif
    int length;
    length = fread (buf,1, chunk_size,inFile);
    //length = inFile.gcount();
#ifdef BIDBG
    XML_ParseBuffer(parser, length, feof(inFile));
#else
    XML_Parse (parser,buf,length,feof(inFile));
#endif
  } while(!feof(inFile));
  fclose (inFile);
  XML_ParserFree (parser);
  // Load meshes into subunit
  image->unitwriter->EndTag ("Unit");
  nummesh = xml->meshes.size();
  corner_min = Vector(FLT_MAX, FLT_MAX, FLT_MAX);
  corner_max = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  *myhudim =xml->hudimage;
  int a;
  meshdata = new Mesh*[nummesh+1];
  for(a=0; a < nummesh; a++) {
    meshdata[a] = xml->meshes[a];
  }
  numhalos = xml->halos.size();
  if(numhalos)
    halos = new Halo*[numhalos];
  else
    halos=NULL;
  for(a=0; a < numhalos; a++) {
    halos[a]=xml->halos[a];
  }
  nummounts = xml->mountz.size();
  if (nummounts)
    mounts = new Mount [nummounts];
  else
    mounts = NULL;
  char parity=0;
  for(a=0; a < nummounts; a++) {
    mounts[a]=*xml->mountz[a];
    delete xml->mountz[a];			//do it stealthily... no cons/destructor
  }
  for (a=0;a<nummounts;a++) {
    static bool half_sounds = XMLSupport::parse_bool(vs_config->getVariable ("audio","every_other_mount","false"));
    if (a%2==parity) {
      int b=a;
      if(a % 4 == 2 && a < (nummounts-1)) 
	if (mounts[a].type.type != weapon_info::PROJECTILE&&mounts[a+1].type.type != weapon_info::PROJECTILE)
	  b=a+1;
      mounts[b].sound = AUDCreateSound (mounts[b].type.sound,mounts[b].type.type!=weapon_info::PROJECTILE);
    } else if ((!half_sounds)||mounts[a].type.type == weapon_info::PROJECTILE) {
      mounts[a].sound = AUDCreateSound (mounts[a].type.sound,false);      
    }
  }
  for( a=0; a<(int)xml->units.size(); a++) {
    SubUnits.prepend(xml->units[a]);
  }
  calculate_extent();
  if (!SubUnit) {
    UpdateCollideQueue();
  }
  *myscale=xml->unitscale;
  string tmpname (filename);
  vector <bsp_polygon> polies;

  colTrees = collideTrees::Get(collideTreeHash);
  if (colTrees) {
    colTrees->Inc();
  }
  BSPTree * bspTree=NULL;
  BSPTree * bspShield=NULL;
  csRapidCollider *colShield=NULL;
  csRapidCollider *colTree=NULL;
  if (xml->shieldmesh) {
    meshdata[nummesh] = xml->shieldmesh;
    if (!colTrees) {
      if (!CheckBSP ((tmpname+"_shield.bsp").c_str())) {
	BuildBSPTree ((tmpname+"_shield.bsp").c_str(), false, meshdata[nummesh]);
      }
      if (CheckBSP ((tmpname+"_shield.bsp").c_str())) {
	bspShield = new BSPTree ((tmpname+"_shield.bsp").c_str());
      }
      if (meshdata[nummesh]) {
	meshdata[nummesh]->GetPolys(polies);
	colShield = new csRapidCollider (polies);
      }
    }
  }
  else {
    SphereMesh * tmp = NULL;
    if (!colTrees) {
#if 0
      tmp= new SphereMesh (rSize(),8,8,"shield.bmp", NULL, false,ONE, ONE);///shield not used right now for collisions
      tmp->GetPolys (polies);
      if (xml->hasColTree)
	colShield = new csRapidCollider (polies);
      else
#endif
	colShield=NULL;
    }
    static int shieldstacks = XMLSupport::parse_int (vs_config->getVariable ("graphics","shield_detail","16"));
    tmp = new SphereMesh (rSize(),shieldstacks,shieldstacks,"shield.bmp", NULL, false,ONE, ONE);
    
    meshdata[nummesh] = tmp;
    bspShield=NULL;
    colShield=NULL;
  }
  meshdata[nummesh]->EnableSpecialFX();
  if (!colTrees) {
    if (xml->hasBSP) {
      tmpname += ".bsp";
      if (!CheckBSP (tmpname.c_str())) {
	BuildBSPTree (tmpname.c_str(), false, xml->bspmesh);
      }
      if (CheckBSP (tmpname.c_str())) {
	bspTree = new BSPTree (tmpname.c_str());
      }	
    } else {
      bspTree = NULL;
    }
    polies.clear();  
    if (!xml->bspmesh) {
      for (int j=0;j<nummesh;j++) {
	meshdata[j]->GetPolys(polies);
      }
    }else {
      xml->bspmesh->GetPolys (polies);
    }
    if (xml->hasColTree ) {
      colTree = new csRapidCollider (polies);    
    }else {
      colTree=NULL;
    }
    colTrees = new collideTrees (collideTreeHash,bspTree,bspShield,colTree,colShield);
  }
  if (xml->bspmesh) {
    delete xml->bspmesh;
  }
  delete xml;
}
