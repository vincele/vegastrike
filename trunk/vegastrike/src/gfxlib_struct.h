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
#ifndef _GFXLIB_STRUCT
#define _GFXLIB_STRUCT
#include "gfx/vec.h"

/// Vertex, Normal, Texture, and (deprecated) Environment Mapping T2F_N3F_V3F format
struct GFXVertex 
{
  float s;
  float t;
  float i;
  float j;
  float k;
  float x;
  float y;
  float z;
  
  GFXVertex(){}
  GFXVertex(const QVector & vert, const Vector & norm, float s, float t) {
    SetVertex(vert.Cast());
    SetNormal(norm);
    SetTexCoord(s, t);    
  }
  GFXVertex(const Vector &vert, const Vector &norm, float s, float t){
    SetVertex(vert);
    SetNormal(norm);
    SetTexCoord(s, t);
  }
  GFXVertex (float x, float y, float z, float i, float j, float k, float s, float t) {this->x=x;this->y=y;this->z=z;this->i=i;this->j=j;this->k=k;this->s=s;this->t=t;}
  GFXVertex &SetTexCoord(float s, float t) {this->s = s; this->t = t; return *this;}
  GFXVertex &SetNormal(const Vector &norm) {i = norm.i; j = norm.j; k = norm.k; return *this;}
  GFXVertex &SetVertex(const Vector &vert) {x = vert.i; y = vert.j; z = vert.k; return *this;}
  Vector GetVertex () {return Vector (x,y,z);}
  const Vector & GetConstVertex () const {return (*((Vector *)&x));}
  Vector GetNormal () {return Vector (i,j,k);}
};

//Stores a color (or any 4 valued vector)
struct GFXColor
{
	float r;
	float g;
	float b;
	float a;
	GFXColor() { }
 GFXColor (const Vector &v, float a=1.0) {
    this->r = v.i;
    this->g = v.j;
    this->b = v.k;
    this->a = a;
  }
  GFXColor(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = 1.0;
  }

  GFXColor(float r, float g, float b, float a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
  }
};

inline	GFXColor operator*(float s, const GFXColor&c) {
	return GFXColor(s *c.r, s*c.g, s*c.b, s*c.a);
}
inline GFXColor operator*(const GFXColor&c, float s) {
	return GFXColor(s *c.r, s*c.g, s*c.b, s*c.a);
}
inline GFXColor operator+(const GFXColor&c0, const GFXColor&c1) {
	return GFXColor(c0.r+c1.r,c0.g+c1.g,c0.b+c1.b,c0.a+c1.a);
}

///This vertex is used for the interleaved array argument for color based arrays T2F_C4F_N3F_V3F 
struct GFXColorVertex  {
  float s;
  float t;
  float r;
  float g;
  float b;
  float a;
  float i;
  float j;
  float k;
  float x;
  float y;
  float z;
  
  GFXColorVertex(){}
  GFXColorVertex(const Vector &vert, const Vector &norm, const GFXColor &rgba, float s, float t){
    SetVertex(vert);
    SetNormal(norm);
    SetColor (rgba);
    SetTexCoord(s, t);
  }
  GFXColorVertex (float x, float y, float z, float i, float j, float k, float r, float g, float b, float a, float s, float t) {this->x=x;this->y=y;this->z=z;this->i=i;this->j=j;this->k=k;this->r = r; this->g = g; this->b=b;this->a=a;this->s=s;this->t=t;}
  GFXColorVertex &SetTexCoord(float s, float t) {this->s = s; this->t = t; return *this;}
  GFXColorVertex &SetNormal(const Vector &norm) {i = norm.i; j = norm.j; k = norm.k; return *this;}
  GFXColorVertex &SetVertex(const Vector &vert) {x = vert.i; y = vert.j; z = vert.k; return *this;}
  GFXColorVertex &SetColor (const GFXColor &col) {r = col.r;g=col.g;b=col.b;a=col.a; return *this;}
  void SetVtx (const GFXVertex & vv) {s = vv.s;t=vv.t;i=vv.i;j=vv.j;k=vv.k;x=vv.x;y=vv.y;z=vv.z;}
};


///important ATTENUATE const STAYS AT ONE...for w compat.
enum LIGHT_TARGET {
  ATTENUATE=1,
  DIFFUSE=2,
  SPECULAR=4,
  AMBIENT=8,
  POSITION=16,
  EMISSION=32
};
///Holds all information for a single light object
class GFXLight {
 protected:
  ///physical GL light its saved in
  int target;
 public:
  ///last is w for positional, otherwise 3 for spec
  float vect[3];
  int options;
  float diffuse [4];
  float specular[4];
  float ambient[4];
  float attenuate[3];
  float direction[3];
  float exp;
  float cutoff;
 public:
  GFXLight() {
    //   vect[0]=vect[1]=vect[2]=vect[3]=0;
   vect[0]=vect[1]=vect[2]=0;
    attenuate[0]=1;attenuate[1]=attenuate[2]=0;
    diffuse[0]=diffuse[1]=diffuse[2]=0;//openGL defaults
    specular[0]=specular[1]=specular[2]=0;//openGL defaults
    ambient[0]=ambient[1]=ambient[2]=options=0;
    diffuse[3]=specular[3]=ambient[3]=1;
    target =-1;//physical GL light its saved in

	direction[0]=direction[1]=direction[2] = 0.0;
	exp=0.0;
	cutoff=180.0;
  }

  GFXLight (const bool enabled, const GFXColor &vect, const GFXColor &diffuse= GFXColor (0,0,0,1), const GFXColor &specular=GFXColor (0,0,0,1), const GFXColor &ambient=GFXColor(0,0,0,1), const GFXColor&attenuate=GFXColor(1,0,0), const GFXColor &direction=GFXColor(0,0,0), float exp=0.0, float cutoff=180.0);
  
  void SetProperties (enum LIGHT_TARGET, const GFXColor & color);
  GFXColor GetProperties (enum LIGHT_TARGET) const;
  void disable ();
  void enable ();
  bool attenuated ();
  void apply_attenuate (bool attenuated);
};
///Contains 4 texture coordinates (deprecated)
struct GFXTVertex // transformed vertex
{
	float x;
	float y;
	float z;
  /// reciprocal homogenous w
	float rhw; 
	int diffuse;
	int specular;
	float s,t;
	float u,v;
};



enum POLYTYPE {
  GFXTRI,
  GFXQUAD,
  GFXLINE,
  GFXTRISTRIP,
  GFXQUADSTRIP,
  GFXTRIFAN,
  GFXLINESTRIP,
  GFXPOLY,
  GFXPOINT
};

/**
 *   Meant to be a huge list of individual quads (like light maps) 
 *   that need to be resizable, etc all to be drawn at once.... nice on GL :-) 
 */
class /*GFXDRVAPI*/ GFXQuadList {
  ///Num vertices currently _allocated_ on quad list 
  int numVertices;
  ///Number of quads to be drawn packed first numQuads*4 vertices
  int numQuads;
  ///Assignments to packed data for quad modification
  int * quadassignments;
  ///all numVertices allocated vertices and color
  union VCDAT {
    GFXVertex * vertices;   
    GFXColorVertex * colors;
  } data;
  ///Is color in this quad list
  GFXBOOL isColor;
  ///number of "dirty" quads, hence gaps in quadassignments that must be assigned before more are allocated
  int Dirty;
 public:
  ///Creates an initial Quad List
  GFXQuadList(GFXBOOL color=GFXFALSE);
  ///Trashes given quad list
  ~GFXQuadList();
  ///Draws all quads contained in quad list
  void Draw();
  ///Adds quad to quad list, an integer indicating number that should be used to henceforth Mod it or delete it
  int AddQuad (const GFXVertex *vertices, const GFXColorVertex * colors=NULL);
  ///Removes quad from Quad list
  void DelQuad (int which);
  ///modifies quad in quad list to contain new vertices and color information
  void ModQuad (int which, const GFXVertex *vertices, float alpha=-1);
  void ModQuad (int which, const GFXColorVertex *vertices);
};
/**
 * A vertex list is a list of any conglomeration of POLY TYPES.
 * It is held for storage in an array of GFXVertex but attempts
 * to use a display list to hold information if possible 
 */
class /*GFXDRVAPI*/ GFXVertexList {
  ///Num vertices allocated
  int numVertices;
  ///Vertices and colors stored
  union VDAT{
    ///The data either does not have color data
    GFXVertex *vertices;  
    ///Or has color data
    GFXColorVertex *colors;
  } data;
  union INDEX {
    unsigned char *b;//stride 1
    unsigned short *s;//stride 2
    unsigned int *i;//stride 4
  } index;
  ///Array of modes that vertices will be drawn with
  GLenum *mode;
  ///Display list number if list is indeed active. 0 otherwise
  int display_list;
  ///number of different mode, drawn lists
  int numlists;
  /**
   * number of vertices in each individual mode.
   * 2 triangles 3 quads and 2 lines would be {6,12,4} as the offsets
   */
  int *offsets;
  ///If vertex list has been mutated since last draw.  Low 3 bits store the stride of the index list (if avail). another bit for if color is presnet
  char changed;
  ///Returns number of Triangles in vertex list (counts tri strips)
  int numTris ();
  ///Returns number of Quads in vertex list (counts quad strips)
  int numQuads();
  ///Looks up the index in the appropriate short, char or int array
  unsigned int GetIndex (int offset) const;
  ///copies nonindexed vertices to dst vertex array
  static void VtxCopy (GFXVertexList * thus, GFXVertex *dst, int offset, int howmany);
  ///Copies nonindex colored vertices to dst vertex array
  static void ColVtxCopy (GFXVertexList * thus, GFXVertex * dst, int offset, int howmany);
  ///Copies indexed colored vertices to dst vertex array
  static void ColIndVtxCopy (GFXVertexList * thus, GFXVertex *dst, int offset, int howmany);
  ///Copies indexed vertices to dst vertex array
  static void IndVtxCopy (GFXVertexList * thus, GFXVertex * dst, int offset, int howmany);
  ///Init function (call from construtor)
  void Init (enum POLYTYPE *poly, int numVertices, const GFXVertex * vert, const GFXColorVertex *colors, int numlists, int *offsets, bool Mutable,unsigned int * indices);
  ///Propagates modifications to the display list
  void RefreshDisplayList();
  void Draw (GLenum *poly, const INDEX index, const int numLists, const int *offsets);
public:
  ///creates a vertex list with 1 polytype and a given number of vertices
  inline GFXVertexList(enum POLYTYPE poly, int numVertices, const GFXVertex *vertices,int numindices, bool Mutable=false, unsigned int * index=NULL){Init (&poly, numVertices, vertices,NULL, 1,&numindices, Mutable,index);}
  ///Creates a vertex list with an arbitrary number of poly types and given vertices, num list and offsets (see above)
  inline GFXVertexList(enum POLYTYPE *poly, int numVertices, const GFXVertex *vertices, int numlists, int *offsets, bool Mutable=false,  unsigned int * index=NULL) {
    Init(poly,numVertices,vertices,NULL,numlists,offsets,Mutable, index);
  }
  ///Creates a vertex list with 1 poly type and color information to boot
  inline GFXVertexList(enum POLYTYPE poly, int numVertices, const GFXColorVertex *colors, int numindices, bool Mutable=false, unsigned int * index=NULL){Init (&poly, numVertices, NULL, colors, 1, &numindices,Mutable,index);}
  ///Creates a vertex list with an arbitrary number of poly types and color
  inline GFXVertexList(enum POLYTYPE *poly, int numVertices,  const GFXColorVertex *colors, int numlists, int *offsets, bool Mutable=false, unsigned int *index=NULL) {
    Init(poly,numVertices,NULL,colors, numlists,offsets,Mutable, index);
  }
  ~GFXVertexList();
  const GFXVertex * GetVertex (int index);
  const GFXColorVertex * GetColorVertex (int index);

  ///Returns the array of vertices to be mutated
  VDAT * BeginMutate (int offset);
  ///Ends mutation and refreshes display list
  void EndMutate (int newsize=0);
  ///Loads the draw state (what is active) of a given vlist for mass drawing
  void LoadDrawState();
  ///Specifies array pointers and loads the draw state of a given vlist for mass drawing
  void BeginDrawState(GFXBOOL lock=GFXTRUE);
  ///Draws a single copy of the mass-loaded vlist
  void Draw();
  void Draw(enum POLYTYPE poly, int numV);
  void Draw(enum POLYTYPE poly, int numV, unsigned char * index);
  void Draw(enum POLYTYPE poly, int numV, unsigned short *index);
  void Draw(enum POLYTYPE poly, int numV, unsigned int *index);
  ///Loads draw state and prepares to draw only once
  void DrawOnce (){LoadDrawState();BeginDrawState(GFXFALSE);Draw();EndDrawState(GFXFALSE);}
  void EndDrawState(GFXBOOL lock=GFXTRUE);
  ///returns a packed vertex list with number of polys and number of tries to passed in arguments. Useful for getting vertex info from a mesh
  void GetPolys (GFXVertex **vert, int *numPolys, int * numTris);
};

/**
 * Stores the Draw Context that a vertex list might be drawn with.
 * Especially useful for mass queued drawing (load matrix, draw... )
 */

/**
 * holds all parameters for materials
 */
struct GFXMaterial
{
  /// ambient rgba, if you don't like these things, ask me to rename them
	float ar;float ag;float ab;float aa;
  /// diffuse rgba
	float dr;float dg;float db;float da;
  /// specular rgba
	float sr;float sg;float sb;float sa;
  /// emissive rgba
	float er;float eg;float eb;float ea;
  /// specular power
	float power; 
};
//Textures may only be cube maps or Texture2d
enum TEXTURE_TARGET {
  TEXTURE2D,
  CUBEMAP
};
///Textures may only be cube maps or Texture2d
enum TEXTURE_IMAGE_TARGET {
  TEXTURE_2D,
  CUBEMAP_POSITIVE_X,
  CUBEMAP_NEGATIVE_X,
  CUBEMAP_POSITIVE_Y,
  CUBEMAP_NEGATIVE_Y,
  CUBEMAP_POSITIVE_Z,
  CUBEMAP_NEGATIVE_Z,
};

/**
 * Unlike OpenGL, 3 matrices are saved, the Model, the View and the Projection
 * Consistency is maintained through pushing the rotation part of view 
 * onto projection matrix
 */
enum MATRIXMODE{
	MODEL,
	PROJECTION,
	VIEW
};

enum TEXTUREFORMAT {
	DUMMY = 0,
	PALETTE8 = 1,
	RGB16 = 2,
	RGBA16 = 3,
	RGB24 = 4,
	RGBA32 = 5,
	RGB32 = 6
};
/**
 * The following state may be activated/deactivated
 * LIGHTING, DEPTHTEST,DEPTHWRITE, TEXTURE0, and TEXTURE1.  Future support
 * For arbitrary number of texture units should be added FIXME
 */
enum STATE {
	LIGHTING,
	DEPTHTEST,
	DEPTHWRITE,
	TEXTURE0,
	TEXTURE1,
	CULLFACE,
	SMOOTH
};

enum CLIPSTATE {
  GFX_NOT_VISIBLE,
  GFX_PARTIALLY_VISIBLE,
  GFX_TOTALLY_VISIBLE

};

/**
 * Address modes various textures may have
 */
enum ADDRESSMODE{
	WRAP,
	MIRROR,
	CLAMP,
	BORDER
};

enum FOGMODE {
  FOG_OFF,
  FOG_EXP,
  FOG_EXP2,
  FOG_LINEAR
};
/**
 * Blend functions that a blendmode may have Not all work for all systems
 */
enum BLENDFUNC{
    ZERO            = 1,
	ONE             = 2, 
    SRCCOLOR        = 3,
	INVSRCCOLOR     = 4, 
    SRCALPHA        = 5,
	INVSRCALPHA     = 6, 
    DESTALPHA       = 7,
	INVDESTALPHA    = 8, 
    DESTCOLOR       = 9,
	INVDESTCOLOR    = 10, 
    SRCALPHASAT     = 11,
    CONSTALPHA    = 12, 
    INVCONSTALPHA = 13,
    CONSTCOLOR = 14,
    INVCONSTCOLOR = 15
};

/** 
 * Different types of filtering. 
 * Bilinear and mipmap are independent params (hence powers of two)
 */
enum FILTER {
  NEAREST=0x0,
  BILINEAR=0x1,
  MIPMAP=0x2,
  TRILINEAR=0x4
};

/**
 * Used for depth and alpha comparisons
 */
enum DEPTHFUNC{
	NEVER,LESS,EQUAL, LEQUAL, GREATER, NEQUAL, GEQUAL, ALWAYS
};


///Pick data structures
struct PickData {
  int name;
  int zmin;
  int zmax;

  PickData(int name, int zmin, int zmax) : name(name), zmin(zmin), zmax(zmax) { }
};

#endif

