#include <assert.h>
#include "star_system.h"
#include "cmd/planet.h"
#include "cmd/unit.h"
#include "cmd/unit_collide.h"
#include "cmd/collection.h"
#include "cmd/click_list.h"
#include "cmd/ai/input_dfa.h"
#include "lin_time.h"
#include "cmd/beam.h"
#include "gfx/sphere.h"
#include "cmd/unit_collide.h"
#include "gfx/halo.h"
#include "gfx/background.h"
#include "gfx/animation.h"
#include "gfx/aux_texture.h"
#include "gfx/star.h"
#include "cmd/bolt.h"
#include <expat.h>
#include "gfx/cockpit.h"
#include "audiolib.h"
#include "cmd/music.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "cmd/cont_terrain.h"
#include "vegastrike.h"
#include "universe.h"
#include "cmd/atmosphere.h"
#include "hashtable.h"


extern Music *muzak;
extern Vector mouseline;

vector<Vector> perplines;
//static SphereMesh *foo;
//static Unit *earth;



Atmosphere *theAtmosphere;




StarSystem::StarSystem(const char * filename, const Vector & centr,const string planetname) : 
  //  primaries(primaries), 
  drawList(new UnitCollection),//what the hell is this...maybe FALSE FIXME
  units(new UnitCollection), 
  missiles(new UnitCollection) {
  ///adds to jumping table;
  name = NULL;
  _Universe->pushActiveStarSystem (this);
  GFXCreateLightContext (lightcontext);
  bolts = new bolt_draw;
  collidetable = new CollideTable;
  cout << "origin: " << centr.i << " " << centr.j << " " << centr.k << " " << planetname << endl;

  current_stage=PHY_AI;
  currentcamera = 0;	
  systemInputDFA = new InputDFA (this);
  numprimaries=0;
  LoadXML(filename,centr);
  if (!name)
    name =strdup (filename);
  AddStarsystemToUniverse(filename);
//  primaries[0]->SetPosition(0,0,0);
  int i;
  Iterator * iter;
  for (i =0;i<numprimaries;i++) {
	  if (primaries[i]->isUnit()==PLANETPTR) {
		iter = ((Planet*)primaries[i])->createIterator();
		drawList->prepend(iter);
		delete iter;
	  } else {
		drawList->prepend (primaries[i]);
	  }
  }
  //iter = primaries->createIterator();
  //iter->advance();
  //earth=iter->current();
  //delete iter;



  // Calculate movement arcs; set behavior of primaries to follow these arcs
  //Iterator *primary_iterator = primaries->createIterator(); 
  //primaries->SetPosition(0,0,5);
  //foo = new SphereMesh(1,5,5,"moon.bmp");
  cam[1].SetProjectionType(Camera::PARALLEL);
  cam[1].SetZoom(1);
  cam[1].SetPosition(Vector(0,0,0));
  cam[1].LookAt(Vector(0,0,0), Vector(0,0,1));
  //cam[1].SetPosition(Vector(0,5,-2.5));
  cam[1].SetSubwindow(0,0,1,1);

  cam[2].SetProjectionType(Camera::PARALLEL);
  cam[2].SetZoom(10.0);
  cam[2].SetPosition(Vector(5,0,0));
  cam[2].LookAt(Vector(0,0,0), Vector(0,-1,0));
  //cam[2].SetPosition(Vector(5,0,-2.5));
  cam[2].SetSubwindow(0.10,0,0.10,0.10);
  UpdateTime();
  time = 0;


  Atmosphere::Parameters params;

  params.radius = 40000;



  params.low_color[0] = GFXColor(0,0.5,0.0);

  params.low_color[1] = GFXColor(0,1.0,0.0);

  params.low_ambient_color[0] = GFXColor(0.0/255.0,0.0/255.0,0.0/255.0);

  params.low_ambient_color[1] = GFXColor(0.0/255.0,0.0/255.0,0.0/255.0);

  params.high_color[0] = GFXColor(0.5,0.0,0.0);

  params.high_color[1] = GFXColor(1.0,0.0,0.0);

  params.high_ambient_color[0] = GFXColor(0,0,0);

  params.high_ambient_color[1] = GFXColor(0,0,0);

  /*

  params.low_color[0] = GFXColor(241.0/255.0,123.0/255.0,67.0/255.0);

  params.low_color[1] = GFXColor(253.0/255.0,65.0/255.0,55.0/255.0);

  params.low_ambient_color[0] = GFXColor(0.0/255.0,0.0/255.0,0.0/255.0);

  params.low_ambient_color[1] = GFXColor(0.0/255.0,0.0/255.0,0.0/255.0);

  params.high_color[0] = GFXColor(60.0/255.0,102.0/255.0,249.0/255.0);

  params.high_color[1] = GFXColor(57.0/255.0,188.0/255.0,251.0/255.0);

  params.high_ambient_color[0] = GFXColor(0,0,0);

  params.high_ambient_color[1] = GFXColor(0,0,0);

  */

  params.scattering = 5;

  theAtmosphere = new Atmosphere(params);
  _Universe->popActiveStarSystem ();

}
void StarSystem::activateLightMap() {
#ifdef NV_CUBE_MAP
  LightMap[0]->MakeActive();
  LightMap[1]->MakeActive();
  LightMap[2]->MakeActive();
  LightMap[3]->MakeActive();
  LightMap[4]->MakeActive();
  LightMap[5]->MakeActive();
#else
    LightMap[0]->MakeActive();
#endif
}

StarSystem::~StarSystem() {
#ifdef NV_CUBE_MAP

  delete LightMap[0];
  delete LightMap[1];
  delete LightMap[2];
  delete LightMap[3];
  delete LightMap[4];
  delete LightMap[5];
#else
  delete LightMap[0];
#endif
  delete bg;
  delete stars;
  delete [] name;
  delete systemInputDFA;
  for (int i=0;i<numprimaries;i++) {
	delete primaries[i];
  }
  delete [] primaries;
  delete bolts;
  delete collidetable;
  GFXDeleteLightContext (lightcontext);
}

UnitCollection * StarSystem::getUnitList () {
  return drawList;
}

ClickList *StarSystem::getClickList() {
  return new ClickList (this, drawList);

}
/**OBSOLETE!
void StarSystem::modelGravity(bool lastframe) {
  for (int i=0;i<numprimaries;i++) {
    primaries[i]->UpdatePhysics (identity_transformation,identity_matrix,lastframe,units)
  }
}	
*/
void StarSystem::AddUnit(Unit *unit) {
  units->prepend(unit);
  drawList->prepend(unit);
}

bool StarSystem::RemoveUnit(Unit *un) {
  bool removed2=false;
  Iterator *iter = units->createIterator();
  Unit *unit;
  while((unit = iter->current())!=NULL) {
    if (unit==un) {
      iter->remove();
      removed2 =true;
      break;
    } else {
      iter->advance();
    }
  }
  delete iter;  
  bool removed =false;
  if (removed2) {
    Iterator *iter = drawList->createIterator();
    Unit *unit;
    while((unit = iter->current())!=NULL) {
      if (unit==un) {
	iter->remove();
	removed =true;
	break;
      }else {
	iter->advance();
      }
    }
    delete iter;  
  }
  return removed;
}
void StarSystem::SwapIn () {
  GFXSetLightContext (lightcontext);
  /*
  Iterator *iter = drawList->createIterator();
  Unit *unit;
  while((unit = iter->current())!=NULL) {
    if (unit->isUnit()==PLANETPTR) {
      ((Planet *)unit)->EnableLights();
    }
    iter->advance();
  }
  delete iter;  
  */
  unsigned int i;
  for (i=0;i<terrains.size();i++) {
    //gotta push this shit somehow
    //terrains[i]->EnableDraw();
  }
  for (i=0;i<contterrains.size();i++) {
    //contterrains[i]->EnableDraw();
  }
}

void StarSystem::SwapOut () {
  /*
  Iterator *iter = drawList->createIterator();
  Unit *unit;
  while((unit = iter->current())!=NULL) {
    if (unit->isUnit()==PLANETPTR) {
      ((Planet *)unit)->DisableLights();
    }
    iter->advance();
  }
  delete iter;
  */
  unsigned int i;
  for (i=0;i<terrains.size();i++) {
    //terrains[i]->DisableDraw();
  }
  for (i=0;i<contterrains.size();i++) {
    //contterrains[i]->DisableDraw();
  }

}
bool shouldfog=false;
extern double interpolation_blend_factor;
void StarSystem::Draw() {
  interpolation_blend_factor = (1./PHY_NUM)*((PHY_NUM*time)/SIMULATION_ATOM+current_stage);
  AnimatedTexture::UpdateAllFrame();
  for (unsigned int i=0;i<contterrains.size();i++) {
    contterrains[i]->AdjustTerrain(this);
  }
  GFXDisable (LIGHTING);
  bg->Draw();

  Iterator *iter = drawList->createIterator();
  Unit *unit;
  shouldfog=false;
  //  fprintf (stderr,"|t%f i%lf|",GetElapsedTime(),interpolation_blend_factor);
  while((unit = iter->current())!=NULL) {
    unit->Draw();
    iter->advance();
  }
  delete iter;
  if (shouldfog) {
    GFXFogMode (FOG_EXP2);
    GFXFogDensity (.0005);
    GFXFogLimits (1,1000);
    GFXFogIndex (0);
    GFXFogColor (GFXColor(.5,.5,.5,.5));
  }else {
    GFXFogMode (FOG_OFF);
  }
  _Universe->AccessCockpit()->SetupViewPort(true);///this is the final, smoothly calculated cam
  
  //  SetViewport();//camera wielding unit is now drawn  Note: Background is one frame behind...big fat hairy deal
  GFXColor tmpcol (0,0,0,1);
  GFXGetLightContextAmbient(tmpcol);
  Mesh::ProcessZFarMeshes();
  Terrain::RenderAll();
  Mesh::ProcessUndrawnMeshes(true);
  

  GFXFogMode (FOG_OFF);
  Matrix ident;

  Identity(ident);

  //Atmosphere::ProcessDrawQueue();
  //theAtmosphere->Draw(Vector(0,1,0),ident);



  GFXPopGlobalEffects();



  GFXLightContextAmbient(tmpcol);
  Halo::ProcessDrawQueue();
  if (shouldfog)
    GFXFogMode (FOG_EXP2);
  Beam::ProcessDrawQueue();
  Animation::ProcessDrawQueue();
  Bolt::Draw();



  stars->Draw();

  if (shouldfog)
    GFXFogMode (FOG_OFF);

  static bool doInputDFA = XMLSupport::parse_bool (vs_config->getVariable ("graphics","MouseCursor","false"));
  _Universe->AccessCockpit()->Draw();
  if (doInputDFA) {
    GFXHudMode (true);
    systemInputDFA->Draw();
    GFXHudMode (false);
  }

}



void StarSystem::Update() {

  Unit *unit;

  time += GetElapsedTime();
  bool firstframe = true;
  _Universe->pushActiveStarSystem(this);
  if(time/SIMULATION_ATOM>=(1./PHY_NUM)) {
    while(time/SIMULATION_ATOM >= (1./PHY_NUM)) { // Chew up all SIMULATION_ATOMs that have elapsed since last update
      Iterator *iter;
      if (current_stage==PHY_AI) {
	iter = drawList->createIterator();
	if (firstframe&&rand()%2) {
	  AUDRefreshSounds();
	}
	while((unit = iter->current())!=NULL) {
	  unit->ExecuteAI(); 
	  unit->ResetThreatLevel();
	  iter->advance();
	}
	delete iter;
	current_stage=TERRAIN_BOLT_COLLIDE;
      } else if (current_stage==TERRAIN_BOLT_COLLIDE) {
	Terrain::CollideAll();
	current_stage=PHY_COLLIDE;
	AnimatedTexture::UpdateAllPhysics();
      } else if (current_stage==PHY_COLLIDE) {
	static int numframes=0;
	numframes++;//don't resolve physics until 2 seconds
	if (numframes>2/(SIMULATION_ATOM)) {
	  iter = drawList->createIterator();
	  while((unit = iter->current())!=NULL) {
		unit->Setnebula(NULL); 
		iter->advance();
	  }
	  delete iter;
	  iter = drawList->createIterator();
	  while((unit = iter->current())!=NULL) {
	    unit->CollideAll();
	    iter->advance();
	  }
	  delete iter;
	}
	current_stage=PHY_TERRAIN;
      } else if (current_stage==PHY_TERRAIN) {
	Terrain::UpdateAll(64);	
	current_stage=PHY_RESOLV;
      } else if (current_stage==PHY_RESOLV) {
	iter = drawList->createIterator();
	AccessCamera()->UpdateCameraSounds();
	muzak->Listen();
	AccessCamera()->SetNebula(NULL);//Update physics should set this
	while((unit = iter->current())!=NULL) {
	  unit->UpdatePhysics(identity_transformation,identity_matrix,Vector (0,0,0),firstframe,units);
	  iter->advance();
	}
	delete iter;
	bolts->UpdatePhysics();
	current_stage=PHY_AI;
	firstframe = false;
      }
      time -= (1./PHY_NUM)*SIMULATION_ATOM;
    }
  }

  _Universe->popActiveStarSystem();
  //  fprintf (stderr,"bf:%lf",interpolation_blend_factor);
}


void StarSystem::SelectCamera(int cam){
    if(cam<NUM_CAM&&cam>=0)
      currentcamera = cam;
}
Camera* StarSystem::AccessCamera(int num){
  if(num<NUM_CAM&&num>=0)
    return &cam[num];
  else
    return NULL;
}
