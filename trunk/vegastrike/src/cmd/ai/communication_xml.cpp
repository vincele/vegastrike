#include <vector>
#include <string>
#include <expat.h>
#include "vegastrike.h"
#include "communication.h"
#include <assert.h>
#include "audiolib.h"
static int unitlevel;
using namespace XMLSupport;
using XMLSupport::EnumMap;
using XMLSupport::Attribute;
using XMLSupport::AttributeList;
namespace CommXML {
  enum Names {
	UNKNOWN,
	NODE,
	EDGE,
	NAME,
	INDEX,
	VALUE,
	SOUND,
	SEXE,
	FILENAME
  };

  const EnumMap::Pair element_names[4] = {
	EnumMap::Pair ("UNKNOWN", UNKNOWN),
	EnumMap::Pair ("Node", NODE),
	EnumMap::Pair ("Edge", EDGE),
	EnumMap::Pair ("Sound", SOUND)
  };
  const EnumMap::Pair attribute_names[6] = {
	EnumMap::Pair ("UNKNOWN", UNKNOWN),
	EnumMap::Pair ("Text", NAME), 
	EnumMap::Pair ("Index", INDEX), 
	EnumMap::Pair ("Relationship", VALUE), 
	EnumMap::Pair ("file", FILENAME),
	EnumMap::Pair ("sex", SEXE)
};


  const EnumMap element_map(element_names, 4);
  const EnumMap attribute_map(attribute_names, 6);

}

static vector <std::string> contrabandlists;
void FSM::beginElement(void *userData, const XML_Char *names, const XML_Char **atts) {
	((FSM*)userData)->beginElement(names, AttributeList(atts));
}

void FSM::beginElement(const string &name, const AttributeList attributes) {
using namespace CommXML;
	AttributeList::const_iterator iter;
	Names elem = (Names)element_map.lookup(name);
	string nam;
	float val;
	int sound=-1;
	unsigned char sexe;
	switch(elem) {
	case SOUND:
		for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
			switch(attribute_map.lookup((*iter).name)) {
			case SEXE:
			  sexe =XMLSupport::parse_int ((*iter).value);
			  break;
			case FILENAME:
			  sound=AUDCreateSoundWAV((*iter).value,false);
			  break;
			}
		}
		if (sound!=-1) {
		  nodes.back().AddSound(sound,sexe);
		}
	  break;
	case UNKNOWN:
		unitlevel++;
//		cerr << "Unknown element start tag '" << name << "' detected " << endl;
		return;
	case NODE:
		unitlevel++;
		for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
			switch(attribute_map.lookup((*iter).name)) {
			case NAME:
				nam=(*iter).value;
				{for (string::iterator i=nam.begin();i!=nam.end();i++) {
					if (*i=='\\') {
						*i='\n';
					}
				}}
				break;
			case VALUE:
				val=parse_float((*iter).value);
				break;
			}
		}
		nodes.push_back(Node(nam,val));
		break;
	case EDGE:
		unitlevel++;
		for(iter = attributes.begin(); iter!=attributes.end(); iter++) {
			if ((attribute_map.lookup((*iter).name))==INDEX) {
				nodes.back().edges.push_back(parse_int((*iter).value));
			}
		}
		break;
        default :
		break;
	}
}
void FSM::endElement(void *userData, const XML_Char *name) {
using namespace CommXML;
//	((Universe::Faction*)userData)->endElement(name);

//	void Universe::Faction::endElement(const string &name) {
	Names elem = (Names)element_map.lookup(name);
	switch(elem) {
	case UNKNOWN:
		unitlevel--;
//		cerr << "Unknown element end tag '" << name << "' detected " << endl;
		break;
	default:
		unitlevel--;
		break;
	}
}

void FSM::LoadXML(const char * filename) {
 using namespace CommXML;
 unitlevel=0;
  const int chunk_size = 16384;
  cout << "CommXML:LoadXML " << filename << endl;
  FILE * inFile = fopen (filename, "r");
  if(!inFile) {
	assert(0);
	return;
  }
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, &FSM::beginElement, &FSM::endElement);
 
  do {
	char buf[chunk_size];
	int length;
	length = fread (buf,1, chunk_size,inFile);
	//length = inFile.gcount();
	XML_Parse(parser, buf,length, feof(inFile));
  } while(!feof(inFile));
  fclose (inFile);
  XML_ParserFree (parser);
}
