/* 
 * Vega Strike
 * Copyright (C) 2001-2002 Alan Shieh
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
#include "gl_globals.h"

#include "gfxlib.h"
#include "vegastrike.h"
#include "vs_globals.h"
#include <stack>
GFXBOOL bTex0 = GFXTRUE;
GFXBOOL bTex1 = GFXTRUE;
int activeTextureStage=-1;
extern GFXBOOL GFXLIGHTING;
void /*GFXDRVAPI*/ GFXEnable (const STATE state)
{

	switch(state)
	{
	case LIGHTING:
	  glEnable(GL_LIGHTING);
	  GFXLIGHTING = GFXTRUE;
	  break;
	case DEPTHTEST:
	  glEnable(GL_DEPTH_TEST);
	  glDepthFunc (GL_LEQUAL);
	  //glDepthFunc (GL_ALWAYS);
	  break;
	case DEPTHWRITE:
	  glDepthMask(1);
	  break;
	case TEXTURE0:
	  bTex0 = GFXTRUE;
	  GFXActiveTexture(0);	
	  glEnable (GL_TEXTURE_2D);		
	  break;
	case TEXTURE1:
	  if (g_game.Multitexture) {
		bTex1 = GFXTRUE;
		GFXActiveTexture (1);
#ifdef NV_CUBE_MAP
		glEnable (GL_TEXTURE_CUBE_MAP_EXT);
#else
		glEnable (GL_TEXTURE_2D);		
#endif
	  }
	  break;
	case CULLFACE:
	  glEnable(GL_CULL_FACE);
	  break;
	}
}

void /*GFXDRVAPI*/ GFXDisable (const STATE state)
{
	
	switch(state)
	{
	case LIGHTING:
	  glDisable(GL_LIGHTING);
	  GFXLIGHTING = GFXFALSE;
	  break;
	case DEPTHTEST:
	  glDisable(GL_DEPTH_TEST);
	  break;
	case DEPTHWRITE:
	  glDepthMask(0);
	  break;
	case TEXTURE0:
	  bTex0 = GFXFALSE;
	  GFXActiveTexture(0);	
	  glDisable (GL_TEXTURE_2D);		
	  break;
	case TEXTURE1:
	  if (g_game.Multitexture) {
		bTex1 = GFXFALSE;
		bTex1 = GFXTRUE;
		GFXActiveTexture (1);
#ifdef NV_CUBE_MAP
		glDisable (GL_TEXTURE_CUBE_MAP_EXT);
#else
		glDisable (GL_TEXTURE_2D);		
#endif
		//		GFXActiveTexture(GL_TEXTURE0_ARB);
	  }
	  break;
	case CULLFACE:
	  glDisable(GL_CULL_FACE);
	  break;
	}

}

void GFXTextureAddressMode(ADDRESSMODE mode)
{
	float BColor [4] = {0,0,0,0};//set border color to clear... dunno if we wanna change?
	switch(mode)
	{
	case WRAP:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);		
		break;
	case CLAMP:
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);		

		break;

	case BORDER:
		
		glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR, BColor);
		break;
	case MIRROR:
		//nope not goin here I hope nVidia extension?
	default:
		return ; // won't work
	}
}

struct BlendMode {
	BLENDFUNC sfactor, dfactor;
	BlendMode() {sfactor = dfactor = ONE;};
} currBlendMode;

stack<BlendMode> blendstack;

void GFXBlendMode(const enum BLENDFUNC src, const enum BLENDFUNC dst)
{
	GLenum sfactor,dfactor;
	switch (src)
	{
	case ZERO:			sfactor = GL_ZERO;            
	break;
	case ONE:			sfactor = GL_ONE;
	break;
	case SRCALPHA:		sfactor = GL_SRC_ALPHA;
	break;
	case INVSRCALPHA:	sfactor = GL_ONE_MINUS_SRC_ALPHA;
	break;
	case DESTALPHA:		sfactor = GL_DST_ALPHA;
	break;
	case INVDESTALPHA:	sfactor = GL_ONE_MINUS_DST_ALPHA;
	break;
	case DESTCOLOR:		sfactor = GL_DST_COLOR;
	break;
	case INVDESTCOLOR:	sfactor = GL_ONE_MINUS_DST_COLOR;
	break;
	case SRCALPHASAT:	sfactor = GL_SRC_ALPHA_SATURATE;
	break;
#ifndef _WIN32
	case CONSTALPHA:   	sfactor=GL_CONSTANT_ALPHA;
	  break;
	case INVCONSTALPHA:	sfactor=GL_ONE_MINUS_CONSTANT_ALPHA;
	  break;
	case CONSTCOLOR:	sfactor=GL_CONSTANT_COLOR;
	  break;
	case INVCONSTCOLOR:	sfactor=GL_ONE_MINUS_CONSTANT_COLOR;
	  break;
#endif
	case SRCCOLOR:
	case INVSRCCOLOR:	
	default:	
	  return;
	  //	return FALSE;
	}
	
	switch (dst)
	{
	case ZERO:		dfactor = GL_ZERO;            
	  break;
	case ONE:		dfactor = GL_ONE;
	  break;
	case SRCCOLOR:		dfactor = GL_SRC_COLOR;
	  break;
	case INVSRCCOLOR:	dfactor = GL_ONE_MINUS_SRC_COLOR;
	  break;
	case SRCALPHA:		dfactor = GL_SRC_ALPHA;
	  break;
	case INVSRCALPHA:	dfactor = GL_ONE_MINUS_SRC_ALPHA;
	  break;
	case DESTALPHA:		dfactor = GL_DST_ALPHA;
	  break;
	case INVDESTALPHA:	dfactor = GL_ONE_MINUS_DST_ALPHA;
	  break;
	  #ifndef _WIN32

	case CONSTALPHA:   	sfactor=GL_CONSTANT_ALPHA;
	  break;
	case INVCONSTALPHA:	sfactor=GL_ONE_MINUS_CONSTANT_ALPHA;
	  break;
	case CONSTCOLOR:	sfactor=GL_CONSTANT_COLOR;
	  break;
	case INVCONSTCOLOR:	sfactor=GL_ONE_MINUS_CONSTANT_COLOR;
	  break;

	case DESTCOLOR:		
	case INVDESTCOLOR:	
	case SRCALPHASAT:	
#endif
		default:		
		return ;
	}
	glBlendFunc (sfactor, dfactor);
	currBlendMode.sfactor = src;
	currBlendMode.dfactor = dst;
}

void GFXPushBlendMode()
{
	blendstack.push(currBlendMode);
}

void GFXPopBlendMode()
{
  if(!blendstack.empty())
    {
      currBlendMode = blendstack.top();
      GFXBlendMode(currBlendMode.sfactor, currBlendMode.dfactor);
      blendstack.pop();
    }
}

void GFXDepthFunc(enum DEPTHFUNC dfunc)
{
	switch (dfunc)
	{
	case NEVER:		glDepthFunc (GL_NEVER);
		break;
	case LESS:		glDepthFunc (GL_LESS);
		break;
	case EQUAL:		glDepthFunc (GL_EQUAL);
		break;
	case LEQUAL:	glDepthFunc (GL_LEQUAL);
		break;

	case GREATER:	glDepthFunc (GL_GREATER);
		break;
	case NEQUAL:	glDepthFunc (GL_NOTEQUAL);
		break;
	case GEQUAL:	glDepthFunc (GL_GEQUAL);
		break;
	case ALWAYS:	glDepthFunc (GL_ALWAYS); 
		break;
	};
}

void /*GFXDRVAPI*/ GFXSelectTexcoordSet(const int stage, const int texset)
{
	if (stage)
	{
		GFXStage1 = texset;
	}
	else
	{
		GFXStage0 = texset;
	}
}

void GFXActiveTexture (const int stage) {
  if (g_game.Multitexture&&stage!=activeTextureStage) {
    glActiveTextureARB(GL_TEXTURE0_ARB+stage);
    activeTextureStage=stage;
  }

}

void GFXAlphaTest (const enum DEPTHFUNC df, const float ref) {
  if (df==ALWAYS) {
    glDisable (GL_ALPHA_TEST);
    return;
  }else {
    glEnable (GL_ALPHA_TEST);
  }
  GLenum tmp;
  switch (df) {
  case NEVER:
    tmp = GL_NEVER;
    break;
  case LESS:
    tmp = GL_LESS;
    break;
  case EQUAL:
    tmp = GL_EQUAL;
    break;
  case LEQUAL:
    tmp = GL_LEQUAL;
    break;
  case GREATER:
    tmp = GL_GREATER;
    break;
  case NEQUAL:
    tmp = GL_NOTEQUAL;
    break;
  case GEQUAL:
    tmp = GL_GEQUAL;
    break;
  case ALWAYS:
    tmp= GL_ALWAYS;
    break;
  } 
  glAlphaFunc (tmp,ref);
}

GFXBOOL GFXSetTexFunc(int stage, int texset)
{

	if (stage)
	{
		GFXStage1 = texset;
	}
	else
	{
		GFXStage0 = texset;
	}	
	if (g_game.Multitexture)
	{
	  GFXActiveTexture(stage);	
	  if (!stage) {
	    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	  }
	  if (stage==1) {
	    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
	  }
	  GFXActiveTexture(stage);
		
	}	
	else return GFXFALSE;
	
	return GFXTRUE;
}

