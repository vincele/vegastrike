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
#include "vegastrike.h"
#include "gl_globals.h"

extern GFXBOOL bTex0;
extern GFXBOOL bTex1;

//#define USE_DISPLAY_LISTS
GFXQuadList::GFXQuadList(GFXBOOL color): numVertices(0),numQuads(0),myVertices(NULL) {
  Dirty = GFXFALSE;
  isColor = color;
}

GFXQuadList::~GFXQuadList()
{
  if(myVertices)
    free(myVertices);
  if (myColors)
    free(myColors);
}

void GFXQuadList::Draw() {
  if (!numQuads)return;
  glVertexPointer(3, GL_FLOAT, sizeof(GFXVertex), &myVertices[0].x);
  glNormalPointer(GL_FLOAT, sizeof(GFXVertex), &myVertices[0].i);
	
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  if (bTex0) 
    glClientActiveTextureARB (GL_TEXTURE0_ARB);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, sizeof(GFXVertex), &myVertices[0].s+GFXStage0*2);
  if (bTex0) {
    glClientActiveTextureARB (GL_TEXTURE1_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  }
  if (isColor) {
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer (4,GL_FLOAT, sizeof (GFXColor), &myColors[0].r);    
  }
  glDrawArrays(GL_QUADS, 0, numQuads*4);
}


int GFXQuadList::AddQuad (GFXVertex *vertices,GFXColor * color) {
  int cur= numQuads*4;
  if (Dirty)
    for (int i=0;i<numQuads;i++) {
      if (quadassignments[i]==-1) {
	quadassignments[i]=numQuads;
	if (vertices) 
	  memcpy (myVertices+cur,vertices,4*sizeof(GFXVertex));
	if (isColor&&color) 
	  memcpy (myColors+cur,color,4*sizeof(GFXColor));      
	numQuads++;
	Dirty--;
	return i;
      }
    }
  
  if (!numVertices) {
    numVertices = 16;
    myVertices = (GFXVertex *)malloc (numVertices*sizeof (GFXVertex));
    myColors = (GFXColor *)malloc (numVertices*sizeof(GFXColor));
    quadassignments = (int *)malloc (numVertices*sizeof(int)/4);
  }else {
    numVertices *=2;    
    myVertices = (GFXVertex *)realloc (myVertices,numVertices*sizeof(GFXVertex));
    myColors = (GFXColor *)realloc (myColors,numVertices*sizeof(GFXColor));    
    quadassignments = (int *)realloc (quadassignments,numVertices*sizeof(int)/4);
  }
  quadassignments[numQuads]=numQuads;
  numQuads++;
  if (vertices) 
    memcpy (myVertices+cur,vertices,4*sizeof(GFXVertex));
  if (isColor&&myColors) 
    memcpy (myColors+cur,color,4*sizeof(GFXColor));
  
  return numQuads-1;
}
void GFXQuadList::DelQuad (int which ) {
  if (which <0||which>numQuads||quadassignments[which]==-1)
    return;
  Dirty++;
  for (int i=numQuads-1;i>=0;i--) {
    if (quadassignments[i]==numQuads-1) {
      memcpy (myVertices+(quadassignments[which]*4),myVertices+((numQuads-1)*4),4*sizeof(GFXVertex));
      if (isColor)
	memcpy (myColors+(quadassignments[which]*4),myColors+((numQuads-1)*4),4*sizeof(GFXColor));
      quadassignments[i]=quadassignments[which];
      quadassignments[which]=-1;
      return;
    }
  }
}
void GFXQuadList::ModQuad (int which, GFXVertex * vertices, GFXColor * colors) {
  if (which <0||which>numQuads||quadassignments[which]==-1)
    return;
  memcpy (myVertices+(quadassignments[which]*4),vertices,4*sizeof(GFXVertex));
  if (isColor&&colors) {
    memcpy (myVertices+(quadassignments[which]*4),colors,4*sizeof(GFXColor));
  }
}
