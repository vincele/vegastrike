//#include <gfx/aux_texture.h>
//#include "gfx/animation.h"
#include <vector>
#include <string>
#include <expat.h>
#include "vegastrike.h"
#include "xml_support.h"
#include <assert.h>
#include "ai/communication.h"
#include "unit_factory.h"
#include "hashtable.h"
#include "cmd/music.h"
#include "faction_generic.h"
//#include "faction_util.h"
static int unitlevel;
using namespace XMLSupport;
using XMLSupport::EnumMap;
using XMLSupport::Attribute;
using XMLSupport::AttributeList;
//using std::sort;
using std::map;

static FSM * getFSM (const std::string & value) {
  static Hashtable <std::string, FSM, 16> fsms;
  FSM * fsm = fsms.Get (value);
  if (fsm) {
    return fsm;
  }else {
    if (value!="FREE_THIS_LOAD") {
      FSM * retval= new FSM (value.c_str());
      fsms.Put (value,retval);
      return retval;
    }
  }
  return NULL;
}
#if 0
static FSM * getFSM (const std::string &value) {
  static map <const std::string, FSM *> fsms;
  map<const std::string,FSM*>::iterator i = fsms.find (value);
  if (i!=fsms.end()) {
    return (*i).second;
  }else {
    if (value!="FREE_THIS_LOAD") {
      FSM * retval = new FSM (value.c_str());
      fsms.insert (pair<const std::string,FSM *>(value,retval));
      return retval;
    }else {
      for (i=fsms.begin();i!=fsms.end();i++) {
	delete ((*i).second);
      }
      return NULL;
    }
  }
}
#endif
namespace FactionXML {
  enum Names {
	UNKNOWN,
	FACTIONS,
	FACTION,
	NAME,
	LOGORGB,
	LOGOA,
	SECLOGORGB,
	SECLOGOA,
	RELATION,
	STATS,
	FRIEND,
	ENEMY,
	CONVERSATION,
	COMM_ANIMATION,
	MOOD_ANIMATION,
	CONTRABAND,
	EXPLOSION,
	SEX,
        BASE_ONLY,
        DOCKABLE_ONLY,
	SPARKRED,
	SPARKGREEN,
	SPARKBLUE,
	SPARKALPHA,
        SHIPMODIFIER
  };

  const EnumMap::Pair element_names[] = {
	EnumMap::Pair ("UNKNOWN", UNKNOWN),
	EnumMap::Pair ("Factions", FACTIONS),
	EnumMap::Pair ("Faction", FACTION),
	EnumMap::Pair ("Friend", FRIEND),
	EnumMap::Pair ("Enemy", ENEMY),
  	EnumMap::Pair ("Stats", STATS),
  	EnumMap::Pair ("CommAnimation", COMM_ANIMATION),
  	EnumMap::Pair ("MoodAnimation", MOOD_ANIMATION),
  	EnumMap::Pair ("Explosion", EXPLOSION),
        EnumMap::Pair ("ShipModifier",SHIPMODIFIER)
  };
  const EnumMap::Pair attribute_names[] = {
	EnumMap::Pair ("UNKNOWN", UNKNOWN),
	EnumMap::Pair ("name", NAME), 
	EnumMap::Pair ("SparkRed", SPARKRED), 
	EnumMap::Pair ("SparkGreen", SPARKGREEN), 
	EnumMap::Pair ("SparkBlue", SPARKBLUE), 
	EnumMap::Pair ("SparkAlpha", SPARKALPHA), 
	EnumMap::Pair ("logoRGB", LOGORGB), 
	EnumMap::Pair ("logoA", LOGOA), 
	EnumMap::Pair ("secLogoRGB", SECLOGORGB), 
	EnumMap::Pair ("secLogoA", SECLOGOA), 
	EnumMap::Pair ("relation",RELATION),
	EnumMap::Pair ("Conversation", CONVERSATION),
	EnumMap::Pair ("Contraband",CONTRABAND),
	EnumMap::Pair ("sex",SEX),
	EnumMap::Pair ("base_only",BASE_ONLY),
	EnumMap::Pair ("dockable_only",DOCKABLE_ONLY)
};


  const EnumMap element_map(element_names, 10);
  const EnumMap attribute_map(attribute_names, 16);

}

static vector <std::string> contrabandlists;
void Faction::beginElement(void *userData, const XML_Char *names, const XML_Char **atts) {
 using namespace FactionXML;
 //Universe * thisuni = (Universe *) userData;
//	((Universe::Faction*)userData)->beginElement(name, AttributeList(atts));
  AttributeList attributes=AttributeList(atts);
  string name=names;
  AttributeList::const_iterator iter;
  Names elem = (Names)element_map.lookup(name);
  char * tmpstr=NULL;
  char RGBfirst=0;
  std::string secString;
  std::string secStringAlph;
  switch(elem) {
  case UNKNOWN:
	unitlevel++;

//	cerr << "Unknown element start tag '" << name << "' detected " << endl;
	return;

  case FACTIONS:
	assert (unitlevel==0);
	unitlevel++;
	break;
  case EXPLOSION:
    assert (unitlevel==2);
    unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case NAME:
	factions.back()->explosion.push_back (FactionUtil::createAnimation((*iter).value.c_str()));
	factions.back()->explosion_name.push_back ((*iter).value);
	break;
      }
    }      
    break;
  case SHIPMODIFIER:
    assert (unitlevel==2);
    unitlevel++;
    {
      string name;float rel=0;
      for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
        switch(attribute_map.lookup((*iter).name)) {
        case NAME:
          name= (*iter).value;
          break;
        case RELATION:
          rel=XMLSupport::parse_float((*iter).value);
          break;
        }
      }
      if (rel!=0&&name.length()) {
        factions.back()->ship_relation_modifier[name]=rel;
      }
    }
    break;
  case COMM_ANIMATION:
    assert (unitlevel==2);
    unitlevel++;
    factions.back()->comm_faces.push_back (Faction::comm_face_t());
    factions.back()->comm_face_sex.push_back (0);
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case SEX:
	factions.back()->comm_face_sex.back() = parse_int ((*iter).value);
	break;
      case DOCKABLE_ONLY:
        factions.back()->comm_faces.back().dockable=parse_bool((*iter).value)?comm_face_t::CYES:comm_face_t::CNO;
        
        break;
      case BASE_ONLY:
        factions.back()->comm_faces.back().base=parse_bool((*iter).value)?comm_face_t::CYES:comm_face_t::CNO;
        break;
      }
    }
    break;
  case MOOD_ANIMATION:
    assert (unitlevel==3);
    unitlevel++;
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case NAME:
		  factions.back()->comm_faces.back().animations.push_back(FactionUtil::createAnimation ((*iter).value.c_str()));
	break;
      }
    }
    break;
  case FACTION:
    assert (unitlevel==1);
    unitlevel++;
    factions.push_back(new Faction ());
    assert(factions.size()>0);
    contrabandlists.push_back ("");
    //	factions[factions.size()-1];
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case SPARKRED:
	factions.back()->sparkcolor[0]=XMLSupport::parse_float((*iter).value);

	break;
      case SPARKGREEN:
	factions.back()->sparkcolor[1]=XMLSupport::parse_float((*iter).value);
	break;
      case SPARKBLUE:
	factions.back()->sparkcolor[2]=XMLSupport::parse_float((*iter).value);
	break;
      case SPARKALPHA:
	factions.back()->sparkcolor[3]=XMLSupport::parse_float((*iter).value);
	break;
      case NAME:
	factions[factions.size()-1]->factionname=new char[strlen((*iter).value.c_str())+1];
	
	strcpy(factions[factions.size()-1]->factionname,(*iter).value.c_str());
		break;

      case CONTRABAND:
	contrabandlists.back()= ((*iter).value);
	break;
      case LOGORGB:
		if (RGBfirst==0||RGBfirst==1) {
			RGBfirst=1;
			tmpstr=new char[strlen((*iter).value.c_str())+1];
			strcpy(tmpstr,(*iter).value.c_str());
		} else {
			RGBfirst =3;
			factions[factions.size()-1]->logo=FactionUtil::createTexture((*iter).value.c_str(),tmpstr);
		}
		break;
	  case LOGOA:
		if (RGBfirst==0||RGBfirst==2) {
			RGBfirst=2;
			tmpstr=new char[strlen((*iter).value.c_str())+1];
			strcpy(tmpstr,(*iter).value.c_str());
		} else {
			RGBfirst =3;
			factions[factions.size()-1]->logo=FactionUtil::createTexture(tmpstr,(*iter).value.c_str());
		}
		break;
      case SECLOGORGB:
	secString= (*iter).value;
	break;

      case SECLOGOA:
	secStringAlph= (*iter).value;
	break;

      }

    }
    factions[factions.size()-1]->secondaryLogo=NULL;
    if (!secString.empty()) {
      if (secStringAlph.empty()) {
	factions[factions.size()-1]->secondaryLogo=FactionUtil::createTexture(secString.c_str(),true);
      }else {
	factions[factions.size()-1]->secondaryLogo=FactionUtil::createTexture(secString.c_str(),secStringAlph.c_str(),true);
      }

    }
	assert (RGBfirst!=0);
	if (RGBfirst==1) {
			factions[factions.size()-1]->logo=FactionUtil::createTexture(tmpstr,true);
	}
	if (RGBfirst==2) {
			factions[factions.size()-1]->logo=FactionUtil::createTexture(tmpstr,tmpstr,true);
	}
	if (tmpstr!=NULL) {
		delete []tmpstr; 
	}
	break;
  case STATS:
  case FRIEND:
  case ENEMY:
    assert (unitlevel==2);
    unitlevel++;
    factions[factions.size()-1]->faction.push_back(faction_stuff());
    assert(factions.size()>0);
    //	factions[factions.size()-1];
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case NAME:
	
	factions[factions.size()-1]->
	  faction[factions[factions.size()-1]->faction.size()-1].stats.name=
	  new char[strlen((*iter).value.c_str())+1];
	
	strcpy(factions[factions.size()-1]->faction[factions[factions.size()-1]->faction.size()-1].stats.name,
	       (*iter).value.c_str());
	break;
      case RELATION:
	factions[factions.size()-1]->faction[factions[factions.size()-1]->faction.size()-1].relationship=parse_float((*iter).value);
	break;
      case CONVERSATION:
	factions[factions.size()-1]->faction[factions[factions.size()-1]->faction.size()-1].conversation=getFSM ((*iter).value);
	break;
	
      }
    }
    break;
  default :
    break;
  }


}
void Faction::endElement(void *userData, const XML_Char *name) {
using namespace FactionXML;
//  ((Faction*)userData)->endElement(name);

//void Faction::endElement(const string &name) {
  Names elem = (Names)element_map.lookup(name);
  switch(elem) {
  case UNKNOWN:
	  unitlevel--;
//	cerr << "Unknown element end tag '" << name << "' detected " << endl;
	break;
  default:
	  unitlevel--;
	break;
  }
}


void Faction::LoadXML(const char * filename, char * xmlbuffer, int buflength) {
using namespace FactionXML;
using namespace VSFileSystem;
  unitlevel=0;
  FILE * inFile;
  const int chunk_size = 16384;
  VSFile f;
  VSError err;
  if( buflength==0 || xmlbuffer==NULL)
  {
	  cout << "FactionXML:LoadXML " << filename << endl;
	  err = f.OpenReadOnly( filename, UnknownFile);
	  if(err>Ok) {
		cout << "Failed to open '" << filename << "' this probably means it can't find the data directory" << endl;
		assert(0);
	  }
  }
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, NULL);
  XML_SetElementHandler(parser, &Faction::beginElement, &Faction::endElement);
 
  if( buflength!=0 && xmlbuffer!=NULL)
  {
	XML_Parse (parser,xmlbuffer,buflength,1);
  }
  else
  {
	XML_Parse(parser, (f.ReadFull()).c_str(),f.Size(),1);
  /*
  do {
#ifdef BIDBG
	char *buf = (XML_Char*)XML_GetBuffer(parser, chunk_size);
#else
	char buf[chunk_size];
#endif
	int length;
	length = VSFileSystem::vs_read (buf,1, chunk_size,inFile);
	//length = inFile.gcount();
#ifdef BIDBG
	XML_ParseBuffer(parser, length, VSFileSystem::vs_feof(inFile));
#else
	XML_Parse(parser, buf,length, VSFileSystem::vs_feof(inFile));
#endif
  } while(!VSFileSystem::vs_feof(inFile));
  */
  f.Close();
  }
  XML_ParserFree (parser);
  ParseAllAllies();
  for (unsigned int i=0;i<factions.size();i++) {
    for (unsigned int j=0;j<factions[i]->faction.size();j++) {
      Faction * fact=factions[i];
      string fname=fact->factionname;

      if (fact->faction[j].conversation==NULL){
		  
		  //if (factions[i]->faction[j].stats.index != 0)	  {
		  if (0) {//we just want OUR faction to use that file when communicating with ANYONE  -- if we want certain factions to have *special* comm info for each other, then we can specify the conversation flag
			  fname = factions[factions[i]->faction[j].stats.index]->factionname;
		  }else {
			  fname = factions[i]->factionname;
		  }
		  string f=fname+".xml";
		  if (VSFileSystem::LookForFile( f, CommFile)>Ok)
			  fname="neutral";
		  factions[i]->faction[j].conversation=getFSM (/*"communications/" +*/ fname + ".xml");
      }else{
        //printf ("Already have converastion for %s with %s\n",fname.c_str(),factions[j]->factionname);
      }
    }
  }
  char * munull=NULL;
  FactionUtil::LoadSerializedFaction(munull);
}
void FactionUtil::LoadContrabandLists() {
  for (unsigned int i=0;i<factions.size()&&i<contrabandlists.size();i++) {
    if (contrabandlists[i].length()>0) {
      factions[i]->contraband = UnitFactory::createUnit (contrabandlists[i].c_str(),true,i);
    }
  }
  contrabandlists.clear();
}
