/*
 * Vega Strike
 * Copyright (C) 2001-2002 Daniel Horn
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
#include <memory.h>
#include "gfx_aux.h"
#include "gfx_mesh.h"
#include "gfx_transform.h"
#include "gfx_aux.h"
#include "gfx_camera.h"
#include <math.h>

#include <string>
using namespace std;

#include "gfxlib.h"

#include "hashtable.h"
#include "vegastrike.h"
static Hashtable<string, Mesh> meshHashTable;

//GFXDRVAPI BOOL Multitex;
BOOL Multitex = TRUE;

void DrawVector(const Vector &start, const Vector &vect)
{ 
  //float x = start.i, y = start.j, z = start.k;
	//glBegin(GL_LINES);
	//glVertex3f(x, y, z);
	//glVertex3f(vect.i + x, vect.j + y, vect.k + z);
	//glEnd();
}

void Mesh::InitUnit()
{
	changed = TRUE;
	vlist = NULL;
	

	//GFXVertex *vertexlist;
	GFXVertex *alphalist;

	vertexlist = NULL;
	stcoords = NULL;
	Decal = NULL;
	alphalist = NULL;
	ResetVectors(p,q,r);
	yrestricted = prestricted = rrestricted = FALSE;
	ymin = pmin = rmin = -PI;
	ymax = pmax = rmax = PI;
	ycur = pcur = rcur = 0;

	//	texturename[0] = -1;
	numforcelogo = numsquadlogo = 0;
	GFXGetMaterial(0, myMat);
	myMatNum = 0;
	scale = Vector(1.0,1.0,1.0);
	refcount = 1;  //FIXME VEGASTRIKE  THIS _WAS_ zero...NOW ONE
	orig = NULL;
}

Mesh::Mesh():Primitive()
{
	InitUnit();
}

Mesh:: Mesh(char * filename/*, Texture* ForceLog, Texture* SquadLog*/):Primitive()
{
	//awful stuff take this OUT
	//if (!SquadLog)
	//	SquadLog = new Texture ("TechSecRGB.bmp","TechSecA.bmp");

	//end awful stuff

	int TexNameLength;
	char *TexName=NULL;
	char * TexNameA=NULL;
	int NumPoints;
	float *x;
	float *y;
	float *z;
	float *i;
	float *j;
	float *k;
	int NumTris;
	int **Tris;
	int NumQuads;
	int **Quads;
	BOOL AlphaMap = FALSE;
	//InitPrimitive();

	InitUnit();
	Mesh *oldmesh;
	if(oldmesh = meshHashTable.Get(string(filename)))//h4w h4s#t4bl3 1z 1337
	{
		*this = *oldmesh;
		oldmesh->refcount++;
		orig = oldmesh;
		return;
	}
	else
	{
		oldmesh = (Mesh*)malloc(sizeof(Mesh));
		meshHashTable.Put(string(filename), oldmesh);
	}

	strcpy(name, filename);


	FILE* fp = NULL;
	int jj;
	fp = fopen (filename, "r+b");
	if (!fp)
	{
	  fprintf (stderr,"Failed to load file %s",filename);
	  exit(1);
	}
		
	TexNameLength = readi(fp);
	if (TexNameLength)
	{
		if (TexNameLength < 0) 
		{
			AlphaMap = TRUE;
			TexNameLength = -TexNameLength;
		}
		
		objtex = TRUE;
		TexName = new char [TexNameLength +5];
		fread (TexName, TexNameLength,1,fp);
		TexName[TexNameLength+4] = '\0';
		TexName[TexNameLength+0] = '.';
		TexName[TexNameLength+1] = 'b';
		TexName[TexNameLength+2] = 'm';
		TexName[TexNameLength+3] = 'p';
	}
	else
	{
		objtex = FALSE;
	}
	NumPoints = readi(fp);
	x = new float [NumPoints];
	y = new float [NumPoints];
	z = new float [NumPoints];
	i = new float [NumPoints];
	j = new float [NumPoints];
	k = new float [NumPoints]; 
	int ii;
	for (ii=0; ii<NumPoints; ii++)
	{
		x[ii] = readf (fp);
		y[ii] = readf (fp);
		z[ii] = readf (fp);
		i[ii] = readf (fp);
		j[ii] = readf (fp);
		k[ii] = readf (fp);
	}
	NumTris = readi (fp);

	Tris = new int* [NumTris];

	for (ii=0; ii< NumTris; ii++)
		Tris[ii] = new int [3];
	for (ii=0; ii< NumTris;ii++)
		for (int jj=0; jj<3; jj++)
			Tris[ii][jj] = readi(fp);
	NumQuads = readi (fp);
	Quads = new int* [NumQuads];
	for (ii=0; ii< NumQuads; ii++)
		Quads[ii] = new int [4];
	for (ii=0; ii< NumQuads;ii++)
		for (int jj=0; jj<4; jj++)
			Quads[ii][jj] = readi(fp);

	int numtrivertex = NumTris*3;
	int numquadvertex = NumQuads*4;
	numvertex = NumTris*3+NumQuads*4;
	//GFXVertex *vertexlist;
	//GFXVertex *alphalist;

	vertexlist = new GFXVertex [numvertex];
	stcoords = new float[numvertex * 2+10];
	numtris = NumTris;
	numquads = NumQuads ;

	jj=0;
	for (ii=0; ii<NumTris; ii++)
	{
		vertexlist[jj].x = x[Tris[ii][0]];
		vertexlist[jj].y = y[Tris[ii][0]];
		vertexlist[jj].z = z[Tris[ii][0]];
		vertexlist[jj].i = i[Tris[ii][0]];
		vertexlist[jj].j = j[Tris[ii][0]];
		vertexlist[jj].k = k[Tris[ii][0]];
		jj++;
		vertexlist[jj].x = x[Tris[ii][1]];
		vertexlist[jj].y = y[Tris[ii][1]];
		vertexlist[jj].z = z[Tris[ii][1]];
		vertexlist[jj].i = i[Tris[ii][1]];
		vertexlist[jj].j = j[Tris[ii][1]];
		vertexlist[jj].k = k[Tris[ii][1]];
		jj++;
		vertexlist[jj].x = x[Tris[ii][2]];
		vertexlist[jj].y = y[Tris[ii][2]];
		vertexlist[jj].z = z[Tris[ii][2]];
		vertexlist[jj].i = i[Tris[ii][2]];
		vertexlist[jj].j = j[Tris[ii][2]];
		vertexlist[jj].k = k[Tris[ii][2]];
		jj++;
	}
	
	for (ii=0; ii<NumQuads; ii++)
	{
		vertexlist[jj].x = x[Quads[ii][0]];
		vertexlist[jj].y = y[Quads[ii][0]];
		vertexlist[jj].z = z[Quads[ii][0]];
		vertexlist[jj].i = i[Quads[ii][0]];
		vertexlist[jj].j = j[Quads[ii][0]];
		vertexlist[jj].k = k[Quads[ii][0]];
		jj++;
		vertexlist[jj].x = x[Quads[ii][1]];
		vertexlist[jj].y = y[Quads[ii][1]];
		vertexlist[jj].z = z[Quads[ii][1]];
		vertexlist[jj].i = i[Quads[ii][1]];
		vertexlist[jj].j = j[Quads[ii][1]];
		vertexlist[jj].k = k[Quads[ii][1]];
		jj++;
		vertexlist[jj].x = x[Quads[ii][2]];
		vertexlist[jj].y = y[Quads[ii][2]];
		vertexlist[jj].z = z[Quads[ii][2]];
		vertexlist[jj].i = i[Quads[ii][2]];
		vertexlist[jj].j = j[Quads[ii][2]];
		vertexlist[jj].k = k[Quads[ii][2]];
		jj++;
		vertexlist[jj].x = x[Quads[ii][3]];
		vertexlist[jj].y = y[Quads[ii][3]];
		vertexlist[jj].z = z[Quads[ii][3]];
		vertexlist[jj].i = i[Quads[ii][3]];
		vertexlist[jj].j = j[Quads[ii][3]];
		vertexlist[jj].k = k[Quads[ii][3]];
		jj++;

	}
	if (objtex)
	{

		jj=0;
		int temp = NumTris*3;
		//float oo256 = .00390625;
		long pos = ftell(fp);
		for (ii=0; ii< temp; ii++)
		{
			vertexlist[ii].s = readf(fp);//*oo256;  
			vertexlist[ii].t = readf (fp);//*oo256;
		}
		
		temp = NumTris*3+NumQuads*4;
		for (; ii<temp; ii++)
		{
			vertexlist[ii].s = readf(fp);//*oo256;
			vertexlist[ii].t = readf(fp);//*oo256;
		}
		if (AlphaMap)
		{
			TexNameA = new char [TexNameLength +5];
			for (int kk = 0; kk < TexNameLength;kk++)
				TexNameA[kk] = TexName[kk];
			TexNameA[TexNameLength+4] = '\0';
			TexNameA[TexNameLength+0] = '.';
			TexNameA[TexNameLength+1] = 'a';
			TexNameA[TexNameLength+2] = 'l';
			TexNameA[TexNameLength+3] = 'p';
			//Decal =	new Texture(TexName, TexNameA);
			Decal =	new Texture(TexName, 0);
		}
		else 
		{
			Decal = new Texture (TexName, 0);
		}
		if (!Decal)
			objtex = FALSE;
	}
	numforcelogo = readi (fp);
	Vector *PolyNormal = new Vector [numforcelogo];
	Vector *center = new Vector [numforcelogo];
	float *sizes = new float [numforcelogo];
	float *rotations = new float [numforcelogo];
	float *offset = new float [numforcelogo];
	char polytype;
	int offst;
	Vector *Ref;
	Ref = new Vector [numforcelogo];
	for (ii=0; ii<numforcelogo;ii++)
	{
		Ref[ii] = Vector (0,0,0);
		center[ii].i = readf(fp);
		center[ii].j = readf(fp);
		center[ii].k = readf(fp);
		polytype = readc (fp);
		switch (polytype)
		{
		case 'T':
		case 't': offst = 3*readi(fp);
			break;
		case 'D':
		case 'A': offst = 3*readi(fp);
			Ref[ii].i= vertexlist[offst+1].x - vertexlist[offst].x;
			Ref[ii].j= vertexlist[offst+1].y - vertexlist[offst].y;
			Ref[ii].k= vertexlist[offst+1].z - vertexlist[offst].z;
			break;
		case 'E':
		case 'B': offst = 3*readi(fp);
			Ref[ii].i= vertexlist[offst+2].x - vertexlist[offst+1].x;
			Ref[ii].j= vertexlist[offst+2].y - vertexlist[offst+1].y;
			Ref[ii].k= vertexlist[offst+2].z - vertexlist[offst+1].z;
			break;
		case 'F':
		case 'C': offst = 3*readi(fp);
			Ref[ii].i= vertexlist[offst].x - vertexlist[offst+2].x;
			Ref[ii].j= vertexlist[offst].y - vertexlist[offst+2].y;
			Ref[ii].k= vertexlist[offst].z - vertexlist[offst+2].z;
			break;
		case 'Q':
		case 'q': offst = 3*numtris+4*readi(fp);
			break;
		case '0':
		case '4':
			offst = 3*numtris+4*readi(fp);
			Ref[ii].i= vertexlist[offst+1].x - vertexlist[offst].x;
			Ref[ii].j= vertexlist[offst+1].y - vertexlist[offst].y;
			Ref[ii].k= vertexlist[offst+1].z - vertexlist[offst].z;
			break;
		case '5':
		case '1': offst = 3*numtris+4*readi(fp);
			Ref[ii].i= vertexlist[offst+2].x - vertexlist[offst+1].x;
			Ref[ii].j= vertexlist[offst+2].y - vertexlist[offst+1].y;
			Ref[ii].k= vertexlist[offst+2].z - vertexlist[offst+1].z;
			break;
		case '6':
		case '2': offst = 3*numtris+4*readi(fp);
			Ref[ii].i= vertexlist[offst+3].x - vertexlist[offst+2].x;
			Ref[ii].j= vertexlist[offst+3].y - vertexlist[offst+2].y;
			Ref[ii].k= vertexlist[offst+3].z - vertexlist[offst+2].z;
			break;
		case '7':
		case '3': offst = 3*numtris+4*readi(fp); //total number of triangles incl pents
			Ref[ii].i= vertexlist[offst].x - vertexlist[offst+3].x;
			Ref[ii].j= vertexlist[offst].y - vertexlist[offst+3].y;
			Ref[ii].k= vertexlist[offst].z - vertexlist[offst+3].z;
			break;
		}
		switch (polytype)
		{
		case '4':
		case '5':
		case '6':
		case '7':
		case 'D':
		case 'E':
		case 'F':
			Ref[ii].i = -Ref[ii].i;
			Ref[ii].j = -Ref[ii].j;
			Ref[ii].k = -Ref[ii].k;
		break;
		}
		PolyNormal[ii] = PolygonNormal(
			Vector(vertexlist[offst].x,vertexlist[offst].y,vertexlist[offst].z),
			Vector(vertexlist[offst+1].x,vertexlist[offst+1].y,vertexlist[offst+1].z),
			Vector(vertexlist[offst+2].x,vertexlist[offst+2].y,vertexlist[offst+2].z)
			);

		sizes[ii] = readf (fp);
		rotations[ii] = readf (fp);
		offset [ii] = readf (fp);
		
	}
	forcelogos = new Logo(numforcelogo,center,PolyNormal,sizes ,rotations, 0.01F, _GFX->getForceLog(),Ref);
	delete [] Ref;
	delete []PolyNormal;
	delete []center;
	delete [] sizes;
	delete [] rotations;
	delete [] offset;
	for (ii=0; ii< numtris*3; ii+=3)
	{
		
		Vector Norm1 (vertexlist[ii+1].x-vertexlist[ii].x,vertexlist[ii+1].y-vertexlist[ii].y,vertexlist[ii+1].z-vertexlist[ii].z);
		Vector Norm2 (vertexlist[ii+2].x-vertexlist[ii].x,vertexlist[ii+2].y-vertexlist[ii].y,vertexlist[ii+2].z-vertexlist[ii].z);
		Vector Normal;
		CrossProduct (Norm2, Norm1, Normal);
		Normalize(Normal);
		vertexlist[ii].i = vertexlist[ii+1].i = vertexlist[ii+2].i = Normal.i;
		vertexlist[ii].j = vertexlist[ii+1].j = vertexlist[ii+2].j = Normal.j;
		vertexlist[ii].k = vertexlist[ii+1].k = vertexlist[ii+2].k = Normal.k;
	}
	for (ii=numtris*3; ii< numtris*3+numquads*4; ii+=4)
	{
		Vector Norm1 (vertexlist[ii+1].x-vertexlist[ii].x,vertexlist[ii+1].y-vertexlist[ii].y,vertexlist[ii+1].z-vertexlist[ii].z);
		Vector Norm2 (vertexlist[ii+3].x-vertexlist[ii].x,vertexlist[ii+3].y-vertexlist[ii].y,vertexlist[ii+3].z-vertexlist[ii].z);
		Vector Normal;
		CrossProduct (Norm2, Norm1, Normal);
		Normalize(Normal);
		vertexlist[ii].i = vertexlist[ii+1].i = vertexlist[ii+2].i = vertexlist[ii+3].i =Normal.i;
		vertexlist[ii].j = vertexlist[ii+1].j = vertexlist[ii+2].j = vertexlist[ii+3].j =Normal.j;
		vertexlist[ii].k = vertexlist[ii+1].k = vertexlist[ii+2].k = vertexlist[ii+3].k =Normal.k;
	}
	numsquadlogo = readi (fp);
	PolyNormal = new Vector [numsquadlogo];
	center = new Vector [numsquadlogo];
	sizes = new float [numsquadlogo];
	rotations = new float [numsquadlogo];
	offset = new float [numsquadlogo];
	//char polytype;
	//int offset;
	Ref = new Vector [numsquadlogo];
	for (ii=0; ii<numsquadlogo;ii++)
	{
		Ref[ii] = Vector (0,0,0);
		center[ii].i = readf(fp);
		center[ii].j = readf(fp);
		center[ii].k = readf(fp);
		polytype = readc (fp);
		switch (polytype)
		{
		case 'T':
		case 't': offst = 3*readi(fp);
			break;
		case 'D':
		case 'A': offst = 3*readi(fp);
			Ref[ii].i= vertexlist[offst+1].x - vertexlist[offst].x;
			Ref[ii].j= vertexlist[offst+1].y - vertexlist[offst].y;
			Ref[ii].k= vertexlist[offst+1].z - vertexlist[offst].z;
			break;
		case 'E':
		case 'B': offst = 3*readi(fp);
			Ref[ii].i= vertexlist[offst+2].x - vertexlist[offst+1].x;
			Ref[ii].j= vertexlist[offst+2].y - vertexlist[offst+1].y;
			Ref[ii].k= vertexlist[offst+2].z - vertexlist[offst+1].z;
			break;
		case 'F':
		case 'C': offst = 3*readi(fp);
			Ref[ii].i= vertexlist[offst].x - vertexlist[offst+2].x;
			Ref[ii].j= vertexlist[offst].y - vertexlist[offst+2].y;
			Ref[ii].k= vertexlist[offst].z - vertexlist[offst+2].z;
			break;
		case 'Q':
		case 'q': offst = 3*numtris+4*readi(fp);
			break;
		case '0':
		case '4':
			offst = 3*numtris+4*readi(fp);
			Ref[ii].i= vertexlist[offst+1].x - vertexlist[offst].x;
			Ref[ii].j= vertexlist[offst+1].y - vertexlist[offst].y;
			Ref[ii].k= vertexlist[offst+1].z - vertexlist[offst].z;
			break;
		case '5':
		case '1': offst = 3*numtris+4*readi(fp);
			Ref[ii].i= vertexlist[offst+2].x - vertexlist[offst+1].x;
			Ref[ii].j= vertexlist[offst+2].y - vertexlist[offst+1].y;
			Ref[ii].k= vertexlist[offst+2].z - vertexlist[offst+1].z;
			break;
		case '6':
		case '2': offst = 3*numtris+4*readi(fp);
			Ref[ii].i= vertexlist[offst+3].x - vertexlist[offst+2].x;
			Ref[ii].j= vertexlist[offst+3].y - vertexlist[offst+2].y;
			Ref[ii].k= vertexlist[offst+3].z - vertexlist[offst+2].z;
			break;
		case '7':
		case '3': offst = 3*numtris+4*readi(fp); //total number of triangles incl pents
			Ref[ii].i= vertexlist[offst].x - vertexlist[offst+3].x;
			Ref[ii].j= vertexlist[offst].y - vertexlist[offst+3].y;
			Ref[ii].k= vertexlist[offst].z - vertexlist[offst+3].z;
			break;
		}
		switch (polytype)
		{
		case '4':
		case '5':
		case '6':
		case '7':
		case 'D':
		case 'E':
		case 'F':
			Ref[ii].i = -Ref[ii].i;
			Ref[ii].j = -Ref[ii].j;
			Ref[ii].k = -Ref[ii].k;
		break;
		}
		PolyNormal[ii] = PolygonNormal(
			Vector(vertexlist[offst].x,vertexlist[offst].y,vertexlist[offst].z),
			Vector(vertexlist[offst+1].x,vertexlist[offst+1].y,vertexlist[offst+1].z),
			Vector(vertexlist[offst+2].x,vertexlist[offst+2].y,vertexlist[offst+2].z)
			);
		sizes[ii] = readf (fp);
		rotations[ii] = readf (fp);
		offset [ii] = readf (fp);
	}
	

	squadlogos = new Logo(numsquadlogo,center,PolyNormal,sizes ,rotations, (float)0.01, _GFX->getSquadLog(), Ref);
	delete [] Ref;
	vlist = new GFXVertexList(numvertex,numtris,numquads, vertexlist);
	//vlist = new GFXVertexList(numtris*4,0,numquads*4, vertexlist+numtris*3);
	long pos = ftell(fp);
	myMatNum = readi(fp);;
	fclose(fp);

	this->orig = oldmesh;
	*oldmesh = *this;
	oldmesh->orig = NULL;
	oldmesh->refcount++;

	delete [] vertexlist;
	delete []PolyNormal;
	delete []center;
	delete [] sizes;
	delete [] rotations;
	delete [] offset;
}

Mesh::~Mesh()
{
	if(!orig)
	{
		if(vlist!=NULL)
			delete vlist;
		//if(vertexlist != NULL)
		//	delete [] vertexlist;
		if(stcoords != NULL)
			delete [] stcoords;
		//if(alphalist != NULL)
		//	delete [] alphalist;
		if(Decal != NULL)
		{
			delete Decal;
		}
		if (squadlogos!=NULL)
			delete squadlogos;
		if (forcelogos!=NULL)
			delete forcelogos;
	}
	else
	{

		orig->refcount--;
		//printf ("orig refcount: %d",refcount);
		if(orig->refcount == 0)
			delete orig;
	}
}
float const ooPI = 1.00F/3.1415926535F;

//#include "d3d_internal.h"

void Mesh::Reflect()
{
	Vector pnt;
	Vector nml [4];
	Vector CamPos;
	float dist, oodist;
	Vector p = pp;
	Vector q = pq;
	//Vector r = pr;
	//Vector pos = ppos;
	int nt3 = 3 * numtris;
	int i;
	float w;
	vertexlist = vlist->LockUntransformed();
	for (i=0; i< nt3;i+=3)
	{
		for (int j=0; j<3;j++)
		{
			int k = i+j;
			pnt.i = p.i*vertexlist[k].x + q.i*vertexlist[k].y+this->r.i * vertexlist[k].z;
			pnt.j = p.j*vertexlist[k].x + q.j*vertexlist[k].y+this->r.j * vertexlist[k].z;
			pnt.k = p.k*vertexlist[k].x + q.k*vertexlist[k].y+this->r.k * vertexlist[k].z;
			nml[j].i = p.i*vertexlist[k].i + q.i*vertexlist[k].j+this->r.i * vertexlist[k].k;
			nml[j].j = p.j*vertexlist[k].i + q.j*vertexlist[k].j+this->r.j * vertexlist[k].k;
			nml[j].k = p.k*vertexlist[k].i + q.k*vertexlist[k].j+this->r.k * vertexlist[k].k;
			
			Camera* TempCam = _GFX->AccessCamera();
			TempCam->GetPosition (CamPos);
			CamPos = CamPos - pnt;
			dist = sqrtf (CamPos.i*CamPos.i + CamPos.j*CamPos.j + CamPos.k*CamPos.k); //use this in glide homogenious coord calc
			oodist = ((float)1.0)/ dist;
			CamPos.i *= oodist;
			CamPos.j *= oodist;
			CamPos.k *= oodist;
			Vector reflect = nml[j] * 2 * nml[j].Dot(CamPos) - CamPos;

			nml[j] = reflect;
			//EQUIV BELOW//float w1 = 1/sqrt (2*(-nml[j].k+1));
			w = sqrtf (((float) .5)/(((float) 1)-nml[j].k));
			vertexlist[k].u = (nml[j].i*w+1.23)*.40625;
			vertexlist[k].v = 1-(nml[j].j*w+1.23)*.40625;
			
		}
		
	}
	int nq4= 4*numquads;
	vertexlist += 3*numtris;//it moves up by 1 vertex each increment with pointer math... I think :)
	for (i=0; i< nq4;i+=4)
	{
		for (int j=0; j<4;j++)
		{
			int k = i+j;
			pnt.i = p.i*vertexlist[k].x + q.i*vertexlist[k].y+this->r.i * vertexlist[k].z;
			pnt.j = p.j*vertexlist[k].x + q.j*vertexlist[k].y+this->r.j * vertexlist[k].z;
			pnt.k = p.k*vertexlist[k].x + q.k*vertexlist[k].y+this->r.k * vertexlist[k].z;
			nml[j].i = p.i*vertexlist[k].i + q.i*vertexlist[k].j+this->r.i * vertexlist[k].k;
			nml[j].j = p.j*vertexlist[k].i + q.j*vertexlist[k].j+this->r.j * vertexlist[k].k;
			nml[j].k = p.k*vertexlist[k].i + q.k*vertexlist[k].j+this->r.k * vertexlist[k].k;
			
			Camera* TempCam = _GFX->AccessCamera();
			TempCam->GetPosition (CamPos);
			CamPos = CamPos - pnt;
			dist = sqrtf (CamPos.i*CamPos.i + CamPos.j*CamPos.j + CamPos.k*CamPos.k); //use this in glide homogenious coord calc
			oodist = ((float)1.0)/ (float)dist;
			CamPos.i *= oodist;
			CamPos.j *= oodist;
			CamPos.k *= oodist;
			Vector reflect = nml[j] * 2 * nml[j].Dot(CamPos) - CamPos;

			nml[j] = reflect;
			//EQUIV BELOWfloat w1 = 1/sqrt (2*(-nml[j].k+1));
			w = sqrtf (((float) .5)/(((float) 1)-nml[j].k));
			vertexlist[k].u = (nml[j].i*w+1.23)*.40625;
			vertexlist[k].v = 1-(nml[j].j*w+1.23)*.40625;

		}
	}
	vlist->UnlockUntransformed();
}

void Mesh::Draw()
{
	GFXSelectMaterial(myMatNum);
	//static float rot = 0;
	GFXColor(1.0, 1.0, 1.0, 1.0);
	UpdateMatrix();
	Reflect();
	GFXEnable(TEXTURE0);
	GFXEnable(TEXTURE1);
	Decal->MakeActive();
	_GFX->getLightMap()->MakeActive();
	GFXSelectTexcoordSet(0, 0);
	GFXSelectTexcoordSet(1, 1);
	vlist->Draw();
	forcelogos->Draw();
	squadlogos->Draw();
	
	//GFXSelectTexcoordSet(1, 0);

	/*
	static float rot = 0;
	GFXColor(1.0, 1.0, 1.0, 1.0);

	UpdateMatrix();
	Reflect();
	GFXBlendMode(ONE, ZERO);

	Decal->MakeActive();
	GFXSelectTexcoordSet(0, 0);
	vlist->Draw();

	GFXBlendMode(ONE, ONE);
	LightMap->MakeActive();
	GFXSelectTexcoordSet(0, 1);
	vlist->Draw();


	GFXSelectTexcoordSet(0, 0);

	GFXBlendMode(ONE, ZERO);

	*/
	/*else
	{
		static float rot = 0;
		GFXColor(1.0, 1.0, 1.0, 1.0);

		UpdateMatrix();
		Reflect();
		GFXBlendMode(ONE, ZERO);

		Decal->MakeActive();
		GFXSelectTexcoordSet(0, 0);
		vlist->Draw();
	
		GFXBlendMode(ONE, ONE);
		LightMap->MakeActive();
		GFXSelectTexcoordSet(0, 1);
		vlist->Draw();


		GFXSelectTexcoordSet(0, 0);

		GFXBlendMode(ONE, ZERO);
	}*/
}

void Mesh::Draw(const Vector &pp, const Vector &pq, const Vector &pr, const Vector &ppos)
{
	this->pp = pp;
	this->pq = pq;
	this->pr = pr;
	this->ppos = ppos;
	Draw();
}

void Mesh::UpdateMatrix()
{
	if(changed)
	{
		MultMatrix(transformation, translation, orientation);
		//glGetFloatv(GL_MODELVIEW_MATRIX, stackstate);
		GFXGetMatrix(MODEL, stackstate);
		changed = FALSE;
	}
	GFXMultMatrix(MODEL, transformation);
}

void Mesh::SetOrientation2()
{
	static Vector y = Vector(0,1,0);

	CrossProduct(y, r, p);
	CrossProduct(r, p, q);
	SetOrientation();
}


void Mesh::SetOrientation(Vector &p, Vector &q, Vector &r)
{
	this->p = p;
	this->q = q;
	this->r = r;

	SetOrientation();
}
void Mesh::SetOrientation()
{
	VectorToMatrix(orientation, p*scale.i,q*scale.j,r*scale.k);
	changed = TRUE;
}

void Mesh::RestrictYaw(float min, float max)
{
	yrestricted = TRUE;
	ymin = min;
	ymax = max;
	if(ycur<ymin)
		ycur = ymin;
	else if(ycur>ymax)
		ycur = ymax;
}
void Mesh::RestrictPitch(float min, float max)
{
	prestricted = TRUE;
	pmin = min;
	pmax = max;
	if(pcur<pmin)
		pcur = pmin;
	else if(pcur>pmax)
		pcur = pmax;
}
void Mesh::RestrictRoll(float min, float max)
{
	rrestricted = TRUE;
	rmin = min;
	rmax = max;
	if(rcur<rmin)
		rcur = rmin;
	else if(rcur>rmax)
		rcur = rmax;
}

BOOL Mesh::Yaw(float rad)
{
	BOOL retval = TRUE;
	if(yrestricted)
	{
		if(ycur+rad<ymin)
		{
			rad = ycur-ymin;
			ycur = ymin;
			retval = FALSE;
		}
		else if(ycur+rad>ymax)
			{
				rad = ymax - ycur;
				ycur = ymax;
				retval = FALSE;
			}
		else
			ycur += rad;
	}
	::Yaw(rad,p,q,r);
	SetOrientation();
	return retval;
}
BOOL Mesh::Pitch(float rad)
{
	BOOL retval = TRUE;
	if(prestricted)
	{
		if(pcur+rad<pmin)
		{
			rad = pcur-pmin;
			pcur = pmin;
			retval = FALSE;
		}
		else if(pcur+rad>pmax)
			{
				rad = pmax - pcur;
				pcur = pmax;
				retval = FALSE;
			}
		else
			pcur += rad;
	}
	::Pitch(rad,p,q,r);
	SetOrientation();
	return retval;
}
BOOL Mesh::Roll(float rad)
{
	BOOL retval = TRUE;
	if(rrestricted)
	{
		if(rcur+rad<rmin)
		{
			rad = rcur-rmin;
			rcur = rmin;
			retval = FALSE;
		}
		else if(rcur+rad>rmax)
			{
				rad = rmax - rcur;
				rcur = rmax;
				retval = FALSE;
			}
		else
			rcur += rad;
	}
	
	::Roll(rad,p,q,r);
	SetOrientation();
	return retval;
}
void Mesh::Destroy()
{
}

void Mesh::SetPosition(float x,float y,float z)
{
	SetPosition(Vector(x,y,z));
	SetPosition();
}
void Mesh::SetPosition(const Vector &origin)
{
	pos = origin;
	SetPosition();
}
void Mesh::SetPosition()
{
	Translate(translation, pos.i,pos.j,pos.k);
	changed = TRUE;
}
void Mesh::XSlide(float factor)
{
	pos += p * factor;
	changed = TRUE;
}
void Mesh::YSlide(float factor)
{
	pos += q * factor;
	changed = TRUE;
}
void Mesh::ZSlide(float factor)
{
	pos += r * factor;
	changed = TRUE;
}


bool Mesh::intersects(const Vector &start, const Vector &end) {
	return bspTree->intersects(start, end);
}

bool Mesh::intersects(const Vector &pt) {
	return bspTree->intersects(pt);
}

bool Mesh::intersects(Mesh *mesh) {
	return bspTree->intersects(mesh->bspTree);
}
