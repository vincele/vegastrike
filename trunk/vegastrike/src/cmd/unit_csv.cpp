#include "unit_generic.h"
#include "csv.h"
#include "savegame.h"
#include "xml_serializer.h"
#include "gfx/sphere.h"
#include "unit_collide.h"
#include "collide/cs_compat.h"
#include "collide/rapcol.h"
#include "gfx/bsp.h"
#include "unit_factory.h"
#include "audiolib.h"
#include "unit_xml.h"
#include "gfx/quaternion.h"
#include "role_bitmask.h"
#include <algorithm>
#define VS_PI 3.1415926535897931

using namespace std;
extern int GetModeFromName (const char * input_buffer);
extern void pushMesh( Unit::XML * xml, const char *filename, const float scale,int faction,class Flightgroup * fg, int startframe, double texturestarttime);
void addShieldMesh( Unit::XML * xml, const char *filename, const float scale,int faction,class Flightgroup * fg);
void addRapidMesh( Unit::XML * xml, const char *filename, const float scale,int faction,class Flightgroup * fg);
void addBSPMesh( Unit::XML * xml, const char *filename, const float scale,int faction,class Flightgroup * fg);
static vector<string> parseSemicolon(string inp) {
  vector<string> ret;
  string::size_type where;
  while((where=inp.find(";"))!=string::npos) {
    ret.push_back(inp.substr(0,where));
    inp=inp.substr(where+1);
  }
  ret.push_back(inp);
  return ret;
}
static vector<vector<string> > parseBracketSemicolon(string inp) {
  vector<vector<string> >ret;  
  string::size_type where;
  while ((where=inp.find("{"))!=string::npos) {
    inp=inp.substr(where+1);
    where=inp.find("}");
    string tmp=inp.substr(0,where);
    inp=inp.substr(where+1);
    ret.push_back(parseSemicolon(inp));
  }
  return ret;
}

static void UpgradeUnit (Unit * un, std::string upgrades) {
  string::size_type when;
  while ((when=upgrades.find("{"))!=string::npos) {
    upgrades=upgrades.substr(when+1);
    string::size_type where = upgrades.find("}");
    unsigned int mountoffset=0;
    unsigned int subunitoffset=0;
    string upgrade = upgrades.substr(0,where);
    string::size_type where1 = upgrade.find(";");
    string::size_type where2 = upgrade.rfind(";");
    if (where1!=string::npos) {
      mountoffset=XMLSupport::parse_int(upgrade.substr(where1+1,where2!=where1?where2:upgrade.length()));      
      if (where2!=where1&&where2!=string::npos) {
        subunitoffset=XMLSupport::parse_int(upgrade.substr(where2+1));
      }
    }
    upgrade = upgrade.substr(0,where1);
    if (upgrade.length()==0) 
      continue;
    Unit *upgradee =UnitFactory::createUnit(upgrade.c_str(),
                                            true,
                                            FactionUtil::
                                            GetFaction("upgrades"));
    double percent=1.0;
    un->Unit::Upgrade (upgradee,
                       mountoffset,
                       subunitoffset,
                       GetModeFromName (upgrade.c_str()),true,percent,NULL);
    upgradee->Kill();    
  }  
}


static void AddMeshes(Unit::XML& xml, std::string meshes,int faction,Flightgroup *fg){
  string::size_type where,wheresf,wherest;
  while ((where=meshes.find("{"))!=string::npos) {
    meshes=meshes.substr(where+1);
    where=meshes.find("}");//matching closing brace
    string mesh = meshes.substr(0,where);
    
    wheresf = mesh.find(";");
    string startf = "0";
    string startt="0";
    if (wheresf!=string::npos) {
      startf=mesh.substr(wheresf+1);
      mesh = mesh.substr(0,wheresf); 
      wherest = startf.find(";");
      if (wherest!=string::npos) {
        startt=startf.substr(wherest+1);
        startf=startf.substr(0,wherest);
      }
    }    
    int startframe = startf=="RANDOM"?-1:(startf=="ASYNC"?-1:atoi(startf.c_str()));
    float starttime = startt=="RANDOM"?-1.0f:atof(startt.c_str());
    
    pushMesh(&xml,mesh.c_str(),xml.unitscale,faction,fg,startframe,starttime);
  }
}
static string nextElement (string&inp) {
  string::size_type where=inp.find(";");
  if (where!=string::npos) {
    string ret=inp.substr(0,where);
    inp=inp.substr(where+1);
    return ret;
  }
  string ret=inp;
  inp="";
  return ret;
}

static bool stob(string inp, bool defaul) {
  if (inp.length()!=0) 
    return XMLSupport::parse_bool(inp);
  return defaul;
}
static double stof(string inp, double def=0) {
  if (inp.length()!=0)
    return XMLSupport::parse_float(inp);
  return def;
}
static int stoi(string inp, int def=0) {
  if (inp.length()!=0)
    return XMLSupport::parse_int(inp);
  return def;
}
extern bool CheckAccessory(Unit *);
extern int parseMountSizes (const char * str);
static Mount * createMount(const std::string& name, int ammo, int volume, float xyscale, float zscale, float func, float maxfunc) //short fix
{
	return new Mount (name.c_str(), ammo,volume,xyscale, zscale,func,maxfunc);

}

static void AddMounts(Unit * thus, Unit::XML &xml, std::string mounts) {
  string::size_type where;
  while ((where=mounts.find("{"))!=string::npos) {
    mounts=mounts.substr(where+1);    
    if ((where=mounts.find("}"))!=string::npos) {
      string mount=mounts.substr(0,where);      
      mounts=mounts.substr(where+1);      
      QVector P;
      QVector Q = QVector (0,1,0);
      QVector R = QVector (0,0,1);
      QVector pos = QVector (0,0,0);
      
      string filename = nextElement(mount);
      int ammo = stoi(nextElement(mount),-1);      
      int volume = stoi(nextElement(mount));      
      string mountsize=nextElement(mount);
      pos.i = stof(nextElement(mount));
      pos.j = stof(nextElement(mount));
      pos.k = stof(nextElement(mount));
      double xyscale = stof(nextElement(mount));
      double zscale = stof(nextElement(mount));
      R.i = stof(nextElement(mount));
      R.j = stof(nextElement(mount));
      R.k = stof(nextElement(mount),1);
      Q.i = stof(nextElement(mount));
      Q.j = stof(nextElement(mount),1);
      Q.k = stof(nextElement(mount));
      float func =stof(nextElement(mount),1);
      float maxfunc =stof(nextElement(mount),1);

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
      unsigned int indx = xml.mountz.size();
      xml.mountz.push_back(createMount (filename.c_str(), ammo,volume,xml.unitscale*xyscale,xml.unitscale*zscale,func,maxfunc));
      xml.mountz[indx]->SetMountOrientation(Quaternion::from_vectors(P.Cast(),Q.Cast(),R.Cast()));
      xml.mountz[indx]->SetMountPosition(xml.unitscale*pos.Cast());
      int mntsiz=weapon_info::NOWEAP;
      if (mountsize.length()) {
        mntsiz=parseMountSizes(mountsize.c_str());
        xml.mountz[indx]->size=mntsiz;
      }else {
        xml.mountz[indx]->size = xml.mountz[indx]->type->size;
      }
    }
  }
  if (xml.mountz.size())
  {
    // DO not destroy anymore, just affect address
    for( int a=0; a<xml.mountz.size(); a++)
      thus->mounts.push_back( *xml.mountz[a]);
    //mounts[a]=*xml.mountz[a];
    //delete xml.mountz[a];			//do it stealthily... no cons/destructor
  }
  unsigned char parity=0;
  for (int a=0;a<xml.mountz.size();a++) {
    static bool half_sounds = XMLSupport::parse_bool(vs_config->getVariable ("audio","every_other_mount","false"));
    if (a%2==parity) {
      int b=a;
      if(a % 4 == 2 && (int) a < (thus->GetNumMounts()-1)) 
        if (thus->mounts[a].type->type != weapon_info::PROJECTILE&&thus->mounts[a+1].type->type != weapon_info::PROJECTILE)
          b=a+1;
      thus->mounts[b].sound = AUDCreateSound (thus->mounts[b].type->sound,thus->mounts[b].type->type!=weapon_info::PROJECTILE);
    } else if ((!half_sounds)||thus->mounts[a].type->type == weapon_info::PROJECTILE) {
      thus->mounts[a].sound = AUDCreateSound (thus->mounts[a].type->sound,thus->mounts[a].type->type!=weapon_info::PROJECTILE);    //lloping also flase in unit_customize  
    }
    if (a>0) {
      if (thus->mounts[a].sound==thus->mounts[a-1].sound&&thus->mounts[a].sound!=-1) {
        printf ("Sound error\n");
      }
    }
  }
}
struct SubUnitStruct{
  string filename;
  QVector pos;
  QVector Q;
  QVector R;
  double restricted;
  SubUnitStruct(string fn, QVector POS, QVector q, QVector r, double res) {
    filename=fn;
    pos=POS;
    Q=q;
    R=r;
    restricted=res;
  }
};
static vector<SubUnitStruct> GetSubUnits(std::string subunits) {
 string::size_type where;
 vector<SubUnitStruct> ret;
  while ((where=subunits.find("{"))!=string::npos) {
    subunits=subunits.substr(where+1);    
    if ((where=subunits.find("}"))!=string::npos) {
      string subunit=subunits.substr(0,where);      
      subunits=subunits.substr(where+1);      
      string filename=nextElement(subunit);
      QVector pos,Q,R;
      pos.i=stof(nextElement(subunit));
      pos.j=stof(nextElement(subunit));
      pos.k=stof(nextElement(subunit));
      R.i=stof(nextElement(subunit));
      R.j=stof(nextElement(subunit));
      R.k=stof(nextElement(subunit));
      Q.i=stof(nextElement(subunit));
      Q.j=stof(nextElement(subunit));
      Q.k=stof(nextElement(subunit));
      double restricted=cos(stof(nextElement(subunit),-1)*VS_PI/180.0);
      ret.push_back(SubUnitStruct(filename,pos,Q,R,restricted));
    }
  }
  return ret;
}
static void AddSubUnits (Unit *thus, Unit::XML &xml, std::string subunits, int faction, std::string modification) {
  vector<SubUnitStruct> su=GetSubUnits(subunits);
  for (vector<SubUnitStruct>::iterator i=su.begin();i!=su.end();++i) {
    string filename=(*i).filename;
    QVector pos=(*i).pos;
    QVector Q= (*i).Q;
    QVector R= (*i).R;
    double restricted=(*i).restricted;
    xml.units.push_back(UnitFactory::createUnit (filename.c_str(),true,faction,modification.c_str(),NULL)); // I set here the fg arg to NULL
    if (xml.units.back()->name=="LOAD_FAILED") {
      xml.units.back()->limits.yaw=0;
      xml.units.back()->limits.pitch=0;
      xml.units.back()->limits.roll=0;
      xml.units.back()->limits.lateral = xml.units.back()->limits.retro = xml.units.back()->limits.forward = xml.units.back()->limits.afterburn=0.0;        
    }
    xml.units.back()->SetRecursiveOwner (thus);
    xml.units.back()->SetOrientation (Q,R);
    R.Normalize();
    xml.units.back()->prev_physical_state = xml.units.back()->curr_physical_state;
    xml.units.back()->SetPosition(pos*xml.unitscale);
    //    xml.units.back()->prev_physical_state= Transformation(Quaternion::from_vectors(P,Q,R),pos);
    //    xml.units.back()->curr_physical_state=xml.units.back()->prev_physical_state;
    xml.units.back()->limits.structurelimits=R.Cast();
    xml.units.back()->limits.limitmin=restricted;
    xml.units.back()->name = filename;
    if (xml.units.back()->image->unitwriter!=NULL) {
      xml.units.back()->image->unitwriter->setName (filename);
    }
    CheckAccessory(xml.units.back());//turns on the ceerazy rotation for the turr          
  }
  for(unsigned int a=0; a<xml.units.size(); a++) {
    thus->SubUnits.prepend(xml.units[a]);
  }
}

void AddDocks (Unit* thus, Unit::XML &xml, string docks) {
  string::size_type where;
  while ((where=docks.find("{"))!=string::npos) {
    docks=docks.substr(where+1);    
    if ((where=docks.find("}"))!=string::npos) {
      string dock=docks.substr(0,where);      

      docks=docks.substr(where+1);      
      QVector pos=QVector(0,0,0);
      bool internal=XMLSupport::parse_bool(nextElement(dock));
      pos.i=stof(nextElement(dock));
      pos.j=stof(nextElement(dock));
      pos.k=stof(nextElement(dock));
      double size=stof(nextElement(dock));
      double minsize=stof(nextElement(dock));
      thus->image->dockingports.push_back (DockingPorts(pos.Cast()*xml.unitscale,size*xml.unitscale,minsize*xml.unitscale,internal));      
    }
  }
}
void AddLights (Unit * thus, Unit::XML &xml, string lights) {
  string::size_type where;
  while ((where=lights.find("{"))!=string::npos) {
    lights= lights.substr(where+1);
    where=lights.find("}");
    if (where!=string::npos) {
      string light = lights.substr(0,where);
      lights=lights.substr(where+1);
      string filename = nextElement(light);
      QVector pos,scale;
      GFXColor halocolor;
      pos.i=stof(nextElement(light));
      pos.j=stof(nextElement(light));
      pos.k=stof(nextElement(light));
      scale.i=xml.unitscale*stof(nextElement(light),1);
      scale.j=scale.k=scale.i;
      halocolor.r=stof(nextElement(light),1);
      halocolor.g=stof(nextElement(light),1);
      halocolor.b=stof(nextElement(light),1);
      halocolor.a=stof(nextElement(light),1);
      double act_speed=stof(nextElement(light));
      thus->addHalo(filename.c_str(),pos*xml.unitscale,scale.Cast(),halocolor,"",act_speed);
    }
  }
}

static void ImportCargo(Unit * thus,string imports) {
  string::size_type where;
  while ((where=imports.find("{"))!=string::npos) {
    imports= imports.substr(where+1);
    where=imports.find("}");
    if (where!=string::npos) {
      string imp = imports.substr(0,where);
      imports=imports.substr(where+1);
      string filename = nextElement(imp);
      double price = stof(nextElement(imp),1);
      double pricestddev = stof(nextElement(imp));
      double quant = stof(nextElement(imp),1);
      double quantstddev = stof(nextElement(imp));
      thus->ImportPartList(filename,price,pricestddev,quant,quantstddev);
    }
  }
}
static void AddCarg (Unit *thus, string cargos) {
 string::size_type where;
  while ((where=cargos.find("{"))!=string::npos) {
    cargos= cargos.substr(where+1);
    where=cargos.find("}");
    if (where!=string::npos) {
      Cargo carg;
      string cargo = cargos.substr(0,where);
      cargos=cargos.substr(where+1);
      carg.content = nextElement(cargo);      
      carg.category = nextElement(cargo);      
      carg.price = stof(nextElement(cargo));
      carg.quantity = stoi(nextElement(cargo));
      carg.mass = stof(nextElement(cargo));
      carg.volume = stof(nextElement(cargo));
      carg.functionality = stof(nextElement(cargo));
      carg.maxfunctionality = stof(nextElement(cargo));
      string ele=nextElement(cargo);
      if (ele.length())
        carg.description= strdup (ele.c_str());
      else
        carg.description="";
      carg.mission = false;
      string mis =nextElement(cargo);
      if (mis.length()) {
        carg.mission=XMLSupport::parse_bool(mis);
      }
      thus->AddCargo(carg,false);
    }
  }
}
void HudDamage(float * dam, string damages) {
  if (dam) {
    for (int i=0;i<1+MAXVDUS+UnitImages::NUMGAUGES;++i) {
      dam[i] = stof(nextElement(damages),0);
    }
  }
}
string WriteHudDamage (Unit * un) {
  string ret;
  const string semi=";";
  if (un->image->cockpit_damage) {
    for (int i=0;i<1+MAXVDUS+UnitImages::NUMGAUGES;++i) {
      ret+=XMLSupport::tostring(un->image->cockpit_damage[i]);
      ret+=semi;
    }
  }
  return ret;
}
string WriteHudDamageFunc (Unit * un) {
  string ret;
  const string semi=";";
  if (un->image->cockpit_damage) {
    int numg=1+MAXVDUS+UnitImages::NUMGAUGES;
    for (int i=numg;i<2*numg;++i) {
      ret+=XMLSupport::tostring(un->image->cockpit_damage[i]);
      ret+=semi;
    }
  }
  return ret;
}
void AddSounds(Unit * thus, string sounds) {
  if (sounds.length()!=0) {
    thus->sound->shield=AUDCreateSoundWAV(nextElement(sounds),false);
    thus->sound->armor=AUDCreateSoundWAV(nextElement(sounds),false);
    thus->sound->hull=AUDCreateSoundWAV(nextElement(sounds),false);
    thus->sound->jump=AUDCreateSoundWAV(nextElement(sounds),false);
    thus->sound->explode=AUDCreateSoundWAV(nextElement(sounds),false);
    thus->sound->cloak=AUDCreateSoundWAV(nextElement(sounds),false);
    thus->sound->engine=AUDCreateSoundWAV(nextElement(sounds),true);
  }
    if (thus->sound->cloak==-1) {
      thus->sound->cloak=AUDCreateSound(vs_config->getVariable ("unitaudio","cloak", "sfx43.wav"),false);
    }
    if (thus->sound->engine==-1) {
      thus->sound->engine=AUDCreateSound (vs_config->getVariable ("unitaudio","afterburner","sfx10.wav"),false);
    }
    if (thus->sound->shield==-1) {
      thus->sound->shield=AUDCreateSound (vs_config->getVariable ("unitaudio","shield","sfx09.wav"),false);
    }
    if (thus->sound->armor==-1) {
      thus->sound->armor=AUDCreateSound (vs_config->getVariable ("unitaudio","armor","sfx08.wav"),false);
    }
    if (thus->sound->hull==-1) {
      thus->sound->hull=AUDCreateSound (vs_config->getVariable ("unitaudio","armor","sfx08.wav"),false);
    }
    if (thus->sound->explode==-1) {
      thus->sound->explode=AUDCreateSound (vs_config->getVariable ("unitaudio","explode","sfx03.wav"),false);
    }
    if (thus->sound->jump==-1) {
      thus->sound->jump=AUDCreateSound (vs_config->getVariable ("unitaudio","explode","sfx43.wav"),false);
    }
}
void LoadCockpit(Unit * thus, string cockpit) {
  /*
  int i=0;
  while (cockpitdamage!=""&&i<2*(UnitImages::NUMGAUGES+1+MAXVDUS)) {    
    thus->image->cockpit_damage[i]=stof(nextElement(cockpitdamage));   
    thus->image->cockpit_damage[i+1]=stof(nextElement(cockpitdamage));
    i+=2;
    }*/
  thus->image->cockpitImage=nextElement(cockpit);
  thus->image->CockpitCenter.i=stof(nextElement(cockpit));   
  thus->image->CockpitCenter.j=stof(nextElement(cockpit));   
  thus->image->CockpitCenter.k=stof(nextElement(cockpit));   

}
static string str(string inp, string def) {
  if (inp.length()==0) return def;
  return inp;
}
static int AssignIf(string inp,float &val,float&val1, float&val2) {
  if (inp.length()) {
    val=stof(inp);
    val1=stof(inp);
    val2=stof(inp);
    return 1;
  }
  return 0;
}
float getFuelConversion(){
  static float fuel_conversion = XMLSupport::parse_float(vs_config->getVariable("physics","FuelConversion",".00144"));
  return fuel_conversion;
}

void Unit::LoadRow(CSVRow &row,string modification, string * netxml) {
  Unit::XML xml;
  xml.unitModifications=modification.c_str();
  xml.randomstartframe=((float)rand())/RAND_MAX;
  xml.randomstartseconds=0;
  xml.calculated_role=false;
  xml.damageiterator=0;
  xml.shieldmesh = NULL;
  xml.bspmesh = NULL;
  xml.rapidmesh = NULL;
  xml.hasBSP = true;
  xml.hasColTree=true;
  xml.unitlevel=0;
  xml.unitscale=1;
  xml.data=xml.shieldmesh=xml.bspmesh=xml.rapidmesh=NULL;//was uninitialized memory
  
  string tmpstr;
  csvRow = row[0];
  //
  image->cargo_volume = atof(row["CargoVolume"].c_str());


  //begin the geometry (and things that depend on stats)

  fullname=row["Name"];
  //image->description=row["Description"];
  if ((tmpstr=row["Hud_image"]).length()!=0) {
    image->hudImage = createVSSprite(tmpstr.c_str());
  }  
  combat_role=ROLES::getRole(row["Combat_Role"]);
  graphicOptions.NumAnimationPoints=stoi(row["Num_Animation_Stages"],0);
  if (graphicOptions.NumAnimationPoints>0)
    graphicOptions.Animating=0;
  xml.unitscale = stof(row["Unit_Scale"],1);
  if (!xml.unitscale) xml.unitscale=1;
  image->unitscale=xml.unitscale;
  AddMeshes(xml,row["Mesh"],faction,getFlightgroup());
  AddDocks(this,xml,row["Dock"]);
  AddSubUnits(this,xml,row["Sub_Units"],faction,modification);
  meshdata= xml.meshes;
  meshdata.push_back(NULL);
  corner_min = Vector(FLT_MAX, FLT_MAX, FLT_MAX);
  corner_max = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  calculate_extent(false);
  AddMounts(this,xml,row["Mounts"]);
  this->image->cargo_volume=stof(row["Hold_Volume"]);
  this->image->equipment_volume=stof(row["Equipment_Space"]);
  ImportCargo(this,row["Cargo_Import"]);
  AddCarg(this,row["Cargo"]);
  AddSounds(this,row["Sounds"]);
  
  LoadCockpit(this,row["Cockpit"]);
  image->CockpitCenter.i=stof(row["CockpitX"])*xml.unitscale;
  image->CockpitCenter.j=stof(row["CockpitY"])*xml.unitscale;
  image->CockpitCenter.k=stof(row["CockpitZ"])*xml.unitscale;
  Mass=stof(row["Mass"],1.0);
  Momentofinertia=stof(row["Moment_Of_Inertia"],1.0);
  fuel=stof(row["Fuel_Capacity"]);
  hull=maxhull = stof(row["Hull"]);
  armor.frontlefttop=stof(row["Armor_Front_Top_Left"]);
  armor.frontrighttop=stof(row["Armor_Front_Top_Right"]);
  armor.backlefttop=stof(row["Armor_Back_Top_Left"]);
  armor.backrighttop=stof(row["Armor_Back_Top_Right"]);
  armor.frontleftbottom=stof(row["Armor_Front_Bottom_Left"]);
  armor.frontrightbottom=stof(row["Armor_Front_Bottom_Right"]);
  armor.backleftbottom=stof(row["Armor_Back_Bottom_Left"]);
  armor.backrightbottom=stof(row["Armor_Back_Bottom_Right"]);
  int shieldcount=0;
  Shield two;
  Shield four;
  Shield eight;
  memset(&two,0,sizeof(Shield));
  memset(&four,0,sizeof(Shield));
  memset(&eight,0,sizeof(Shield));
  shieldcount+=AssignIf(row["Shield_Front_Top_Right"],
                        two.shield2fb.front,four.shield4fbrl.front,eight.shield8.frontrighttop);
  shieldcount+=AssignIf(row["Shield_Front_Top_Left"],
                        two.shield2fb.front,four.shield4fbrl.front,eight.shield8.frontlefttop);
  shieldcount+=AssignIf(row["Shield_Back_Top_Left"],
                        two.shield2fb.back,four.shield4fbrl.back,eight.shield8.backlefttop);
  shieldcount+=AssignIf(row["Shield_Back_Top_Right"],
                        two.shield2fb.back,four.shield4fbrl.back,eight.shield8.backrighttop);

  shieldcount+=AssignIf(row["Shield_Front_Bottom_Left"],
                        two.shield2fb.front,four.shield4fbrl.left,eight.shield8.frontleftbottom);
  shieldcount+=AssignIf(row["Shield_Front_Bottom_Right"],
                        two.shield2fb.front,four.shield4fbrl.right,eight.shield8.frontrightbottom);
  shieldcount+=AssignIf(row["Shield_Back_Bottom_Left"],
                        two.shield2fb.back,four.shield4fbrl.left,eight.shield8.backleftbottom);
  shieldcount+=AssignIf(row["Shield_Back_Bottom_Right"],
                        two.shield2fb.back,four.shield4fbrl.right,eight.shield8.backrightbottom);
  two.shield2fb.frontmax=two.shield2fb.front;
  two.shield2fb.backmax=two.shield2fb.back;
  four.shield4fbrl.frontmax=four.shield4fbrl.front;
  four.shield4fbrl.backmax=four.shield4fbrl.back;
  four.shield4fbrl.rightmax=four.shield4fbrl.right;
  four.shield4fbrl.leftmax=four.shield4fbrl.left;

  eight.shield8.frontlefttopmax=eight.shield8.frontlefttop;
  eight.shield8.frontrighttopmax=eight.shield8.frontrighttop;
  eight.shield8.backrighttopmax=eight.shield8.backrighttop;
  eight.shield8.backlefttopmax=eight.shield8.backlefttop;
  eight.shield8.frontleftbottommax=eight.shield8.frontleftbottom;
  eight.shield8.frontrightbottommax=eight.shield8.frontrightbottom;
  eight.shield8.backrightbottommax=eight.shield8.backrightbottom;
  eight.shield8.backleftbottommax=eight.shield8.backleftbottom;
  
  if (shieldcount==8) {
    shield=eight;
    shield.number=8;
    }else if (shieldcount==4){
    shield=four;
    shield.number=4;
  }else {
    shield=two;
    shield.number=2;
  }
  shield.leak = (char)(stof(row["Shield_Leak"])*100.0);
  shield.recharge=stof(row["Shield_Recharge"]);
  maxwarpenergy=warpenergy=stof(row["Warp_Capacitor"]);
  maxenergy=energy=stof(row["Primary_Capacitor"]);
  recharge=stof(row["Reactor_Recharge"]);
  jump.drive=XMLSupport::parse_bool(row["Jump_Drive_Present"])?-1:-2;
  jump.delay = stoi(row["Jump_Drive_Delay"]);
  image->forcejump=XMLSupport::parse_bool(row["Wormhole"]);
  jump.energy=stof(row["Outsystem_Jump_Cost"]);
  jump.insysenergy=stof(row["Warp_Usage_Cost"]);
  afterburnenergy=stof(row["Afterburner_Usage_Cost"]);
  if (stoi(row["Afterburner_Type"])==1)
    afterburnenergy*=-1;
  limits.yaw=stof(row["Maneuver_Yaw"])*VS_PI/180.;
  limits.pitch=stof(row["Maneuver_Pitch"])*VS_PI/180.;
  limits.roll=stof(row["Maneuver_Roll"])*VS_PI/180.;
  {
    std::string t,tn,tp;
    t=row["Yaw_Governor"];
    tn=row["Yaw_Governor_Right"];
    tp=row["Yaw_Governor_Left"];
    computer.max_yaw_right=stof(tn.length()>0?tn:t)*VS_PI/180.;
    computer.max_yaw_left=stof(tp.length()>0?tp:t)*VS_PI/180.;
    t=row["Pitch_Governor"];
    tn=row["Pitch_Governor_Up"];
    tp=row["Pitgh_Governor_Down"];
    computer.max_pitch_up=stof(tn.length()>0?tn:t)*VS_PI/180.;
    computer.max_pitch_down=stof(tp.length()>0?tp:t)*VS_PI/180.;
    t=row["Roll_Governor"];
    tn=row["Roll_Governor_Right"];
    tp=row["Roll_Governor_Left"];
    computer.max_roll_right=stof(tn.length()>0?tn:t)*VS_PI/180.;
    computer.max_roll_left=stof(tp.length()>0?tp:t)*VS_PI/180.;
  }
  static float game_accel=XMLSupport::parse_float(vs_config->getVariable("physics","game_accel","1"));
  static float game_speed=XMLSupport::parse_float(vs_config->getVariable("physics","game_speed","1"));
  limits.afterburn = stof(row["Afterburner_Accel"])*game_accel*game_speed;
  limits.forward = stof(row["Forward_Accel"])*game_accel*game_speed;
  limits.retro = stof(row["Retro_Accel"])*game_accel*game_speed;
  limits.lateral = .5*(stof(row["Left_Accel"])+stof(row["Right_Accel"]))*game_accel*game_speed;
  limits.vertical = .5*(stof(row["Top_Accel"])+stof(row["Bottom_Accel"]))*game_accel*game_speed;
  computer.max_combat_speed=stof(row["Default_Speed_Governor"])*game_speed;
  computer.max_combat_ab_speed=stof(row["Afterburner_Speed_Governor"])*game_speed;
  computer.itts = stob(row["ITTS"],true);
  computer.radar.color=stob(row["Radar_Color"],true);
  computer.radar.maxrange=stof(row["Radar_Range"],FLT_MAX);
  computer.radar.maxcone=cos(stof(row["Max_Cone"],180)*VS_PI/180);
  computer.radar.trackingcone=cos(stof(row["Tracking_Cone"],180)*VS_PI/180);
  computer.radar.lockcone=cos(stof(row["Lock_Cone"],180)*VS_PI/180);
  cloakmin=(int)(stof(row["Cloak_Min"])*2147483136);
  if (cloakmin<0) cloakmin=0;
  image->cloakglass=XMLSupport::parse_bool(row["Cloak_Glass"]);
  if ((cloakmin&0x1)&&!image->cloakglass) {
    cloakmin-=1;
  }
  if ((cloakmin&0x1)==0&&image->cloakglass) {
    cloakmin+=1;
  }


  if (!XMLSupport::parse_bool(row["Can_Cloak"]))
    cloaking=-1;
  else
    cloaking = (int)(-2147483647)-1;
  image->cloakrate = (int)(2147483136.*stof(row["Cloak_Rate"])); //short fix  
  image->cloakenergy=stof(row["Cloak_Energy"]);
  image->repair_droid=stoi(row["Repair_Droid"]);
  image->ecm = stoi(row["ECM_Rating"]);
  if (image->ecm<0) image->ecm*=-1;
  if (image->cockpit_damage){
    HudDamage(image->cockpit_damage,row["Hud_Functionality"]);
    HudDamage(image->cockpit_damage+1+MAXVDUS+UnitImages::NUMGAUGES,row["Max_Hud_Functionality"]);
  }
  image->LifeSupportFunctionality=stof(row["Lifesupport_Functionality"]);
  image->LifeSupportFunctionalityMax=stof(row["Max_Lifesupport_Functionality"]);
  image->CommFunctionality=stof(row["Comm_Functionality"]);
  image->CommFunctionalityMax=stof(row["Max_Comm_Functionality"]);
  image->fireControlFunctionality=stof(row["FireControl_Functionality"]);
  image->fireControlFunctionalityMax=stof(row["Max_FireControl_Functionality"]);
  image->SPECDriveFunctionality=stof(row["SPECDrive_Functionality"]);
  image->SPECDriveFunctionalityMax=stof(row["Max_SPECDrive_Functionality"]);
  computer.slide_start=stoi(row["Slide_Start"]);
  computer.slide_end=stoi(row["Slide_End"]);
  UpgradeUnit(this,row["Upgrades"]);
  
  this->image->explosion_type = row["Explosion"];
  if (image->explosion_type.length())
    cache_ani (image->explosion_type);
  AddLights(this,xml,row["Light"]);

  xml.shieldmesh_str = row["Shield_Mesh"];
  if (xml.shieldmesh_str.length()){
    addShieldMesh(&xml,xml.shieldmesh_str.c_str(),xml.unitscale,faction,getFlightgroup());
    meshdata.back()=xml.shieldmesh;
  }else {
    static int shieldstacks = XMLSupport::parse_int (vs_config->getVariable ("graphics","shield_detail","16"));
    meshdata.back()= new SphereMesh (rSize(),shieldstacks,shieldstacks,vs_config->getVariable("graphics","shield_texture","shield.bmp").c_str(), NULL, false,ONE, ONE);
  }
  meshdata.back()->EnableSpecialFX();
  //Begin the Pow-w-w-war Zone Collide Tree Generation
  {
    xml.bspmesh_str = row["BSP_Mesh"];
    xml.rapidmesh_str = row["Rapid_Mesh"];
    vector<bsp_polygon> polies;
    std::string collideTreeHash = VSFileSystem::GetHashName(string(modification)+"#"+row[0]);
    this->colTrees = collideTrees::Get(collideTreeHash);
    if (this->colTrees) {
      this->colTrees->Inc();
    }
    BSPTree * bspTree=NULL;
    BSPTree * bspShield=NULL;
    csRapidCollider *colShield=NULL;
    csRapidCollider *colTree=NULL;
    string tmpname = row[0];//key
    if (!this->colTrees) {
      string val;
      xml.hasBSP=1;
      xml.hasColTree=1;
      if ((val=row["Use_BSP"]).length()) {
        xml.hasBSP = XMLSupport::parse_bool(val);
      }
      if ((val=row["Use_Rapid"]).length()) {
        xml.hasColTree= XMLSupport::parse_bool(val);
      }
      if (xml.shieldmesh) {
        if (!CheckBSP ((tmpname+"_shield.bsp").c_str())) {
          BuildBSPTree ((tmpname+"_shield.bsp").c_str(), false, meshdata.back());
        }
        if (CheckBSP ((tmpname+"_shield.bsp").c_str())) {
          bspShield = new BSPTree ((tmpname+"_shield.bsp").c_str());
        }
        if (meshdata.back()) {
          meshdata.back()->GetPolys(polies);
          colShield = new csRapidCollider (polies);
        }
      }
      if (xml.rapidmesh_str.length())
        addRapidMesh(&xml,xml.rapidmesh_str.c_str(),xml.unitscale,faction,getFlightgroup());
      else 
        xml.rapidmesh=NULL;
      if (xml.bspmesh_str.length())
        addBSPMesh(&xml,xml.bspmesh_str.c_str(),xml.unitscale,faction,getFlightgroup());
      else
        xml.bspmesh=NULL;
      if (xml.hasBSP) {
        tmpname += ".bsp";
        if (!CheckBSP (tmpname.c_str())) {
          BuildBSPTree (tmpname.c_str(), false, xml.bspmesh);
        }
        if (CheckBSP (tmpname.c_str())) {
          bspTree = new BSPTree (tmpname.c_str());
        }	
      } else {
        bspTree = NULL;
      }
      polies.clear();
      if (xml.rapidmesh) {
        xml.rapidmesh->GetPolys(polies);
      }
      csRapidCollider * csrc=NULL;
      if (xml.hasColTree) {
        csrc=getCollideTree(Vector(1,1,1),
                            xml.rapidmesh?
                            &polies:NULL);
      }
      this->colTrees = new collideTrees (collideTreeHash,
                                         bspTree,
                                         bspShield,
                                         csrc,
                                         colShield);
      if (xml.rapidmesh&&xml.hasColTree) {
        //if we have a special rapid mesh we need to generate things now
        for (int i=1;i<collideTreesMaxTrees;++i) {
          if (!this->colTrees->rapidColliders[i]) {
            unsigned int which = 1<<i;
            this->colTrees->rapidColliders[i]= 
              getCollideTree(Vector (1,1,which),
                             &polies);
          }
        }
      }
      if (xml.bspmesh) {
        delete xml.bspmesh;
        xml.bspmesh=NULL;
      }
      if (xml.rapidmesh) {
        delete xml.rapidmesh;
        xml.rapidmesh=NULL;
      }
    }
  }
}
CSVRow GetUnitRow(string filename, bool subu, int faction, bool readlast, bool &rread) {
  std::string hashname = filename+"__"+FactionUtil::GetFactionName(faction);
  for (int i=((int)unitTables.size())-(readlast?1:2);i>=0;--i) {
    unsigned int where;
    if (unitTables[i]->RowExists(hashname,where)) {
      rread=true;
      return CSVRow(unitTables[i],where);
    }else if (unitTables[i]->RowExists(filename,where)) {
      rread=true;
      return CSVRow(unitTables[i],where); 
    }
  }
  rread=false;
  return CSVRow();
}

void Unit::WriteUnit (const char * modifications) {
  static bool UNITTAB = XMLSupport::parse_bool(vs_config->getVariable("physics","UnitTable","false"));
  if (UNITTAB) {
    bool bad=false;
    if (!modifications)bad=true;
    if (!bad) {
      if(!strlen(modifications)) {
        bad=true;
      }
    }
    if (bad) {
      fprintf(stderr,"Cannot Write out unit file %s %s that has no filename\n",name.c_str(),csvRow.c_str());
      return;
    }
    std::string savedir = modifications;
    VSFileSystem::CreateDirectoryHome( VSFileSystem::savedunitpath+"/"+savedir);
    VSFile f;
    //string filepath( savedir+"/"+this->filename);
    //cerr<<"Saving Unit to : "<<filepath<<endl;
    VSError err = f.OpenCreateWrite( savedir+"/"+name+".csv", UnitFile);
    if (err>Ok) {
      fprintf( stderr, "!!! ERROR : Writing saved unit file : %s\n", f.GetFullPath().c_str() );
      return;
    }
    std::string towrite = WriteUnitString();
    f.Write(towrite.c_str(),towrite.length());
    f.Close();
  }else {
    if (image->unitwriter)
      image->unitwriter->Write(modifications);
    for (un_iter ui= getSubUnits();(*ui)!=NULL;++ui) {
      (*ui)->WriteUnit(modifications);
    } 
  }
}
using XMLSupport::tostring;

static void mapToStringVec(map<string,string> a, vector<string> &key, vector<string> &value) {
  for (map<string,string>::iterator i = a.begin();i!=a.end();++i) {
    key.push_back(i->first);
    value.push_back(i->second);
  }
}
static string tos(float val) {
  return XMLSupport::tostring(val);
}
static string tos(double val) {
  return XMLSupport::tostring((float)val);
}
static string tos(unsigned int val) {
  return XMLSupport::tostring(val);
}
static string tos(bool val) {
  return XMLSupport::tostring((int)val);
}
static string tos(int val) {
  return XMLSupport::tostring(val);
}
string Unit::WriteUnitString () {
  static bool UNITTAB = XMLSupport::parse_bool(vs_config->getVariable("physics","UnitTable","false"));
  string ret="";
  if (UNITTAB) {
    //this is the fillin part
    //fixme

    for (int i=unitTables.size()-1;i>=0;--i) {
      unsigned int where;
      string val;
      if (unitTables[i]->RowExists(csvRow,where)) {
        CSVRow row(unitTables[i],where);        
        map<string,string> unit;        
        for (int jj=0;
             jj<row.size();
             ++jj) {
          if (jj!=0)
            unit[row.getKey(jj)]=row[jj];
        }
        // mutable things
        unit["Equipment_Space"]=XMLSupport::tostring(image->equipment_volume);
        unit["Hold_Volume"]=XMLSupport::tostring(image->cargo_volume);
        string mountstr;
        double unitScale=  stof(unit["Unit_Scale"],1);
        {//mounts
          for (int j=0;j<mounts.size();++j) {
            char mnt[1024];
            Matrix m;
            Transformation tr(mounts[j].GetMountOrientation(),
                              mounts[j].GetMountLocation().Cast());
            tr.to_matrix(m);
            string printedname=mounts[j].type->weapon_name;
            if (mounts[j].status==Mount::DESTROYED||mounts[j].status==Mount::UNCHOSEN) {
              printedname="";
            }
            mountstr+="{"+printedname+";"+XMLSupport::tostring(mounts[j].ammo)+";"+XMLSupport::tostring(mounts[j].volume)+";"+lookupMountSize(mounts[j].size);
            sprintf(mnt,";%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf}",
                    m.p.i/unitScale,
                    m.p.j/unitScale,
                    m.p.k/unitScale,
                    (double)mounts[j].xyscale/unitScale,
                    (double)mounts[j].zscale/unitScale,
                    (double)m.getR().i,
                    (double)m.getR().j,
                    (double)m.getR().k,
                    (double)m.getQ().i,
                    (double)m.getQ().j,
                    (double)m.getQ().k,
                    (double)mounts[j].functionality,
                    (double)mounts[j].maxfunctionality
                    );
            mountstr+=mnt;
          }
          unit["Mounts"]=mountstr;
        }
        {//subunits
          vector<SubUnitStruct> subunits=GetSubUnits(unit["Sub_Units"]);
          if (subunits.size()) {
            unsigned int k=0;
            Unit * subun;
            for (;k<subunits.size();++k) {
              subunits[k].filename="destroyed_blank";
            }
            k=0;
            for (un_iter su=this->getSubUnits();(subun=(*su))!=NULL;++su,++k) {
              unsigned int j=k;
              for (;j<subunits.size();++j) {
                if ((subun->Position()-subunits[j].pos).MagnitudeSquared()<.00000001) {
                  //we've got a hit
                  break;
                }
              }
              if (j==subunits.size()) j=k;
              subunits[j].filename=subun->name;
            }
            string str;
            for (k=0;k<subunits.size();++k) {
              char tmp[1024];
              sprintf(tmp,";%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf;%lf}",
                      subunits[k].pos.i,
                      subunits[k].pos.j,
                      subunits[k].pos.k,
                      subunits[k].R.i,
                      subunits[k].R.j,
                      subunits[k].R.k,
                      subunits[k].Q.i,
                      subunits[k].Q.j,
                      subunits[k].Q.k,
                      ((double)acos(subunits[k].restricted)*180./VS_PI));
              str+="{"+subunits[k].filename+tmp;
            }
            unit["Sub_Units"]=str;
          }
        }
        {
          string carg;
          for (unsigned int i=0;i<numCargo();++i) {
            Cargo *c=&GetCargo(i);
            char tmp[2048];
            sprintf (tmp,";%f;%d;%f;%f;%f;%f;;%s}",
                     c->price,
                     c->quantity,
                     c->mass,
                     c->volume,
                     c->functionality,
                     c->maxfunctionality,
                     c->mission?"true":"false");
            carg+="{"+c->content+";"+c->category+tmp;
          }
          unit["Cargo"]=carg;
        }
        unit["Mass"]=tos(Mass);
        unit["Moment_Of_Inertia"]=tos(Momentofinertia);
        unit["Fuel_Capacity"]=tos(fuel);
        unit["Hull"]=tos(hull);
        unit["Armor_Front_Top_Left"]=tos(armor.frontlefttop);
        unit["Armor_Front_Top_Right"]=tos(armor.frontrighttop);
        unit["Armor_Back_Top_Left"]=tos(armor.backlefttop);
        unit["Armor_Back_Top_Right"]=tos(armor.backrighttop);
        unit["Armor_Front_Bottom_Left"]=tos(armor.frontleftbottom);
        unit["Armor_Front_Bottom_Right"]=tos(armor.frontrightbottom);
        unit["Armor_Back_Bottom_Left"]=tos(armor.backleftbottom);
        unit["Armor_Back_Bottom_Right"]=tos(armor.backrightbottom);        
        {
          unit["Shield_Front_Top_Right"]="";
          unit["Shield_Front_Top_Left"]="";
          unit["Shield_Back_Top_Right"]="";
          unit["Shield_Back_Top_Left"]="";
          unit["Shield_Front_Bottom_Right"]="";
          unit["Shield_Front_Bottom_Left"]="";
          unit["Shield_Back_Bottom_Right"]="";
          unit["Shield_Back_Bottom_Left"]="";

          switch (shield.number) {
          case 8:
            unit["Shield_Front_Top_Right"]=tos(shield.shield8.frontrighttopmax);
            unit["Shield_Front_Top_Left"]=tos(shield.shield8.frontlefttopmax);
            unit["Shield_Back_Top_Right"]=tos(shield.shield8.backrighttopmax);
            unit["Shield_Back_Top_Left"]=tos(shield.shield8.backlefttopmax);
            unit["Shield_Front_Bottom_Right"]=tos(shield.shield8.frontrightbottommax);
            unit["Shield_Front_Bottom_Left"]=tos(shield.shield8.frontleftbottommax);
            unit["Shield_Back_Bottom_Right"]=tos(shield.shield8.backrightbottommax);
            unit["Shield_Back_Bottom_Left"]=tos(shield.shield8.backleftbottommax);            
            break;
          case 4:
            unit["Shield_Front_Top_Right"]=tos(shield.shield4fbrl.frontmax);            
            unit["Shield_Back_Top_Right"]=tos(shield.shield4fbrl.backmax);
            unit["Shield_Front_Bottom_Right"]=tos(shield.shield4fbrl.rightmax);
            unit["Shield_Front_Bottom_Left"]=tos(shield.shield4fbrl.leftmax);            
            break;
          case 2:
          default:
            //assume 2 shields
            unit["Shield_Front_Top_Right"]=tos(shield.shield2fb.frontmax);            
            unit["Shield_Back_Top_Right"]=tos(shield.shield2fb.backmax);
            break;
          }
        }
        unit["Shield_Leak"]=tos(shield.leak/100.0);
        unit["Shield_Recharge"]=tos(shield.recharge);
        unit["Warp_Capacitor"]=tos(maxwarpenergy);
        unit["Primary_Capacitor"]=tos(maxenergy);
        unit["Reactor_Recharge"]=tos(recharge);
        unit["Jump_Drive_Present"]=tos(jump.drive>=-1);
        unit["Jump_Drive_Delay"]=tos(jump.delay);
        unit["Wormhole"]=tos(image->forcejump!=0);
        unit["Outsystem_Jump_Cost"]=tos(jump.energy);
        unit["Warp_Usage_Cost"]=tos(jump.insysenergy);
        unit["Afterburner_Usage_Cost"]=tos(afterburnenergy>0?afterburnenergy:-afterburnenergy);
        unit["Afterburner_Type"]=tos(afterburnenergy>=0?0:1);
        unit["Maneuver_Yaw"]=tos(limits.yaw*180/(VS_PI));
        unit["Maneuver_Pitch"]=tos(limits.pitch*180/(VS_PI));
        unit["Maneuver_Roll"]=tos(limits.roll*180/(VS_PI));
        unit["Yaw_Governor_Right"]=tos(computer.max_yaw_right*180/VS_PI);
        unit["Yaw_Governor_Left"]=tos(computer.max_yaw_left*180/VS_PI);
        unit["Pitch_Governor_Up"]=tos(computer.max_pitch_up*180/VS_PI);
        unit["Pitch_Governor_Down"]=tos(computer.max_pitch_down*180/VS_PI);
        unit["Roll_Governor_Right"]=tos(computer.max_roll_right*180/VS_PI);
        unit["Roll_Governor_Left"]=tos(computer.max_roll_left*180/VS_PI);
        static float game_accel=XMLSupport::parse_float(vs_config->getVariable("physics","game_accel","1"));
        static float game_speed=XMLSupport::parse_float(vs_config->getVariable("physics","game_speed","1"));
        unit["Afterburner_Accel"]=tos(limits.afterburn/(game_accel*game_speed));
        unit["Forward_Accel"]=tos(limits.forward/(game_accel*game_speed));
        unit["Retro_Accel"]=tos(limits.retro/(game_accel*game_speed));
        unit["Left_Accel"]=unit["Right_Accel"]=tos(limits.lateral/(game_accel*game_speed));
        unit["Bottom_Accel"]=unit["Top_Accel"]=tos(limits.vertical/(game_accel*game_speed));
        unit["Default_Speed_Governor"]=tos(computer.max_combat_speed/game_speed);
        unit["Afterburner_Speed_Governor"]=tos(computer.max_combat_ab_speed/game_speed);
        unit["ITTS"]=tos(computer.itts);
        unit["Radar_Color"]=tos(computer.radar.color);
        unit["Radar_Range"]=tos(computer.radar.maxrange);
        unit["Tracking_Cone"]=tos(acos(computer.radar.trackingcone)*180./VS_PI);
        unit["Max_Cone"]=tos(acos(computer.radar.maxcone)*180./VS_PI);
        unit["Lock_Cone"]=tos(acos(computer.radar.lockcone)*180./VS_PI);
        unit["Cloak_Min"]=tos(cloakmin/2147483136.);
        unit["Can_Cloak"]=tos(cloaking!=-1);
        unit["Cloak_Rate"]=tos(fabs(image->cloakrate/2147483136.));
        unit["Cloak_Energy"]=tos(image->cloakenergy);
        unit["Cloak_Glass"]=tos(image->cloakglass);
        unit["Repair_Droid"]=tos(image->repair_droid);
        unit["ECM"]=tos(image->ecm>0?image->ecm:-image->ecm);
        unit["Hud_Functionality"]=WriteHudDamage(this);
        unit["Max_Hud_Functionality"]=WriteHudDamageFunc(this);

        unit["Lifesupport_Functionality"]=tos(image->LifeSupportFunctionality);       
        unit["Max_Lifesupport_Functionality"]=tos(image->LifeSupportFunctionalityMax);
        unit["Comm_Functionality"]=tos(image->CommFunctionality);
        unit["Max_Comm_Functionality"]=tos(image->CommFunctionalityMax);        
        unit["Comm_Functionality"]=tos(image->CommFunctionality);
        unit["Max_Comm_Functionality"]=tos(image->CommFunctionalityMax);
        unit["FireControl_Functionality"]=tos(image->fireControlFunctionality);
        unit["Max_FireControl_Functionality"]=tos(image->fireControlFunctionalityMax);
        unit["SPECDrive_Functionality"]=tos(image->SPECDriveFunctionality);
        unit["Max_SPECDrive_Functionality"]=tos(image->SPECDriveFunctionalityMax);
        unit["Slide_Start"]=tos(computer.slide_start);
        unit["Slide_End"]=tos(computer.slide_end);
        unit["Cargo_Import"]=unit["Upgrades"]="";//make sure those are empty        
        vector<string>keys,values;
        keys.push_back("Key");
        values.push_back(csvRow);//key has to come first
        mapToStringVec(unit,keys,values);
        return writeCSV(keys,values);
      }
    }
    fprintf (stderr,"Failed to locate base mesh for %s %s %s\n",csvRow.c_str(),name.c_str(),fullname.c_str());
  }else {
    if (image->unitwriter)
      ret = image->unitwriter->WriteString();
    for (un_iter ui= getSubUnits();(*ui)!=NULL;++ui) {
      ret = ret + ((*ui)->WriteUnitString());
    }
  }
  return ret;
}
void UpdateMasterPartList(Unit * ret) {
  for (int i=0;i<_Universe->numPlayers();++i) {
    Cockpit* cp = _Universe->AccessCockpit(i);
    std::vector<std::string>* addedcargoname= &cp->savegame->getMissionStringData("master_part_list_content");
    std::vector<std::string>* addedcargocat= &cp->savegame->getMissionStringData("master_part_list_category");
    std::vector<std::string>* addedcargovol= &cp->savegame->getMissionStringData("master_part_list_volume");
    std::vector<std::string>* addedcargoprice= &cp->savegame->getMissionStringData("master_part_list_price");
    std::vector<std::string>* addedcargomass= &cp->savegame->getMissionStringData("master_part_list_mass");
    std::vector<std::string>* addedcargodesc= &cp->savegame->getMissionStringData("master_part_list_description");
    for (unsigned int j=0;j<addedcargoname->size();++j) {
      Cargo carg;
      carg.content=(*addedcargoname)[j];
      carg.category=(j<addedcargocat->size()?(*addedcargocat)[j]:std::string("Uncategorized"));
      carg.volume=(j<addedcargovol->size()?XMLSupport::parse_float((*addedcargovol)[j]):1.0);
      carg.price=(j<addedcargoprice->size()?XMLSupport::parse_float((*addedcargoprice)[j]):0.0);
      carg.mass=(j<addedcargomass->size()?XMLSupport::parse_float((*addedcargomass)[j]):.01);
      carg.description=(j<addedcargodesc->size()?(*addedcargodesc)[j]:std::string("No Description Added"));
      carg.quantity=1;
      ret->GetImageInformation().cargo.push_back(carg);
    }
  }
  std::sort(ret->GetImageInformation().cargo.begin(),  ret->GetImageInformation().cargo.end());
  {
    Cargo last_cargo;
    for (int i=ret->numCargo()-1;i>=0;--i) {
      if (ret->GetCargo(i).content==last_cargo.content&&
          ret->GetCargo(i).category==last_cargo.category) {
        ret->RemoveCargo(i,ret->GetCargo(i).quantity,true);
      }else {
        last_cargo=ret->GetCargo(i);
      }
    }
  }
}
Unit * Unit::makeMasterPartList() {
  static std::string mpl = vs_config->getVariable("data","master_part_list","master_part_list");
  Unit * ret = new Unit();
  ret->name="master_part_list";
  CSVTable table(mpl);
  unsigned int siz=table.rows.size();
  unsigned int i;
  for (i=0;i<siz;++i) {
    CSVRow row(&table,i);
    Cargo carg;
    carg.content=row["file"];
    carg.category=row["categoryname"];
    carg.volume=stof(row["volume"],1);
    carg.mass=stof(row["mass"],1);
	carg.quantity=1;
    carg.price=stoi(row["price"],1);
    carg.description=row["description"];    
    ret->GetImageInformation().cargo.push_back(carg);
  } 
  UpdateMasterPartList(ret);
  return ret;
}
