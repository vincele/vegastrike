/* 
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
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

#if defined(HAVE_SDL)
#include <SDL/SDL.h>
#endif

#if defined(WITH_MACOSX_BUNDLE)
#import <sys/param.h>
#endif
#ifdef _WIN32
#include <direct.h>
#endif
#include "gfxlib.h"
#include "in_kb.h"
#include "lin_time.h"
#include "main_loop.h"
#include "config_xml.h"
#include "cmd/script/mission.h"
#include "audiolib.h"
#include "vs_path.h"
#include "vs_globals.h"
#include "gfx/animation.h"
#include "cmd/unit.h"
#include "gfx/cockpit.h"
#include "python/init.h"
#include "savegame.h"
#include "force_feedback.h"
#include "gfx/hud.h"
#include "gldrv/winsys.h"
/*
 * Globals 
 */
game_data_t g_game;

Universe *_Universe;
FILE * fpread=NULL;

ForceFeedback *forcefeedback;

TextPlane *bs_tp=NULL;

/* 
 * Function definitions
 */

void setup_game_data ( ){ //pass in config file l8r??
  g_game.audio_frequency_mode=4;//22050/16
  g_game.sound_enabled =1;
  g_game.music_enabled=1;
  g_game.sound_volume=1;
  g_game.music_volume=1;
  g_game.warning_level=20;
  g_game.capture_mouse=GFXFALSE;
  g_game.y_resolution = 768;
  g_game.x_resolution = 1024;
  g_game.fov=78;
  g_game.MouseSensitivityX=2;
  g_game.MouseSensitivityY=4;

}
void ParseCommandLine(int argc, char ** CmdLine);
void cleanup(void)
{
  printf ("Thank you for playing!\n");
  _Universe->WriteSaveGame();
  //    write_config_file();
  AUDDestroy();
  //destroyObjects();
  Unit::ProcessDeleteQueue();
//  delete _Universe;
    delete [] CONFIGFILE;

  
}

VegaConfig *vs_config;
Mission *mission;
vector <Mission *> active_missions;
double benchmark=-1.0;

char mission_name[1024];

void bootstrap_main_loop();

#if defined(WITH_MACOSX_BUNDLE)
 #undef main
#endif

int main( int argc, char *argv[] ) 
{
	CONFIGFILE=0;
	mission_name[0]='\0';
//	char *TEST_STUFF=NULL;
//	*TEST_STUFF='0';//MAKE IT CRASH
#if defined(WITH_MACOSX_BUNDLE)||defined(_WIN32)
    // We need to set the path back 2 to make everything ok.
    char *parentdir;
	int pathlen=strlen(argv[0]);
	parentdir=new char[pathlen+1];
    char *c;
    strncpy ( parentdir, argv[0], pathlen+1 );
    c = (char*) parentdir;

    while (*c != '\0')     /* go to end */
        c++;
    
    while ((*c != '/')&&(*c != '\\'))      /* back up to parent */
        c--;
    
    *c++ = '\0';             /* cut off last part (binary name) */
  
    chdir (parentdir);/* chdir to the binary app's parent */
	delete []parentdir;
#if defined(WITH_MACOSX_BUNDLE)
    chdir ("../../../");/* chdir to the .app's parent */
#endif
#endif
    /* Print copyright notice */
  fprintf( stderr, "Vega Strike "  " \n"
	     "See http://www.gnu.org/copyleft/gpl.html for license details.\n\n" );
    /* Seed the random number generator */
    

    if(benchmark<0.0){
      srand( time(NULL) );
    }
    else{
      // in benchmark mode, always use the same seed
      srand(171070);
    }
    //this sets up the vegastrike config variable
    setup_game_data(); 
    // loads the configuration file .vegastrikerc from home dir if such exists
    ParseCommandLine(argc,argv);
	if (CONFIGFILE==0) {
		CONFIGFILE=new char[42];
		sprintf(CONFIGFILE,"vegastrike.config");
	}
		
    initpaths();
    //can use the vegastrike config variable to read in the default mission
    if (mission_name[0]=='\0')
      strcpy(mission_name,vs_config->getVariable ("general","default_mission","test1.mission").c_str());
    //might overwrite the default mission with the command line

#ifdef HAVE_BOOST

    //	Python::init();

#endif

#if defined(HAVE_SDL)
    // && defined(HAVE_SDL_MIXER)
  if (  SDL_InitSubSystem( SDL_INIT_JOYSTICK )) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        winsys_exit(1);
    }

#endif
#if 0
    InitTime();
    UpdateTime();
#endif

    AUDInit();

    /* Set up a function to clean up when program exits */
    winsys_atexit( cleanup );
    /*
#if defined(HAVE_SDL) && defined(HAVE_SDL_MIXER)

    init_audio_data();
    init_audio();
    init_joystick();

#endif
    */
    _Universe= new Universe(argc,argv,vs_config->getVariable ("general","galaxy","milky_way.xml").c_str());   

    _Universe->Loop(bootstrap_main_loop);
    return 0;
}
  static Animation * SplashScreen = NULL;
static bool BootstrapMyStarSystemLoading=true;
void SetStarSystemLoading (bool value) {
  BootstrapMyStarSystemLoading=value;
}

void bootstrap_draw (const std::string &message, float x, float y, Animation * newSplashScreen) {

  static Animation *ani=NULL;
  if (!BootstrapMyStarSystemLoading) {
    return;
  }
  if(SplashScreen==NULL){
    // if there's no splashscreen, we don't draw on it
    // this happens, when the splash screens texture is loaded
    return;
  }

  if (newSplashScreen!=NULL) {
    ani = newSplashScreen;
  }
  UpdateTime();

  Matrix tmp;
  Identity (tmp);
  BootstrapMyStarSystemLoading=false;
  static Texture dummy ("white.bmp");
  BootstrapMyStarSystemLoading=true;
  dummy.MakeActive();
  GFXDisable(LIGHTING);
  GFXDisable(DEPTHTEST);
  GFXBlendMode (ONE,ZERO);
  GFXEnable (TEXTURE0);
  GFXDisable (TEXTURE1);
  GFXColor4f (1,1,1,1);
  GFXClear (GFXTRUE);
  GFXLoadIdentity (PROJECTION);
  GFXLoadIdentityView();
  GFXLoadMatrix (MODEL,tmp);
  GFXBeginScene();
  bs_tp->SetPos (x,y);
  bs_tp->SetCharSize (.4,.8);
  ScaleMatrix (tmp,Vector (7,7,0));
  GFXLoadMatrix (MODEL,tmp);
  if (ani) {
    ani->UpdateAllFrame();
    ani->DrawNow(tmp);
  }
  bs_tp->Draw (message);  
  GFXEndScene();

}
extern Unit **fighters;


bool SetPlayerLoc (Vector &sys, bool set) {
  static Vector mysys;
  static bool isset=false;
  if (set) {
    isset = true;
    mysys = sys;
    return true;
  }else {
    if (isset)
      sys =mysys;
    return isset;
  }  
  
}
bool SetPlayerSystem (std::string &sys, bool set) {
  static std::string mysys;
  static bool isset=false;
  if (set) {
    isset = true;
    mysys = sys;
    return true;
  }else {
    if (isset)
      sys =mysys;
    return isset;
  }
}

void bootstrap_main_loop () {

  static bool LoadMission=true;
  InitTime();

  //  bootstrap_draw ("Beginning Load...",SplashScreen);
  //  bootstrap_draw ("Beginning Load...",SplashScreen);
  if (LoadMission) {
    LoadMission=false;
    active_missions.push_back(mission=new Mission(mission_name));

    mission->initMission();

    bs_tp=new TextPlane();
 
    SplashScreen = new Animation (mission->getVariable ("splashscreen",vs_config->getVariable ("graphics","splash_screen","vega_splash.ani")).c_str(),0);
    bootstrap_draw ("Vegastrike Loading...",-.135,0,SplashScreen);

    Vector pos;
    string planetname;

    mission->GetOrigin(pos,planetname);
    bool setplayerloc=false;
    string mysystem = mission->getVariable("system","sol.system");
    int numplayers = XMLSupport::parse_int (mission->getVariable ("num_players","1"));
    vector <std::string>playername;
    for (int p=0;p<numplayers;p++) {
      playername.push_back(vs_config->getVariable("player"+((p>0)?tostring(p+1):string("")),"callsign",""));
    }
    _Universe->SetupCockpits(playername);
    float credits=XMLSupport::parse_float (mission->getVariable("credits","0"));
    g_game.difficulty=XMLSupport::parse_float (mission->getVariable("difficulty","1"));
    string savegamefile = mission->getVariable ("savegame","");
    vector <SavedUnits> savedun;
    vector <string> playersaveunit;
	vector <StarSystem *> ss;
	vector <string> starsysname;
	vector <Vector> playerNloc;
    for (unsigned int k=0;k<_Universe->cockpit.size();k++) {
      bool setplayerXloc=false;
      std::string psu;
      if (k==0) {
		Vector myVec;
		if (SetPlayerLoc (myVec,false)) {
		  _Universe->cockpit[0]->savegame->SetPlayerLocation(myVec);
		}
		std::string st;
		if (SetPlayerSystem (st,false)) {
		  _Universe->cockpit[0]->savegame->SetStarSystem(st);
		}
      }
      vector <SavedUnits> saved=_Universe->cockpit[k]->savegame->ParseSaveGame (savegamefile,mysystem,mysystem,pos,setplayerXloc,credits,psu);
      playersaveunit.push_back(psu);
      _Universe->cockpit[k]->credits=credits;
  	  ss.push_back (_Universe->Init (mysystem,Vector(0,0,0),planetname));
	  if (setplayerXloc) {
	   	  playerNloc.push_back(pos);
	  }else {
		  playerNloc.push_back(Vector(FLT_MAX,FLT_MAX,FLT_MAX));
	  }
	  setplayerloc=setplayerXloc;//FIX ME will only set first player where he was

      for (unsigned int j=0;j<saved.size();j++) {
		savedun.push_back(saved[j]);
      }

    }
    SetStarSystemLoading (true);
    InitializeInput();

    vs_config->bindKeys();//gotta do this before we do ai

    createObjects(playersaveunit,ss,playerNloc);

    if (setplayerloc&&fighters) {
      if (fighters[0]) {
	//fighters[0]->SetPosition (Vector (0,0,0));
      }
    }
    
    while (!savedun.empty()) {
      AddUnitToSystem (&savedun.back());
      savedun.pop_back();
    }
    

    forcefeedback=new ForceFeedback();

    UpdateTime();
    delete SplashScreen;
    SplashScreen= NULL;
    SetStarSystemLoading (false);
    _Universe->LoadContrabandLists();
    _Universe->Loop(main_loop);
    ///return to idle func which now should call main_loop mohahahah
  }
  ///Draw Texture
  
} 




void ParseCommandLine(int argc, char ** lpCmdLine) {
  std::string st;
  Vector PlayerLocation;
  for (int i=1;i<argc;i++) {
    if(lpCmdLine[i][0]=='-') {
      switch(lpCmdLine[i][1]){
      case 'r':
      case 'R':
	break;
      case 'M':
      case 'm':
		  if (!(lpCmdLine[i][2]=='1'&&lpCmdLine[i][3]=='\0')) {
	  		CONFIGFILE=new char[40+strlen(lpCmdLine[i])+1];
			sprintf(CONFIGFILE,"vegastrike.config.%splayer",lpCmdLine[i]+2);
		  }
	break;
      case 'S':
      case 's':
	g_game.sound_enabled=1;
	break;
      case 'f':
      case 'F':
	break;
      case 'P':
      case 'p':
	sscanf (lpCmdLine[i]+2,"%f,%f,%f",&PlayerLocation.i,&PlayerLocation.j,&PlayerLocation.k);
	SetPlayerLoc (PlayerLocation,true);
	break;
      case 'J':
      case 'j'://low rez
	st=string ((lpCmdLine[i])+2);
	SetPlayerSystem (st ,true);
	break;
      case 'A'://average rez
      case 'a': 
	g_game.y_resolution = 600;
	g_game.x_resolution = 800;
	break;
      case 'H':
      case 'h'://high rez
	g_game.y_resolution = 768;
	g_game.x_resolution = 1024;
	break;
      case 'V':
      case 'v':
	g_game.y_resolution = 1024;
	g_game.x_resolution = 1280;
	break;
      case 'D':
      case 'd':
	break;
      case 'G':
      case 'g':
	//viddrv = "GLDRV.DLL";
	break;
      case '-':
	// long options
	if(strcmp(lpCmdLine[i],"--benchmark")==0){
	  //benchmark=30.0;
	  benchmark=atof(lpCmdLine[i+1]);
	  i++;
	}
      }
    }
    else{
      // no "-" before it - it's the mission name
      strncpy (mission_name,lpCmdLine[i],1023);
	  mission_name[1023]='\0';
    }
  }
  //FILE *fp = fopen("vid.cfg", "rb");
  //  GUID temp;
  //fread(&temp, sizeof(GUID), 1, fp);
  //fread(&temp, sizeof(GUID), 1, fp);
  //fread(&_ViewPortWidth, sizeof(DWORD), 1, fp);
  //fread(&_ViewPortHeight, sizeof(DWORD), 1, fp);
  //fread(&_ColDepth,sizeof(DWORD),1,fp);
  //fclose(fp);
}
