#include <expat.h>
#include "xml_support.h"
#include "star_system.h"
#include "gfx/background.h"
#include "gfx/env_map_gent.h"
#include "gfx/aux_texture.h"
#include "cmd/planet.h"
#include "cmd/unit_factory.h"
#include "gfx/star.h"
#include "vs_globals.h"
#include "vs_path.h"
#include "config_xml.h"
#include "vegastrike.h"
#include "cmd/cont_terrain.h"
#include <assert.h>	/// needed for assert() calls.
#include "gfx/mesh.h"
#include "cmd/building.h"
#include "cmd/ai/aggressive.h"
#include "cmd/ai/fire.h"
#include "cmd/atmosphere.h"
#include "cmd/nebula.h"
#include "cmd/asteroid.h"
#include "cmd/enhancement.h"
#include "main_loop.h"
#include "cmd/script/flightgroup.h"
void StarSystem::beginElement(void *userData, const XML_Char *name, const XML_Char **atts) {
  ((StarSystem*)userData)->beginElement(name, AttributeList(atts));
}

void StarSystem::endElement(void *userData, const XML_Char *name) {
  ((StarSystem*)userData)->endElement(name);
}

vector <char *> ParseDestinations (const string &value) {
  vector <char *> tmp;
  int i;
  int j;
  int k;
  for (j=0;value[j]!=0;){
    for (i=0;value[j]!=' '&&value[j]!='\0';i++,j++) {
    }
    tmp.push_back(new char [i+1]);
    for (k=0;k<i;k++) {
      tmp[tmp.size()-1][k]=value[k+j-i];
    }
    tmp[tmp.size()-1][i]='\0';
    if (value[j]!=0)
      j++;
  }
  return tmp;
}

namespace StarXML {
  enum Names {
    UNKNOWN,
    XFILE,
    X,
    Y,
    Z,
    RI,
    RJ,
    RK,
    SI,
    SJ,
    SK,
    QI,
    QJ,
    QK,
    NAME,
    RADIUS,
    GRAVITY,
    YEAR,
    DAY,
    PPOSITION,
    SYSTEM,
    PLANET,
    UNIT,
    EMRED,
    EMGREEN,
    EMBLUE,
    EMALPHA,
    BACKGROUND,
    STARS,
    STARSPREAD,
    NEARSTARS,
    FADESTARS,
    REFLECTIVITY,
    ALPHA,
    DESTINATION,
    JUMP,
    FACTION,
    LIGHT,
    COLL,
    ATTEN,
    DIFF,
    SPEC,
    AMB,
    TERRAIN,
    CONTTERRAIN,
    MASS,
    BUILDING,
    VEHICLE,
    ATMOSPHERE,
	NEBULA,
    NEBFILE,
    ASTEROID,
    SCALEX,
    NUMWRAPS,
    DIFFICULTY,
    REFLECTNOLIGHT,
    ENHANCEMENT,
    SCALEATMOS,
    SCALESYSTEM,
    CITYLIGHTS,
    INSIDEOUT,
    INNERRADIUS,
    OUTERRADIUS,
    NUMSLICES,
    RING,
    WRAPX,
    WRAPY
  };

  const EnumMap::Pair element_names[] = {
    EnumMap::Pair ("UNKNOWN", UNKNOWN),
    EnumMap::Pair ("Planet", PLANET),
    EnumMap::Pair ("System", SYSTEM),
    EnumMap::Pair ("Unit", UNIT),
    EnumMap::Pair ("Enhancement", ENHANCEMENT),
    EnumMap::Pair ("Jump", JUMP),
    EnumMap::Pair ("Light", LIGHT),
    EnumMap::Pair ("Attenuated",ATTEN),
    EnumMap::Pair ("Diffuse",DIFF),
    EnumMap::Pair ("Specular",SPEC),
    EnumMap::Pair ("Ambient",AMB),
    EnumMap::Pair ("Terrain",TERRAIN),
    EnumMap::Pair ("ContinuousTerrain",CONTTERRAIN),
    EnumMap::Pair ("Building",BUILDING),
    EnumMap::Pair ("Vehicle",VEHICLE),
    EnumMap::Pair ("Atmosphere",ATMOSPHERE),
    EnumMap::Pair ("Nebula",NEBULA),
    EnumMap::Pair ("Asteroid",ASTEROID),
    EnumMap::Pair ("RING",RING),
    EnumMap::Pair ("citylights",CITYLIGHTS)
  };
  const EnumMap::Pair attribute_names[] = {
    EnumMap::Pair ("UNKNOWN", UNKNOWN),
    EnumMap::Pair ("background", BACKGROUND), 
    EnumMap::Pair ("stars", STARS),
    EnumMap::Pair ("nearstars", NEARSTARS),
    EnumMap::Pair ("fadestars", FADESTARS),
    EnumMap::Pair ("starspread", STARSPREAD), 
    EnumMap::Pair ("reflectivity", REFLECTIVITY), 
    EnumMap::Pair ("file", XFILE),
    EnumMap::Pair ("alpha", ALPHA),
    EnumMap::Pair ("destination", DESTINATION), 
    EnumMap::Pair ("x", X), 
    EnumMap::Pair ("y", Y), 
    EnumMap::Pair ("z", Z), 
    EnumMap::Pair ("ri", RI), 
    EnumMap::Pair ("rj", RJ), 
    EnumMap::Pair ("rk", RK), 
    EnumMap::Pair ("si", SI),     
    EnumMap::Pair ("sj", SJ),     
    EnumMap::Pair ("sk", SK),
    EnumMap::Pair ("qi", QI), 
    EnumMap::Pair ("qj", QJ), 
    EnumMap::Pair ("qk", QK), 
    EnumMap::Pair ("name", NAME),
    EnumMap::Pair ("radius", RADIUS),
    EnumMap::Pair ("gravity", GRAVITY),
    EnumMap::Pair ("year", YEAR),
    EnumMap::Pair ("day", DAY),
    EnumMap::Pair ("position", PPOSITION),
    EnumMap::Pair ("Red", EMRED),
    EnumMap::Pair ("Green", EMGREEN),
    EnumMap::Pair ("Blue", EMBLUE),
    EnumMap::Pair ("Alfa", EMALPHA),
    EnumMap::Pair ("ReflectNoLight",REFLECTNOLIGHT),
    EnumMap::Pair ("faction", FACTION),
    EnumMap::Pair ("Light", LIGHT),
    EnumMap::Pair ("Mass", MASS),
    EnumMap::Pair ("ScaleX", SCALEX),
    EnumMap::Pair ("NumWraps", NUMWRAPS),
    EnumMap::Pair ("NumSlices", NUMSLICES),
    EnumMap::Pair ("Difficulty", DIFFICULTY),
    EnumMap::Pair ("ScaleAtmosphereHeight", SCALEATMOS),
    EnumMap::Pair ("ScaleSystem",SCALESYSTEM),
    EnumMap::Pair ("InsideOut",INSIDEOUT),
    EnumMap::Pair ("InnerRadius",INNERRADIUS),
    EnumMap::Pair ("OuterRadius",OUTERRADIUS),
    EnumMap::Pair ("WrapX",WRAPX),
    EnumMap::Pair ("WrapY",WRAPY)
    
  };

  const EnumMap element_map(element_names, 20);
  const EnumMap attribute_map(attribute_names, 47);
}

using XMLSupport::EnumMap;
using XMLSupport::Attribute;
using XMLSupport::AttributeList;
using namespace StarXML;

static Vector ComputeRotVel (float rotvel, const QVector &r, const QVector & s) {
  if ((r.i||r.j||r.k)&&(s.i||s.j||s.k)) {
    QVector retval = r.Cross (s);
    retval.Normalize();
    retval= retval * rotvel;
    return retval.Cast();
  }else {
    return Vector (0,rotvel,0);
  }
}



static void GetLights (const vector <GFXLight> &origlights, vector <GFXLightLocal> &curlights, const char *str) {
  int tint;
  char isloc;
  char * tmp=strdup (str);
  GFXLightLocal  lloc;
  char * st =tmp;
  int numel;
  while ((numel=sscanf (st,"%d%c",&tint,&isloc))>0) {
    assert (tint<(int)origlights.size());
    lloc.ligh = origlights[tint];
    lloc.islocal = (numel>1&&isloc=='l');
    curlights.push_back (lloc);
    while (isspace(*st) )
      st++;
    while (isalnum (*st)) {
      st++;
    }
  }
  free (tmp);
} 
void setStaticFlightgroup (vector<Flightgroup *> &fg, const std::string &nam,int faction) {
  while (faction>=(int)fg.size()) {
    fg.push_back(new Flightgroup());
    fg.back()->nr_ships=0;
  }
  if (fg[faction]->nr_ships==0) {
    fg[faction]->flightgroup_nr=faction;
    fg[faction]->pos.i=fg[faction]->pos.j=fg[faction]->pos.k=0;
    //    fg[faction]->rot[0]=fg[faction]->rot[1]=fg[faction]->rot[2]=0;
    fg[faction]->nr_ships=0;
    fg[faction]->ainame="default";
     fg[faction]->faction=_Universe->GetFaction(faction);
    fg[faction]->type="Base";
    fg[faction]->nr_waves_left=0;
    fg[faction]->nr_ships_left=0;
    fg[faction]->name=nam;
  }
  fg[faction]->nr_ships++;
  fg[faction]->nr_ships_left++;
}
Flightgroup *getStaticBaseFlightgroup (int faction) {
  static vector <Flightgroup *> fg;//warning mem leak...not big O(num factions)
  setStaticFlightgroup (fg,"Base",faction);
  return fg[faction];
}
Flightgroup *getStaticStarFlightgroup (int faction) {
  static vector <Flightgroup *> fg;//warning mem leak...not big O(num factions)
  setStaticFlightgroup (fg,"Base",faction);
  return fg[faction];
}

Flightgroup *getStaticNebulaFlightgroup (int faction) {
  static vector <Flightgroup *> fg;
  setStaticFlightgroup (fg,"Nebula",faction);
  return fg[faction];
}
Flightgroup *getStaticAsteroidFlightgroup (int faction) {
  static vector <Flightgroup *> fg;
  setStaticFlightgroup (fg,"Asteroid",faction);
  return fg[faction];
}
Flightgroup *getStaticUnknownFlightgroup (int faction) {
  static vector <Flightgroup *> fg;
  setStaticFlightgroup (fg,"Unknown",faction);
  return fg[faction];
}
//sorry boyz...I'm just a tourist with a frag nav console--could you tell me where I am?
static Unit * getTopLevelOwner( ) {//returns terrible memory--don't dereference...ever...not even aligned
  return (Unit *)0x31337;
}
extern BLENDFUNC parse_alpha (const char *);
void parse_dual_alpha (const char * alpha, BLENDFUNC & blendSrc, BLENDFUNC &blendDst) {
  blendSrc=ONE;
  blendDst=ZERO;
  if (alpha==NULL) {  
  }else if (alpha[0]=='\0') {
  }else {
      char *s=strdup (alpha);
      char *d=strdup (alpha);
      blendSrc=SRCALPHA;
      blendDst=INVSRCALPHA;
      if (2==sscanf (alpha,"%s %s",s,d)) {
	if (strcmp(s,"true")!=0) {
	  blendSrc = parse_alpha (s);
	  blendDst = parse_alpha (d);
	}
      }
      free (s);
      free (d);
    }
}


void StarSystem::beginElement(const string &name, const AttributeList &attributes) {
  static int neutralfaction=_Universe->GetFaction("neutral");
  static float asteroiddiff = XMLSupport::parse_float (vs_config->getVariable ("physics","AsteroidDifficulty",".4"));
  std::string myfile;
  vector <GFXLightLocal> curlights;
  xml->cursun.i=0;
  GFXColor tmpcol(0,0,0,1);
  LIGHT_TARGET tmptarg= POSITION;
  xml->cursun.j=0;
  string citylights;
  float scaleatmos=10;
  char * nebfile;
  bool insideout=false;
  int faction=0;
  BLENDFUNC blendSrc=ONE;
  BLENDFUNC blendDst=ZERO;
  bool isdest=false;
  xml->cursun.k=0;	
  static float yearscale = XMLSupport::parse_float (vs_config->getVariable ("physics","YearScale","10"));
  static float dayscale = yearscale;//XMLSupport::parse_float (vs_config->getVariable ("physics","DayScale","10"));
  GFXMaterial ourmat;
  GFXGetMaterial (0,ourmat);
  vs_config->getColor ("planet_mat_ambient",&ourmat.ar);
  vs_config->getColor ("planet_mat_diffuse",&ourmat.dr);
  vs_config->getColor ("planet_mat_specular",&ourmat.sr);
  vs_config->getColor ("planet_mat_emmissive",&ourmat.er);
  int numwraps=1;
  float scalex=1;
  vector <char *>dest;
  char * filename =NULL;
  string fullname="unknw";
  float gravity=0;
  float velocity=0;
  float position=0;
  float rotvel=0;
  QVector S(0,0,0), R(0,0,0);
  QVector  pos(0,0,0);
  Names elem = (Names)element_map.lookup(name);
  float radius=1;
  AttributeList::const_iterator iter;
  switch(elem) {
  case UNKNOWN:
    xml->unitlevel++;

    //    cerr << "Unknown element start tag '" << name << "' detected " << endl;
    return;

  case SYSTEM:
    assert (xml->unitlevel==0);
    xml->unitlevel++;
    pos = QVector (0,0,0);
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case SCALESYSTEM:
	xml->scale = parse_float ((*iter).value);
	break;
      case REFLECTIVITY:
	xml->reflectivity=parse_float ((*iter).value);
	break;
      case BACKGROUND:
	xml->backgroundname=(*iter).value;
	break;
      case NEARSTARS:
	xml->numnearstars = parse_int((*iter).value);
	break;
      case STARS:
	xml->numstars = parse_int ((*iter).value);
	break;
      case STARSPREAD:
	xml->starsp = parse_float ((*iter).value);
	break;
      case FADESTARS:
	xml->fade = parse_bool ((*iter).value);
	break;
      case NAME:
	this->name = new char [strlen((*iter).value.c_str())+1];
	strcpy(this->name,(*iter).value.c_str());
	break;
      case X:
	pos.i=parse_float((*iter).value);
	break;
      case Y:
	pos.j=parse_float((*iter).value);
	break;
      case Z:
	pos.k=parse_float((*iter).value);
	break;
      }

    }

    break;
  case RING:
    {
      xml->unitlevel++;
      std::string myfile("planets/ring.png");

      blendSrc=SRCALPHA;
      blendDst=INVSRCALPHA;
      Unit  * p = (Unit *)xml->moons.back()->GetTopPlanet(xml->unitlevel-1);  
      if (p!=NULL)
	if (p->isUnit()==PLANETPTR) {
	  int wrapx = 1;
	  int wrapy = 1;
	  int numslices=6;
	  float iradius = p->rSize()*1.25;
	  float oradius = p->rSize()*1.75;
	  R.Set(1,0,0);
	  S.Set(0,1,0);
	  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
	    switch(attribute_map.lookup((*iter).name)) {
	    case XFILE:
	      myfile = (*iter).value;
	      break;
	    case ALPHA:
	      parse_dual_alpha ((*iter).value.c_str(),blendSrc,blendDst);
	      break;
	    case INNERRADIUS:
	      iradius = parse_float ((*iter).value)*xml->scale;	  
	      break;
	    case OUTERRADIUS:
	      oradius = parse_float ((*iter).value)*xml->scale;	  
	      break;
	    case NUMSLICES:
	      numslices = parse_int ((*iter).value);
	      break;
	    case WRAPX:
	      wrapx = parse_int ((*iter).value);
	      break;
	    case WRAPY:
	      wrapy = parse_int ((*iter).value);
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
	    case SI:
	      S.i=parse_float((*iter).value);
	      break;
	    case SJ:
	      S.j=parse_float((*iter).value);
	      break;
	    case SK:
	      S.k=parse_float((*iter).value);
	      break;

	    default:
	      break;
	    }
	  }
	  if (p!=NULL) {
	    ((Planet *)p)->AddRing (myfile,iradius,oradius,R,S,numslices,wrapx, wrapy,blendSrc,blendDst);
	  }
	}
      break;
    }
  case CITYLIGHTS:
    {
      std::string myfile("planets/Dirt_light.png");
      xml->unitlevel++;
      blendSrc=SRCALPHA;
      blendDst=ONE;
      int wrapx=1;
      bool inside_out=false;
      int wrapy=1;
      Unit  * p = (Unit *)xml->moons.back()->GetTopPlanet(xml->unitlevel-1);  
      if (p!=NULL)
	if (p->isUnit()==PLANETPTR) {
	  float radius = p->rSize();
	  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
	    switch(attribute_map.lookup((*iter).name)) {
	    case XFILE:
	      myfile = (*iter).value;
	      break;
	    case ALPHA:
	      parse_dual_alpha ((*iter).value.c_str(),blendSrc,blendDst);
	      break;
	    case RADIUS:
	      radius = parse_float ((*iter).value)*xml->scale;	  
	      break;
	    case WRAPX:
	      wrapx = parse_int ((*iter).value);
	      break;
	    case WRAPY:
	      wrapy = parse_int ((*iter).value);
	      break;
	    case INSIDEOUT:
	      inside_out=parse_bool((*iter).value);
	      break;
	    default:
	      break;
	    }
	  }
	  ((Planet *)p)->AddCity (myfile,radius,wrapx,wrapy,blendSrc,blendDst,inside_out);
	}
      break;
    }
    
  case ATMOSPHERE:
    {
      std::string myfile("sol/earthcloudmaptrans.png");
      xml->unitlevel++;
      blendSrc=SRCALPHA;
      blendDst=INVSRCALPHA;
      Unit  * p = (Unit *)xml->moons.back()->GetTopPlanet(xml->unitlevel-1);  
      if (p!=NULL)
	if (p->isUnit()==PLANETPTR) {
	  float radius = p->rSize()*1.075;
	  for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
	    switch(attribute_map.lookup((*iter).name)) {
	    case XFILE:
	      myfile = (*iter).value;
	      break;
	    case ALPHA:
	      parse_dual_alpha ((*iter).value.c_str(),blendSrc,blendDst);
	      break;
	    case RADIUS:
	      radius = parse_float ((*iter).value)*xml->scale;	  
	      break;
	    default:
	      break;
	    }
	  }
	  ((Planet *)p)->AddAtmosphere (myfile,radius,blendSrc,blendDst);
	}
      break;
    }
    {
      Atmosphere::Parameters params;
      //NOTHING NEED TO RECODE
      params.low_color[0] = GFXColor(0,0.5,0.0);

      params.low_color[1] = GFXColor(0,1.0,0.0);

      params.low_ambient_color[0] = GFXColor(0.0/255.0,0.0/255.0,0.0/255.0);

      params.low_ambient_color[1] = GFXColor(0.0/255.0,0.0/255.0,0.0/255.0);

      params.high_color[0] = GFXColor(0.5,0.0,0.0);

      params.high_color[1] = GFXColor(1.0,0.0,0.0);

      params.high_ambient_color[0] = GFXColor(0,0,0);

      params.high_ambient_color[1] = GFXColor(0,0,0);

      params.scattering = 5;

      Atmosphere * a =  new Atmosphere(params); 
      if (xml->unitlevel>2) {
	assert(xml->moons.size()!=0);
	Planet * p =xml->moons.back()->GetTopPlanet(xml->unitlevel-1);
	if (p)
	  p->setAtmosphere (a);
	else
	  fprintf (stderr,"atmosphere loose. no planet for it");
      } 
      
    }
    break;
  case TERRAIN:
  case CONTTERRAIN:
    xml->unitlevel++;
    S = QVector (1,0,0);
    R = QVector (0,0,1);
    pos = QVector (0,0,0);
    radius=-1;
    position=parse_float (vs_config->getVariable ("terrain","mass","100"));
    gravity=0;

    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case NUMWRAPS:
	numwraps = parse_int ((*iter).value);
	break;
      case SCALEX:
	scalex = parse_float((*iter).value);
	break;
      case SCALEATMOS:
	scaleatmos = parse_float((*iter).value);
	break;
      case XFILE:
	myfile = (*iter).value;
	break;
      case GRAVITY:
	gravity=parse_float((*iter).value);
	break;
      case MASS:
	position = parse_float ((*iter).value);
	break;
      case RADIUS:
	radius = parse_float ((*iter).value);
	break;
      case X:
	pos.i = parse_float ((*iter).value)*xml->scale;
	break;
      case Y:
	pos.j = parse_float ((*iter).value)*xml->scale;
	break;
      case Z:
	pos.k = parse_float ((*iter).value)*xml->scale;
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
	S.i=parse_float((*iter).value);
	break;
      case QJ:
	S.j=parse_float((*iter).value);
	break;
      case QK:
	S.k=parse_float((*iter).value);
	break;
      }	
    }
    {
      Vector TerrainScale (XMLSupport::parse_float (vs_config->getVariable ("terrain","xscale","1")),XMLSupport::parse_float (vs_config->getVariable ("terrain","yscale","1")),XMLSupport::parse_float (vs_config->getVariable ("terrain","zscale","1")));
      Matrix t;
      Identity(t);
      float y =S.Magnitude();
      Normalize(S);
      float z =R.Magnitude();
      Normalize(R);
      TerrainScale.i*=z;
      TerrainScale.k*=z;
      TerrainScale.j*=y;      
      t.r[3]=S.i*TerrainScale.j;
      t.r[4]=S.j*TerrainScale.j;
      t.r[5]=S.k*TerrainScale.j;
      t.r[6]=R.i*TerrainScale.k;
      t.r[7]=R.j*TerrainScale.k;
      t.r[8]=R.k*TerrainScale.k;
      S = S.Cross (R);
      t.r[0]=S.i*TerrainScale.i;
      t.r[1]=S.j*TerrainScale.i;
      t.r[2]=S.k*TerrainScale.i;
      t.p=pos+xml->systemcentroid.Cast();
      if (myfile.length()) {
	if (elem==TERRAIN) {
	  terrains.push_back (new Terrain (myfile.c_str(),TerrainScale,position,radius));
	  xml->parentterrain = terrains.back();
	  terrains.back()->SetTransformation(t);
	}else {
	  contterrains.push_back (new ContinuousTerrain (myfile.c_str(),TerrainScale,position));
	  xml->ct =contterrains.back();;
	  contterrains.back()->SetTransformation (t);
	  if (xml->unitlevel>2) {
	    assert(xml->moons.size()!=0);
	    Planet * p =xml->moons.back()->GetTopPlanet(xml->unitlevel-1);
	    if (p) {
	      xml->ct->DisableDraw();
	      p->setTerrain (xml->ct,scalex,numwraps,scaleatmos);
		  PlanetaryTransform ** tmpp = (PlanetaryTransform**) &xml->parentterrain;
	      p->getTerrain(*tmpp);
	    }
	  } 
	}
      }
    }
    break;
  case LIGHT: 
    assert (xml->unitlevel==1);
    xml->unitlevel++;
    xml->lights.push_back (GFXLight (true,Vector(0,0,0)));
    break;
  case ATTEN:
    tmptarg=ATTENUATE;
    goto addlightprop;
  case AMB:
    tmptarg=AMBIENT;
    goto addlightprop;
  case SPEC:
    tmptarg=SPECULAR;
    goto addlightprop;
  case DIFF:
    tmptarg=DIFFUSE;
    goto addlightprop;
  addlightprop:
    assert (xml->unitlevel==2);
    assert (xml->lights.size());
    xml->unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case EMRED:
	tmpcol.r=parse_float ((*iter).value);
	break;
      case EMGREEN:
	tmpcol.g=parse_float ((*iter).value);
	break;
      case EMBLUE:
	tmpcol.b=parse_float ((*iter).value);
	break;
      case EMALPHA:
	tmpcol.a=parse_float ((*iter).value);
	break;
      }
    }
    xml->lights.back().SetProperties (tmptarg,tmpcol);
    break;
  case JUMP:
  case PLANET:
    assert (xml->unitlevel>0);
    xml->unitlevel++;
    S = QVector (1,0,0);
    R = QVector (0,0,1);
    filename = new char [1];
    filename[0]='\0';
    citylights=string("");
    blendSrc=ONE;
    blendDst=ZERO;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case NAME:
	fullname=(*iter).value;
	//	cout << "\nFOUND planet/unit name " << fullname << endl;
	bootstrap_draw("Loading "+fullname);
	break;
      case XFILE:
	delete []filename;
	filename = new char [strlen((*iter).value.c_str())+1];
	strcpy(filename,(*iter).value.c_str());
	break;
      case DESTINATION:
	dest=ParseDestinations((*iter).value);
	isdest=true;
	break;
      case ALPHA:
	parse_dual_alpha ((*iter).value.c_str(),blendSrc,blendDst);
	break;
      case CITYLIGHTS:
	citylights = (*iter).value;
	break;

      case INSIDEOUT:
	insideout=XMLSupport::parse_bool ((*iter).value);
	break;
      case LIGHT:
	GetLights (xml->lights,curlights,(*iter).value.c_str());
		//assert (parse_int ((*iter).value)<xml->lights.size());
	//curlights.push_back (xml->lights[parse_int ((*iter).value)]);
	break;
      case FACTION:
	faction = _Universe->GetFaction ((*iter).value.c_str());
	break;
      case EMRED:
	ourmat.er = parse_float((*iter).value);
	break;
      case EMGREEN:
	ourmat.eg = parse_float((*iter).value);
	break;
      case EMBLUE:
	ourmat.eb = parse_float((*iter).value);
	break;
      case EMALPHA:
	ourmat.ea = parse_float((*iter).value);
	break;
      case REFLECTNOLIGHT:
	ourmat.sr=ourmat.sg=ourmat.sb=ourmat.dr=ourmat.dg=ourmat.db=
	  ourmat.ar=ourmat.ag=ourmat.ab=0;
	break;
      case RI:
	R.i=parse_float((*iter).value)*xml->scale;
	break;
      case RJ:
	R.j=parse_float((*iter).value)*xml->scale;
	break;
      case RK:
	R.k=parse_float((*iter).value)*xml->scale;
	break;
      case SI:
	S.i=parse_float((*iter).value)*xml->scale;
	break;
      case SJ:
	S.j=parse_float((*iter).value)*xml->scale;
	break;
      case SK:
	S.k=parse_float((*iter).value)*xml->scale;
	break;
      case X:
 	xml->cursun.i=parse_float((*iter).value)*xml->scale;
 	break;

      case Y:
 	xml->cursun.j=parse_float((*iter).value)*xml->scale;
 	break;

      case Z:
 	xml->cursun.k=parse_float((*iter).value)*xml->scale;
 	break;

      case RADIUS:
	radius=parse_float((*iter).value);
	break;
      case PPOSITION:
	position=parse_float((*iter).value);
	break;
      case DAY:
	if (fabs (parse_float ((*iter).value))>.00001) {
	  rotvel = 2*M_PI/(dayscale*parse_float ((*iter).value));
	}
	break;
      case YEAR:
	if (fabs (parse_float ((*iter).value))>.00001) {
	  velocity=2*M_PI/(yearscale*parse_float((*iter).value));
	}
	break;
      case GRAVITY:
	gravity=parse_float((*iter).value);
	break;
      }

    }
    if (isdest==false) {
      radius*=xml->scale;
    }
    if (xml->unitlevel>2) {
      assert(xml->moons.size()!=0);
      Unit * un =xml->moons[xml->moons.size()-1]->beginElement(R,S,velocity,ComputeRotVel (rotvel,R,S),position,gravity,radius,filename,blendSrc,blendDst,dest,xml->unitlevel-1, ourmat,curlights,false,faction,fullname,insideout);
      if (un) {
	un->SetOwner (getTopLevelOwner());
      }
    } else {

      
      xml->moons.push_back(UnitFactory::createPlanet(R,S,velocity,ComputeRotVel (rotvel,R,S), position,gravity,radius,filename,blendSrc,blendDst,dest, xml->cursun.Cast()+xml->systemcentroid.Cast(), NULL, ourmat,curlights,faction,fullname,insideout));

      xml->moons[xml->moons.size()-1]->SetPosAndCumPos(R+S+xml->cursun.Cast()+xml->systemcentroid.Cast());
      xml->moons.back()->SetOwner (getTopLevelOwner());
    }
    delete []filename;
    break;
  case UNIT:
  case BUILDING:
  case VEHICLE:
  case NEBULA:
  case ASTEROID:
  case ENHANCEMENT:
    assert (xml->unitlevel>0);
    xml->unitlevel++;
    S = QVector (0,1,0);
    R = QVector (0,0,1);
    nebfile = new char [1];
    nebfile[0]='\0';
    filename = new char [1];
    filename[0]='\0';
    fullname="unkn-unit";
    scalex = asteroiddiff;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case NAME:
	fullname=(*iter).value;
	cout << "\nFOUND unit name " << fullname << endl;
	break;
      case XFILE:
	delete []filename;
	filename = new char [strlen((*iter).value.c_str())+1];
	strcpy(filename,(*iter).value.c_str());
	break;
      case NEBFILE:
	delete []nebfile;
	nebfile = new char [strlen((*iter).value.c_str())+1];
	strcpy(nebfile,(*iter).value.c_str());
	break;
      case DIFFICULTY:
	scalex = parse_float ((*iter).value);
	break;
      case DESTINATION:
	dest=ParseDestinations((*iter).value);
	break;
      case FACTION:
	faction = _Universe->GetFaction ((*iter).value.c_str());
	break;
      case RI:
	R.i=parse_float((*iter).value)*xml->scale;
	break;
      case RJ:
	R.j=parse_float((*iter).value)*xml->scale;
	break;
      case RK:
	R.k=parse_float((*iter).value)*xml->scale;
	break;
      case SI:
	S.i=parse_float((*iter).value)*xml->scale;
	break;
      case SJ:
	S.j=parse_float((*iter).value)*xml->scale;
	break;
      case SK:
	S.k=parse_float((*iter).value)*xml->scale;
	break;
      case X:
 	xml->cursun.i=parse_float((*iter).value)*xml->scale;

 	break;
      case Y:
 	xml->cursun.j=parse_float((*iter).value)*xml->scale;

 	break;
      case Z:
 	xml->cursun.k=parse_float((*iter).value)*xml->scale;

 	break;

      case PPOSITION:
	position=parse_float((*iter).value);
	break;
      case DAY:
	if (fabs (parse_float ((*iter).value))>.00001) {
	  rotvel = 2*M_PI/(dayscale*parse_float ((*iter).value));
	}
	break;
      case YEAR:
	if (fabs (parse_float ((*iter).value))>.00001) {
	  velocity=2*M_PI/(yearscale*parse_float((*iter).value));
	}
	break;
      }

    }  
    if (((elem==UNIT||elem==NEBULA||elem==ENHANCEMENT||elem==ASTEROID)||(xml->ct==NULL&&xml->parentterrain==NULL))&&(xml->unitlevel>2)) {
      assert(xml->moons.size()!=0);
	  Unit * un;
	  Planet * plan =xml->moons.back()->GetTopPlanet(xml->unitlevel-1);
	  if (elem==UNIT) {
	    Flightgroup *fg =getStaticBaseFlightgroup (faction);
	    plan->AddSatellite(un=UnitFactory::createUnit(filename,false,faction,"",fg,fg->nr_ships-1));
	    un->setFullname(fullname);
	  } else if (elem==NEBULA) {
	    Flightgroup *fg =getStaticNebulaFlightgroup (faction);
		    plan->AddSatellite(un=UnitFactory::createNebula(filename,false,faction,fg,fg->nr_ships-1));			
	  } else if (elem==ASTEROID) {
	    Flightgroup *fg =getStaticAsteroidFlightgroup (faction);
	    plan->AddSatellite (un=UnitFactory::createAsteroid (filename,faction,fg,fg->nr_ships-1,scalex));
	  } else if (elem==ENHANCEMENT) {
	    plan->AddSatellite (un=UnitFactory::createEnhancement (filename,faction,string("")));
	  }
	  while (!dest.empty()) {
	    un->AddDestination (dest.back());
	    dest.pop_back();
	  }
	  un->SetAI(new PlanetaryOrbit (un,velocity,position,R,S, QVector (0,0,0), plan));

	  //     xml->moons[xml->moons.size()-1]->Planet::beginElement(R,S,velocity,position,gravity,radius,filename,NULL,vector <char *>(),xml->unitlevel-((xml->parentterrain==NULL&&xml->ct==NULL)?1:2),ourmat,curlights,true,faction);
	  if (elem==UNIT&&un->faction!=neutralfaction) {
	    un->SetTurretAI ();
	    un->EnqueueAI(new Orders::FireAt (.2,15));
	  }
	  un->SetOwner (getTopLevelOwner());//cheating so nothing collides at top lev
	  un->SetAngularVelocity (ComputeRotVel (rotvel,R,S));
    } else {
      if ((elem==BUILDING||elem==VEHICLE)&&xml->ct==NULL&&xml->parentterrain!=NULL) {
	Unit * b = UnitFactory::createBuilding (xml->parentterrain,elem==VEHICLE,filename,false,faction,string(""));
	b->SetPosAndCumPos (xml->cursun.Cast()+xml->systemcentroid.Cast());
	b->EnqueueAI( new Orders::AggressiveAI ("default.agg.xml", "default.int.xml"));
	AddUnit (b);
	  while (!dest.empty()) {
	    b->AddDestination (dest.back());
	    dest.pop_back();
	  }

      }else if ((elem==BUILDING||elem==VEHICLE)&&xml->ct!=NULL) {
	Unit * b=UnitFactory::createBuilding (xml->ct,elem==VEHICLE,filename,false,faction);
	b->SetPlanetOrbitData ((PlanetaryTransform *)xml->parentterrain);
	b->SetPosAndCumPos (xml->cursun.Cast()+xml->systemcentroid.Cast());
	b->EnqueueAI( new Orders::AggressiveAI ("default.agg.xml", "default.int.xml"));
	  b->SetTurretAI ();
	    b->EnqueueAI(new Orders::FireAt (.2,15));
	AddUnit (b);
	  while (!dest.empty()) {
	    b->AddDestination (dest.back());
	    dest.pop_back();
	  }
      }else {
   	    if (elem==UNIT) {
	      Flightgroup *fg =getStaticBaseFlightgroup (faction);
	      Unit *moon_unit=UnitFactory::createUnit(filename,false,faction,"",fg,fg->nr_ships-1);
	      moon_unit->setFullname(fullname);
	      xml->moons.push_back((Planet *)moon_unit);
	    }else if (elem==NEBULA){
	      Flightgroup *fg =getStaticNebulaFlightgroup (faction);
	      xml->moons.push_back ((Planet *)UnitFactory::createNebula (filename,false,faction,fg,fg->nr_ships-1));
	    } else if (elem==ASTEROID){
	    Flightgroup *fg =getStaticAsteroidFlightgroup (faction);
	      xml->moons.push_back ((Planet *)UnitFactory::createAsteroid (filename,faction,fg,fg->nr_ships-1,scalex));
	    } else if (elem==ENHANCEMENT) {
	      xml->moons.push_back ((Planet *)UnitFactory::createEnhancement (filename,faction,string("")));
	    }
	    while (!dest.empty()) {
	      xml->moons.back()->AddDestination (dest.back());
	      dest.pop_back();
	    }
	    xml->moons.back()->SetAI(new PlanetaryOrbit(xml->moons[xml->moons.size()-1],velocity,position,R,S,xml->cursun.Cast()+xml->systemcentroid.Cast(), NULL));

	    xml->moons.back()->SetPosAndCumPos(R+S+xml->cursun.Cast()+xml->systemcentroid.Cast());
	    xml->moons.back()->SetOwner (getTopLevelOwner());
	    if (elem==UNIT&&xml->moons.back()->faction!=neutralfaction) {

	      xml->moons.back()->SetTurretAI ();
	      xml->moons.back()->EnqueueAI(new Orders::FireAt (.2,15));
	    }

      }
    }
    delete []filename;
    break;
	
  default:
	
    break;
  }
}

void StarSystem::endElement(const string &name) {
  Names elem = (Names)element_map.lookup(name);

  switch(elem) {
  case UNKNOWN:
    xml->unitlevel--;
    //    cerr << "Unknown element end tag '" << name << "' detected " << endl;
    break;
  case CONTTERRAIN:
    xml->unitlevel--;
    xml->ct = NULL;
    break;
  case TERRAIN:
    xml->parentterrain=NULL;
    xml->unitlevel--;
    break;
  default:
    xml->unitlevel--;
    break;
  }
  if (xml->unitlevel==0) {

  }
}


void StarSystem::LoadXML(const char *filename, const Vector & centroid, const float timeofyear) {
  //  shield.number=0;
  const int chunk_size = 16384;
  // rrestricted=yrestricted=prestricted=false;
  string file = GetCorrectStarSysPath (filename);
  FILE * inFile=NULL;
  if (file.length()) {
    inFile = fopen (file.c_str(), "r");
    if(!inFile) {
      printf("StarSystem: file not found %s\n",filename);
      
      return;
    }
  }else {
    printf("StarSystem: file not found %s\n",filename);
    return;
  }

  xml = new StarXML;
  xml->scale=1;
  xml->parentterrain=NULL;
  xml->ct=NULL;
  xml->systemcentroid=centroid;
  xml->timeofyear = timeofyear;
  xml->fade = (vs_config->getVariable ("graphics","starblend","true")==string("true"));
  xml->starsp = 150;
  xml->numnearstars=400;
  xml->numstars=800;
  xml->backgroundname = string("cube");
  xml->reflectivity=.7;
  xml->unitlevel=0;
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, &StarSystem::beginElement, &StarSystem::endElement);
  
  do {
    char *buf = (XML_Char*)XML_GetBuffer(parser, chunk_size);
    int length;
    
    length = fread (buf,1, chunk_size,inFile);
    //length = inFile.gcount();
    XML_ParseBuffer(parser, length, feof(inFile));
  } while(!feof(inFile));
  fclose (inFile);
  XML_ParserFree (parser);
  Iterator * iter;
  unsigned int i;
  for (i =0;i<xml->moons.size();i++) {
    if (xml->moons[i]->isUnit()==PLANETPTR) {
      iter = ((Planet*)xml->moons[i])->createIterator();
      Unit * un;
      while ((un = iter->current())) {
	drawList.prepend(un);
	iter->advance();
      }
      delete iter;
    } else {
      drawList.prepend (xml->moons[i]);
    }
  }
  
    
#ifdef NV_CUBE_MAP
  printf("using NV_CUBE_MAP\n");
  LightMap[0]=new Texture ((xml->backgroundname+"_right_light.bmp").c_str(),1,BILINEAR,CUBEMAP,CUBEMAP_POSITIVE_X);
  LightMap[1]=new Texture ((xml->backgroundname+"_left_light.bmp").c_str(),1,BILINEAR,CUBEMAP,CUBEMAP_NEGATIVE_X);
  LightMap[2]=new Texture ((xml->backgroundname+"_up_light.bmp").c_str(),1,BILINEAR,CUBEMAP,CUBEMAP_POSITIVE_Y);
  LightMap[3]=new Texture ((xml->backgroundname+"_down_light.bmp").c_str(),1,BILINEAR,CUBEMAP,CUBEMAP_NEGATIVE_Y);
  LightMap[4]=new Texture ((xml->backgroundname+"_front_light.bmp").c_str(),1,BILINEAR,CUBEMAP,CUBEMAP_POSITIVE_Z);
  LightMap[5]=new Texture ((xml->backgroundname+"_back_light.bmp").c_str(),1,BILINEAR,CUBEMAP,CUBEMAP_NEGATIVE_Z);
#else
  string bglight= 
	  MakeSharedStarSysPath (xml->backgroundname+"_light.bmp");
  FILE * tempo = fopen (bglight.c_str(),"rb");
  if (!tempo) {
      EnvironmentMapGeneratorMain (xml->backgroundname.c_str(),bglight.c_str(), 0,xml->reflectivity,1);
  }else {
    fclose (tempo);
  }
  LightMap[0] = new Texture(bglight.c_str(), 1,MIPMAP,TEXTURE2D,TEXTURE_2D,GFXTRUE);
#endif
  bg = new Background(xml->backgroundname.c_str(),xml->numstars,g_game.zfar*.9);
  stars = new Stars (xml->numnearstars, xml->starsp);
  stars->SetBlend (xml->fade,xml->fade);
  delete xml;
}

