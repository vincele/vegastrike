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
#include "gl_globals.h"
//#include "gfx_transform_vector.h"

vector <GFXMaterial> materialinfo;
int selectedmaterial = -1;
void /*GFXDRVAPI*/ GFXSetMaterial(unsigned int &number, const GFXMaterial &material)
{
    number = -1;//warning unsigned.... 
  for (unsigned int i=0;i<materialinfo.size();i++){
    if (memcmp (&materialinfo[i],&material,sizeof(GFXMaterial))==0) {
      number = i;
      break;
    }
  }
  if (number==-1) {
    number = materialinfo.size();
    materialinfo.push_back (material);
  }

}

void GFXModifyMaterial (unsigned int number, const GFXMaterial &material) {
  materialinfo [number]=material;
}
GFXBOOL /*GFXDRVAPI*/ GFXGetMaterial(unsigned int number, GFXMaterial &material)
{
  if (number<0||number>=materialinfo.size())
    return GFXFALSE;
  material = materialinfo[number];
  return GFXTRUE;
}

void /*GFXDRVAPI*/ GFXSelectMaterial(unsigned int number)
{
  if (number!=selectedmaterial){
	float matvect[4];
	matvect[0] = materialinfo[number].ar;
	matvect[1] = materialinfo[number].ag;
	matvect[2] = materialinfo[number].ab;
	matvect[3] = materialinfo[number].aa;
	glMaterialfv(GL_FRONT, GL_AMBIENT, matvect);

	matvect[0] = materialinfo[number].dr;
	matvect[1] = materialinfo[number].dg;
	matvect[2] = materialinfo[number].db;
	matvect[3] = materialinfo[number].da;
	glMaterialfv(GL_FRONT, GL_DIFFUSE, matvect);

	matvect[0] = materialinfo[number].sr;
	matvect[1] = materialinfo[number].sg;
	matvect[2] = materialinfo[number].sb;
	matvect[3] = materialinfo[number].sa;
	glMaterialfv(GL_FRONT, GL_SPECULAR, matvect);

	matvect[0] = materialinfo[number].er;
	matvect[1] = materialinfo[number].eg;
	matvect[2] = materialinfo[number].eb;
	matvect[3] = materialinfo[number].ea;
	glMaterialfv(GL_FRONT, GL_EMISSION, matvect);

	glMaterialfv(GL_FRONT, GL_SHININESS, &materialinfo[number].power);
	selectedmaterial = number;
  }

}
