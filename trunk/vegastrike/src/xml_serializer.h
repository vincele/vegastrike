#ifndef _XML_SERIALIZER_H_
#define _XML_SERIALIZER_H_
#include "xml_support.h"
#include <vector>
#include <string>
using std::string;
using std::vector;
struct XMLType {
  union wordlength {
    int *i;
    float *f;
    void * p;
    char * c;
    short * s;
    unsigned short * us;
    unsigned char * uc;
    int hardint;
    float hardfloat;
  } w;
  std::string str;
  XMLType (int *myint) {w.i =myint;}
  XMLType (int myhardint) {w.hardint=myhardint;}
  XMLType (float myhardfloat) {w.hardfloat = myhardfloat;}
  XMLType (float  *myfloat) {w.f =myfloat;}
  XMLType (void * myvoid) {w.p=myvoid;}
  XMLType (char * mychar) {w.c=mychar;}
  XMLType (short * mychar) {w.s=mychar;}
  XMLType (unsigned short * mychar) {w.us=mychar;}
  XMLType (unsigned char * mychar) {w.uc=mychar;}
  XMLType (const std::string &s):str(s) {w.p=&this->str;}
  XMLType (const std::string &s, void * v):str(s) {w.p=v;}
  XMLType (const std::string &s, int myhardint):str(s) {w.hardint=myhardint;}
  XMLType (const std::string &s, float *f):str(s) {w.f=f;}
};
typedef std::string (XMLHandler)(const XMLType &input, void * mythis);
XMLHandler intHandler;
XMLHandler floatHandler;
XMLHandler intStarHandler;
XMLHandler shortStarHandler;
XMLHandler shortToFloatHandler;
XMLHandler ushortStarHandler;
XMLHandler charStarHandler;
XMLHandler ucharStarHandler;
XMLHandler negationCharStarHandler;
XMLHandler floatStarHandler;
XMLHandler fabsFloatStarHandler;
XMLHandler absShortStarHandler;
XMLHandler absIntStarHandler;

XMLHandler scaledFloatStarHandler;
XMLHandler angleStarHandler;
XMLHandler negationFloatStarHandler;
XMLHandler negationIntStarHandler;
XMLHandler stringStarHandler;
XMLHandler stringHandler;
XMLHandler lessNeg1Handler;
XMLHandler cloakHandler;




struct XMLElement {
  std::string elem;
  XMLType value;
  XMLHandler *handler;
  XMLElement (const std::string &ele, const XMLType &val, XMLHandler *hand):value(val) {
    elem = ele;
    handler = hand;
  }
  void Write (FILE * fp,void *mythis);
};
struct XMLnode {
  XMLnode *up;
  std::string val;
  vector <XMLElement> elements;
  vector <XMLnode> subnodes;
  XMLnode () {up=NULL;}
  XMLnode (const std::string &val, XMLnode * newup) {this->val=val;up=newup;}
  void Write (FILE * fp, void *mythis, int tablevel);
};
class XMLSerializer {
  std::string filename;
  std::string savedir;
  void * mythis;
  XMLnode topnode; 
  XMLnode * curnode;
 public:
  XMLSerializer(const char * filename, const char * modificationname, void *mythis);
  void AddTag (const std::string &tag);
  void AddElement (const std::string &element, XMLHandler *handler, const XMLType &input);
  void Write (const char * modificationname="");
  void EndTag (const std::string endname=string(""));
  std::string getName (){return filename;}
  void setName (const std::string & fil) {this->filename = fil;}
};
#endif
