
#include "vs_path.h"
#include "vs_globals.h"
#include "vegastrike.h"
#include "gauge.h"
#include "cockpit.h"
#include "universe.h"
#include "star_system.h"
#include "cmd/unit.h"
#include "cmd/unit_factory.h"
#include "cmd/iterator.h"
#include "cmd/collection.h"
#include "hud.h"
#include "vdu.h"
#include "lin_time.h"//for fps
#include "config_xml.h"
#include "lin_time.h"
#include "cmd/images.h"
#include "cmd/script/mission.h"
#include "cmd/script/msgcenter.h"
#include "cmd/ai/flyjoystick.h"
#include "cmd/ai/firekeyboard.h"
#include "cmd/ai/aggressive.h"
#include "main_loop.h"
#include <assert.h>	// needed for assert() calls
#include "savegame.h"
#include "animation.h"
#include "mesh.h"
#include "universe_util.h"
#include "in_mouse.h"
#include "gui/glut_support.h"
#include "networking/netclient.h"
extern float rand01();
#define SWITCH_CONST .9

void DrawRadarCircles (float x, float y, float wid, float hei, const GFXColor &col) {
	GFXColorf(col);
	GFXEnable(SMOOTH);
	GFXCircle (x,y,wid/2,hei/2);
	GFXCircle (x,y,wid/2.4,hei/2.4);
	GFXCircle (x,y,wid/6,hei/6);
	const float sqrt2=sqrt( (double)2)/2;
	GFXBegin(GFXLINE);
		GFXVertex3f(x+(wid/6)*sqrt2,y+(hei/6)*sqrt2,0);
		GFXVertex3f(x+(wid/2.4)*sqrt2,y+(hei/2.4)*sqrt2,0);
		GFXVertex3f(x-(wid/6)*sqrt2,y+(hei/6)*sqrt2,0);
		GFXVertex3f(x-(wid/2.4)*sqrt2,y+(hei/2.4)*sqrt2,0);
		GFXVertex3f(x-(wid/6)*sqrt2,y-(hei/6)*sqrt2,0);
		GFXVertex3f(x-(wid/2.4)*sqrt2,y-(hei/2.4)*sqrt2,0);
		GFXVertex3f(x+(wid/6)*sqrt2,y-(hei/6)*sqrt2,0);
		GFXVertex3f(x+(wid/2.4)*sqrt2,y-(hei/2.4)*sqrt2,0);
	GFXEnd();
	GFXDisable(SMOOTH);
}
 void GameCockpit::LocalToRadar (const Vector & pos, float &s, float &t) {
  s = (pos.k>0?pos.k:0)+1;
  t = 2*sqrtf(pos.i*pos.i + pos.j*pos.j + s*s);
  s = -pos.i/t;
  t = pos.j/t;
}

void GameCockpit::LocalToEliteRadar (const Vector & pos, float &s, float &t,float &h){
  s=-pos.i/1000.0;
  t=pos.k/1000.0;
  h=pos.j/1000.0;
}


GFXColor GameCockpit::unitToColor (Unit *un,Unit *target) {
 	if(target->isUnit()==PLANETPTR){
	  // this is a planet
	  return planet;
	}
	else if(target==un->Target()&&draw_all_boxes){//if we only draw target box we cannot tell what faction enemy is!
	  // my target
	  return targeted;
	}
	else if(target->Target()==un){
	  // the other ships is targetting me
	  return targetting;
	}

	// other spaceships
	static bool reltocolor=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","DrawTheirRelationColor","true"));
	if (reltocolor) {
	  return relationToColor(target->getRelation(un));
	}else {
	  return relationToColor(un->getRelation(target));
	}
}

GFXColor GameCockpit::relationToColor (float relation) {
 if (relation>0) {
    return GFXColor (relation*friendly.r+(1-relation)*neutral.r,relation*friendly.g+(1-relation)*neutral.g,relation*friendly.b+(1-relation)*neutral.b,relation*friendly.a+(1-relation)*neutral.a);
  } 
 else if(relation==0){
   return GFXColor(neutral.r,neutral.g,neutral.b,neutral.a);
}else { 
    return GFXColor (-relation*enemy.r+(1+relation)*neutral.r,-relation*enemy.g+(1+relation)*neutral.g,-relation*enemy.b+(1+relation)*neutral.b,-relation*enemy.a+(1+relation)*neutral.a);
  }
}
void GameCockpit::DrawNavigationSymbol (const Vector &Loc, const Vector & P, const Vector & Q, float size) {
  GFXColor4f (1,1,1,1);
  static bool draw_nav_symbol=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawNavSymbol","false"));
  if (draw_nav_symbol) {
    size = .125*GFXGetZPerspective (size);
    GFXBegin (GFXLINE);
    GFXVertexf(Loc+P*size);
    GFXVertexf(Loc+.125*P*size);
    GFXVertexf(Loc-P*size);
    GFXVertexf(Loc-.125*P*size);
    GFXVertexf(Loc+Q*size);
    GFXVertexf(Loc+.125*Q*size);
    GFXVertexf(Loc-Q*size);
    GFXVertexf(Loc-.125*Q*size);
    GFXVertexf(Loc+.0625*Q*size);
    GFXVertexf(Loc+.0625*P*size);
    GFXVertexf(Loc-.0625*Q*size);
    GFXVertexf(Loc-.0625*P*size);
    GFXVertexf(Loc+.9*P*size+.125*Q*size);
    GFXVertexf(Loc+.9*P*size-.125*Q*size);
    GFXVertexf(Loc-.9*P*size+.125*Q*size);
    GFXVertexf(Loc-.9*P*size-.125*Q*size);
    GFXVertexf(Loc+.9*Q*size+.125*P*size);
    GFXVertexf(Loc+.9*Q*size-.125*P*size);
    GFXVertexf(Loc-.9*Q*size+.125*P*size);
    GFXVertexf(Loc-.9*Q*size-.125*P*size);
    
    GFXEnd();
  }
}

float GameCockpit::computeLockingSymbol(Unit * par) {
  return par->computeLockingPercent();
}
inline void DrawOneTargetBox (const QVector & Loc, const float rSize, const Vector &CamP, const Vector & CamQ, const Vector & CamR, float lock_percent, bool ComputerLockon, bool Diamond=false) {
  if (Diamond) {
    float ModrSize=rSize/1.41;
    GFXBegin (GFXLINESTRIP); 
    GFXVertexf (Loc+(.75*CamP+CamQ).Cast()*ModrSize);
    GFXVertexf (Loc+(CamP+.75*CamQ).Cast()*ModrSize);
    GFXVertexf (Loc+(CamP-.75*CamQ).Cast()*ModrSize);
    GFXVertexf (Loc+(.75*CamP-CamQ).Cast()*ModrSize);
    GFXVertexf (Loc+(-.75*CamP-CamQ).Cast()*ModrSize);
    GFXVertexf (Loc+(-CamP-.75*CamQ).Cast()*ModrSize);
    GFXVertexf (Loc+(.75*CamQ-CamP).Cast()*ModrSize);
    GFXVertexf (Loc+(CamQ-.75*CamP).Cast()*ModrSize);
    GFXVertexf (Loc+(.75*CamP+CamQ).Cast()*ModrSize);
    GFXEnd();    
  }else if (ComputerLockon) {
    GFXBegin (GFXLINESTRIP); 
    GFXVertexf (Loc+(CamP+CamQ).Cast()*rSize);
    GFXVertexf (Loc+(CamP-CamQ).Cast()*rSize);
    GFXVertexf (Loc+(-CamP-CamQ).Cast()*rSize);
    GFXVertexf (Loc+(CamQ-CamP).Cast()*rSize);
    GFXVertexf (Loc+(CamP+CamQ).Cast()*rSize);
    GFXEnd();
  }else {
    GFXBegin(GFXLINE);
    GFXVertexf (Loc+(CamP+CamQ).Cast()*rSize);
    GFXVertexf (Loc+(CamP+.66*CamQ).Cast()*rSize);

    GFXVertexf (Loc+(CamP-CamQ).Cast()*rSize);
    GFXVertexf (Loc+(CamP-.66*CamQ).Cast()*rSize);

    GFXVertexf (Loc+(-CamP-CamQ).Cast()*rSize);
    GFXVertexf (Loc+(-CamP-.66*CamQ).Cast()*rSize);


    GFXVertexf (Loc+(CamQ-CamP).Cast()*rSize);
    GFXVertexf (Loc+(CamQ-.66*CamP).Cast()*rSize);


    GFXVertexf (Loc+(CamP+CamQ).Cast()*rSize);
    GFXVertexf (Loc+(CamP+.66*CamQ).Cast()*rSize);


    GFXVertexf (Loc+(CamP+CamQ).Cast()*rSize);
    GFXVertexf (Loc+(.66*CamP+CamQ).Cast()*rSize);


    GFXVertexf (Loc+(CamP-CamQ).Cast()*rSize);
    GFXVertexf (Loc+(.66*CamP-CamQ).Cast()*rSize);

    GFXVertexf (Loc+(-CamP-CamQ).Cast()*rSize);
    GFXVertexf (Loc+(-.66*CamP-CamQ).Cast()*rSize);

    GFXVertexf (Loc+(CamQ-CamP).Cast()*rSize);
    GFXVertexf (Loc+(.66*CamQ-CamP).Cast()*rSize);

    GFXVertexf (Loc+(CamP+CamQ).Cast()*rSize);
    GFXVertexf (Loc+(.66*CamP+CamQ).Cast()*rSize);

    GFXEnd();
  }
  if (lock_percent<.99) {
    if (lock_percent<0) {
      lock_percent=0;
    }
    float max=2.05;
    //    fprintf (stderr,"lock percent %f\n",lock_percent);
    float coord = 1.05+(max-1.05)*lock_percent;//rSize/(1-lock_percent);//this is a number between 1 and 100
   
    double rtot = 1./sqrtf(2);
    float theta = 4*M_PI*lock_percent;
    Vector LockBox (-cos(theta)*rtot,-rtot,sin(theta)*rtot);
    //    glLineWidth (4);
    Vector TLockBox (rtot*LockBox.i+rtot*LockBox.j,rtot*LockBox.j-rtot*LockBox.i,LockBox.k);
    Vector SLockBox (TLockBox.j,TLockBox.i,TLockBox.k);
    QVector Origin = (CamP+CamQ).Cast()*(rSize*coord);
    TLockBox = (TLockBox.i*CamP+TLockBox.j*CamQ+TLockBox.k*CamR);
    SLockBox = (SLockBox.i*CamP+SLockBox.j*CamQ+SLockBox.k*CamR);
    double r1Size = rSize*.58;
    GFXBegin (GFXLINESTRIP);
    max*=rSize*.75;
    if (lock_percent==0) {
      GFXVertexf (Loc+CamQ.Cast()*max*1.5);
      GFXVertexf (Loc+CamQ.Cast()*max);
    }

    GFXVertexf (Loc+Origin+(TLockBox.Cast()*r1Size));
    GFXVertexf (Loc+Origin);
    GFXVertexf (Loc+Origin+(SLockBox.Cast()*r1Size));
    if (lock_percent==0) {
      GFXVertexf (Loc+CamP.Cast()*max);
      GFXVertexf (Loc+CamP.Cast()*max*1.5);
      GFXEnd();
      GFXBegin(GFXLINESTRIP);
      GFXVertexf (Loc-CamP.Cast()*max);
    }else {
      GFXEnd();
      GFXBegin(GFXLINESTRIP);
    }
    GFXVertexf (Loc-Origin-(SLockBox.Cast()*r1Size));
    GFXVertexf (Loc-Origin);
    GFXVertexf (Loc-Origin-(TLockBox.Cast()*r1Size));

    Origin=(CamP-CamQ).Cast()*(rSize*coord);
    if (lock_percent==0) {
      GFXVertexf (Loc-CamQ.Cast()*max);
      GFXVertexf (Loc-CamQ.Cast()*max*1.5);
      GFXVertexf (Loc-CamQ.Cast()*max);
    }else {
      GFXEnd();
      GFXBegin(GFXLINESTRIP);
    }

    GFXVertexf (Loc+Origin+(TLockBox.Cast()*r1Size));
    GFXVertexf (Loc+Origin);
    GFXVertexf (Loc+Origin-(SLockBox.Cast()*r1Size));
    if (lock_percent==0) {
      GFXVertexf (Loc+CamP.Cast()*max);
      GFXEnd();
      GFXBegin(GFXLINESTRIP);
      GFXVertexf (Loc-CamP.Cast()*max*1.5);
      GFXVertexf (Loc-CamP.Cast()*max);
    }else {
      GFXEnd();
      GFXBegin(GFXLINESTRIP);
    }

    GFXVertexf (Loc-Origin+(SLockBox.Cast()*r1Size));
    GFXVertexf (Loc-Origin);
    GFXVertexf (Loc-Origin-(TLockBox.Cast()*r1Size));

    if (lock_percent==0) {
      GFXVertexf (Loc+CamQ.Cast()*max);
    }
    GFXEnd();
    glLineWidth (1);
  }

}

static GFXColor DockBoxColor (const string& name) {
  GFXColor dockbox;
  vs_config->getColor(name,&dockbox.r);    
  return dockbox;
}
inline void DrawDockingBoxes(Unit * un,Unit *target, const Vector & CamP, const Vector & CamQ, const Vector & CamR) {
  if (target->IsCleared (un)) {
    static GFXColor dockboxstop = DockBoxColor("docking_box_halt");
    static GFXColor dockboxgo = DockBoxColor("docking_box_proceed");
    if (dockboxstop.r==1&&dockboxstop.g==1&&dockboxstop.b==1) {
      dockboxstop.r=1;
      dockboxstop.g=0;
      dockboxstop.b=0;
    }
    if (dockboxgo.r==1&&dockboxgo.g==1&&dockboxgo.b==1) {
      dockboxgo.r=0;
      dockboxgo.g=1;
      dockboxgo.b=.5;
    }

    const vector <DockingPorts> d = target->DockingPortLocations();
    for (unsigned int i=0;i<d.size();i++) {
      float rad = d[i].radius/sqrt(2.0);
      GFXDisable (DEPTHTEST);
      GFXDisable (DEPTHWRITE);
      GFXColorf (dockboxstop);
      DrawOneTargetBox (Transform (target->GetTransformation(),d[i].pos.Cast())-_Universe->AccessCamera()->GetPosition(),rad ,CamP, CamQ, CamR,1,true,true);
      GFXEnable (DEPTHTEST);
      GFXEnable (DEPTHWRITE);
      GFXColorf (dockboxgo);
      DrawOneTargetBox (Transform (target->GetTransformation(),d[i].pos.Cast())-_Universe->AccessCamera()->GetPosition(),rad ,CamP, CamQ, CamR,1,true,true);

    }
    GFXDisable (DEPTHTEST);
    GFXDisable (DEPTHWRITE);
    GFXColor4f(1,1,1,1);

  }
}

void GameCockpit::DrawTargetBoxes(){
  
  Unit * un = parent.GetUnit();
  if (!un)
    return;
  if (un->GetNebula()!=NULL)
    return;

  StarSystem *ssystem=_Universe->activeStarSystem();
  UnitCollection *unitlist=&ssystem->getUnitList();
  //UnitCollection::UnitIterator *uiter=unitlist->createIterator();
  un_iter uiter=unitlist->createIterator();
  
  Vector CamP,CamQ,CamR;
  _Universe->AccessCamera()->GetPQR(CamP,CamQ,CamR);
 
  GFXDisable (TEXTURE0);
  GFXDisable (TEXTURE1);
  GFXDisable (DEPTHTEST);
  GFXDisable (DEPTHWRITE);
  GFXBlendMode (SRCALPHA,INVSRCALPHA);
  GFXDisable (LIGHTING);

  Unit *target=uiter.current();
  while(target!=NULL){
    if(target!=un){
        QVector Loc(target->Position());

	GFXColor drawcolor=unitToColor(un,target);
	GFXColorf(drawcolor);

	if(target->isUnit()==UNITPTR){

	  if (un->Target()==target) {
	    DrawDockingBoxes(un,target,CamP,CamQ, CamR);
	    DrawOneTargetBox (Loc, target->rSize(), CamP, CamQ, CamR,computeLockingSymbol(un),true);
	  }else {
	    DrawOneTargetBox (Loc, target->rSize(), CamP, CamQ, CamR,computeLockingSymbol(un),false);
	  }
	}
    }
    target=(++uiter);
  }

  GFXEnable (TEXTURE0);

}


void GameCockpit::DrawTargetBox () {
  float speed,range;
  static GFXColor black_and_white=DockBoxColor ("black_and_white"); 
  Unit * un = parent.GetUnit();
  if (!un)
    return;
  if (un->GetNebula()!=NULL)
    return;
  Unit *target = un->Target();
  if (!target)
    return;
  Vector CamP,CamQ,CamR;
  _Universe->AccessCamera()->GetPQR(CamP,CamQ,CamR);
  //Vector Loc (un->ToLocalCoordinates(target->Position()-un->Position()));
  QVector Loc(target->Position()-_Universe->AccessCamera()->GetPosition());
  GFXDisable (TEXTURE0);
  GFXDisable (TEXTURE1);
  GFXDisable (DEPTHTEST);
  GFXDisable (DEPTHWRITE);
  GFXBlendMode (SRCALPHA,INVSRCALPHA);
  GFXDisable (LIGHTING);
  DrawNavigationSymbol (un->GetComputerData().NavPoint,CamP,CamQ, CamR.Cast().Dot((un->GetComputerData().NavPoint).Cast()-_Universe->AccessCamera()->GetPosition()));
  GFXColorf (un->GetComputerData().radar.color?unitToColor(un,target):black_and_white);

  if(draw_line_to_target){
    QVector my_loc(_Universe->AccessCamera()->GetPosition());
    GFXBegin(GFXLINESTRIP);
    GFXVertexf(my_loc);
    GFXVertexf(Loc);
    
    Unit *targets_target=target->Target();
    if(draw_line_to_targets_target && targets_target!=NULL){
      QVector ttLoc(targets_target->Position());
      GFXVertexf(ttLoc);
    }
    GFXEnd();
  }
  DrawOneTargetBox (Loc, target->rSize(), CamP, CamQ, CamR,computeLockingSymbol(un),un->TargetLocked());
  DrawDockingBoxes(un,target,CamP,CamQ,CamR);
  if (always_itts || un->GetComputerData().itts) {
    un->getAverageGunSpeed (speed,range);
    float err = (.01*(1-un->CloakVisible()));
   QVector iLoc = target->PositionITTS (un->Position(),speed)-_Universe->AccessCamera()->GetPosition()+10*err*QVector (-.5*.25*un->rSize()+rand()*.25*un->rSize()/RAND_MAX,-.5*.25*un->rSize()+rand()*.25*un->rSize()/RAND_MAX,-.5*.25*un->rSize()+rand()*.25*un->rSize()/RAND_MAX);
    
    GFXBegin (GFXLINESTRIP);
    if(draw_line_to_itts){
      GFXVertexf(Loc);
      GFXVertexf(iLoc);
    }
    GFXVertexf (iLoc+(CamP.Cast())*.25*un->rSize());
    GFXVertexf (iLoc+(-CamQ.Cast())*.25*un->rSize());
    GFXVertexf (iLoc+(-CamP.Cast())*.25*un->rSize());
    GFXVertexf (iLoc+(CamQ.Cast())*.25*un->rSize());
    GFXVertexf (iLoc+(CamP.Cast())*.25*un->rSize());
    GFXEnd();
  }
  GFXEnable (TEXTURE0);
  GFXEnable (DEPTHTEST);
  GFXEnable (DEPTHWRITE);

}

void GameCockpit::Eject() {
  ejecting=true;
}
void GameCockpit::DrawBlips (Unit * un) {
  static GFXColor black_and_white=DockBoxColor ("black_and_white"); 

  Unit::Computer::RADARLIM * radarl = &un->GetComputerData().radar;
  UnitCollection * drawlist = &_Universe->activeStarSystem()->getUnitList();
  un_iter iter = drawlist->createIterator();
  Unit * target;
  Unit * makeBigger = un->Target();
  float s,t;
  float xsize,ysize,xcent,ycent;
  Radar->GetSize (xsize,ysize);
  xsize = fabs (xsize);
  ysize = fabs (ysize);
  Radar->GetPosition (xcent,ycent);
  GFXDisable (TEXTURE0);
  GFXDisable (LIGHTING);
  if ((!g_game.use_sprites)||Radar->LoadSuccess()) {
    DrawRadarCircles (xcent,ycent,xsize,ysize,textcol);
  }
  GFXPointSize (2);
  GFXBegin(GFXPOINT);
  while ((target = iter.current())!=NULL) {
    if (target!=un) {
      double dist;
      if (!un->InRange (target,dist,makeBigger==target,true,true)) {
	if (makeBigger==target) {
	  un->Target(NULL);
	}
	iter.advance();	
	continue;
      }
      Vector localcoord (un->LocalCoordinates(target));
      LocalToRadar (localcoord,s,t);
      GFXColor localcol (radarl->color?unitToColor (un,target):black_and_white);
      
      GFXColorf (localcol);
      
      float rerror = ((un->GetNebula()!=NULL)?.03:0)+(target->GetNebula()!=NULL?.06:0);
      Vector v(xcent+xsize*(s-.5*rerror+(rerror*rand())/RAND_MAX),ycent+ysize*(t+-.5*rerror+(rerror*rand())/RAND_MAX),0);
      if (target!=makeBigger) {
	GFXVertexf(v);
      }else {
	GFXEnd();
	GFXBegin(GFXLINE);
	GFXVertex3d(v.i+(7.8)/g_game.x_resolution,v.j,v.k); //I need to tell it to use floats...
	GFXVertex3d(v.i-(7.5)/g_game.x_resolution,v.j,v.k); //otherwise, it gives an error about
	GFXVertex3d(v.i,v.j-(7.5)/g_game.y_resolution,v.k); //not knowning whether to use floats
	GFXVertex3d(v.i,v.j+(7.8)/g_game.y_resolution,v.k); //or doubles.
	GFXEnd();
	GFXBegin (GFXPOINT);
      }      
    }
    iter.advance();
  }
  GFXEnd();
  GFXPointSize (1);
  GFXColor4f (1,1,1,1);
  GFXEnable (TEXTURE0);
}

void GameCockpit::DrawEliteBlips (Unit * un) {
  static GFXColor black_and_white=DockBoxColor ("black_and_white"); 
  Unit::Computer::RADARLIM * radarl = &un->GetComputerData().radar;
  UnitCollection * drawlist = &_Universe->activeStarSystem()->getUnitList();
  un_iter iter = drawlist->createIterator();
  Unit * target;
  Unit * makeBigger = un->Target();
  float s,t,es,et,eh;
  float xsize,ysize,xcent,ycent;
  Radar->GetSize (xsize,ysize);
  xsize = fabs (xsize);
  ysize = fabs (ysize);
  Radar->GetPosition (xcent,ycent);
  GFXDisable (TEXTURE0);
  GFXDisable (LIGHTING);
  if (Radar->LoadSuccess()) {
    DrawRadarCircles (xcent,ycent,xsize,ysize,textcol);
  }
  while ((target = iter.current())!=NULL) {
    if (target!=un) {
      double mm;

      if (!un->InRange (target,mm,(makeBigger==target),true,true)) {
	if (makeBigger==target) {
	  un->Target(NULL);
	}
	iter.advance();	
	continue;
      }
      Vector localcoord (un->LocalCoordinates(target));

	LocalToRadar (localcoord,s,t);
	LocalToEliteRadar(localcoord,es,et,eh);


      GFXColor localcol (radarl->color?unitToColor (un,target):black_and_white);
      GFXColorf (localcol);
      int headsize=4;
#if 1
      if (target==makeBigger) {
	headsize=6;
	//cout << "localcoord" << localcoord << " s=" << s << " t=" << t << endl;
      }
#endif
      float xerr,yerr,y2,x2;
      float rerror = ((un->GetNebula()!=NULL)?.03:0)+(target->GetNebula()!=NULL?.06:0);
      xerr=xcent+xsize*(es-.5*rerror+(rerror*rand())/RAND_MAX);
      yerr=ycent+ysize*(et+-.5*rerror+(rerror*rand())/RAND_MAX);
      x2=xcent+xsize*((es+0)-.5*rerror+(rerror*rand())/RAND_MAX);
      y2=ycent+ysize*((et+t)-.5*rerror+(rerror*rand())/RAND_MAX);

      //      printf("xerr,yerr: %f %f xcent %f xsize %f\n",xerr,yerr,xcent,xsize);

      ///draw the foot
      GFXPointSize(2);
      GFXBegin(GFXPOINT);
      GFXVertex3f (xerr,yerr,0);
      GFXEnd();

      ///draw the leg
      GFXBegin(GFXLINESTRIP);
      GFXVertex3f(x2,yerr,0);
      GFXVertex3f(x2,y2,0);
      GFXEnd();

      ///draw the head
      GFXPointSize(headsize);
      GFXBegin(GFXPOINT);
      GFXVertex3f(xerr,y2,0);
      GFXEnd();

      GFXPointSize(1);
    }
    iter.advance();
  }
  GFXPointSize (1);
  GFXColor4f (1,1,1,1);
  GFXEnable (TEXTURE0);
}
float GameCockpit::LookupTargetStat (int stat, Unit *target) {
  static float game_speed = XMLSupport::parse_float (vs_config->getVariable("physics","game_speed","1"));
  static float fpsval=0;
  const float fpsmax=1;
  static float numtimes=fpsmax;
  unsigned short armordat[4];
  Unit * tmpunit;
  switch (stat) {
  case UnitImages::SHIELDF:
    return target->FShieldData();
  case UnitImages::SHIELDR:
    return target->RShieldData();
  case UnitImages::SHIELDL:
    return target->LShieldData();
  case UnitImages::SHIELDB:
    return target->BShieldData();
  case UnitImages::ARMORF:
  case UnitImages::ARMORR:
  case UnitImages::ARMORL:
  case UnitImages::ARMORB:
    target->ArmorData (armordat);
    if (armordat[stat-UnitImages::ARMORF]>StartArmor[stat-UnitImages::ARMORF]) {
      StartArmor[stat-UnitImages::ARMORF]=armordat[stat-UnitImages::ARMORF];
    }
    return ((float)armordat[stat-UnitImages::ARMORF])/StartArmor[stat-UnitImages::ARMORF];
  case UnitImages::FUEL:
    if (maxfuel>0) return target->FuelData()/maxfuel;return 0;
  case UnitImages::ENERGY:
    return target->EnergyData();
  case UnitImages::HULL:
    if (maxhull<target->GetHull()) {
      maxhull = target->GetHull();
    }
    return target->GetHull()/maxhull;
  case UnitImages::EJECT:
    return (((target->GetHull()/maxhull)<.25)&&(target->BShieldData()<.25||target->FShieldData()<.25))?1:0;
  case UnitImages::LOCK:
    if  ((tmpunit = target->GetComputerData().threat.GetUnit())) {
      return (tmpunit->cosAngleTo (target,*(float*)&armordat[0],FLT_MAX,FLT_MAX)>.95);
    }
    return 0;
  case UnitImages::KPS:
    return (target->GetVelocity().Magnitude())*10/game_speed;
  case UnitImages::SETKPS:
    return target->GetComputerData().set_speed*10/game_speed;
  case UnitImages::AUTOPILOT:
    if (target) {
      return (target->AutoPilotTo(target)?1:0);
    }
    return 0;
  case UnitImages::COCKPIT_FPS:
    if (fpsval>=0&&fpsval<.5*FLT_MAX)
      numtimes-=.1+fpsval;
    if (numtimes<=0) {
      numtimes = fpsmax;
      fpsval = GetElapsedTime();
    }
	if( fpsval)
   	 return 1./fpsval;
  case UnitImages::COCKPIT_LAG:
    if ( Network != NULL)
   	 return Network[0].getLag();
	else
	 return 0;
  }
  return 1;
}
void GameCockpit::DrawGauges(Unit * un) {

  int i;
  for (i=0;i<UnitImages::KPS;i++) {
    if (gauges[i]) {
      gauges[i]->Draw(LookupTargetStat (i,un));
/*      if (rand01()>un->GetImageInformation().cockpit_damage[0]) {
        static Animation gauge_ani("static.ani",true,.1,BILINEAR);
        gauge_ani.DrawAsSprite(Radar);
      }*/
      float damage = un->GetImageInformation().cockpit_damage[(1+MAXVDUS+i)%(MAXVDUS+1+UnitImages::NUMGAUGES)];
      if (gauge_time[i]>=0) {
        if (damage>.0001&&(cockpit_time>(gauge_time[i]+(1-damage)))) {
	  if (rand01()>SWITCH_CONST) {
            gauge_time[i]=-cockpit_time;
          }
        } else {
          static Animation vdu_ani("static.ani",true,.1,BILINEAR);
          vdu_ani.DrawAsSprite(gauges[i]);	
        }
      } else {
        if (cockpit_time>(((1-(-gauge_time[i]))+damage))) {
	      if (rand01()>SWITCH_CONST) {
            gauge_time[i]=cockpit_time;
          }
        }
      }
    }
  }
  if (!text)
	  return;
  text->SetSize (2,-2);
  GFXColorf (textcol);
  for (i=UnitImages::KPS;i<UnitImages::NUMGAUGES;i++) {
    if (gauges[i]) {
      float sx,sy,px,py;
      gauges[i]->GetSize (sx,sy);
      gauges[i]->GetPosition (px,py);
      text->SetCharSize (sx,sy);
      text->SetPos (px,py);
      int tmp = (int)LookupTargetStat (i,un);
      char ourchar[32];
      sprintf (ourchar,"%d", tmp);
      GFXColorf (textcol);
      text->Draw (string (ourchar));
    }
  }
  GFXColor4f (1,1,1,1);
}
void GameCockpit::Init (const char * file) {
  Cockpit::Init( file);
  if (Panel.size()>0) {
    float x,y;
    Panel.front()->GetPosition (x,y);
    Panel.front()->SetPosition (x,y+viewport_offset);  
  }
}
void GameCockpit::Delete () {
  int i;
  if (text) {
    delete text;
    text = NULL;
  }
  if (mesh) {
    delete mesh;
    mesh = NULL;
  }
  viewport_offset=cockpit_offset=0;
  for (i=0;i<4;i++) {
    if (Pit[i]) {
      delete Pit[i];
      Pit[i] = NULL;
    }
  }
  for (i=0;i<UnitImages::NUMGAUGES;i++) {
    if (gauges[i]) {
      delete gauges[i];
      gauges[i]=NULL;
    }
  }
  if (Radar) {
    delete Radar;
    Radar = NULL;
  }
  unsigned int j;
  for (j=0;j<vdu.size();j++) {
    if (vdu[j]) {
      delete vdu[j];
      vdu[j]=NULL;
    }
  }
  vdu.clear();
  for (j=0;j<Panel.size();j++) {
    assert (Panel[j]);
    delete Panel[j];
  }
  Panel.clear();
}
void GameCockpit::InitStatic () {  
  int i;
  for (i=0;i<UnitImages::NUMGAUGES;i++) {
    gauge_time[i]=0;
  }
  for (i=0;i<MAXVDUS;i++) {
    vdu_time[i]=0;
  }

  radar_time=0;
  cockpit_time=0;
}

/***** WARNING CHANGED ORDER *****/
GameCockpit::GameCockpit (const char * file, Unit * parent,const std::string &pilot_name): Cockpit( file, parent, pilot_name),textcol (1,1,1,1),text(NULL)
{
  static int headlag = XMLSupport::parse_int (vs_config->getVariable("graphics","head_lag","10"));
  int i;
  for (i=0;i<headlag;i++) {
    headtrans.push_back (Matrix());
    Identity(headtrans.back());
  }
  for (i=0;i<UnitImages::NUMGAUGES;i++) {
    gauges[i]=NULL;
  }
  mesh=NULL;
  Radar=Pit[0]=Pit[1]=Pit[2]=Pit[3]=NULL;

  draw_all_boxes=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawAllTargetBoxes","false"));
  draw_line_to_target=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawLineToTarget","false"));
  draw_line_to_targets_target=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawLineToTargetsTarget","false"));
  draw_line_to_itts=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawLineToITTS","false"));
  always_itts=XMLSupport::parse_bool(vs_config->getVariable("graphics","hud","drawAlwaysITTS","false"));
  radar_type=vs_config->getVariable("graphics","hud","radarType","WC");

  friendly=GFXColor(-1,-1,-1,-1);
  enemy=GFXColor(-1,-1,-1,-1);
  neutral=GFXColor(-1,-1,-1,-1);
  targeted=GFXColor(-1,-1,-1,-1);
  targetting=GFXColor(-1,-1,-1,-1);
  planet=GFXColor(-1,-1,-1,-1);
  if (friendly.r==-1) {
    vs_config->getColor ("enemy",&enemy.r);
    vs_config->getColor ("friend",&friendly.r);
    vs_config->getColor ("neutral",&neutral.r);
    vs_config->getColor("target",&targeted.r);
    vs_config->getColor("targetting_ship",&targetting.r);
    vs_config->getColor("planet",&planet.r);
  }
}
void GameCockpit::SelectProperCamera () {
    SelectCamera(view);
}
static vector <int> respawnunit;
static vector <int> switchunit;
static vector <int> turretcontrol;
static vector <int> suicide;
void RespawnNow (Cockpit * cp) {
  while (respawnunit.size()<=_Universe->numPlayers())
    respawnunit.push_back(0);
  for (unsigned int i=0;i<_Universe->numPlayers();i++) {
    if (_Universe->AccessCockpit(i)==cp) {
      respawnunit[i]=2;
    }
  }
}
void GameCockpit::SwitchControl (int,KBSTATE k) {
  if (k==PRESS) {
    while (switchunit.size()<=_Universe->CurrentCockpit())
      switchunit.push_back(0);
    switchunit[_Universe->CurrentCockpit()]=1;
  }

}
void SuicideKey (int, KBSTATE k) {
  if (k==PRESS) {
    while (suicide.size()<=_Universe->CurrentCockpit())
      suicide.push_back(0);
    suicide[_Universe->CurrentCockpit()]=1;
  }
  
}

class UnivMap {
  Sprite * ul;
  Sprite * ur;
  Sprite * ll;
  Sprite * lr;
public:
  UnivMap (Sprite * ull, Sprite *url, Sprite * lll, Sprite * lrl) {
    ul=ull;
    ur=url;
    ll=lll;
    lr = lrl;
  }
  void Draw() {
    if (ul||ur||ll||lr) {
      GFXBlendMode(SRCALPHA,INVSRCALPHA);
      GFXEnable(TEXTURE0);
      GFXDisable(TEXTURE1);
      GFXColor4f(1,1,1,1);
    }
    if (ul) 
      ul->Draw();
    if (ur)
      ur->Draw();
    if (ll)
      ll->Draw();
    if (lr)
      lr->Draw();
  }
  bool isNull() {
    return ul==NULL;
  }
};
std::vector <UnivMap> univmap;
void MapKey (int, KBSTATE k) {
  if (k==PRESS) {
    static Sprite ul("upper-left-map.spr");
    static Sprite ur("upper-right-map.spr");
    static Sprite ll("lower-left-map.spr");
    static Sprite lr("lower-right-map.spr");
    while (univmap.size()<=_Universe->CurrentCockpit())
      univmap.push_back(UnivMap(NULL,NULL,NULL,NULL));
    if (univmap[_Universe->CurrentCockpit()].isNull()) {
      univmap[_Universe->CurrentCockpit()]=UnivMap (&ul,&ur,&ll,&lr);
    }else {
      univmap[_Universe->CurrentCockpit()]=UnivMap(NULL,NULL,NULL,NULL);
    } 
  } 
}


void GameCockpit::TurretControl (int,KBSTATE k) {
  if (k==PRESS) {
    while (turretcontrol.size()<=_Universe->CurrentCockpit())
      turretcontrol.push_back(0);
    turretcontrol[_Universe->CurrentCockpit()]=1;
  }

}
void GameCockpit::Respawn (int,KBSTATE k) {
  if (k==PRESS) {
    while (respawnunit.size()<=_Universe->CurrentCockpit())
      respawnunit.push_back(0);
    respawnunit[_Universe->CurrentCockpit()]=1;
  }

}

static void FaceTarget (Unit * un) {
  Unit * targ = un->Target();
  if (targ) {
    QVector dir (targ->Position()-un->Position());
    dir.Normalize();
    Vector p,q,r;
    un->GetOrientation(p,q,r);
    QVector qq(q.Cast());
    qq = qq+QVector (.001,.001,.001);
    un->SetOrientation (qq,dir);
  }
}

// SAME AS IN COCKPIT BUT ADDS SETVIEW and ACCESSCAMERA -> ~ DUPLICATE CODE
int GameCockpit::Autopilot (Unit * target) {
  int retauto = 0;
  if (target) {
    Unit * un=NULL;
    if ((un=GetParent())) {
      if ((retauto = un->AutoPilotTo(un))) {//can he even start to autopilot
	SetView (CP_PAN);

	static bool face_target_on_auto = XMLSupport::parse_bool (vs_config->getVariable ( "physics","face_on_auto", "false"));
	if (face_target_on_auto) {
	  FaceTarget(un);
	}	  
	static double averagetime = GetElapsedTime()/getTimeCompression();
	static double numave = 1.0;
	averagetime+=GetElapsedTime()/getTimeCompression();
	static float autospeed = XMLSupport::parse_float (vs_config->getVariable ("physics","autospeed",".020"));//10 seconds for auto to kick in;
	numave++;
	AccessCamera(CP_PAN)->myPhysics.SetAngularVelocity(Vector(0,0,0));
	AccessCamera(CP_PAN)->myPhysics.ApplyBalancedLocalTorque(_Universe->AccessCamera()->P,
							      _Universe->AccessCamera()->R,
							      averagetime*autospeed/(numave));
	zoomfactor=1.5;
	static float autotime = XMLSupport::parse_float (vs_config->getVariable ("physics","autotime","10"));//10 seconds for auto to kick in;

	autopilot_time=autotime;
	autopilot_target.SetUnit (target);
      }
    }
  }
  return retauto;
}
void SwitchUnits (Unit * ol, Unit * nw) {
  bool pointingtool=false;
  bool pointingtonw=false;

  for (int i=0;i<_Universe->numPlayers();i++) {
    if (i!=(int)_Universe->CurrentCockpit()) {
      if (_Universe->AccessCockpit(i)->GetParent()==ol)
	pointingtool=true;
      if (_Universe->AccessCockpit(i)->GetParent()==nw)
	pointingtonw=true;
    }
  }

  if (ol&&(!pointingtool)) {
    Unit * oltarg = ol->Target();
    if (oltarg) {
      if (ol->getRelation (oltarg)>=0) {
	ol->Target(NULL);
      }
    }
    ol->PrimeOrders();
    ol->SetAI (new Orders::AggressiveAI ("default.agg.xml","default.int.xml"));
    ol->SetVisible (true);
  }
  if (nw) {
    nw->PrimeOrders();
    nw->EnqueueAI (new FireKeyboard (_Universe->CurrentCockpit(),_Universe->CurrentCockpit()));
    nw->EnqueueAI (new FlyByJoystick (_Universe->CurrentCockpit()));
    static bool LoadNewCockpit = XMLSupport::parse_bool (vs_config->getVariable("graphics","UnitSwitchCockpitChange","false"));
    if (nw->getCockpit().length()>0&&LoadNewCockpit) {
      _Universe->AccessCockpit()->Init (nw->getCockpit().c_str());
    }else {
      static bool DisCockpit = XMLSupport::parse_bool (vs_config->getVariable("graphics","SwitchCockpitToDefaultOnUnitSwitch","false"));
      if (DisCockpit) {
	_Universe->AccessCockpit()->Init ("disabled-cockpit.cpt");
      }
    }
  }
}
static void SwitchUnitsTurret (Unit *ol, Unit *nw) {
  static bool FlyStraightInTurret = XMLSupport::parse_bool (vs_config->getVariable("physics","ai_pilot_when_in_turret","true"));
  if (FlyStraightInTurret) {
    SwitchUnits (ol,nw);
  }else {
    ol->PrimeOrders();
    SwitchUnits (NULL,nw);
    
  }

}

extern void reset_time_compression(int i, KBSTATE a);
void GameCockpit::Shake (float amt) {
  static float shak= XMLSupport::parse_float(vs_config->getVariable("graphics","cockpit_shake","3"));
  static float shak_max= XMLSupport::parse_float(vs_config->getVariable("graphics","cockpit_shake_max","20"));
  shakin+=shak;
  if (shakin>shak_max) {
    shakin=shak_max;
  }
}

static void DrawCrosshairs (float x, float y, float wid, float hei, const GFXColor &col) {
	GFXColorf(col);
	GFXDisable(TEXTURE0);
	GFXDisable(LIGHTING);
	GFXBlendMode(SRCALPHA,INVSRCALPHA);
	GFXEnable(SMOOTH);
	GFXCircle(x,y,wid/4,hei/4);
	GFXCircle(x,y,wid/7,hei/7);
	GFXDisable(SMOOTH);
	GFXBegin(GFXLINE);
		GFXVertex3f(x-(wid/2),y,0);
		GFXVertex3f(x-(wid/6),y,0);
		GFXVertex3f(x+(wid/2),y,0);
		GFXVertex3f(x+(wid/6),y,0);
		GFXVertex3f(x,y-(hei/2),0);
		GFXVertex3f(x,y-(hei/6),0);
		GFXVertex3f(x,y+(hei/2),0);
		GFXVertex3f(x,y+(hei/6),0);
		GFXVertex3f(x-.001,y+.001,0);
		GFXVertex3f(x+.001,y-.001,0);
		GFXVertex3f(x+.001,y+.001,0);
		GFXVertex3f(x-.001,y-.001,0);
	GFXEnd();
	GFXEnable(TEXTURE0);
}

void GameCockpit::Draw() { 
  cockpit_time+=GetElapsedTime();
  if (cockpit_time>=100000)
    InitStatic();
  _Universe->AccessCamera()->UpdateGFX (GFXFALSE,GFXFALSE,GFXTRUE);
  GFXDisable (TEXTURE1);
  GFXLoadIdentity(MODEL);
  GFXDisable(LIGHTING);
  GFXDisable (DEPTHTEST);
  GFXDisable (DEPTHWRITE);
  GFXColor4f(1,1,1,1);
  DrawTargetBox();
  if(draw_all_boxes){
    DrawTargetBoxes();
  }
  if (view<CP_CHASE) {
    if (mesh) {
      Unit * par=GetParent();
      if (par) {
	//	Matrix mat;
      
	GFXLoadIdentity(MODEL);

	GFXDisable (DEPTHTEST);
	GFXDisable(DEPTHWRITE);
	GFXEnable (TEXTURE0);
	GFXEnable (LIGHTING);
	Vector P,Q,R;
	AccessCamera(CP_FRONT)->GetPQR (P,Q,R);

	headtrans.push_back (Matrix());
	VectorAndPositionToMatrix(headtrans.back(),P,Q,R,QVector(0,0,0));
	static float theta=0;
	static float shake_speed = XMLSupport::parse_float(vs_config->getVariable ("graphics","shake_speed","50"));
	theta+=shake_speed*GetElapsedTime();
	static float shake_reduction = XMLSupport::parse_float(vs_config->getVariable ("graphics","shake_reduction","8"));

	headtrans.front().p.i=shakin*cos(theta);//AccessCamera()->GetPosition().i+shakin*cos(theta);
	headtrans.front().p.j=shakin*cos(1.2*theta);//AccessCamera()->GetPosition().j+shakin*cos(theta);
	headtrans.front().p.k=0;//AccessCamera()->GetPosition().k;
	if (shakin>0) {
	  shakin-=GetElapsedTime()*shake_reduction;
	  if (shakin<=0) {
	    shakin=0;
	  }
	}

	mesh->DrawNow(1,true,headtrans.front());
	headtrans.pop_front();
	GFXDisable (LIGHTING);
	GFXDisable( TEXTURE0);
      }
    }
  }
  GFXHudMode (true);
  GFXColor4f (1,1,1,1);
  GFXBlendMode (ONE,ONE);
  GFXDisable (DEPTHTEST);
  GFXDisable (DEPTHWRITE);

  Unit * un;
  if (view==CP_FRONT) {
    if (Panel.size()>0) {
      static bool drawCrosshairs=parse_bool(vs_config->getVariable("graphics","draw_rendered_crosshairs","true"));
      if (drawCrosshairs) {
        float x,y,wid,hei;
        Panel.front()->GetSize(wid,hei);
        Panel.front()->GetPosition(x,y);
        DrawCrosshairs(x,y,wid,hei,textcol);
      } else {
        Panel.front()->Draw();//draw crosshairs
      }
    }
  }
  static bool mouseCursor = XMLSupport::parse_bool (vs_config->getVariable ("joystick","mouse_cursor","false"));
  if (mouseCursor) {  
    GFXBlendMode (SRCALPHA,INVSRCALPHA);
    GFXColor4f (1,1,1,1);
    
    //    GFXEnable(TEXTURE0);
    //    GFXDisable (DEPTHTEST);
    //    GFXDisable(TEXTURE1);
    static int revspr = XMLSupport::parse_bool (vs_config->getVariable ("joystick","reverse_mouse_spr","true"))?1:-1;
    static string blah = vs_config->getVariable("joystick","mouse_crosshair","crosshairs.spr");
    static Sprite MouseSprite (blah.c_str(),BILINEAR,GFXTRUE);
    MouseSprite.SetPosition (-1+float(mousex)/(.5*g_game.x_resolution),-revspr+float(revspr*mousey)/(.5*g_game.y_resolution));
    
    MouseSprite.Draw();
    //    DrawGlutMouse(mousex,mousey,&MouseSprite);
    //    DrawGlutMouse(mousex,mousey,&MouseSprite);
  }
  RestoreViewPort();
  GFXBlendMode (ONE,ZERO);
  GFXAlphaTest (GREATER,.1);
  GFXColor4f(1,1,1,1);
  if (view<CP_CHASE) {
    if (Pit[view]) 
      Pit[view]->Draw();
  }
  GFXAlphaTest (ALWAYS,0);
  GFXBlendMode (SRCALPHA,INVSRCALPHA);
  GFXColor4f(1,1,1,1);
	bool die=true;
  if ((un = parent.GetUnit())) {
    if (view==CP_FRONT) {//only draw crosshairs for front view
      if (Radar) {
	//Radar->Draw();
	if(radar_type=="Elite"){
	  DrawEliteBlips(un);
	}
	else{
	  DrawBlips(un);
	}
/*	if (rand01()>un->GetImageInformation().cockpit_damage[0]) {
		static Animation radar_ani("round_static.ani",true,.1,BILINEAR);
		radar_ani.DrawAsSprite(Radar);	
	}*/
	float damage =(un->GetImageInformation().cockpit_damage[0]);
      if (radar_time>=0) {
        if (damage>.001&&(cockpit_time>radar_time+(1-damage))) {
	  if (rand01()>SWITCH_CONST) {
            radar_time=-cockpit_time;
          }
        } else {
          static Animation radar_ani("static_round.ani",true,.1,BILINEAR);
          radar_ani.DrawAsSprite(Radar);	
        }
      } else {
        if (cockpit_time>((1-(-radar_time))+damage)) {
	  if (rand01()>SWITCH_CONST) {
            radar_time=cockpit_time;
          }
        }
      }
      }

      DrawGauges(un);
      
      GFXColor4f(1,1,1,1);
      for (unsigned int j=1;j<Panel.size();j++) {
	Panel[j]->Draw();
      }
      GFXColor4f(1,1,1,1);
      for (unsigned int vd=0;vd<vdu.size();vd++) {
	if (vdu[vd]) {
	  vdu[vd]->Draw(un,textcol);
	  GFXColor4f (1,1,1,1);
	  float damage = un->GetImageInformation().cockpit_damage[(1+vd)%(MAXVDUS+1)];
	  if (vdu_time[vd]>=0) {
	    if (damage>.001&&(cockpit_time>(vdu_time[vd]+(1-damage)))) {
	      if (rand01()>SWITCH_CONST) {
		vdu_time[vd]=-cockpit_time;
	      }
	    } else {
	      static Animation vdu_ani("static.ani",true,.1,BILINEAR);
	      GFXEnable(TEXTURE0);
	      vdu_ani.DrawAsSprite(vdu[vd]);	
	    }
	  } else {
	    if (cockpit_time>((1-(-vdu_time[vd]))+(damage))) {
	      if (rand01()>SWITCH_CONST) {
		vdu_time[vd]=cockpit_time;
	      }
	    }
	  }
	  //process VDU, damage VDU, targetting VDU
	}
      }

    }
    GFXColor4f (1,1,1,1);
    if (un->GetHull()>=0)
      die = false;
    if (un->Threat()!=NULL) {
      if (getTimeCompression()>1) {
	reset_time_compression(0,PRESS);
      }
      un->Threaten (NULL,0);
    }
    if (_Universe->CurrentCockpit()<univmap.size()) {
      univmap[_Universe->CurrentCockpit()].Draw();
    }
  }
  if (die) {
	if (un) {
		if (un->GetHull()>=0) {
			die=false;
		}
	}
	if (die) {
	static float dietime = 0;
	if (text) {
		GFXColor4f (1,1,1,1);
		text->SetSize(1,-1);
		float x; float y;
		if (dietime==0) {
		  if (respawnunit.size()>_Universe->CurrentCockpit()) 
		    if (respawnunit[_Universe->CurrentCockpit()]==1) {
		      respawnunit[_Universe->CurrentCockpit()]=0;
		    }
			text->GetCharSize (x,y);
			text->SetCharSize (x*4,y*4);
			text->SetPos (0-(x*2*14),0-(y*2));
			mission->msgcenter->add("game","all","You Have Died!");
		}
		GFXColorf (textcol);
		text->Draw ("You Have Died!");
		GFXColor4f (1,1,1,1);
	}
	dietime +=GetElapsedTime();
	SetView (CP_PAN);
	zoomfactor=dietime*10;
	}
  }
  GFXAlphaTest (ALWAYS,0);  
  GFXHudMode (false);
  GFXEnable (DEPTHWRITE);
  GFXEnable (DEPTHTEST);
}
int GameCockpit::getScrollOffset (unsigned int whichtype) {
  for (unsigned int i=0;i<vdu.size();i++) {
    if (vdu[i]->getMode()&whichtype) {
      return vdu[i]->scrolloffset;
    }
  }
  return 0;
}




Unit * GetFinalTurret(Unit * baseTurret) {
  Unit * un = baseTurret;
  un_iter uj= un->getSubUnits();
  Unit * tur;
  while ((tur=uj.current())) {
    SwitchUnits (NULL,tur);
    un = GetFinalTurret (tur);
    ++uj;
  }
  return un;
}

void GameCockpit::Update () {
  if (autopilot_time!=0) {
    autopilot_time-=SIMULATION_ATOM;
    if (autopilot_time<= 0) {
      AccessCamera(CP_PAN)->myPhysics.SetAngularVelocity(Vector(0,0,0));
      SetView(CP_FRONT);
      autopilot_time=0;
      Unit * par = GetParent();
      if (par) {
	Unit * autoun = autopilot_target.GetUnit();
	autopilot_target.SetUnit(NULL);
	if (autoun) {
	  par->AutoPilotTo(autoun);
	}
      }
    }
  }
  Unit * par=GetParent();
  if (!par) {
    if (respawnunit.size()>_Universe->CurrentCockpit())
      if (respawnunit[_Universe->CurrentCockpit()]){
	parentturret.SetUnit(NULL);
	zoomfactor=1.5;
	respawnunit[_Universe->CurrentCockpit()]=0;
	string newsystem = savegame->GetStarSystem()+".system";
	StarSystem * ss = _Universe->GenerateStarSystem (newsystem.c_str(),"",Vector(0,0,0));
	_Universe->getActiveStarSystem(0)->SwapOut();
	this->activeStarSystem=ss;
	_Universe->pushActiveStarSystem(ss);


	vector <StarSystem *> saved;
	while (_Universe->getNumActiveStarSystem()) {
	  saved.push_back (_Universe->activeStarSystem());
	  _Universe->popActiveStarSystem();
	}
	if (!saved.empty()) {
	  saved.back()=ss;
	}
	unsigned int mysize = saved.size();
	for (unsigned int i=0;i<mysize;i++) {
	  _Universe->pushActiveStarSystem (saved.back());
	  saved.pop_back();
	}
	ss->SwapIn();
	int fgsnumber = 0;
	if (fg) {
	  fgsnumber=fg->flightgroup_nr++;
	  fg->nr_ships++;
	  fg->nr_ships_left++;
	}
	Unit * un = GameUnitFactory::createUnit (unitfilename.c_str(),false,this->unitfaction,unitmodname,fg,fgsnumber);
	un->SetCurPosition (UniverseUtil::SafeEntrancePoint (savegame->GetPlayerLocation()));
	ss->AddUnit (un);

	this->SetParent(un,unitfilename.c_str(),unitmodname.c_str(),savegame->GetPlayerLocation());
	//un->SetAI(new FireKeyboard ())
	SwitchUnits (NULL,un);
	credits = savegame->GetSavedCredits();
	CockpitKeys::Pan(0,PRESS);
	CockpitKeys::Inside(0,PRESS);
	savegame->ReloadPickledData();
	_Universe->popActiveStarSystem();
      }
  }
  if (turretcontrol.size()>_Universe->CurrentCockpit())
  if (turretcontrol[_Universe->CurrentCockpit()]) {
    turretcontrol[_Universe->CurrentCockpit()]=0;
    Unit * par = GetParent();
    if (par) {
      static int index=0;
      int i=0;bool tmp=false;bool tmpgot=false;
      if (parentturret.GetUnit()==NULL) {
	tmpgot=true;
	un_iter ui= par->getSubUnits();
	Unit * un;
	while ((un=ui.current())) {
		if (_Universe->isPlayerStarship(un)){
			++ui;
			continue;
		}

	  if (i++==index) {
	    index++;
	    if (un->name.find ("accessory")==string::npos) {
	      tmp=true;
	      SwitchUnitsTurret(par,un);
	      parentturret.SetUnit(par);
	      Unit * finalunit = GetFinalTurret(un);
	      this->SetParent(finalunit,this->unitfilename.c_str(),this->unitmodname.c_str(),savegame->GetPlayerLocation());
	      break;
	    }
	  }
	  ++ui;
	}
      }
      if (tmp==false) {
	if (tmpgot) index=0;
	Unit * un = parentturret.GetUnit();
	if (un&&(!_Universe->isPlayerStarship(un))) {
	  
	  SetParent (un,unitfilename.c_str(),this->unitmodname.c_str(),savegame->GetPlayerLocation());
	  SwitchUnits (NULL,un);
	  parentturret.SetUnit(NULL);
	  un->SetTurretAI();
	}
      }
    }
  }
  if (switchunit.size()>_Universe->CurrentCockpit())
  if (switchunit[_Universe->CurrentCockpit()]) {
    parentturret.SetUnit(NULL);

    zoomfactor=1.5;
    static int index=0;
    switchunit[_Universe->CurrentCockpit()]=0;
    un_iter ui= _Universe->activeStarSystem()->getUnitList().createIterator();
    Unit * un;
    bool found=false;
    int i=0;
    while ((un=ui.current())) {
      if (un->faction==this->unitfaction) {
	
	if ((i++)>=index&&(!_Universe->isPlayerStarship(un))) {
	  found=true;
	  index++;
	  Unit * k=GetParent(); 
	  SwitchUnits (k,un);
	  this->SetParent(un,this->unitfilename.c_str(),this->unitmodname.c_str(),savegame->GetPlayerLocation());
	  //un->SetAI(new FireKeyboard ())
	  break;
	}
      }
      ++ui;
    }
    if (!found)
      index=0;
  }
  if (ejecting) {
    ejecting=false;
    Unit * un = GetParent();
    if (un) {
      un->EjectCargo((unsigned int)-1);
    }
  }else {
  if (suicide.size()>_Universe->CurrentCockpit()) {
    if (suicide[_Universe->CurrentCockpit()]) {
      Unit * un=NULL;
      if ((un = parent.GetUnit())) {
	unsigned short armor[4];
	un->ArmorData(armor);
	un->DealDamageToHull(Vector(0,0,.1),un->GetHull()+2+armor[0]);
      }
      suicide[_Universe->CurrentCockpit()]=0;
    }

  }
  }

}
GameCockpit::~GameCockpit () {
  Delete();
  delete savegame;
}

void GameCockpit::VDUSwitch (int vdunum) {
  if (vdunum<(int)vdu.size()) {
    if (vdu[vdunum]) {
      vdu[vdunum]->SwitchMode();
    }
  }
}
void GameCockpit::ScrollVDU (int vdunum, int howmuch) {
  if (vdunum<(int)vdu.size()) {
    if (vdu[vdunum]) {
      vdu[vdunum]->Scroll(howmuch);
    }
  }
}
void GameCockpit::ScrollAllVDU (int howmuch) {
  for (unsigned int i=0;i<vdu.size();i++) {
    ScrollVDU (i,howmuch);
  }
}


void GameCockpit::SetCommAnimation (Animation * ani) {
  for (unsigned int i=0;i<vdu.size();i++) {
    if (vdu[i]->SetCommAnimation (ani)) {
      break;
    }
  }
}
void GameCockpit::RestoreViewPort() {
  _Universe->AccessCamera()->RestoreViewPort(0,0);
}

static void ShoveCamBehindUnit (int cam, Unit * un, float zoomfactor) {
  QVector unpos = un->GetPlanetOrbit()?un->LocalPosition():un->Position();
  _Universe->AccessCamera(cam)->SetPosition(unpos-_Universe->AccessCamera()->GetR().Cast()*un->rSize()*zoomfactor);
}
void GameCockpit::SetupViewPort (bool clip) {
  _Universe->AccessCamera()->RestoreViewPort (0,(view==CP_FRONT?viewport_offset:0));
   GFXViewPort (0,(int)((view==CP_FRONT?viewport_offset:0)*g_game.y_resolution), g_game.x_resolution,g_game.y_resolution);
  _Universe->AccessCamera()->setCockpitOffset (view<CP_CHASE?cockpit_offset:0);
  Unit * un, *tgt;
  if ((un = parent.GetUnit())) {
    un->UpdateHudMatrix (CP_FRONT);
    un->UpdateHudMatrix (CP_LEFT);
    un->UpdateHudMatrix (CP_RIGHT);
    un->UpdateHudMatrix (CP_BACK);
    un->UpdateHudMatrix (CP_CHASE);
    
    Vector p,q,r;
    _Universe->AccessCamera(CP_FRONT)->GetOrientation(p,q,r);
    Vector tmp = r;
    r = -p;
    p = tmp;
    _Universe->AccessCamera(CP_LEFT)->SetOrientation(p,q,r);
    _Universe->AccessCamera(CP_FRONT)->GetOrientation(p,q,r);
    tmp = r;
    r = p;
    p = -tmp;
    _Universe->AccessCamera(CP_RIGHT)->SetOrientation(p,q,r);
    _Universe->AccessCamera(CP_FRONT)->GetOrientation(p,q,r);
    r = -r;
    p = -p;
    _Universe->AccessCamera(CP_BACK)->SetOrientation(p,q,r);
#ifdef IWANTTOPVIEW
    _Universe->AccessCamera(CP_FRONT)->GetOrientation(p,q,r);
    tmp=r;
    r = -q;
    q = tmp;
    _Universe->AccessCamera(CP_CHASE)->SetOrientation(p,q,r);
#endif
    tgt = un->Target();
    if (tgt) {
      
      Vector p,q,r,tmp;
      un->GetOrientation (p,q,r);
      r = (tgt->Position()-un->Position()).Cast();
      r.Normalize();
      CrossProduct (r,q,tmp);
      CrossProduct (tmp,r,q);
      _Universe->AccessCamera(CP_VIEWTARGET)->SetOrientation(tmp,q,r);
      _Universe->AccessCamera(CP_TARGET)->SetOrientation(tmp,q,r);
      //      _Universe->AccessCamera(CP_PANTARGET)->SetOrientation(tmp,q,r);
      ShoveCamBehindUnit (CP_TARGET,tgt,zoomfactor);
      ShoveCamBehindUnit (CP_PANTARGET,tgt,zoomfactor);
    }else {
      un->UpdateHudMatrix (CP_VIEWTARGET);
      un->UpdateHudMatrix (CP_TARGET);
      un->UpdateHudMatrix (CP_PANTARGET);
    }
    ShoveCamBehindUnit (CP_CHASE,un,zoomfactor);
    //    ShoveCamBehindUnit (CP_PANTARGET,un,zoomfactor);




    ShoveCamBehindUnit (CP_PAN,un,zoomfactor);
    un->SetVisible(view>=CP_CHASE);

  }
  _Universe->AccessCamera()->UpdateGFX(clip?GFXTRUE:GFXFALSE);
    
  //  parent->UpdateHudMatrix();
}
void GameCockpit::SelectCamera(int cam){
    if(cam<NUM_CAM&&cam>=0)
      currentcamera = cam;
}
Camera* GameCockpit::AccessCamera(int num){
  if(num<NUM_CAM&&num>=0)
    return &cam[num];
  else
    return NULL;
}
