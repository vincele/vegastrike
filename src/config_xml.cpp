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

/*
  xml Configuration written by Alexander Rawass <alexannika@users.sourceforge.net>
*/

#include <expat.h>
#include "xml_support.h"

#include "vegastrike.h"

#include "config_xml.h"
#include "easydom.h"
#include "cmd/ai/flykeyboard.h"
#include "cmd/ai/firekeyboard.h"
#include "cmd/music.h"
#include "gfx/loc_select.h"
#include "audiolib.h"
#include "in_joystick.h"

#include "main_loop.h" // for CockpitKeys


//#include "vs_globals.h"
//#include "vegastrike.h"


VegaConfig::VegaConfig(char *configfile){

  configNodeFactory *domf = new configNodeFactory();

  configNode *top=(configNode *)domf->LoadXML(configfile);

  if(top==NULL){
    cout << "Panic exit - no configuration" << endl;
    exit(0);
  }
  //top->walk(0);
  
  initCommandMap();
  initKeyMap();

  variables=NULL;
  colors=NULL;

  // set hatswitches to off
  for(int h=0;h<MAX_HATSWITCHES;h++){
    hatswitch_margin[h]=2.0;
    for(int v=0;v<MAX_VALUES;v++){
      hatswitch[h][v]=2.0;
    }
  }

  for(int i=0;i<=3;i++){
    axis_axis[i]=-1;
    axis_joy[i]=-1;
  }

  checkConfig(top);
}

/*
for i in `cat cmap` ; do echo "  command_map[\""$i"\"]=FlyByKeyboard::"$i ";" ; done
 */
#if 1
const float volinc = 1;
const float dopinc = .1;
void incvol (int i, KBSTATE a) {
	if (a==DOWN) {
		AUDChangeVolume (AUDGetVolume()+volinc);
	}
}
void decvol (int i, KBSTATE a) {
	if (a==DOWN) {
		AUDChangeVolume (AUDGetVolume()-volinc);
	}
}
void mute (int i, KBSTATE a) { 
//	if (a==PRESS)
//		AUDChangeVolume (0);broken
}

void incdop (int i, KBSTATE a) {
	if (a==DOWN) {
		AUDChangeDoppler (AUDGetDoppler()+dopinc);
	}

}
void decdop (int i, KBSTATE a) {
	if (a==DOWN) {
		AUDChangeDoppler (AUDGetDoppler()-dopinc);
	}
}
#endif
void VegaConfig::initKeyMap(){
  // mapping from special key string to glut key
  key_map["space"]=' ';
  key_map["return"]=13;
  key_map["function-1"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F1;
  key_map["function-2"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F2;
  key_map["function-3"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F3;
  key_map["function-4"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F4;
  key_map["function-5"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F5;
  key_map["function-6"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F6;
  key_map["function-7"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F7;
  key_map["function-8"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F8;
  key_map["function-9"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F9;
  key_map["function-10"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F10;
  key_map["function-11"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F11;
  key_map["function-12"]=KEY_SPECIAL_OFFSET+GLUT_KEY_F12;

  key_map["cursor-left"]=KEY_SPECIAL_OFFSET+GLUT_KEY_LEFT;
  key_map["cursor-up"]=KEY_SPECIAL_OFFSET+GLUT_KEY_UP;
  key_map["cursor-right"]=KEY_SPECIAL_OFFSET+GLUT_KEY_RIGHT;
  key_map["cursor-down"]=KEY_SPECIAL_OFFSET+GLUT_KEY_DOWN;

  key_map["cursor-pageup"]=KEY_SPECIAL_OFFSET+GLUT_KEY_PAGE_UP;
  key_map["cursor-pagedown"]=KEY_SPECIAL_OFFSET+GLUT_KEY_PAGE_DOWN;
  key_map["cursor-home"]=KEY_SPECIAL_OFFSET+GLUT_KEY_HOME;
  key_map["cursor-end"]=KEY_SPECIAL_OFFSET+GLUT_KEY_END;
  key_map["cursor-insert"]=KEY_SPECIAL_OFFSET+GLUT_KEY_INSERT;
  key_map["backspace"]=8;
  key_map["capslock"]=96;
  key_map["cursor-delete"]=127;
  key_map["tab"]='\t';
  key_map["esc"]=27;

}

#if 0
  sed 's/\(.*void \)\(.*\)(.*/ command_map[\"Cockpit::\2\"]=CockpitKeys::\2;/'
#endif

  using namespace CockpitKeys;

void VegaConfig::initCommandMap(){
#if 1
  //  I don't knwo why this gives linker errors!
  command_map["NoPositionalKey"]=mute;
  command_map["DopplerInc"]=incdop;
  command_map["DopplerDec"]=decdop;
  command_map["VolumeInc"]=incvol;
  command_map["VolumeDec"]=decvol;
#endif

  // mapping from command string to keyboard handler
  command_map["SheltonKey"]=FlyByKeyboard::SheltonKey ;
  command_map["StartKey"]=FlyByKeyboard::StartKey ;
  command_map["StopKey"]=FlyByKeyboard::StopKey ;
  command_map["UpKey"]=FlyByKeyboard::UpKey ;
  command_map["DownKey"]=FlyByKeyboard::DownKey ;
  command_map["LeftKey"]=FlyByKeyboard::LeftKey ;
  command_map["RightKey"]=FlyByKeyboard::RightKey ;
  command_map["ABKey"]=FlyByKeyboard::ABKey ;
  command_map["AccelKey"]=FlyByKeyboard::AccelKey ;
  command_map["DecelKey"]=FlyByKeyboard::DecelKey ;
  command_map["RollLeftKey"]=FlyByKeyboard::RollLeftKey ;
  command_map["RollRightKey"]=FlyByKeyboard::RollRightKey ;

  command_map["FireKey"]=FireKeyboard::FireKey ;
  command_map["MissileKey"]=FireKeyboard::MissileKey ;
  command_map["TargetKey"]=FireKeyboard::TargetKey ;
  command_map["WeapSelKey"]=FireKeyboard::WeapSelKey ;
  command_map["MisSelKey"]=FireKeyboard::MisSelKey ;
  command_map["CloakKey"]=FireKeyboard::CloakKey;

 command_map["Cockpit::PitchDown"]=CockpitKeys::PitchDown;
 command_map["Cockpit::PitchUp"]=CockpitKeys::PitchUp;
 command_map["Cockpit::YawLeft"]=CockpitKeys::YawLeft;
 command_map["Cockpit::YawRight"]=CockpitKeys::YawRight;
 command_map["Cockpit::Inside"]=CockpitKeys::Inside;
 command_map["Cockpit::ZoomOut"]=CockpitKeys::ZoomOut ;
 command_map["Cockpit::ZoomIn"]=CockpitKeys::ZoomIn ;
 command_map["Cockpit::InsideLeft"]=CockpitKeys::InsideLeft;
 command_map["Cockpit::InsideRight"]=CockpitKeys::InsideRight;
 command_map["Cockpit::InsideBack"]=CockpitKeys::InsideBack;
 command_map["Cockpit::SwitchLVDU"]=CockpitKeys::SwitchLVDU;
 command_map["Cockpit::SwitchRVDU"]=CockpitKeys::SwitchRVDU;
 command_map["Cockpit::Behind"]=CockpitKeys::Behind;
 command_map["Cockpit::Pan"]=CockpitKeys::Pan;
 command_map["Cockpit::SkipMusicTrack"]=CockpitKeys::SkipMusicTrack;

 command_map["Cockpit::Quit"]=CockpitKeys::Quit;

 // command_map["LocationSelect::MoveMoveHandle"]=LocationSelect::MouseMoveHandle;
 //command_map["LocationSelect::incConstanst"]=LocationSelect::incConstant;
 //command_map["LocationSelect::decConstant"]=LocationSelect::decConstant;

 //command_map["CoordinateSelect::MouseeMoveHandle"]=CoordinateSelect::MouseMoveHandle;

}

bool VegaConfig::checkConfig(configNode *node){
  if(node->Name()!="vegaconfig"){
    cout << "this is no Vegastrike config file" << endl;
    return false;
  }

  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);

    if(cnode->Name()=="variables"){
      doVariables(cnode);
    }
    else if(cnode->Name()=="colors"){
      doColors(cnode);
    }
    else if(cnode->Name()=="bindings"){
      bindings=cnode; // delay the bindings until keyboard/joystick is initialized
      //doBindings(cnode);
    }
    else{
      cout << "Unknown tag: " << cnode->Name() << endl;
    }
  }
  return true;
}

void VegaConfig::doVariables(configNode *node){
  if(variables!=NULL){
    cout << "only one variable section allowed" << endl;
    return;
  }
  variables=node;

  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    checkSection(cnode,SECTION_VAR);
  }
}

void VegaConfig::checkSection(configNode *node, enum section_t section_type){
    if(node->Name()!="section"){
      cout << "not a section" << endl;
    return;
  }

  string section=node->attr_value("name");
  if(section.empty()){
    cout << "no name given for section" << endl;
  }
  
  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    if(section_type==SECTION_COLOR){
      checkColor(cnode);
    }
    else if(section_type==SECTION_VAR){
      checkVar(cnode);
    }
  }

}

void VegaConfig::checkVar(configNode *node){
    if(node->Name()!="var"){
      cout << "not a variable" << endl;
    return;
  }

  string name=node->attr_value("name");
  string value=node->attr_value("value");

  //  cout << "checking var " << name << " value " << value << endl;
  if(name.empty() || value.empty()){
    cout << "no name or value given for variable" << endl;
  }
}

bool VegaConfig::checkColor(configNode *node){
  if(node->Name()!="color"){
    cout << "no color definition" << endl;
    return false;
  }

  if(node->attr_value("name").empty()){
    cout << "no color name given" << endl;
    return false;
  }

  vColor *color;


  if(node->attr_value("ref").empty()){
    string r=node->attr_value("r");
    string g=node->attr_value("g");
    string b=node->attr_value("b");
    string a=node->attr_value("a");
    if(r.empty() || g.empty() || b.empty() || a.empty()){
      cout << "neither name nor r,g,b given for color " << node->Name() << endl;
      return false;
    }
    float rf=atof(r.data());
    float gf=atof(g.data());
    float bf=atof(b.data());
    float af=atof(a.data());

    color=new vColor;

    color->r=rf;
    color->g=gf;
    color->b=bf;
    color->a=af;
  }
  else{
    float refcol[4];

    string ref_section=node->attr_value("section");
    if(ref_section.empty()){
      cout << "you have to give a referenced section when referencing colors" << endl;
      ref_section="default";
    }

    //    cout << "refsec: " << ref_section << " ref " << node->attr_value("ref") << endl;
    getColor(ref_section,node->attr_value("ref"),refcol);

    color=new vColor;

    color->r=refcol[0];
    color->g=refcol[1];
    color->b=refcol[2];
    color->a=refcol[3];

  }

  color->name=node->attr_value("name");

  node->color=color;
  //  colors.push_back(color);

  return true;
}


void VegaConfig::doColors(configNode *node){
  if(colors!=NULL){
    cout << "only one variable section allowed" << endl;
    return;
  }
  colors=node;

  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    checkSection(cnode,SECTION_COLOR);
  }

#if 0
  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    checkColor(cnode);
  }
#endif
}

void VegaConfig::doBindings(configNode *node){
  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);

    if((cnode)->Name()=="bind"){
      checkBind(cnode);
    }
    else if(((cnode)->Name()=="axis")){
      doAxis(cnode);
    }
    else{
      cout << "Unknown tag: " << (cnode)->Name() << endl;
    }
  }
}

void VegaConfig::doAxis(configNode *node){

  string name=node->attr_value("name");
  string joystick=node->attr_value("joystick");
  string axis=node->attr_value("axis");

  if(name.empty() || joystick.empty() || axis.empty()){
    cout << "no correct axis desription given " << endl;
    return;
  }

  int joy_nr=atoi(joystick.data());
  int axis_nr=atoi(axis.data());

  // no checks for correct number yet 

  if(name=="x"){
    axis_axis[0]=axis_nr;
    axis_joy[0]=joy_nr;
  }
  else if(name=="y"){
    axis_axis[1]=axis_nr;
    axis_joy[1]=joy_nr;
  }
  else if(name=="z"){
    axis_axis[2]=axis_nr;
    axis_joy[2]=joy_nr;
  }
  else if(name=="throttle"){
    axis_axis[3]=axis_nr;
    axis_joy[3]=joy_nr;
  }
  else if(name=="hatswitch"){
    string nr_str=node->attr_value("nr");
    string margin_str=node->attr_value("margin");

    if(nr_str.empty() || margin_str.empty()){
      cout << "you have to assign a number and a margin to the hatswitch" << endl;
      return;
    }
    int nr=atoi(nr_str.c_str());

    float margin=atof(margin_str.c_str());
    hatswitch_margin[nr]=margin;

    hatswitch_axis[nr]=axis_nr;
    hatswitch_joystick[nr]=joy_nr;

    vector<easyDomNode *>::const_iterator siter;
  
    hs_value_index=0;
    for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
      configNode *cnode=(configNode *)(*siter);
      checkHatswitch(nr,cnode);
    }
  }
  else{
    cout << "unknown axis " << name << endl;
    return;
  }

}

void VegaConfig::checkHatswitch(int nr,configNode *node){
  if(node->Name()!="hatswitch"){
    cout << "not a hatswitch node " << endl;
    return;
  }

  string strval=node->attr_value("value");

  float val=atof(strval.c_str());

  if(val>1.0 || val<-1.0){
    cout << "only hatswitch values from -1.0 to 1.0 allowed" << endl;
    return;
  }

  hatswitch[nr][hs_value_index]=val;

  cout << "setting hatswitch nr " << nr << " " << hs_value_index << " = " << val << endl;

  hs_value_index++;
}

void VegaConfig::checkBind(configNode *node){
  if(node->Name()!="bind"){
    cout << "not a bind node " << endl;
    return;
  }

  string cmdstr=node->attr_value("command");
  
  KBHandler handler=command_map[cmdstr];
  
  if(handler==NULL){
    cout << "No such command: " << cmdstr << endl;
    return;
  }

  if(!(node->attr_value("key").empty())){
      // now map the command to a callback function and bind it
    
    string keystr=node->attr_value("key");

    if(keystr.length()==1){
      BindKey(keystr[0],handler);
    }
    else{
      int glut_key=key_map[keystr];
      if(glut_key==0){
	cout << "No such special key: " << keystr << endl;
	return;
      }
      BindKey(glut_key,handler);
    }

    //    cout << "bound key " << keystr << " to " << cmdstr << endl;

  }
  else if(!(node->attr_value("button").empty())){
    if(!(node->attr_value("button").empty())){
      int button_nr=atoi(node->attr_value("button").data());

      string joy_str=node->attr_value("joystick");
      string hat_str=node->attr_value("hatswitch");

      if(joy_str.empty()){
	// it has to be the hatswitch
	if(hat_str.empty()){
	  cout << "you got to give a hatswitch number" << endl ;
	  return;
	}

	int hatswitch_nr=atoi(hat_str.c_str());

	BindHatswitchKey(hatswitch_nr,button_nr,handler);
	
	//	cout << "Bound hatswitch nr " << hatswitch_nr << " button: " << button_nr << " to " << cmdstr << endl;
      }
      else{
	// joystick button
	int joystick_nr=atoi(joy_str.c_str());

	// now map the command to a callback function and bind it

	// yet to check for correct buttons/joy-nr


	BindJoyKey(joystick_nr,button_nr,handler);

	//cout << "Bound joy= " << joystick_nr << " button= " << button_nr << "to " << cmdstr << endl;
      }
    }
    else{
      cout << "you must specify the joystick nr. to use" << endl;
      return;
    }
  }
  else{
    cout << "no key or joystick binding found" << endl;
    return;
  }


}

string VegaConfig::getVariable(string section,string name,string defaultval){
   vector<easyDomNode *>::const_iterator siter;
  
  for(siter= variables->subnodes.begin() ; siter!=variables->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    string scan_name=(cnode)->attr_value("name");
    //    cout << "scanning section " << scan_name << endl;

    if(scan_name==section){
      return getVariable(cnode,name,defaultval);
    }
  }

  cout << "WARNING: no section named " << section << endl;

  return defaultval;
 
}
string VegaConfig::getVariable(configNode *section,string name,string defaultval){
    vector<easyDomNode *>::const_iterator siter;
  
  for(siter= section->subnodes.begin() ; siter!=section->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    if((cnode)->attr_value("name")==name){
      return (cnode)->attr_value("value");
    }
  }

  cout << "WARNING: no var named " << name << " in section " << section->attr_value("name") << " using default: " << defaultval << endl;

  return defaultval;
 
}

void VegaConfig::getColor(string section, string name, float color[4]){
   vector<easyDomNode *>::const_iterator siter;
  
  for(siter= colors->subnodes.begin() ; siter!=colors->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    string scan_name=(cnode)->attr_value("name");
    //          cout << "scanning section " << scan_name << endl;

    if(scan_name==section){
      getColor(cnode,name,color);
      return;
    }
  }

  cout << "WARNING: no section named " << section << endl;

  return;
  
}

void VegaConfig::getColor(configNode *node,string name,float color[4]){
  vector<easyDomNode *>::const_iterator siter;
  
  for(siter= node->subnodes.begin() ; siter!=node->subnodes.end() ; siter++){
    configNode *cnode=(configNode *)(*siter);
    //            cout << "scanning color " << (cnode)->attr_value("name") << endl;
    if((cnode)->attr_value("name")==name){
      color[0]=(cnode)->color->r;
      color[1]=(cnode)->color->g;
      color[2]=(cnode)->color->b;
      color[3]=(cnode)->color->a;
      return;
    }
  }

  color[0]=1.0;
  color[1]=1.0;
  color[2]=1.0;
  color[3]=1.0;

  cout << "WARNING: color " << name << " not defined, using default (white)" << endl;
}

void VegaConfig::bindKeys(){
  doBindings(bindings);
}
