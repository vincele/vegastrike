#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "lin_time.h"
#include "cmd/unit.h"
#include "cmd/unit_factory.h"
#include "vegastrike.h"
#include "vs_globals.h"
#include "in.h"
#include "gfx/mesh.h"
#include "gfx/sprite.h"
#include "physics.h"
//#include "cmd_hud.h"
#include "gfxlib.h"
#include "cmd/bolt.h"
#include "gfx/loc_select.h"
#include <string>
#include "cmd/ai/input_dfa.h"
#include "cmd/collection.h"
#include "star_system.h"
#include "cmd/planet.h"
#include "gfx/sphere.h"
#include "gfx/coord_select.h"
#include "cmd/building.h"
#include "cmd/ai/fire.h"
#include "cmd/ai/aggressive.h"
#include "cmd/ai/navigation.h"
#include "cmd/beam.h"
#include  "gfx/halo.h"
#include "gfx/matrix.h"
#include "cmd/ai/flyjoystick.h"
#include "cmd/ai/firekeyboard.h"
#include "cmd/ai/script.h"
#include "gfx/cockpit.h"
#include "gfx/aux_texture.h"
#include "gfx/background.h"
#include "cmd/music.h"
#include "main_loop.h"
#include "cmd/music.h"
#include "audiolib.h"
#include "cmd/nebula.h"
#include "vs_path.h"
#include "cmd/script/mission.h"
#include "xml_support.h"
#include "config_xml.h"
#include "cmd/ai/missionscript.h"
#include "cmd/enhancement.h"
#include "cmd/cont_terrain.h"
#include "cmd/script/flightgroup.h"
#include "force_feedback.h"

using namespace std;

 Music * muzak=NULL;

#define KEYDOWN(name,key) (name[key] & 0x80)

Unit **fighters;


 GFXBOOL capture;
GFXBOOL quit = GFXFALSE;

/*11-7-98
 *Cool shit happened when a position rotation matrix from a unit was used for the drawing of the background... not very useful though
 */

/*
class Orbit:public AI{
	float count;
public:
	Orbit(Unit *parent1):AI(parent1){count = 0;};
	Orbit():AI()
	{
		count = 0;
	};
	AI *Execute()
	{
		//parent->Position(); // query the position
	  //parent->ZSlide(0.100F);
	  //parent->Pitch(PI/180);
		count ++;
		if(30 == count)
		{
		  //			Unit *parent = this->parent;
			//delete this;
			//return new Line(parent);
			return this;
		}
		else
			return this;
	}
};
*/

const float timek = .001;
bool _Slew = true;

namespace CockpitKeys {


 void SkipMusicTrack(int,KBSTATE newState) {
   if(newState==PRESS){
     printf("skipping\n");
    muzak->Skip();
   }
 }

 void PitchDown(int,KBSTATE newState) {
	static Vector Q;
	static Vector R;
	if(newState==PRESS) {
		Q = _Universe->AccessCamera()->Q;
		R = _Universe->AccessCamera()->R;
		_Universe->AccessCamera()->myPhysics.ApplyBalancedLocalTorque(-Q, R,timek);
		//a =1;
	}
	if (_Slew&&newState==RELEASE) {
	  _Universe->AccessCamera()->myPhysics.SetAngularVelocity(Vector(0,0,0));
	}//a=0;
	
}

 void PitchUp(int,KBSTATE newState) {
	
	static Vector Q;
	static Vector R;

	if(newState==PRESS) {
		Q = _Universe->AccessCamera()->Q;
		R = _Universe->AccessCamera()->R;
		_Universe->AccessCamera()->myPhysics.ApplyBalancedLocalTorque(Q, R,timek);
		
	}
	if (_Slew&&newState==RELEASE) {
	  _Universe->AccessCamera()->myPhysics.SetAngularVelocity(Vector(0,0,0));
	}
}

  void YawLeft(int,KBSTATE newState) {
	
	static Vector P;
	static Vector R;
	if(newState==PRESS) {
		P = _Universe->AccessCamera()->P;
		R = _Universe->AccessCamera()->R;
		_Universe->AccessCamera()->myPhysics.ApplyBalancedLocalTorque(-P, R,timek);
		
	}
	if (_Slew&&newState==RELEASE) {
	  _Universe->AccessCamera()->myPhysics.SetAngularVelocity(Vector(0,0,0));
	}
}

  void YawRight(int,KBSTATE newState) {
	
	static Vector P;
	static Vector R;
	if(newState==PRESS) {
		P = _Universe->AccessCamera()->P;
		R = _Universe->AccessCamera()->R;
		_Universe->AccessCamera()->myPhysics.ApplyBalancedLocalTorque(P, R,timek);
	
	}
	if (_Slew&&newState==RELEASE) {
	  _Universe->AccessCamera()->myPhysics.SetAngularVelocity(Vector(0,0,0));
	}
}

  void Quit(int,KBSTATE newState) {
    /*
    if (newState==PRESS) {
      ouch = (!ouch);
    }
    */
	if(newState==PRESS||newState==DOWN) {
	  for (unsigned int i=0;i<active_missions.size();i++) {
	    if (active_missions[i]) {
	      active_missions[i]->DirectorEnd();
	    }
	  }
	  delete forcefeedback;
	  winsys_exit(0);
	}
   
  }
bool cockpitfront=true;
  void Inside(int,KBSTATE newState) {
    {
      static bool back= XMLSupport::parse_bool (vs_config->getVariable ("graphics","background","true"));
      _Universe->activeStarSystem()->getBackground()->EnableBG(back);
    }
  const int cockpiton=1;
  const int backgroundoff=2;
  const int max = 4;
  static int tmp=(XMLSupport::parse_bool (vs_config->getVariable ("graphics","cockpit","true"))?1:0)+((!XMLSupport::parse_bool (vs_config->getVariable ("graphics","background","true")))?2:0);
  if(newState==PRESS&&cockpitfront) {
    if ((tmp&cockpiton)&&_Universe->AccessCockpit()->GetParent()) {
      _Universe->AccessCockpit()->Init (_Universe->AccessCockpit()->GetParent()->getCockpit().c_str());	    
    }else {
      _Universe->AccessCockpit()->Init ("disabled-cockpit.cpt");
    }
	static int i=1;
	if (i--==1){
	  
	  //_Universe->activeStarSystem()->getBackground()->EnableBG(!(backgroundoff));
	}
    tmp--;
    if (tmp<0) tmp=max-1;
  }
  if(newState==PRESS||newState==DOWN) {
    cockpitfront=true;
    _Universe->AccessCockpit()->SetView (CP_FRONT);
  }
}
  void ZoomOut (int, KBSTATE newState) {
  if(newState==PRESS||newState==DOWN) 
  _Universe->AccessCockpit()->zoomfactor+=GetElapsedTime()/getTimeCompression();  
}
  void ScrollUp (int, KBSTATE newState) {

    if(newState==PRESS/*||newState==DOWN*/){
     _Universe->AccessCockpit()->ScrollAllVDU (-1);
   }    
  }
  void ScrollDown (int, KBSTATE newState) {

   if(newState==PRESS/*||newState==DOWN*/){
     _Universe->AccessCockpit()->ScrollAllVDU (1);
   }    

  }
  void ZoomIn (int, KBSTATE newState) {
  if(newState==PRESS||newState==DOWN) 
  _Universe->AccessCockpit()->zoomfactor-=GetElapsedTime()/getTimeCompression();  
}

  void InsideLeft(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	  cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_LEFT);
	}
}
  void InsideRight(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	    cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_RIGHT);
	}
}
  void PanTarget(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	    cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_PANTARGET);
	}
  }
  void ViewTarget(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	    cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_VIEWTARGET);
	}
  }
  void OutsideTarget(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	    cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_TARGET);
	}
  }


  void InsideBack(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	    cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_BACK);
	}
}

  void SwitchLVDU(int,KBSTATE newState) {

	if(newState==PRESS) {
	  _Universe->AccessCockpit()->VDUSwitch (0);
	}
  }
  void SwitchRVDU(int,KBSTATE newState) {

	if(newState==PRESS) {
	  _Universe->AccessCockpit()->VDUSwitch (1);
	}
  }
  void SwitchMVDU(int,KBSTATE newState) {

	if(newState==PRESS) {
	  _Universe->AccessCockpit()->VDUSwitch (2);
	}
  }
  void SwitchULVDU(int,KBSTATE newState) {

	if(newState==PRESS) {
	  _Universe->AccessCockpit()->VDUSwitch (3);
	}
  }
  void SwitchURVDU(int,KBSTATE newState) {

	if(newState==PRESS) {
	  _Universe->AccessCockpit()->VDUSwitch (4);
	}
  }
  void SwitchUMVDU(int,KBSTATE newState) {

	if(newState==PRESS) {
	  _Universe->AccessCockpit()->VDUSwitch (5);
	}
  }

  void Behind(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	  cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_CHASE);
	}
}
  void Pan(int,KBSTATE newState) {

	if(newState==PRESS||newState==DOWN) {
	  cockpitfront=false;
	  _Universe->AccessCockpit()->SetView (CP_PAN);
	}
}

}

using namespace CockpitKeys;
ContinuousTerrain * myterrain;
Unit *carrier=NULL;
Unit *fighter = NULL;
Unit *fighter2=NULL;
int numf = 0;
CoordinateSelect *locSel=NULL;
//Background * bg = NULL;
SphereMesh *bg2=NULL;
ClickList *shipList =NULL;
Unit *midway = NULL;
/*
int oldx =0;
int  oldy=0;
void startselect (KBSTATE k, int x,int y, int delx, int dely, int mod) {
  if (k==PRESS) {
    oldx = x;
    oldy = y;
  }
}

void clickhandler (KBSTATE k, int x, int y, int delx, int dely, int mod) {

  if (k==DOWN) {
    UnitCollection *c = shipList->requestIterator (oldx,oldy,x,y);
    if (c->createIterator()->current()!=NULL)
      fprintf (stderr,"Select Box Hit single target");
    if (c->createIterator()->advance()!=NULL)
      fprintf (stderr,"Select Box Hit Multiple Targets");
  }else {
    oldx = x;
    oldy = y;
  }
  if (k==PRESS) {
    fprintf (stderr,"click?");
    UnitCollection * c = shipList->requestIterator (x,y);
    if (c->createIterator()->current()!=NULL)
      fprintf (stderr,"Hit single target");
    if (c->createIterator()->advance()!=NULL)
      fprintf (stderr,"Hit Multiple Targets");
    fprintf (stderr,"\n");
    
  }
}
*/

void InitializeInput() {

	BindKey(27,0, Quit); // always have quit on esc
}

//Cockpit *cockpit;
static Texture *tmpcockpittexture;
void createObjects(std::vector <std::string> &fighter0name, std::vector <StarSystem *> &ssys, std::vector <QVector>& savedloc ) {
  vector <std::string> fighter0mods;
  vector <int> fighter0indices;
  //  GFXFogMode (FOG_OFF);


  Vector TerrainScale (XMLSupport::parse_float (vs_config->getVariable ("terrain","xscale","1")),XMLSupport::parse_float (vs_config->getVariable ("terrain","yscale","1")),XMLSupport::parse_float (vs_config->getVariable ("terrain","zscale","1")));

  myterrain=NULL;
  std::string stdstr= mission->getVariable("terrain","");
  if (stdstr.length()>0) {
    Terrain * terr = new Terrain (stdstr.c_str(), TerrainScale,XMLSupport::parse_float (vs_config->getVariable ("terrain","mass","100")), XMLSupport::parse_float (vs_config->getVariable ("terrain", "radius", "10000")));
    Matrix tmp;
    ScaleMatrix (tmp,TerrainScale);
    //    tmp.r[0]=TerrainScale.i;tmp[5]=TerrainScale.j;tmp[10]=TerrainScale.k;
    QVector pos;
    mission->GetOrigin (pos,stdstr);
    tmp.p = -pos;
    terr->SetTransformation (tmp);

  }
  stdstr= mission->getVariable("continuousterrain","");
  if (stdstr.length()>0) {
    myterrain=new ContinuousTerrain (stdstr.c_str(),TerrainScale,XMLSupport::parse_float (vs_config->getVariable ("terrain","mass","100")));
    Matrix tmp;
    Identity (tmp);
    QVector pos;
    mission->GetOrigin (pos,stdstr);
    tmp.p=-pos;
    myterrain->SetTransformation (tmp);
  }
  //  qt = new QuadTree("terrain.xml");
  /****** 
  locSel = new LocationSelect(Vector (0,-2,2),
			      Vector(1,0,-1), 
			      Vector (-1,0,-1));
//GOOD!!
  ****/
  BindKey (1,CoordinateSelect::MouseMoveHandle);

  //int numf=mission->number_of_flightgroups;
  int numf=mission->number_of_ships;

  //  cout << "numships: " << numf << endl;

  fighters = new Unit * [numf];
  int * tmptarget = new int [numf];

  GFXEnable(TEXTURE0);
  GFXEnable(TEXTURE1);

  map<string,int> targetmap;


  char fightername [1024]="hornet.xunit";
  int a=0;

  vector<Flightgroup *>::const_iterator siter;
  vector<Flightgroup *> fg=mission->flightgroups;
  int squadnum=0;
  for(siter= fg.begin() ; siter!=fg.end() ; siter++){
    Flightgroup *fg=*siter;
    string fg_name=fg->name;
    string fullname=fg->type;// + ".xunit";
    //    int fg_terrain = fg->terrain_nr;
    //    bool isvehicle = (fg->unittype==Flightgroup::VEHICLE);
    strcpy(fightername,fullname.c_str());
    string ainame=fg->ainame;
    float fg_radius=0.0;

    for(int s=0;s < fg->nr_ships;s++){
      numf++;
      QVector pox (1000+150*a,100*a,100);
      
      pox.i=fg->pos.i+s*fg_radius*3;
      pox.j=fg->pos.j+s*fg_radius*3;
      pox.k=fg->pos.k+s*fg_radius*3;
      //	  cout << "loop pos " << fg_name << " " << pox.i << pox.j << pox.k << " a=" << a << endl;
      
      if (pox.i==pox.j&&pox.j==pox.k&&pox.k==0) {
	pox.i=rand()*10000./RAND_MAX-5000;
	pox.j=rand()*10000./RAND_MAX-5000;
	pox.k=rand()*10000./RAND_MAX-5000;
	
      }
      
      
      tmptarget[a]=_Universe->GetFaction(fg->faction.c_str()); // that should not be in xml?
      int fg_terrain=-1;
      //	  cout << "before unit" << endl;
      if (fg_terrain==-1||(fg_terrain==-2&&myterrain==NULL)) {
	string modifications ("");
	if (s==0&&squadnum<(int)fighter0name.size()) {
	  _Universe->AccessCockpit(squadnum)->activeStarSystem=ssys[squadnum];
  	  fighter0indices.push_back(a);
	  if (fighter0name[squadnum].length()==0)
	    fighter0name[squadnum]=string(fightername);
	  else
	    strcpy(fightername,fighter0name[squadnum].c_str());
	  if (mission->getVariable ("savegame","").length()>0) {
		  if (savedloc[squadnum].i!=FLT_MAX) {
			pox = LaunchUnitNear(savedloc[squadnum]);
		  }
	    fighter0mods.push_back(modifications =vs_config->getVariable (string("player")+((squadnum>0)?tostring(squadnum+1):string("")),"callsign","")+mission->getVariable("savegame",""));
	  }else {
	    fighter0mods.push_back("");
	  }
	}
    if (squadnum<(int)fighter0name.size()) {
		_Universe->pushActiveStarSystem (_Universe->AccessCockpit(squadnum)->activeStarSystem);
	}
  	fighters[a] = UnitFactory::createUnit(fightername, false,tmptarget[a],modifications,fg,s);
    _Universe->activeStarSystem()->AddUnit(fighters[a]);
	if (s==0&&squadnum<(int)fighter0name.size()) {
		_Universe->AccessCockpit(squadnum)->Init (fighters[a]->getCockpit().c_str());
	    _Universe->AccessCockpit(squadnum)->SetParent(fighters[a],fighter0name[squadnum].c_str(),fighter0mods[squadnum].c_str(),pox);
	}

    if (squadnum<(int)fighter0name.size()) {
		_Universe->popActiveStarSystem ();
	}

      }else {
	  bool isvehicle=false;
	if (fg_terrain==-2) {

	  fighters[a]= UnitFactory::createBuilding (myterrain,isvehicle,fightername,false,tmptarget[a],string(""),fg);
	}else {
	  
	  if (fg_terrain>=(int)_Universe->activeStarSystem()->numTerrain()) {
	    ContinuousTerrain * t;
	    assert (fg_terrain-_Universe->activeStarSystem()->numTerrain()<_Universe->activeStarSystem()->numContTerrain());
	    t =_Universe->activeStarSystem()->getContTerrain(fg_terrain-_Universe->activeStarSystem()->numTerrain());
	    fighters[a]= UnitFactory::createBuilding (t,isvehicle,fightername,false,tmptarget[a],string(""),fg);
	  }else {
	    Terrain *t=_Universe->activeStarSystem()->getTerrain(fg_terrain);
	    fighters[a]= UnitFactory::createBuilding (t,isvehicle,fightername,false,tmptarget[a],string(""),fg);
	  }
	  
	}
      _Universe->activeStarSystem()->AddUnit(fighters[a]);

      }
      fighters[a]->SetPosAndCumPos (pox);
      
      fg_radius=fighters[a]->rSize();
      
      //    fighters[a]->SetAI(new Order());
      
      // cout << "before ai" << endl;
      
      if (benchmark>0.0  || (s!=0||squadnum>=(int)fighter0name.size())) {
	if(ainame[0]!='_'){
	  string ai_agg=ainame+".agg.xml";
	  string ai_int=ainame+".int.xml";
	  
	  char ai_agg_c[1024];
	  char ai_int_c[1024];
	  strcpy(ai_agg_c,ai_agg.c_str());
	  strcpy(ai_int_c,ai_int.c_str());
	  //      printf("1 - %s  2 - %s\n",ai_agg_c,ai_int_c);
	  
	  fighters[a]->EnqueueAI( new Orders::AggressiveAI (ai_agg_c, ai_int_c));
	}
	else{
	  string modulename=ainame.substr(1);
	  
	  fighters[a]->EnqueueAI( new AImissionScript(modulename));
	  //fighters[a]->SetAI( new AImissionScript(modulename));
	}
	fighters[a]->SetTurretAI ();
      }
      a++;
    } // for nr_ships
    squadnum++;
  } // end of for flightgroups
  
  for (int rr=0;rr<a;rr++) {
    for (int k=0;k<a-1;k++) {
      int j=rand()%a;
      if (_Universe->GetRelation(tmptarget[rr],tmptarget[j])<0) {
	fighters[rr]->Target (fighters[j]);
	break;
      }
    }
  }//now it just sets their faction :-D


  delete [] tmptarget;
  muzak = new Music (fighters[0]);
  AUDListenerSize (fighters[0]->rSize()*4);
  for (unsigned int cnum=0;cnum<fighter0indices.size();cnum++) {
    if(benchmark==-1){
      fighters[fighter0indices[cnum]]->EnqueueAI(new FlyByJoystick (cnum));
      fighters[fighter0indices[cnum]]->EnqueueAI(new FireKeyboard (cnum,cnum));
    }
    fighters[fighter0indices[cnum]]->SetTurretAI ();
  }

  shipList = _Universe->activeStarSystem()->getClickList();
  locSel = new CoordinateSelect (QVector (0,0,5));
  UpdateTime();
//  _Universe->activeStarSystem()->AddUnit(UnitFactory::createNebula ("mynebula.xml","nebula",false,0,NULL,0));

  mission->DirectorInitgame();
}
void AddUnitToSystem (const SavedUnits *su) {
  Unit * un=NULL;
  switch (su->type) {
  case ENHANCEMENTPTR:
    un = UnitFactory::createEnhancement (su->filename.c_str(),_Universe->GetFaction (su->faction.c_str()),string(""));
    un->SetPosition(QVector(0,0,0));
    break;
  case UNITPTR:
  default:
    un = UnitFactory::createUnit (su->filename.c_str(),false,_Universe->GetFaction (su->faction.c_str()));
    un->EnqueueAI (new Orders::AggressiveAI ("default.agg.xml", "default.int.xml"));
    un->SetTurretAI ();
    un->SetPosition (QVector(rand()*10000./RAND_MAX-5000,rand()*10000./RAND_MAX-5000,rand()*10000./RAND_MAX-5000));

    break;
  }
  _Universe->activeStarSystem()->AddUnit(un);

}
void destroyObjects() {  
  if (myterrain)
    delete myterrain;
  Terrain::DeleteAll();
  delete tmpcockpittexture;
  delete muzak;
  //  delete cockpit;
  delete [] fighters;
  delete locSel;
}



int getmicrosleep () {
  static int microsleep = XMLSupport::parse_int (vs_config->getVariable ("audio","threadtime","2000"));
  return microsleep;
}

void restore_main_loop() {
  RestoreKB();
  //  winsys_show_cursor(false);
  RestoreMouse();
  GFXLoop (main_loop);

}
void main_loop() {
  _Universe->StartDraw();
  if(myterrain){
    myterrain->AdjustTerrain(_Universe->activeStarSystem());
  }


}



