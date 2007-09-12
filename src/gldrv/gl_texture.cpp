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
#define GL_EXT_texture_env_combine 1
#include "gldrv/sdds.h"
#include "gl_globals.h"
#include "vs_globals.h"
#include "vegastrike.h"
#include "config_xml.h"
#include "gfxlib.h"

#ifndef GL_TEXTURE_CUBE_MAP_EXT
#define GL_TEXTURE_CUBE_MAP_EXT           0x8513
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT 0x851A
#endif

//#define  MAX_TEXTURES 16384
static GLint MAX_TEXTURE_SIZE=256;

extern GLenum GetGLTextureTarget(enum TEXTURE_TARGET texture_target);

GLenum GetUncompressedTextureFormat (TEXTUREFORMAT textureformat)
{
	switch (textureformat) {

		case RGB24:
			return GL_RGB;
		case RGB32:
			return GL_RGB;
		case DXT1RGBA:
		case DXT3:
		case DXT5:
		case DXT1:
		case RGBA32:
			return GL_RGBA;
		case RGBA16:
			return GL_RGBA16;
		case RGB16:
			return GL_RGB16;
		default:
			return GL_RGBA;
	}
}


struct GLTexture
{
	//  unsigned char *texture;
	GLubyte * palette;
	int width;
	int height;
	int iwidth;					 // Interface width
	int iheight;				 // Interface height
	int texturestage;
	GLuint name;
	GFXBOOL alive;
	GLenum textureformat;
	GLenum targets;
	enum FILTER mipmapped;
};
//static GLTexture *textures=NULL;
//static GLEnum * targets=NULL;

static vector <GLTexture> textures;
static int activetexture[32]=
{
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1
};

static void ConvertPalette(unsigned char *dest, unsigned char *src)
{
	for(int a=0; a<256; a++, dest+=4, src+=4) {
		memcpy(dest, src, 3);
		dest[3] = 255;
	}

}


int tmp_abs (int num)
{
	return num<0?-num:num;
}


bool isPowerOfTwo (int num, int &which)
{
	which=0;
	while (tmp_abs(num)>1) {
		if ((num/2)*2!=num) {
			return false;
		}
		which++;
		num/=2;
	}
	return true;
}


static GLint round2(GLint n)
{
	GLint m;

	for (m = 1; m < n; m *= 2);

	/* m>=n */
	if (m - n <= n - m / 2) {
		return m;
	}
	else {
		return m / 2;
	}
}


static GLint bytes_per_pixel(GLenum format, GLenum type)
{
	GLint n, m;

	switch (format) {
		case GL_COLOR_INDEX:
		case GL_STENCIL_INDEX:
		case GL_DEPTH_COMPONENT:
		case GL_RED:
		case GL_GREEN:
		case GL_BLUE:
		case GL_ALPHA:
		case GL_LUMINANCE:
			n = 1;
			break;
		case GL_LUMINANCE_ALPHA:
			n = 2;
			break;
		case GL_RGB:
		case GL_BGR:
			n = 3;
			break;
		case GL_RGBA:
		case GL_BGRA:
#ifdef GL_EXT_abgr
		case GL_ABGR_EXT:
#endif
			n = 4;
			break;
		default:
			n = 0;
	}

	switch (type) {
		case GL_UNSIGNED_BYTE:
			m = sizeof(GLubyte);
			break;
		case GL_BYTE:
			m = sizeof(GLbyte);
			break;
		case GL_BITMAP:
			m = 1;
			break;
		case GL_UNSIGNED_SHORT:
			m = sizeof(GLushort);
			break;
		case GL_SHORT:
			m = sizeof(GLshort);
			break;
		case GL_UNSIGNED_INT:
			m = sizeof(GLuint);
			break;
		case GL_INT:
			m = sizeof(GLint);
			break;
		case GL_FLOAT:
			m = sizeof(GLfloat);
			break;
		default:
			m = 0;
	}

	return n * m;
}


static GLint appleBuild2DMipmaps(GLenum target, GLint components,
GLsizei width, GLsizei height, GLenum format,
GLenum type, const void *data)
{
	GLint w, h, maxsize;
	void *image, *newimage;
	GLint neww, newh, level, bpp;
	int error;
	GLboolean done;
	GLint retval = 0;
	GLint unpackrowlength, unpackalignment, unpackskiprows, unpackskippixels;
	GLint packrowlength, packalignment, packskiprows, packskippixels;

	if (width < 1 || height < 1)
		return GLU_INVALID_VALUE;

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);

	w = round2(width);
	if (w > maxsize) {
		w = maxsize;
	}
	h = round2(height);
	if (h > maxsize) {
		h = maxsize;
	}

	bpp = bytes_per_pixel(format, type);
	if (bpp == 0) {
		/* probably a bad format or type enum */
		return GLU_INVALID_ENUM;
	}

	/* Get current glPixelStore values */
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &unpackrowlength);
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpackalignment);
	glGetIntegerv(GL_UNPACK_SKIP_ROWS, &unpackskiprows);
	glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &unpackskippixels);
	glGetIntegerv(GL_PACK_ROW_LENGTH, &packrowlength);
	glGetIntegerv(GL_PACK_ALIGNMENT, &packalignment);
	glGetIntegerv(GL_PACK_SKIP_ROWS, &packskiprows);
	glGetIntegerv(GL_PACK_SKIP_PIXELS, &packskippixels);

	/* set pixel packing */
	glPixelStorei(GL_PACK_ROW_LENGTH, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_SKIP_ROWS, 0);
	glPixelStorei(GL_PACK_SKIP_PIXELS, 0);

	done = GL_FALSE;

	if (w != width || h != height) {
		/* must rescale image to get "top" mipmap texture image */
		image = malloc((w + 4) * h * bpp);
		if (!image) {
			return GLU_OUT_OF_MEMORY;
		}
		error = gluScaleImage(format, width, height, type, data,
			w, h, type, image);
		if (error) {
			retval = error;
			done = GL_TRUE;
		}
	}
	else {
		image = (void *) data;
	}

	level = 0;
	while (!done) {
		if (image != data) {
			/* set pixel unpacking */
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
			glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		}

		glTexImage2D(target, level, components, w, h, 0, format, type, image);

		if (w == 1 && h == 1)
			break;

		neww = (w < 2) ? 1 : w / 2;
		newh = (h < 2) ? 1 : h / 2;
		newimage = malloc((neww + 4) * newh * bpp);
		if (!newimage) {
			return GLU_OUT_OF_MEMORY;
		}

		error = gluScaleImage(format, w, h, type, image,
			neww, newh, type, newimage);
		if (error) {
			retval = error;
			done = GL_TRUE;
		}

		if (image != data) {
			free(image);
		}
		image = newimage;

		w = neww;
		h = newh;
		level++;
	}

	if (image != data) {
		free(image);
	}

	/* Restore original glPixelStore state */
	glPixelStorei(GL_UNPACK_ROW_LENGTH, unpackrowlength);
	glPixelStorei(GL_UNPACK_ALIGNMENT, unpackalignment);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, unpackskiprows);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, unpackskippixels);
	glPixelStorei(GL_PACK_ROW_LENGTH, packrowlength);
	glPixelStorei(GL_PACK_ALIGNMENT, packalignment);
	glPixelStorei(GL_PACK_SKIP_ROWS, packskiprows);
	glPixelStorei(GL_PACK_SKIP_PIXELS, packskippixels);

	return retval;
}


#ifdef __APPLE__
#define gluBuild2DMipmaps appleBuild2DMipmaps
#endif

GFXBOOL /*GFXDRVAPI*/ GFXCreateTexture(int width, int height, TEXTUREFORMAT textureformat, int *handle, char *palette , int texturestage, enum FILTER mipmap, enum TEXTURE_TARGET texture_target, enum ADDRESSMODE address_mode)
{
	static bool verbose_debug = XMLSupport::parse_bool(vs_config->getVariable("data","verbose_debug","false"));
	int dummy=0;
	if ((mipmap&(MIPMAP|TRILINEAR))&&!isPowerOfTwo (width,dummy)) {
		VSFileSystem::vs_fprintf (stderr,"Width %d not a power of two",width);
		//    assert (false);
	}
	if ((mipmap&(MIPMAP|TRILINEAR))&&!isPowerOfTwo (height,dummy)) {
		VSFileSystem::vs_fprintf (stderr,"Height %d not a power of two",height);
		//    assert (false);

	}
	GFXActiveTexture(texturestage);
	//case 3:  ... 3 pass... are you insane? well look who's talking to himself! oh.. good point :)
	*handle = 0;
	while (*handle<textures.size()) {
		if (!textures[*handle].alive) {
			//VSFileSystem::vs_fprintf (stderr,"got dead tex");
			break;
		}
		else {
			(*handle)++;
		}
	}
	if ((*handle)==textures.size()) {
		if(verbose_debug) {
			VSFileSystem::vs_fprintf (stderr,"x");
		}
		textures.push_back(GLTexture());
		textures.back().palette=NULL;
		textures.back().alive=GFXTRUE;
		textures.back().name=-1;
		textures.back().width=textures.back().height=textures.back().iwidth=textures.back().iheight=1;
	}

	if (address_mode==DEFAULT_ADDRESS_MODE) switch (texture_target) {
		case TEXTURE1D:
		case TEXTURE2D:
#ifdef GL_EXT_texture3D
		case TEXTURE3D: address_mode = WRAP; break;
#endif
		case CUBEMAP:   address_mode = CLAMP; break;
		default:        address_mode = WRAP; break;
	}
	switch (texture_target) {
		case TEXTURE1D: textures [*handle].targets=GL_TEXTURE_1D; break;
		case TEXTURE2D: textures [*handle].targets=GL_TEXTURE_2D; break;
#ifdef GL_EXT_texture3D
		case TEXTURE3D: textures [*handle].targets=GL_TEXTURE_3D; break;
#endif
		case CUBEMAP: textures [*handle].targets=GL_TEXTURE_CUBE_MAP_EXT; break;
	}
	if(verbose_debug) {
		VSFileSystem::vs_fprintf (stderr,"y");
	}
								 //for those libs with stubbed out handle gen't
	textures[*handle].name = *handle+1;
	//VSFileSystem::vs_fprintf (stderr,"Texture Handle %d",*handle);
	textures[*handle].alive = GFXTRUE;
	textures[*handle].texturestage = texturestage;
	textures[*handle].mipmapped = mipmap;
	if(verbose_debug) {
		VSFileSystem::vs_fprintf (stderr,"z");
	}
	glGenTextures (1,&textures[*handle].name);
	glBindTexture (textures[*handle].targets,textures[*handle].name);
	activetexture[texturestage]=*handle;
	GFXTextureAddressMode(address_mode,texture_target);
	if (textures[*handle].mipmapped&(TRILINEAR|MIPMAP)&&gl_options.mipmap>=2) {
		glTexParameteri (textures[*handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (textures[*handle].mipmapped&TRILINEAR&&gl_options.mipmap>=3) {
			glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		}
		else {
			glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		}
	}
	else {
		if (textures[*handle].mipmapped==NEAREST||gl_options.mipmap==0) {
			glTexParameteri (textures[*handle].targets, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		}
		else {
			glTexParameteri (textures[*handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri (textures[*handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}
	glTexParameterf (textures[*handle].targets,GL_TEXTURE_PRIORITY,.5);
	textures[*handle].width = width;
	textures[*handle].height = height;
	textures[*handle].iwidth = width;
	textures[*handle].iheight = height;
	textures[*handle].palette=NULL;
	if (palette&&textureformat == PALETTE8) {
		if(verbose_debug) {
			VSFileSystem::vs_fprintf (stderr," palette ");
		}
		textures[*handle].palette = (GLubyte *)malloc (sizeof (GLubyte)*1024);
		ConvertPalette(textures[*handle].palette, (unsigned char *)palette);
	}
	textures[*handle].textureformat = GetUncompressedTextureFormat(textureformat);
	if(verbose_debug) {
		VSFileSystem::vs_fprintf (stderr,"w");
	}
	//  GFXActiveTexture(0);
	return GFXTRUE;
}


void /*GFXDRVAPI*/ GFXPrioritizeTexture (unsigned int handle, float priority) {
glPrioritizeTextures (1,
#if defined(__APPLE__)
	(GLuint*)
#endif
	&handle,&priority);
}


void /*GFXDRVAPI*/ GFXAttachPalette (unsigned char *palette, int handle)
{
	ConvertPalette(textures[handle].palette, palette);
	//memcpy (textures[handle].palette,palette,768);
}


static void DownSampleTexture (unsigned char **newbuf,const unsigned char * oldbuf, int &height, int &width, int pixsize, int handle, int maxheight,int maxwidth,float newfade)
{
	assert(pixsize<=4);

	int i,j,k,l,m,n,o,p;
	if (MAX_TEXTURE_SIZE<maxwidth)
		maxwidth=MAX_TEXTURE_SIZE;
	if (MAX_TEXTURE_SIZE<maxheight)
		maxheight=MAX_TEXTURE_SIZE;
	int newwidth = width>maxwidth?maxwidth:width;
	int scalewidth = width/newwidth;
	int newheight = height>maxheight?maxheight:height;
	int scaleheight = height/newheight;
	int inewfade = (int)(newfade*0x100);

	// Proposed downsampling code -- end
	if ((scalewidth!=2)||(scaleheight!=2)||(inewfade != 0x100)) {
		// Generic, area average downsampling (optimized)
		//    Principle: The main optimizations/features
		//        a) integer arithmetic, with propper scaling for propper saturation
		//        b) unrolled loops (more parallelism, if the optimizer supports it)
		//        c) improved locality due to 32-pixel chunking
		int wmask = scalewidth-1;
		int hmask = scaleheight-1;
		int tshift= 0;
		int ostride = newwidth*pixsize;
		int istride = width*pixsize;
		int rowstride = scaleheight*istride;
		int chunkstride = 32*pixsize;
		int ichunkstride = scalewidth*chunkstride;
		int wshift = 0;
		int hshift = 0;
		int amask=wmask; while (amask) amask>>=1,tshift++,wshift++;
		amask=hmask; while (amask) amask>>=1,tshift++,hshift++;
		int tmask = (1<<tshift)-1;
		*newbuf = (unsigned char*)malloc(newheight*newwidth*pixsize*sizeof(unsigned char));
		unsigned int temp[32*4];
		unsigned char *orow = (*newbuf);
		const unsigned char *irow = oldbuf;
		for (i=0;i<newheight;i++,orow+=ostride,irow+=rowstride) {
			const unsigned char *crow = irow;
			unsigned char *orow2= orow;
			for (j=0; j<newwidth; j+=32,crow+=ichunkstride,orow2+=chunkstride) {
				const unsigned char *crow2 = crow;
				for (k=0; k<chunkstride; k++) temp[k]=0;
				for (m=0; m<scaleheight; m++,crow2+=istride)
					for (k=n=l=0; (k<chunkstride) && (j+l<newwidth); k+=pixsize,l++)
						for (o=0; o<scalewidth; o++)
							(temp[k+0]+=crow2[n++]),
							(pixsize>1) && (temp[k+1]+=crow2[n++]),
							(pixsize>2) && (temp[k+2]+=crow2[n++]),
								 //Unrolled loop
							(pixsize>3) && (temp[k+3]+=crow2[n++]);

				for (k=l=0; (k<chunkstride)&&(j+l<newwidth); k+=pixsize,l++)
					(orow2[k+0]=(unsigned char)((((temp[k+0]+tmask)>>tshift)*inewfade + 0x80*(0x100-inewfade))>>8)),
					(pixsize>1) && (orow2[k+1]=(unsigned char)((((temp[k+1]+tmask)>>tshift)*inewfade + 0x80*(0x100-inewfade))>>8)),
					(pixsize>2) && (orow2[k+2]=(unsigned char)((((temp[k+2]+tmask)>>tshift)*inewfade + 0x80*(0x100-inewfade))>>8)),
								 //Unrolled loop
					(pixsize>3) && (orow2[k+3]=(unsigned char)((((temp[k+3]+tmask)>>tshift)*inewfade + 0x80*(0x100-inewfade))>>8));
			}
		}
	}
	else {
		// Specific purpose downsampler: 2x2 averaging
		//    a) Very little overhead
		//    b) Very common case (mipmap generation)
		*newbuf = (unsigned char*)malloc(newheight*newwidth*pixsize*sizeof(unsigned char));
		unsigned char *orow = (*newbuf);
		int ostride = newwidth*pixsize;
		int istride = width*pixsize;
		const unsigned char *irow[2] = {oldbuf,oldbuf+istride};
		unsigned int temp[4];
		for (i=0; i<newheight; i++,irow[0]+=2*istride,irow[1]+=2*istride,orow+=ostride) {
			for (j=k=0; j<newwidth; j++,k+=pixsize) {
				(temp[0] =irow[0][(k<<1)+0]),
					(pixsize>1) && (temp[1] =irow[0][(k<<1)+1]),
					(pixsize>2) && (temp[2] =irow[0][(k<<1)+2]),
								 //Unrolled loop
					(pixsize>3) && (temp[3] =irow[0][(k<<1)+3]);

				(temp[0]+=irow[0][(k<<1)+pixsize+0]),
					(pixsize>1) && (temp[1]+=irow[0][(k<<1)+pixsize+1]),
					(pixsize>2) && (temp[2]+=irow[0][(k<<1)+pixsize+2]),
								 //Unrolled loop
					(pixsize>3) && (temp[3]+=irow[0][(k<<1)+pixsize+3]);

				(temp[0]+=irow[1][(k<<1)+0]),
					(pixsize>1) && (temp[1]+=irow[1][(k<<1)+1]),
					(pixsize>2) && (temp[2]+=irow[1][(k<<1)+2]),
								 //Unrolled loop
					(pixsize>3) && (temp[3]+=irow[1][(k<<1)+3]);

				(temp[0]+=irow[1][(k<<1)+pixsize+0]),
					(pixsize>1) && (temp[1]+=irow[1][(k<<1)+pixsize+1]),
					(pixsize>2) && (temp[2]+=irow[1][(k<<1)+pixsize+2]),
								 //Unrolled loop
					(pixsize>3) && (temp[3]+=irow[1][(k<<1)+pixsize+3]);

				(orow[k+0]=(unsigned char)((temp[0]+3)>>2)),
					(pixsize>1) && (orow[k+1]=(unsigned char)((temp[1]+3)>>2)),
					(pixsize>2) && (orow[k+2]=(unsigned char)((temp[2]+3)>>2)),
								 //Unrolled loop
					(pixsize>3) && (orow[k+3]=(unsigned char)((temp[3]+3)>>2));
			}
		}
	};
	/*
	// Original downsampling code -- begin
	*newbuf = (unsigned char*)malloc(newheight*newwidth*pixsize*sizeof(unsigned char));
	float *temp = (float *)malloc (pixsize*sizeof(float));
	for (i=0;i<newheight;i++) {
	  for (j=0;j<newwidth;j++) {
		for (m=0;m<pixsize;m++) {
	  temp[m]=0;
		}
		float xshift = width*(float)j/(float)newwidth;
		float yshift = height*(float)i/(float)newheight;
		int x =(int)xshift;
		int y =(int)yshift;
		yshift-=y;
		xshift-=x;
		xshift+=.5;
		yshift+=.5;
		if (xshift>1)xshift=1;
		if (yshift>1)yshift=1;
		x=x<width?x:width-1;
		y=y<height?y:height-1;
		int xpp = x+1<width?x+1:x;
		int ypp = y+1<height?y+1:y;
		for (m=0;m<pixsize;++m) {
		  temp[m] += (1-yshift)*(1-xshift)*oldbuf[(x+y*width)*pixsize+m];
		  temp[m] += (1-yshift)*xshift*oldbuf[(xpp+y*width)*pixsize+m];
		  temp[m] += yshift*(1-xshift)*oldbuf[(x+ypp*width)*pixsize+m];
		  temp[m] += yshift*xshift*oldbuf[(xpp+ypp*width)*pixsize+m];
		}
		for (m=0;m<pixsize;m++) {
	  (*newbuf)[m+pixsize*(j+i*newwidth)] = (unsigned char)((1-newfade)*128+temp[m]*newfade);
		}
	  }
	}
	free (temp);
	//Original downsampling code -- end
	*/

	width = newwidth;
	height= newheight;
}


static GLenum RGBCompressed (GLenum internalformat)
{
	if (gl_options.compression) {
		internalformat = GL_COMPRESSED_RGB_ARB;
		if (gl_options.s3tc) {
			internalformat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		}
	}
	return internalformat;
}


static GLenum RGBACompressed (GLenum internalformat)
{
	if (gl_options.compression) {
		internalformat = GL_COMPRESSED_RGBA_ARB;
		if (gl_options.s3tc) {
			switch (gl_options.compression) {
				case 3:
					internalformat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
					break;
				case 2:
					internalformat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
					break;
				case 1:
					internalformat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
					break;
			}
		}
	}
	return internalformat;
}


GLenum GetTextureFormat (TEXTUREFORMAT textureformat)
{
	switch(textureformat) {
		case RGB32:
			return RGBCompressed (GL_RGB);
		case RGBA32:
			return RGBACompressed(GL_RGBA);
		case RGBA16:
			return RGBACompressed (GL_RGBA16);
		case RGB16:
			return RGBCompressed (GL_RGB16);
		case DXT1:
			return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case DXT1RGBA:
			return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;		
		case DXT3:
			return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case DXT5:
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		default:
		case DUMMY:
		case RGB24:
			return RGBCompressed (GL_RGB);
	}
}


GLenum GetImageTarget (TEXTURE_IMAGE_TARGET imagetarget)
{
	GLenum image2D;
	switch (imagetarget) {
		case TEXTURE_2D:
			image2D = GL_TEXTURE_2D;
			break;
		case CUBEMAP_POSITIVE_X:
			image2D = GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT;
			break;
		case CUBEMAP_NEGATIVE_X:
			image2D=GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT;
			break;
		case CUBEMAP_POSITIVE_Y:
			image2D = GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT;
			break;
		case CUBEMAP_NEGATIVE_Y:
			image2D=GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT;
			break;
		case CUBEMAP_POSITIVE_Z:
			image2D = GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT;
			break;
		case CUBEMAP_NEGATIVE_Z:
			image2D=GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT;
			break;
	}
	return image2D;
}


GFXBOOL /*GFXDRVAPI*/ GFXTransferSubTexture (unsigned char * buffer, int handle, int x, int y, unsigned int width, unsigned int height, enum TEXTURE_IMAGE_TARGET imagetarget) {
GLenum image2D=GetImageTarget (imagetarget);
glBindTexture(textures[handle].targets, textures[handle].name);

//  internalformat = GetTextureFormat (handle);

glTexSubImage2D(image2D, 0, x,y,width,height,textures[handle].textureformat,GL_UNSIGNED_BYTE,buffer);
return GFXTRUE;
}

	
GFXBOOL /*GFXDRVAPI*/ GFXTransferTexture (unsigned char *buffer, int handle,  TEXTUREFORMAT internformat, enum TEXTURE_IMAGE_TARGET imagetarget,int maxdimension, GFXBOOL detail_texture)
{
	if (handle<0)
		return GFXFALSE;
	int logsize=1;
	int logwid=1;
	int mmap = 1;
	unsigned char *data = NULL;
	bool comptemp = gl_options.compression;
	if(internformat >= PNGPALETTE8){
		gl_options.compression = false;
		if(internformat == PNGRGB24)
			internformat = RGB24;
		else if(internformat == PNGRGBA32)
			internformat = RGBA32;
		else 
			internformat = PALETTE8;
	}
	if ((textures[handle].mipmapped&(TRILINEAR|MIPMAP))&&(!isPowerOfTwo (textures[handle].width,logwid)|| !isPowerOfTwo (textures[handle].height,logsize))) {
		static unsigned char NONPOWEROFTWO[1024]= {
			255,127,127,255,
			255,255,0,255,
			255,255,0,255,
			255,127,127,255
		};
		buffer=NONPOWEROFTWO;
		textures[handle].width=2;
		textures[handle].height=2;
		//    assert (false);
	}
	logsize = logsize>logwid?logsize:logwid;
	if (maxdimension==65536) {
		maxdimension = gl_options.max_texture_dimension;
	}
	int error;
	unsigned char * tempbuf = NULL;
	GLenum internalformat;
	GLenum image2D=GetImageTarget (imagetarget);
	glBindTexture(textures[handle].targets, textures[handle].name);
	int block = 16;
	int offset1 = 0;
	if(internformat == DXT1)
		block = 8;	
	if(internformat >= DXT1 && internformat <= DXT5){
		while(textures[handle].width > maxdimension || textures[handle].height > maxdimension){
			offset1 = ((textures[handle].width +3)/4)*((textures[handle].height +3)/4) * block;
			textures[handle].width >>=1;
			textures[handle].height >>=1;
			textures[handle].iwidth >>=1;
			textures[handle].iheight >>=1;
		}
	}
	else if (textures[handle].iwidth>maxdimension||textures[handle].iheight>maxdimension||textures[handle].iwidth>MAX_TEXTURE_SIZE||textures[handle].iheight>MAX_TEXTURE_SIZE) {
#if !defined(GL_COLOR_INDEX8_EXT)
		if (internformat != PALETTE8) {
#else
		if (internformat != PALETTE8||gl_options.PaletteExt) {
#endif
			textures[handle].height = textures[handle].iheight;
			textures[handle].width  = textures[handle].iwidth;
			DownSampleTexture (&tempbuf,buffer,textures[handle].height,textures[handle].width,(internformat==PALETTE8?1:(internformat==RGBA32?4:3))* sizeof(unsigned char ), handle,maxdimension,maxdimension,1);
			buffer = tempbuf;	
		}
	}
	if (!gl_options.s3tc) {
		if(internformat >=DXT1 && internformat <= DXT5){
			unsigned char *tmpbuffer = buffer +offset1;
			ddsDecompress(tmpbuffer,data,internformat,textures[handle].height,textures[handle].width);
			buffer = data;
			internformat = RGBA32;
		}
	}
	if (internformat!=PALETTE8 && internformat != PNGPALETTE8) {
		internalformat = GetTextureFormat (internformat);
		if ((textures[handle].mipmapped&&gl_options.mipmap>=2)||detail_texture) {
			if (detail_texture) {
				static FILTER fil = XMLSupport::parse_bool(vs_config->getVariable("graphics","detail_texture_trilinear","true"))?TRILINEAR:MIPMAP;
				textures[handle].mipmapped=     fil;
				glTexParameteri (textures[handle].targets, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				if (fil&TRILINEAR) {
					glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				} else {
					glTexParameteri (textures[handle].targets, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				}

			}
			int width=textures[handle].width,height=textures[handle].height;
			int count=0;
			static int blankout = XMLSupport::parse_int(vs_config->getVariable("graphics","detail_texture_blankout","3"));
			static int fullout = XMLSupport::parse_int(vs_config->getVariable("graphics","detail_texture_full_color","1"))-1;
			float numdivisors = logsize>fullout+blankout?(1./(logsize-fullout-blankout)):1;
			float detailscale=1;
			//feenableexcept(0);
			if(internformat >= DXT1 && internformat <= DXT5){
				int height = textures[handle].height;
				int width = textures[handle].width;
				int size = 0;
				int blocksize = 16;
				int i = 0;
				unsigned int offset = 0;
				//printf("shouldn't be here\n");
				if(internformat == DXT1|| internformat == DXT1RGBA)
					blocksize = 8;
				size = ((width +3)/4) * ((height +3)/4) * blocksize;
//				glTexParameteri( image2D, GL_GENERATE_MIPMAP, GL_FALSE );
				while(width && height){
					glCompressedTexImage2D_p(image2D,i,internalformat,width,height,0,size,buffer+offset1+offset);
					if(width ==1 && height == 1) break;
					if(width != 1)
						width >>=1;
					if(height != 1)
						height >>=1;
					offset += size;
					size = ((width +3)/4) * ((height +3)/4) * blocksize;
					++i;
				}
			} else {
				//printf("build mipmaps format %d, width %d, height, %d, tformat %d \n",internalformat, textures[handle].width,textures[handle].height,textures[handle].textureformat);
				gluBuild2DMipmaps(image2D, internalformat, textures[handle].width, textures[handle].height, textures[handle].textureformat, GL_UNSIGNED_BYTE, buffer);
//				gluBuild2DMipmaps(image2D, GL_RGBA, textures[handle].width, textures[handle].height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
//				glTexImage2D(image2D, 0, internalformat, textures[handle].width, textures[handle].height, 0, textures[handle].textureformat, GL_UNSIGNED_BYTE, buffer);
				//printf("done\n");
			}
			if (tempbuf) 
				free(tempbuf);
			tempbuf=NULL;
		} else{
			if(internformat >= DXT1 && internformat <= DXT5){
				int height = textures[handle].height;
				int width = textures[handle].width;
				int size = 0;
				int blocksize = 16;
				if(internformat == DXT1)
					blocksize = 8;
				size = ((width +3)/4) * ((height +3)/4) * blocksize;
				glCompressedTexImage2D_p(image2D,0,internalformat,width,height,0,size,buffer+offset1);
			} else 
				glTexImage2D(image2D, 0, internalformat, textures[handle].width, textures[handle].height, 0, textures[handle].textureformat, GL_UNSIGNED_BYTE, buffer);
		}
	} else {
		// IRIX has no GL_COLOR_INDEX8 extension
#if defined(GL_COLOR_INDEX8_EXT)
		if (gl_options.PaletteExt) {
			error = glGetError();
			glColorTable_p(textures[handle].targets, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, textures[handle].palette);
			error = glGetError();
			if (error) {
				if (tempbuf)
					free(tempbuf);
				gl_options.compression = comptemp;
				if(data)
					free(data);
				return GFXFALSE;
			}
			if(internformat >= DXT1 && internformat <= DXT5){
				int height = textures[handle].height;
				int width = textures[handle].width;
				int size = 0;
				int blocksize = 16;
				int i = 0;
				unsigned int offset = 0;
				//printf("shouldn't be here\n");
				if(internformat == DXT1|| internformat == DXT1RGBA)
					blocksize = 8;
				size = ((width +3)/4) * ((height +3)/4) * blocksize;
				
				while(width && height){
					glCompressedTexImage2D_p(image2D,i,internalformat,width,height,0,size,buffer+offset1+offset);
					width >>=1;
					height >>=1;
					offset += size;
					size = ((width +3)/4) * ((height +3)/4) * blocksize;
					++i;
				}
			} else {
				if ((textures[handle].mipmapped&(MIPMAP|TRILINEAR))&&gl_options.mipmap>=2) {
					gluBuild2DMipmaps(image2D, GL_COLOR_INDEX8_EXT, textures[handle].width, textures[handle].height, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buffer);
				} else {
					glTexImage2D(image2D, 0, GL_COLOR_INDEX8_EXT, textures[handle].width, textures[handle].height, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, buffer);
				}
			}
#if 0
			error = glGetError();
			if (error) {
				if (tempbuf)
					free(tempbuf);
				gl_options.compression = comptemp;
				if(data)
					free(data);
				return GFXFALSE;
			}
#endif
		} else
#endif
		{
			if(internformat >= DXT1 && internformat <= DXT5){
				int height = textures[handle].height;
				int width = textures[handle].width;
				int size = 0;
				int blocksize = 16;
				int i = 0;
				unsigned int offset = 0;
				//printf("shouldn't be here\n");
				if(internformat == DXT1|| internformat == DXT1RGBA)
					blocksize = 8;
				size = ((width +3)/4) * ((height +3)/4) * blocksize;
				
				while(width && height){
					glCompressedTexImage2D_p(image2D,i,internalformat,width,height,0,size,buffer+offset1+offset);
					width >>=1;
					height >>=1;
					offset += size;
					size = ((width +3)/4) * ((height +3)/4) * blocksize;
					++i;
				}
			} else {
				int nsize = 4*textures[handle].iheight*textures[handle].iwidth;
				unsigned char * tbuf =(unsigned char *) malloc (sizeof(unsigned char)*nsize);
				//      textures[handle].texture = tbuf;
				int j =0;
				for (int i=0; i< nsize; i+=4) {
					tbuf[i] = textures[handle].palette[4*buffer[j]];
					tbuf[i+1] = textures[handle].palette[4*buffer[j]+1];
					tbuf[i+2] = textures[handle].palette[4*buffer[j]+2];
					//used to be 255
					tbuf[i+3]= textures[handle].palette[4*buffer[j]+3];
					j ++;
				}
				GFXTransferTexture(tbuf,handle,RGBA32,imagetarget,maxdimension,detail_texture);
				free (tbuf);
			}
		}
	}
	if (tempbuf)
		free(tempbuf);
	gl_options.compression = comptemp;
	if(data)
		free(data);
	return GFXTRUE;
}

void /*GFXDRVAPI*/ GFXDeleteTexture (int handle) 
{
	if (textures[handle].alive) {
		glDeleteTextures(1, &textures[handle].name);
		for (size_t i=0;i<sizeof(activetexture)/sizeof(int);++i) {
			if (activetexture[i]==handle) {
				activetexture[i]=-1;
			}
		}
	}
	if (textures[handle].palette) {
		free  (textures[handle].palette);
		textures[handle].palette=0;
	}
	textures[handle].alive = GFXFALSE;
}


void GFXInitTextureManager() {
	for (int handle=0;handle<textures.size();handle++) {
		textures[handle].palette=NULL;
		textures[handle].width=textures[handle].height=textures[handle].iwidth=textures[handle].iheight=0;
		textures[handle].texturestage=0;
		textures[handle].name=0;
		textures[handle].alive=0;
		textures[handle].textureformat=DUMMY;
		textures[handle].targets=0;
		textures[handle].mipmapped=NEAREST;
	}
	glGetIntegerv (GL_MAX_TEXTURE_SIZE,&MAX_TEXTURE_SIZE);
}


void GFXDestroyAllTextures () {
	for (int handle=0;handle<textures.size();handle++) {
		GFXDeleteTexture (handle);
	}
}


void GFXTextureCoordGenMode(int stage, GFXTEXTURECOORDMODE tex, const float params[4],const float paramt[4]) {
	if (stage&&stage>=gl_options.Multitexture) return;

	GFXActiveTexture(stage);
	switch (tex) {
		case NO_GEN:
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);
			break;
		case EYE_LINEAR_GEN:
			glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
			glTexGenfv(GL_S,GL_EYE_PLANE,params);
			glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_EYE_LINEAR);
			glTexGenfv(GL_T,GL_EYE_PLANE,paramt);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);
			break;
		case OBJECT_LINEAR_GEN:
			glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
			glTexGenfv(GL_S,GL_OBJECT_PLANE,params);
			glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_OBJECT_LINEAR);
			glTexGenfv(GL_T,GL_OBJECT_PLANE,paramt);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);
			break;
		case SPHERE_MAP_GEN:
			glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
			glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);
			break;
		case CUBE_MAP_GEN:
#ifdef NV_CUBE_MAP
			glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_REFLECTION_MAP_NV);
			glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_REFLECTION_MAP_NV);
			glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_REFLECTION_MAP_NV);
			glEnable(GL_TEXTURE_GEN_S);
			glEnable(GL_TEXTURE_GEN_T);
			glEnable(GL_TEXTURE_GEN_R);
#else
			assert(0);
#endif
			break;
	}
}


void /*GFXDRVAPI*/ GFXSelectTexture(int handle, int stage) {
	if (stage&&stage>=gl_options.Multitexture) return;

	if (activetexture[stage]!=handle) {
		GFXActiveTexture(stage);
		activetexture[stage] = handle;
		if (gl_options.Multitexture||(stage==0))
			glBindTexture(textures[handle].targets, textures[handle].name);
	}
}


void GFXTextureEnv (int stage, GFXTEXTUREENVMODES mode, float arg2) {
	if (stage&&stage>=gl_options.Multitexture) return;

	GLenum type;
	GFXActiveTexture(stage);
	switch (mode) {
		case GFXREPLACETEXTURE:
			type = GL_REPLACE;
			goto ENVMODE;
		case GFXADDTEXTURE:
			type = GL_ADD;
			goto ENVMODE;
		case GFXMODULATETEXTURE:
			type = GL_MODULATE;
			ENVMODE:
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,type);
			break;
		case GFXINTERPOLATETEXTURE:
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_ARB,GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA_ARB,GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB,GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB,GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_ALPHA_ARB,GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_ALPHA_ARB,GL_SRC_ALPHA);
			{
				GLfloat arg2v[4]= {
					0,0,0,1.0-arg2
				};
				glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,arg2v);
			}
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_INTERPOLATE_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_INTERPOLATE_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
			glTexEnvi(GL_TEXTURE_ENV,GL_ALPHA_SCALE,1);
			break;
		case GFXCOMPOSITETEXTURE:
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_CONSTANT);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_ARB,GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE2_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND2_RGB_ARB,GL_ONE_MINUS_SRC_ALPHA);
			{
				GLfloat arg2v[4]= {
					0,0,0,arg2
				};
				glTexEnvfv(GL_TEXTURE_ENV,GL_TEXTURE_ENV_COLOR,arg2v);
			}
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_INTERPOLATE_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
			glTexEnvi(GL_TEXTURE_ENV,GL_ALPHA_SCALE,1);
			break;
		case GFXADDSIGNEDTEXTURE:
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_ADD_SIGNED_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_REPLACE);
			glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,1);
			glTexEnvi(GL_TEXTURE_ENV,GL_ALPHA_SCALE,1);
			break;
		case GFXDETAILTEXTURE:
			glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_RGB_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE0_ALPHA_ARB,GL_PREVIOUS_ARB);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND0_ALPHA_ARB,GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_RGB_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_SOURCE1_ALPHA_ARB,GL_TEXTURE);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_RGB_ARB,GL_SRC_COLOR);
			glTexEnvi(GL_TEXTURE_ENV,GL_OPERAND1_ALPHA_ARB,GL_SRC_ALPHA);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_RGB_ARB,GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV,GL_COMBINE_ALPHA_ARB,GL_MODULATE);
			glTexEnvi(GL_TEXTURE_ENV,GL_RGB_SCALE_ARB,2);
			glTexEnvi(GL_TEXTURE_ENV,GL_ALPHA_SCALE,2);
			break;
	}
}


#ifndef GL_CLAMP_TO_EDGE_EXT
#define GL_CLAMP_TO_EDGE_EXT              0x812F
#endif
#ifndef GL_CLAMP_TO_BORDER_ARB
#define GL_CLAMP_TO_BORDER_ARB            0x812D
#endif

void GFXTextureWrap(int stage, GFXTEXTUREWRAPMODES mode, enum TEXTURE_TARGET target) {
	if (stage&&stage>=gl_options.Multitexture) return;

	GFXActiveTexture(stage);
	GLenum tt=GetGLTextureTarget(target);
	GLenum e1=GL_REPEAT;
	GLenum e2=0;
	switch (mode) {
		case GFXCLAMPTEXTURE: e1=GL_CLAMP; e2=GL_CLAMP_TO_EDGE_EXT; break;
		case GFXREPEATTEXTURE:e1=GL_REPEAT;e2=0; break;
		case GFXBORDERTEXTURE:e1=GL_CLAMP; e2=GL_CLAMP_TO_BORDER_ARB; break;
	}
	glTexParameteri(tt, GL_TEXTURE_WRAP_S, e1);
	if (target != TEXTURE1D) glTexParameteri(tt, GL_TEXTURE_WRAP_T, e1);
	if (target == TEXTURE3D) glTexParameteri(tt, GL_TEXTURE_WRAP_R, e1);
	if (e2) {
		glTexParameteri(tt, GL_TEXTURE_WRAP_S, e2);
		if (target != TEXTURE1D) glTexParameteri(tt, GL_TEXTURE_WRAP_T, e2);
		if (target == TEXTURE3D) glTexParameteri(tt, GL_TEXTURE_WRAP_R, e2);
	}
}