#include "gfxlib.h"
#include "click_list.h"
#include "unit.h"
#include "vegastrike.h"
#include "vs_globals.h"
extern Vector mouseline;
extern vector<Vector> perplines;

Vector MouseCoordinate (int mouseX, int mouseY) {
  return GFXDeviceToEye(mouseX, mouseY);
}


bool ClickList::queryShip (int mouseX, int mouseY,Unit *ship) {   
  Vector mousePoint = MouseCoordinate (mouseX, mouseY);

    //mousePoint.k= -mousePoint.k;
  Vector CamP,CamQ,CamR;
  QVector CamPos;
  _Universe->AccessCamera()->GetPQR(CamP,CamQ,CamR);
  mousePoint = Transform (CamP,CamQ,CamR,mousePoint);	
  _Universe->AccessCamera()->GetPosition(CamPos);    
  //  if (ship->querySphere(CamP,mousePoint,0)){  //FIXME  bounding spheres seem to be broken
  mousePoint.Normalize();
  mouseline =mousePoint + CamPos.Cast();
  // 
  int tmp = ship->queryBoundingBox(CamPos,mousePoint,0);
  if (tmp)
    //fprintf (stderr, "bounding box hit\n");
  if (ship->querySphereClickList(CamPos,mousePoint.Cast(),0)){  // camera position is not actually the center of the camera
    //fprintf (stderr, "bounding sphere hit\n");
    
    if (tmp) return true;
  }
  return false;
}

ClickList::ClickList ( StarSystem *parSystem, UnitCollection *parIter) {
  lastSelected = NULL;
  lastCollection = NULL;
  parentSystem = parSystem;
  parentIter = parIter;
}

UnitCollection * ClickList::requestIterator (int minX,int minY, int maxX, int maxY) {
 
  UnitCollection * uc = new UnitCollection();///arrgh dumb last collection thing to cycel through ships
    if (minX==maxX||minY==maxY)
      return uc;//nothing in it
    UnitCollection::UnitIterator * UAye = new un_iter(uc->createIterator());
    UnitCollection::UnitIterator * myParent = new un_iter(parentIter->createIterator());
    Matrix view;
    float frustmat [16];
    float l, r, b, t , n, f;
    float drivel [16];
    GFXGetFrustumVars (true,&l,&r,&b,&t,&n,&f);
    GFXFrustum (frustmat, drivel,l*(-2.*minX/g_game.x_resolution+1) /*  *g_game.MouseSensitivityX*/,r*(2.*maxX/g_game.x_resolution-1)/*  *g_game.MouseSensitivityX*/,t*(-2.*minY/g_game.y_resolution+1) /*  *g_game.MouseSensitivityY*/,b*(2.*maxY/g_game.y_resolution-1)/*  *g_game.MouseSensitivityY*/,n,f);
    _Universe->AccessCamera()->GetView (view);
    float frustum [6][4];
    GFXCalculateFrustum(frustum,view,frustmat);
    Unit * un;
    while ((un=myParent->current())) {
      if ((un)->queryFrustum(frustum)) {
	UAye->preinsert (un);
      }
      myParent->advance(); 
    }
    delete myParent;
    delete UAye;
    return uc;
    
}

UnitCollection * ClickList::requestIterator (int mouseX, int mouseY) {
  perplines = vector<Vector>();
    UnitCollection * uc = new UnitCollection ();
    UnitCollection::UnitIterator * UAye = new un_iter (uc->createIterator());
    UnitCollection::UnitIterator * myParent = new un_iter (parentIter->createIterator());
    Unit * un;
    while ((un = myParent->current())) {
	if (queryShip(mouseX,mouseY,un))
	  UAye->preinsert (un);
	myParent->advance();
    }
    delete myParent;
    delete UAye;
    return uc;
}


Unit * ClickList::requestShip (int mouseX, int mouseY) {
  bool equalCheck=false;
  UnitCollection *uc = requestIterator (mouseX,mouseY);
  UnitCollection::UnitIterator * UAye = NULL;
  Unit *un;
  if (lastCollection!=NULL) {
    equalCheck=true;
    UAye=new un_iter(uc->createIterator());
    UnitCollection::UnitIterator *lastiter = new un_iter(lastCollection->createIterator());
    Unit *lastun;
    while (equalCheck&& (un = UAye->current())&&(lastun=lastiter->current())) {
      if (un !=lastun) {
	equalCheck=false;
      }
      UAye->advance();
      lastiter->advance();
    }    
    delete lastiter;
    delete lastCollection;
    delete UAye;
  }
  float minDistance=1e+10;
  float tmpdis;
  Unit * targetUnit=NULL;
  if (equalCheck&&lastSelected) {//the person clicked the same place and wishes to cycle through units from front to back
    float morethan = lastSelected->getMinDis(_Universe->AccessCamera()->GetPosition());//parent system for access cam
    UAye = new un_iter (uc->createIterator());
    while ((un=UAye->current())) {
      tmpdis = un->getMinDis (_Universe->AccessCamera()->GetPosition());//parent_system? FIXME (for access cam
      if (tmpdis>morethan&&tmpdis<minDistance) {
	minDistance=tmpdis;
	targetUnit=un;
      }
      UAye->advance();
    }
    delete UAye;
  }
  if (targetUnit==NULL) {//ok the click location is either different, or 
    //he clicked on the back of the list and wishes to start over
    UAye = new un_iter (uc->createIterator());
    while ((un=UAye->current())) {
      tmpdis = un->getMinDis (_Universe->AccessCamera()->GetPosition());//parent_system FIXME
      if (tmpdis<minDistance) {
	minDistance=tmpdis;
	targetUnit=un;
      }
      UAye->advance();
    }
   
    delete UAye;  
  }
  lastCollection = uc;
  lastSelected = targetUnit;
  return targetUnit;
}
