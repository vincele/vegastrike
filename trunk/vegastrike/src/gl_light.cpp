/* 
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn & Alan Shieh
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

#include "gfxlib.h"

#include "gl_light.h"


int GFX_MAX_LIGHTS=8;
int GFX_OPTIMAL_LIGHTS=4;
GFXBOOL GFXLIGHTING=GFXFALSE;

int _currentContext=0;
vector <vector <gfx_light> > _local_lights_dat;
vector <GFXColor> _ambient_light;
vector <gfx_light> * _llights=NULL;

//currently stored GL lights!
OpenGLLights* GLLights=NULL;//{-1,-1,-1,-1,-1,-1,-1,-1};


GFXLight::GFXLight (const bool enabled,const GFXColor &vect, const GFXColor &diffuse, const GFXColor &specular, const GFXColor &ambient, const GFXColor &attenuate) {
  target = -1;
  options = 0;
  memcpy (this->vect,&vect,sizeof(float)*3);
  memcpy (this->diffuse,&diffuse,sizeof(float)*4);
  memcpy (this->specular,&specular,sizeof(float)*4);
  memcpy (this->ambient,&ambient,sizeof(float)*4);
  memcpy (this->attenuate,&attenuate,sizeof(float)*3);
  apply_attenuate (attenuated());
  if (enabled)
    this->enable();
  else
    this->disable();
}
void GFXLight::disable () {options &= (~GFX_LIGHT_ENABLED);}
void GFXLight::enable (){options |= GFX_LIGHT_ENABLED;}
bool GFXLight::attenuated() {return (attenuate[0]!=1)||(attenuate[1]!=0)||(attenuate[2]!=0); }
void GFXLight::apply_attenuate (bool attenuated) {
  options = attenuated
    ? (options|GFX_ATTENUATED)
    : (options&(~GFX_ATTENUATED));
}

void /*GFXDRVAPI*/ GFXLight::SetProperties(enum LIGHT_TARGET lighttarg, const GFXColor &color) {
  switch (lighttarg) {
  case DIFFUSE:
    diffuse[0]=color.r;
    diffuse[1]=color.g;
    diffuse[2]=color.b;
    diffuse[3]=color.a;
    break;
  case SPECULAR:
    specular[0]=color.r;
    specular[1]=color.g;
    specular[2]=color.b;
    specular[3]=color.a;
    break;
  case AMBIENT:
    ambient[0]=color.r;
    ambient[1]=color.g;
    ambient[2]=color.b;
    ambient[3]=color.a;
    break;
  case POSITION:
    vect[0]=color.r;
    vect[1]=color.g;
    vect[2]=color.b;
    break;
  case ATTENUATE:
    attenuate[0]=color.r;
    attenuate[1]=color.g;
    attenuate[2]=color.b;
    break;
  }
  apply_attenuate (attenuated());
}



GFXBOOL /*GFXDRVAPI*/ GFXSetCutoff (float ttcutoff) {
  if (ttcutoff<0) 
    return GFXFALSE;
  intensity_cutoff=ttcutoff;
  return GFXTRUE;
}
void /*GFXDRVAPI*/ GFXSetOptimalIntensity (float intensity, float saturate) {
  optintense = intensity;
  optsat = saturate;
}
GFXBOOL /*GFXDRVAPI*/ GFXSetOptimalNumLights (int numLights) {
  if (numLights>GFX_MAX_LIGHTS||numLights<0)
    return GFXFALSE;
  GFX_OPTIMAL_LIGHTS=numLights;
  return GFXTRUE;
}



GFXBOOL /*GFXDRVAPI*/ GFXSetSeparateSpecularColor(GFXBOOL spec) {
#ifndef WIN32
	if (spec) {
    glLightModeli (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SEPARATE_SPECULAR_COLOR);
  }else {
    glLightModeli (GL_LIGHT_MODEL_COLOR_CONTROL,GL_SINGLE_COLOR);
    return GFXFALSE;
  }
#else
	return GFXFALSE;
#endif
	return GFXTRUE;
}


GFXBOOL /*GFXDRVAPI*/ GFXLightContextAmbient (const GFXColor &amb) {
  if (_currentContext >=_ambient_light.size())
    return GFXFALSE;
  (_ambient_light[_currentContext])=amb;
  //  (_ambient_light[_currentContext])[1]=amb.g;
  //  (_ambient_light[_currentContext])[2]=amb.b;
  //  (_ambient_light[_currentContext])[3]=amb.a;
  float tmp[4]={amb.r,amb.g,amb.b,amb.a};
  glLightModelfv (GL_LIGHT_MODEL_AMBIENT,tmp);
  return GFXTRUE;
}

GFXBOOL /*GFXDRVAPI*/ GFXCreateLight (int &light, const GFXLight &templatecopy, const bool global) {
  for (light=0;light<_llights->size();light++) {
    if ((*_llights)[light].Target()==-2)
      break;
  }
  if (light==_llights->size()) {	
    _llights->push_back (gfx_light());
  }
  
  return (*_llights)[light].Create(templatecopy,global);
}

void /*GFXDRVAPI*/ GFXDeleteLight (int light) {
  (*_llights)[light].Kill();
}


GFXBOOL /*GFXDRVAPI*/ GFXSetLight(int light, enum LIGHT_TARGET lt, const GFXColor &color) {
  if ((*_llights)[light].Target()==-2) {
    return GFXFALSE;
  }
  (*_llights)[light].ResetProperties(lt,color);
  
  return GFXTRUE;
}

GFXBOOL /*GFXDRVAPI*/ GFXEnableLight (int light) {
  assert (light>=0&&light<=_llights->size()); 
  //    return FALSE;
  if ((*_llights)[light].Target()==-2) {
    return GFXFALSE;
  }
  (*_llights)[light].Enable();
  return GFXTRUE;
}

GFXBOOL /*GFXDRVAPI*/ GFXDisableLight (int light) {
  assert (light>=0&&light<=_llights->size()); 

  if ((*_llights)[light].Target()==-2) {
    return GFXFALSE;
  }
  (*_llights)[light].Disable();
  return GFXTRUE;
}


static void SetupGLLightGlobals();

void /*GFXDRVAPI*/ GFXCreateLightContext (int & con_number) {
  static GFXBOOL LightInit=GFXFALSE;
  if (!LightInit) {
    LightInit = GFXTRUE;
    SetupGLLightGlobals();
  }
  con_number = _local_lights_dat.size();
  _currentContext= con_number;
  _ambient_light.push_back (GFXColor (0,0,0,1));
  _local_lights_dat.push_back (vector <gfx_light>());
  GFXSetLightContext (con_number);
}

void /*GFXDRVAPI*/ GFXDeleteLightContext(int con_number) {
  _local_lights_dat[con_number]=vector <gfx_light>();
}

void /*GFXDRVAPI*/ GFXSetLightContext (int con_number) {
  int GLLindex=0;
  unsigned int i;
  _currentContext = con_number;
  _llights = &_local_lights_dat[con_number];
  glLightModelfv (GL_LIGHT_MODEL_AMBIENT,(GLfloat *) &(_ambient_light[con_number]));
  //reset all lights so they arean't in GLLights
  for (i=0;i<_llights->size();i++) (*_llights)[i].Target()=-1;

  for (i=0;i<_llights->size()&&GLLindex<GFX_MAX_LIGHTS;i++) {
    if ((*_llights)[i].enabled()&&(!(*_llights)[i].LocalLight())) {
      GLLights[GLLindex].index=-1;//make it clobber completley! no trace of old light.
      (*_llights)[i].ClobberGLLight (GLLindex);
      GLLindex++;
    }
  }
  for (;GLLindex<GFX_MAX_LIGHTS;GLLindex++) {
    GLLights[GLLindex].index=-1;
    GLLights[GLLindex].options=OpenGLLights::GLL_OFF;
    glDisable (GL_LIGHT0+GLLindex);
  }
}


void GFXDestroyAllLights () {
  lighttable.Clear();
  if (GLLights)
    free (GLLights);
}

static void SetupGLLightGlobals () {
   int i;
   glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);//don't want lighting coming from infinity....we have to take the hit due to sphere mapping matrix tweaking
    //
    glGetIntegerv(GL_MAX_LIGHTS,&GFX_MAX_LIGHTS);
    if (!GLLights) {
      GLLights= (OpenGLLights *)malloc (sizeof(OpenGLLights)*GFX_MAX_LIGHTS);
      for (i=0;i<GFX_MAX_LIGHTS;i++) {
	GLLights[i].index=-1;
      }
    }
}


