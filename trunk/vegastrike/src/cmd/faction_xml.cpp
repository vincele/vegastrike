#include <vector>
#include <string>
#include <expat.h>
#include <gfx/aux_texture.h>
#include "vegastrike.h"
#include "xml_support.h"
#include <assert.h>
#include <algorithm>


static int unitlevel;
using namespace XMLSupport;
using XMLSupport::EnumMap;
using XMLSupport::Attribute;
using XMLSupport::AttributeList;
using std::sort;
namespace FactionXML {
  enum Names {
	UNKNOWN,
	FACTIONS,
	FACTION,
	NAME,
	LOGORGB,
	LOGOA,
	RELATION,
	STATS,
	FRIEND,
	ENEMY
  };

  const EnumMap::Pair element_names[] = {
	EnumMap::Pair ("UNKNOWN", UNKNOWN),
	EnumMap::Pair ("Factions", FACTIONS),
	EnumMap::Pair ("Faction", FACTION),
	EnumMap::Pair ("Friend", FRIEND),
	EnumMap::Pair ("Enemy", ENEMY),
  	EnumMap::Pair ("Stats", STATS)
  };
  const EnumMap::Pair attribute_names[] = {
	EnumMap::Pair ("UNKNOWN", UNKNOWN),
	EnumMap::Pair ("name", NAME), 
	EnumMap::Pair ("logoRGB", LOGORGB), 
	EnumMap::Pair ("logoA", LOGOA), 
	EnumMap::Pair ("relation",RELATION), 
};


  const EnumMap element_map(element_names, 6);
  const EnumMap attribute_map(attribute_names, 5);

}


using namespace FactionXML;


void Universe::Faction::beginElement(void *userData, const XML_Char *names, const XML_Char **atts) {
  Universe * thisuni = (Universe *) userData;
//	((Universe::Faction*)userData)->beginElement(name, AttributeList(atts));
  AttributeList attributes=AttributeList(atts);
  string name=names;
  AttributeList::const_iterator iter;
  Names elem = (Names)element_map.lookup(name);
  char * tmpstr=NULL;
  char RGBfirst=0;
  switch(elem) {
  case UNKNOWN:
	unitlevel++;

//	cerr << "Unknown element start tag '" << name << "' detected " << endl;
	return;

  case FACTIONS:
	assert (unitlevel==0);
	unitlevel++;
	break;
  case FACTION:
	assert (unitlevel==1);
	unitlevel++;
	thisuni->factions.push_back(new Faction ());
	assert(thisuni->factions.size()>0);
//	thisuni->factions[thisuni->factions.size()-1];
    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
      case NAME:
		thisuni->factions[thisuni->factions.size()-1]->factionname=new char[strlen((*iter).value.c_str())+1];
		strcpy(thisuni->factions[thisuni->factions.size()-1]->factionname,(*iter).value.c_str());
		break;
      case LOGORGB:
		if (RGBfirst==0||RGBfirst==1) {
			RGBfirst=1;
			tmpstr=new char[strlen((*iter).value.c_str())+1];
			strcpy(tmpstr,(*iter).value.c_str());
		} else {
			RGBfirst =3;
			thisuni->factions[thisuni->factions.size()-1]->logo=new Texture((*iter).value.c_str(),tmpstr);
		}
		break;
	  case LOGOA:
		if (RGBfirst==0||RGBfirst==2) {
			RGBfirst=2;
			tmpstr=new char[strlen((*iter).value.c_str())+1];
			strcpy(tmpstr,(*iter).value.c_str());
		} else {
			RGBfirst =3;
			thisuni->factions[thisuni->factions.size()-1]->logo=new Texture(tmpstr,(*iter).value.c_str());
		}
		break;
      }

    }
	assert (RGBfirst!=0);
	if (RGBfirst==1) {
			thisuni->factions[thisuni->factions.size()-1]->logo=new Texture(tmpstr);
	}
	if (RGBfirst==2) {
			thisuni->factions[thisuni->factions.size()-1]->logo=new Texture(tmpstr,tmpstr);
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
	thisuni->factions[thisuni->factions.size()-1]->faction.push_back(faction_stuff());
	assert(thisuni->factions.size()>0);
//	thisuni->factions[thisuni->factions.size()-1];
	    for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(attribute_map.lookup((*iter).name)) {
  case NAME:
	
    thisuni->factions[thisuni->factions.size()-1]->
		faction[thisuni->factions[thisuni->factions.size()-1]->faction.size()-1].stats.name=
		new char[strlen((*iter).value.c_str())+1];

	strcpy(thisuni->factions[thisuni->factions.size()-1]->faction[thisuni->factions[thisuni->factions.size()-1]->faction.size()-1].stats.name,
		(*iter).value.c_str());
	break;
      case RELATION:
	thisuni->factions[thisuni->factions.size()-1]->faction[thisuni->factions[thisuni->factions.size()-1]->faction.size()-1].relationship=parse_float((*iter).value);
	break;
      }

    }
	break;
  }
}
void Universe::Faction::endElement(void *userData, const XML_Char *name) {
//  ((Universe::Faction*)userData)->endElement(name);

//void Universe::Faction::endElement(const string &name) {
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
int Universe::GetFaction (const char * factionname) {
#ifdef _WIN32
  #define strcasecmp stricmp
#endif
 for (unsigned int i=0;i<factions.size();i++) {
    if (strcasecmp (factionname, factions[i]->factionname)==0) {
      return i;
    }
  }
  return 0;
}

Texture * Universe::getForceLogo (int faction) {
  return factions[faction]->logo;
}
//fixme--add squads in here
Texture *Universe::getSquadLogo (int faction) {
  return getForceLogo (faction);
}


float Universe::GetRelation (int Myfaction, int TheirFaction) {
  if (Myfaction==TheirFaction)
    return 1;
  assert (factions[Myfaction]->faction[TheirFaction].stats.index == TheirFaction);
  return factions[Myfaction]->faction[TheirFaction].relationship;
}
Universe::Faction::Faction() {
	logo=NULL;
	factionname=NULL;
}
Universe::Faction::~Faction() {
	delete logo;
	delete [] factionname;
}

void Universe::Faction::ParseAllAllies(Universe * thisuni) {
	for (unsigned int i=0;i<thisuni->factions.size();i++) {
		thisuni->factions[i]->ParseAllies(thisuni);
		
	}
}
void Universe::Faction::ParseAllies (Universe * thisuni) {
	unsigned int i,j;
	vector <faction_stuff> tempvec;
	for (i=0;i<faction.size();i++) {
		for (j=0; j<thisuni->factions.size();j++) {
			if (strcmp (faction[i].stats.name,thisuni->factions[j]->factionname)==0) {
				delete [] faction[i].stats.name;
				faction[i].stats.index = j;
				break;
			}
		}
	}
	for (i=0;i<thisuni->factions.size();i++) {
		tempvec.push_back (faction_stuff());
		tempvec[i].stats.index=i;
		tempvec[i].relationship =0;
	}
	for (i=0;i<faction.size();i++) {
		tempvec[faction[i].stats.index].relationship = faction[i].relationship;
	}
	faction = tempvec;
	/*
	while (faction.size()<thisuni->factions.size()) {
		faction.push_back (faction_stuff());
		faction[faction.size()-1].stats.index=-1;
	}
	faction_stuff tmp;
	tmp.stats.index ==0;
	for (i=0;i<faction.size();i++) {
		if (tmp.stats.index == i) {
			faction[i].relationship= tmp.relationship;
			faction[i].stats.index = faction[i].stats.index;
		} else {
			tmp.relationship = faction[i].relationship;
			tmp.stats.index = faction[i].stats.index;
			if (faction[i].stats.index!=i) {
				faction[i].relationship = 0;

			}
		}
		for (j=0;j<faction.size();j++) {
			if (faction[j].stats.index==i) {
				faction[i].relationship= faction[j].relationship;
				faction[i].stats.index = faction[j].stats.index;
			}
		}
		faction[i].stats.index=i;
	}
	*/
}
void Universe::Faction::LoadXML(const char * filename, Universe * thisuni) {
  unitlevel=0;
  const int chunk_size = 16384;
  FILE * inFile = fopen (filename, "r");
  if(!inFile) {
	assert(0);
	return;
  }
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, thisuni);
  XML_SetElementHandler(parser, &Universe::Faction::beginElement, &Universe::Faction::endElement);
 
  do {
	char *buf = (XML_Char*)XML_GetBuffer(parser, chunk_size);
	int length;
	length = fread (buf,1, chunk_size,inFile);
	//length = inFile.gcount();
	XML_ParseBuffer(parser, length, feof(inFile));
  } while(!feof(inFile));
  fclose (inFile);
  XML_ParserFree (parser);
  ParseAllAllies(thisuni);
}
