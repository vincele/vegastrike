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
#ifndef _TEXTURE_H_
#define _TEXTURE_H_
#include "gfxlib.h"
#include "gfxlib_struct.h"

#ifdef __cplusplus
#include <string>
#else
#include <string.h>
#endif

using std::string;	// needed for access to std::string

/**
 *  The texture class defines a method of loading bitmap textures
 *  It also defines a hash table in which textures are kept and looked up
 *  to prevent the loading of duplicate textures
 */
struct Texture{
  ///The file name used to load this texture
  char * texfilename;
  ///the filter mode of this texture
  enum FILTER ismipmapped;
  ///the dimensions of this texture
  unsigned int sizeX; unsigned int sizeY;
  ///The data of this texture (used in between functions, deleted)
  unsigned char *data;
  ///The bitmode of this texture
  enum {_8BIT, _24BIT, _24BITRGBA} mode;
  ///if we statically allocate it, then gl_texture will kill it when destructor is called...and if we delete this texture we be messed
  unsigned char * palette;  
  ///The GFXname of this texture
  int name;
  ///The multitexture stage of this texture
  int stage;
  ///The original data that would represent this texture
  Texture *original;
  ///The number of references on the original data
  int refcount;
  ///The target this will go to (cubemap or otherwise)
  enum TEXTURE_TARGET texture_target; enum TEXTURE_IMAGE_TARGET image_target;
  ///Returns if this texture is actually already loaded
  GFXBOOL checkold(std::string s, bool shared, string & hashname);
  ///Loads the old texture
  void setold();
  ///Inits the class with default values
  void InitTexture() {
    original = 0;
    refcount = 0;
    name = -1;
  }
  ///Binds this texture to GFX library
  int Bind();
  ///Transfers this texture to GFX library
  void Transfer();
public:
  ///Creates a texture with a single bitmap as color data and another grayscale .bmp as alpha data
  Texture(const char *,const char *, int stage = 0, enum FILTER mipmap= MIPMAP, enum TEXTURE_TARGET target=TEXTURE2D, enum TEXTURE_IMAGE_TARGET imagetarget=TEXTURE_2D, float alpha=1, int zeroval=0);
  ///Creates a texture with only color data as a single bitmap
  Texture(const char * FileName, int stage = 0, enum FILTER mipmap = MIPMAP, enum TEXTURE_TARGET target=TEXTURE2D, enum TEXTURE_IMAGE_TARGET imagetarget=TEXTURE_2D);
  ///Texture copy constructor that increases appropriate refcounts
  Texture (Texture *t);
  ///Destructor for texture
  virtual ~Texture();
  ///Do not use! For inheritors only
  Texture();
  ///Whether or not the string exists as a texture
  static Texture * Exists (std::string s);
  ///Whether or not the color and alpha data already exist
  static Texture * Exists (std::string s, std::string a);
  ///A way to sort the texture by the original address (to make sure like textures stick togehter
  virtual bool operator < (const Texture &b);
  ///A way to test if the texture is equal to another based on original values
  virtual bool operator == (const Texture &b);
  ///Binds the texture in the GFX library
  virtual void MakeActive();
  ///If the texture has loaded properly returns true
  virtual bool LoadSuccess () {
    return (name>=0);
  }
  ///Changes priority of texture
  virtual void Prioritize (float);
};

#endif
