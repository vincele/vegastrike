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
#include "cmd/nebula.h"
#include "galaxy_gen.h"
#include "cmd/script/mission.h"
#include "in_kb.h"
#include "cmd/script/flightgroup.h"
#include "load_mission.h"
extern Music *muzak;
extern Vector mouseline;

vector<Vector> perplines;
//static SphereMesh *foo;
//static Unit *earth;

extern void reset_time_compression (int,KBSTATE);

Atmosphere *theAtmosphere;

extern string RemoveDotSystem (const char *input);
string StarSystem::getFileName() {
  return getStarSystemSector (filename)+string("/")+RemoveDotSystem(getStarSystemName (filename).c_str());
}
string StarSystem::getName () {
  return string(name);
}

StarSystem::StarSystem(const char * filename, const Vector & centr,const float timeofyear) {

  no_collision_time=0;//(int)(1+2.000/SIMULATION_ATOM);
  ///adds to jumping table;
  name = NULL;
  _Universe->pushActiveStarSystem (this);
  GFXCreateLightContext (lightcontext);
  bolts = new bolt_draw;
  collidetable = new CollideTable(this);
  //  cout << "origin: " << centr.i << " " << centr.j << " " << centr.k << " " << planetname << endl;

  current_stage=PHY_AI;

  systemInputDFA = new InputDFA (this);

  LoadXML(filename,centr,timeofyear);
  if (!name)
    name =strdup (filename);
  AddStarsystemToUniverse(filename);
//  primaries[0]->SetPosition(0,0,0);

  //iter = primaries->createIterator();
  //iter->advance();
  //earth=iter->current();
  //delete iter;



  // Calculate movement arcs; set behavior of primaries to follow these arcs
  //Iterator *primary_iterator = primaries->createIterator(); 
  //primaries->SetPosition(0,0,5);
  //foo = new SphereMesh(1,5,5,"moon.bmp");
  //cam[1].SetProjectionType(Camera::PARALLEL);
  //cam[1].SetZoom(1);
  //cam[1].SetPosition(Vector(0,0,0));
  //cam[1].LookAt(Vector(0,0,0), Vector(0,0,1));
  //cam[1].SetPosition(Vector(0,5,-2.5));
  //cam[1].SetSubwindow(0,0,1,1);

  //cam[2].SetProjectionType(Camera::PARALLEL);
  //cam[2].SetZoom(10.0);
  //cam[2].SetPosition(Vector(5,0,0));
  //cam[2].LookAt(Vector(0,0,0), Vector(0,-1,0));
  //cam[2].SetPosition(Vector(5,0,-2.5));
  //cam[2].SetSubwindow(0.10,0,0.10,0.10);
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
  GFXActiveTexture (1);
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
  GFXActiveTexture (0);
}

StarSystem::~StarSystem() {
  _Universe->activeStarSystem()->SwapOut();
  _Universe->pushActiveStarSystem(this);
  _Universe->activeStarSystem()->SwapIn();  
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
  /* //FIXME  after doign so much debugging I think you shouldn't delete this
  for (int i=0;i<numprimaries;i++) {
	delete primaries[i];
	}
	delete [] primaries;

  */

  delete bolts;


  UnitCollection::UnitIterator iter = drawList.createIterator();
  Unit *unit;
  //  fprintf (stderr,"|t%f i%lf|",GetElapsedTime(),interpolation_blend_factor);
  while((unit = iter.current())!=NULL) {
    unit->Kill(false);
    iter.advance();
  }
  delete collidetable;
  _Universe->activeStarSystem()->SwapOut();
  //  GFXDeleteLightContext (lightcontext);
  _Universe->popActiveStarSystem();
  _Universe->activeStarSystem()->SwapIn();  
  RemoveStarsystemFromUniverse();
  
}


ClickList *StarSystem::getClickList() {
  return new ClickList (this, &drawList);

}
/**OBSOLETE!
void StarSystem::modelGravity(bool lastframe) {
  for (int i=0;i<numprimaries;i++) {
    primaries[i]->UpdatePhysics (identity_transformation,identity_matrix,lastframe,units)
  }
}	
*/
void StarSystem::AddUnit(Unit *unit) {
  units.prepend(unit);
  drawList.prepend(unit);
}

bool StarSystem::RemoveUnit(Unit *un) {
  bool removed2=false;
  UnitCollection::UnitIterator iter = units.createIterator();
  Unit *unit;
  while((unit = iter.current())!=NULL) {
    if (unit==un) {
      iter.remove();
      removed2 =true;
      break;
    } else {
      iter.advance();
    }
  }
  bool removed =false;
  if (removed2) {
    UnitCollection::UnitIterator iter = drawList.createIterator();
    Unit *unit;
    while((unit = iter.current())!=NULL) {
      if (unit==un) {
	iter.remove();
	removed =true;
	break;
      }else {
	iter.advance();
      }
    }
  }
  return removed;
}
void StarSystem::SwapIn () {
  GFXSetLightContext (lightcontext);

  /*
  for (un_iter un_i=drawList.createIterator();NULL!= (un=*un_i);++un_i) {
    un->SwapInHalos();

  }
  */
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
  for (un_iter un_i=drawList.createIterator();NULL!= (un=*un_i);++un_i) {
    un->SwapOutHalos();
    Halo::ProcessDrawQueue();

  }
  */
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

extern double interpolation_blend_factor;
//#define UPDATEDEBUG  //for hard to track down bugs
void StarSystem::Draw(bool DrawCockpit) {
  GFXEnable (DEPTHTEST);
  GFXEnable (DEPTHWRITE);

#ifdef UPDATEDEBUG
  fprintf (stderr,"begin Draw");
  fflush (stderr);
#endif
  interpolation_blend_factor = (1./PHY_NUM)*((PHY_NUM*time)/SIMULATION_ATOM+current_stage);
#ifdef UPDATEDEBUG
  fprintf (stderr,"ani");
  fflush (stderr);
#endif
  GFXColor4f(1,1,1,1);
  if (DrawCockpit) {
    AnimatedTexture::UpdateAllFrame();
  }
  for (unsigned int i=0;i<contterrains.size();i++) {
    contterrains[i]->AdjustTerrain(this);
  }

  GFXDisable (LIGHTING);
#ifdef UPDATEDEBUG
  fprintf (stderr,"bg");
  fflush (stderr);
#endif
  bg->Draw();

  UnitCollection::UnitIterator iter = drawList.createIterator();
  Unit *unit;
  //  fprintf (stderr,"|t%f i%lf|",GetElapsedTime(),interpolation_blend_factor);
#ifdef UPDATEDEBUG
  fprintf (stderr,"cp");
  fflush (stderr);
#endif
  if (_Universe->AccessCockpit()->GetParent()==NULL) {
#ifdef UPDATEDEBUG
  fprintf (stderr,"cpu");
  fflush (stderr);
#endif
    _Universe->AccessCamera()->UpdateGFX (GFXTRUE);
  }
#ifdef UPDATEDEBUG
  fprintf (stderr,">un<");
  fflush (stderr);
#endif
  while((unit = iter.current())!=NULL) {
    unit->Draw();
    iter.advance();
  }
#ifdef UPDATEDEBUG
  fprintf (stderr,"fog");
  fflush (stderr);
#endif

  GFXFogMode (FOG_OFF);

#ifdef UPDATEDEBUG
  fprintf (stderr,"vp");
  fflush (stderr);
#endif
  _Universe->AccessCockpit()->SetupViewPort(true);///this is the final, smoothly calculated cam
  //  SetViewport();//camera wielding unit is now drawn  Note: Background is one frame behind...big fat hairy deal
  GFXColor tmpcol (0,0,0,1);
  GFXGetLightContextAmbient(tmpcol);
#ifdef UPDATEDEBUG
  fprintf (stderr,"farmsh");
  fflush (stderr);
#endif
  Mesh::ProcessZFarMeshes();
#ifdef UPDATEDEBUG
  fprintf (stderr,"terr");
  fflush (stderr);
#endif
  GFXEnable (DEPTHTEST);
  GFXEnable (DEPTHWRITE);
  //need to wait for lights to finish
  Planet::ProcessTerrains();
  Terrain::RenderAll();
#ifdef UPDATEDEBUG
  fprintf (stderr,"nearmsh");
  fflush (stderr);
#endif
  Mesh::ProcessUndrawnMeshes(true);
  Nebula * neb;

  Matrix ident;

  Identity(ident);

  //Atmosphere::ProcessDrawQueue();
  //theAtmosphere->Draw(Vector(0,1,0),ident);



  GFXPopGlobalEffects();



  GFXLightContextAmbient(tmpcol);
#ifdef UPDATEDEBUG
  fprintf (stderr,"halo");
  fflush (stderr);
#endif


  
#ifdef UPDATEDEBUG
  fprintf (stderr,"bem");
  fflush (stderr);
#endif

  if ((neb = _Universe->AccessCamera()->GetNebula())) {
    neb->SetFogState();
  }
  Beam::ProcessDrawQueue();
#ifdef UPDATEDEBUG
  fprintf (stderr,"anidq");
  fflush (stderr);
#endif

#ifdef UPDATEDEBUG
  fprintf (stderr,"bolt");
  fflush (stderr);
#endif
  Bolt::Draw();

#ifdef UPDATEDEBUG
  fprintf (stderr,"star");
  fflush (stderr);
#endif
  //  if (_Universe->AccessCamera()->GetNebula()!=NULL)
  GFXFogMode (FOG_OFF);
  Animation::ProcessDrawQueue();
  Halo::ProcessDrawQueue();
  stars->Draw();
  StarSystem::DrawJumpStars();

  static bool doInputDFA = XMLSupport::parse_bool (vs_config->getVariable ("graphics","MouseCursor","false"));
#ifdef UPDATEDEBUG
  fprintf (stderr,"cpDraw");
  fflush (stderr);
#endif
  if (DrawCockpit) {
    _Universe->AccessCockpit()->Draw();
    if (doInputDFA) {
      GFXHudMode (true);
      systemInputDFA->Draw();
      GFXHudMode (false);
    }
  }
#ifdef UPDATEDEBUG
  fprintf (stderr,"end Draw\n");
  fflush (stderr);
#endif
}
void StarSystem::ExecuteUnitAI () {
  un_iter iter = this->getUnitList().createIterator();
  Unit * unit=NULL;
	while((unit = iter.current())!=NULL) {
	  unit->ExecuteAI(); 
	  unit->ResetThreatLevel();
	  iter.advance();
	}
}
void StarSystem::UpdateUnitPhysics (bool firstframe) {
  un_iter iter = this->getUnitList().createIterator();
  Unit * unit=NULL;
	while((unit = iter.current())!=NULL) {
#ifdef OLD_AUTOPILOT
	if (owner) 
	  if (owner->getRelation(unit)<0) {
	    static float neardist =XMLSupport::parse_float(vs_config->getVariable("physics","autodist","4000"));
	    Vector diff (owner->Position()-unit->Position());
	    if (diff.Dot(diff)<neardist*neardist) {
	      if (getTimeCompression!=.0000001) {//if not paused
		reset_time_compression(0,PRESS);
	      }
	    }
	  }
#endif
	  unit->UpdatePhysics(identity_transformation,identity_matrix,Vector (0,0,0),firstframe,&this->gravitationalUnits());
	  iter.advance();
	}
}

extern float getTimeCompression();
void StarSystem::Update(float priority , bool executeDirector) {

  Unit *unit;
#ifdef UPDATEDEBUG
  fprintf (stderr,"begin Update");
  fflush (stderr);
#endif
  bool firstframe = true;

  ///this makes it so systems without players may be simulated less accurately
  for (int k=0;k<_Universe->numPlayers();k++) {
    if (_Universe->AccessCockpit(k)->activeStarSystem==this) {
      priority=1;
    }
  }
  float normal_simulation_atom = SIMULATION_ATOM;
  SIMULATION_ATOM/=(priority/getTimeCompression());
  ///just be sure to restore this at the end

  time += GetElapsedTime();
  _Universe->pushActiveStarSystem(this);
  //WARNING PERFORMANCE HACK!!!!!
    if (time>2*SIMULATION_ATOM) {
      time = 2*SIMULATION_ATOM;
    }
  if(time/SIMULATION_ATOM>=(1./PHY_NUM)) {
    while(time/SIMULATION_ATOM >= (1./PHY_NUM)) { // Chew up all SIMULATION_ATOMs that have elapsed since last update
      UnitCollection::UnitIterator iter;
      if (current_stage==PHY_AI) {
	if (firstframe&&rand()%2) {
	  if (this==_Universe->getActiveStarSystem(0)) {
#ifdef UPDATEDEBUG
  fprintf (stderr,"Snd");
  fflush (stderr);
#endif
	    AUDRefreshSounds();
	  }
	}
#ifdef UPDATEDEBUG
  fprintf (stderr,"AI");
  fflush (stderr);
#endif
  ExecuteUnitAI();
	current_stage=TERRAIN_BOLT_COLLIDE;
      } else if (current_stage==TERRAIN_BOLT_COLLIDE) {
#ifdef UPDATEDEBUG
  fprintf (stderr,"TerCol");
  fflush (stderr);
#endif
	Terrain::CollideAll();
#ifdef UPDATEDEBUG
  fprintf (stderr,"Ani");
  fflush (stderr);
#endif
	AnimatedTexture::UpdateAllPhysics();
	///Do gravitation!
	iter = units.createIterator();
	  while((unit = iter.current())!=NULL) {
	    //gravitate //FIXME
	    iter.advance();
	  }
	  //FIXME somehow only works if called once per frame
#ifdef UPDATEDEBUG
  fprintf (stderr,"DelQ");
  fflush (stderr);
#endif
	  Unit::ProcessDeleteQueue();
	  current_stage=MISSION_SIMULATION;
      }else if (current_stage==MISSION_SIMULATION) {
#define RUN_ONLY_FOR_PLAYER_STARSYSTEM
#ifdef RUN_ONLY_FOR_PLAYER_STARSYSTEM
	if (_Universe->getActiveStarSystem(0)==this) {
#endif
	  if (executeDirector) {
	    unsigned int curcockpit= _Universe->CurrentCockpit();
	    for (unsigned int i=0;i<active_missions.size();i++) {
	      if (active_missions[i]) {
			  _Universe->SetActiveCockpit(active_missions[i]->player_num);
			   StarSystem * ss=_Universe->AccessCockpit()->activeStarSystem;
			  if (ss) _Universe->pushActiveStarSystem(ss);
			  mission=active_missions[i];
			  active_missions[i]->DirectorLoop();
			  active_missions[i]->DirectorBenchmark();
			  if (ss)_Universe->popActiveStarSystem();
	      }
	    }
		_Universe->SetActiveCockpit(curcockpit);
		mission=active_missions[0];
		processDelayedMissions();
	  }
#ifdef RUN_ONLY_FOR_PLAYER_STARSYSTEM
	}
#endif

	current_stage=PHY_COLLIDE;
      }else if (current_stage==PHY_COLLIDE) {
#ifdef NO_COLLISION_TIME
	if (no_collision_time) {
	  no_collision_time--;//don't resolve physics until 2 seconds
	}else 
#endif
	  {
#ifdef UPDATEDEBUG
  fprintf (stderr,"neb");
  fflush (stderr);
#endif
	  iter = drawList.createIterator();
	  while((unit = iter.current())!=NULL) {
		unit->SetNebula(NULL); 
		iter.advance();
	  }
	  iter = drawList.createIterator();
#ifdef UPDATEDEBUG
  fprintf (stderr,"Coll");
  fflush (stderr);
#endif
	  while((unit = iter.current())!=NULL) {
	    unit->CollideAll();
	    iter.advance();
	  }

	}
	UpdateMissiles();//do explosions
	current_stage=PHY_TERRAIN;
      } else if (current_stage==PHY_TERRAIN) {
#ifdef UPDATEDEBUG
  fprintf (stderr,"TerU");
  fflush (stderr);
#endif
	Terrain::UpdateAll(64);	
	unsigned int i=_Universe->CurrentCockpit();
	for (int j=0;j<_Universe->numPlayers();j++) {
	  if (_Universe->AccessCockpit(j)->activeStarSystem==this) {
	    _Universe->SetActiveCockpit(j);
	    _Universe->AccessCockpit(j)->Update();
	  }
	}
	_Universe->SetActiveCockpit(i);
	current_stage=PHY_RESOLV;
      } else if (current_stage==PHY_RESOLV) {
	iter = drawList.createIterator();
#ifdef UPDATEDEBUG
  fprintf (stderr,"muzak");
  fflush (stderr);
#endif
	if (this==_Universe->getActiveStarSystem(0)) {
	  _Universe->AccessCockpit(0)->AccessCamera()->UpdateCameraSounds();
	  if (muzak)
		  muzak->Listen();
	}
	if (_Universe->AccessCockpit()->activeStarSystem==this){
	  Nebula * neb;
	  if ((neb=_Universe->AccessCamera()->GetNebula())) {
	    if (neb->getFade()<=0) {
	      _Universe->AccessCamera()->SetNebula(NULL);//Update physics should set this
	    }
	  }
	}
#ifdef UPDATEDEBUG
  fprintf (stderr,"unphi");
  fflush (stderr);
#endif
      Unit * owner = _Universe->AccessCockpit()->GetParent();
      if (owner) {
	if (owner->InCorrectStarSystem(this)) {
	  if (getTimeCompression()>1) {//if not paused
	    if (!owner->AutoPilotTo (owner,true)) {
	      
	      reset_time_compression(0,PRESS);
	    }
	  }
	}
      }
      UpdateUnitPhysics(firstframe);
#ifdef UPDATEDEBUG
  fprintf (stderr,"boltphi");
  fflush (stderr);
#endif
	bolts->UpdatePhysics();
	for (unsigned int i=0;i<active_missions.size();i++) {
	  active_missions[i]->BriefingUpdate();//waste of farkin time
	}
	current_stage=PHY_AI;
	firstframe = false;
      }
      time -= (1./PHY_NUM)*SIMULATION_ATOM;
    }
  }
#ifdef UPDATEDEBUG
  fprintf (stderr,"endupd\n");
  fflush (stderr);
#endif
  UnitCollection::FreeUnusedNodes();
  collidetable->Update();
  SIMULATION_ATOM =  normal_simulation_atom;
  _Universe->popActiveStarSystem();
  //  fprintf (stderr,"bf:%lf",interpolation_blend_factor);
}


