#include "terrain.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "xml_support.h"
#include "star_system.h"
#include "unit.h"
#include "gfx/vec.h"
#include "vegastrike.h"
#include "universe.h"
#include <vector>
#include "iterator.h"
#include "collection.h"
#include "building.h"
static std::vector <Terrain *> allterrains;





Terrain::Terrain (const char * filename, const Vector & scales, const float mass, const float radius,updateparity *updatetransform):QuadTree (filename, scales,radius),  TotalSizeX(0),TotalSizeZ(0), mass(mass), whichstage (0) {

  this->updatetransform = updatetransform;
  allterrains.push_back (this);
  draw=true;
  //  this->mass =  XMLSupport::parse_float (vs_config->getVariable ("terrain","mass","1000"));
}

Terrain::~Terrain() {
  for (unsigned int i=0;i<allterrains.size();i++) {
    if (allterrains[i]==this) {
      allterrains.erase (allterrains.begin()+i);
      break;
    }
  }
}

void Terrain::SetTransformation(Matrix Mat ) {
  QuadTree::SetTransformation (Mat);
}

void Terrain::ApplyForce (Unit * un, const Vector & normal, float dist) {
  //  fprintf (stderr,"Unit %s has collided at <%f %f %f>", un->name.c_str(),vec.i,vec.j,vec.k);
  un->ApplyForce (normal*.4*un->GetMass()*fabs(normal.Dot ((un->GetVelocity()/SIMULATION_ATOM))+fabs (dist)/(SIMULATION_ATOM)));
  un->ApplyDamage (un->Position()-normal*un->rSize(),-normal,  .5*fabs(normal.Dot(un->GetVelocity()))*mass*SIMULATION_ATOM,GFXColor(1,1,1,1));
}

void Terrain::Collide (Unit *un) {
  Vector norm;
  if (un->isUnit()==BUILDINGPTR) {
    return;
  }
  float dist =GetHeight (un->Position(),norm,TotalSizeX,TotalSizeZ)-un->rSize();
  if (dist < 0) {
    ApplyForce (un,norm,-dist);
  } 
}
void Terrain::DisableDraw() {
  draw=false;
}
void Terrain::EnableDraw() {
  draw=true;
}
void Terrain::Collide () {
  Iterator *iter;
  iter = _Universe->activeStarSystem()->getUnitList()->createIterator();
  Unit *unit;
  while((unit = iter->current())!=NULL) {
    Collide (unit);
    iter->advance();
  }
  delete iter;
}
static GFXColor getTerrainColor() {
  float col[4];
  vs_config->gethColor ("terrain", "terrain_ambient",col,0x000000ff);
  return GFXColor (col[0],col[1],col[2],col[3]);
}
void Terrain::CollideAll () {

  for (unsigned int i=0;i<allterrains.size();i++) {
    allterrains[i]->Collide();
  }
}
void Terrain::DeleteAll () {
  while (!allterrains.empty()) {
    delete allterrains.front();
  }
}

void Terrain::RenderAll () {
  static GFXColor terraincolor (getTerrainColor ());
  GFXColor tmpcol (0,0,0,1);
  GFXGetLightContextAmbient(tmpcol);
  GFXLightContextAmbient(terraincolor);  
  for (unsigned int i=0;i<allterrains.size();i++) {
    if (allterrains[i]->draw) {
      allterrains[i]->Render();
    }
  }
  GFXLightContextAmbient(tmpcol);  
}
void Terrain::UpdateAll (int resolution ) {
  int res = 4;
  if (resolution==0) {
    res=0; 
  } else {
    while (resolution > res) {
    res *=4;
    }
  }
  for (unsigned int i=0;i<allterrains.size();i++) {
    allterrains[i]->Update (res,allterrains[i]->whichstage%res,allterrains[i]->updatetransform);
    allterrains[i]->whichstage++;
  }
  

}
