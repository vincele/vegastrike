#include <png.h>
#include <stdlib.h>

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

typedef unsigned char * (textureTransform) (int &bpp, int &color_type, unsigned int &width, unsigned int &height, unsigned char ** row_pointers);

unsigned char * heightmapTransform (int &bpp, int &color_type, unsigned int &width, unsigned int &height, unsigned char ** row_pointers) {
  unsigned short * dat = (unsigned short *) malloc (sizeof (unsigned short)*width*height);
  if ((bpp==8&&color_type==PNG_COLOR_TYPE_RGB_ALPHA)||color_type==PNG_COLOR_TYPE_GRAY ||color_type==PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (bpp==8&&color_type==PNG_COLOR_TYPE_GRAY) {
      for (unsigned int i=0;i<height;i++) {
	unsigned int iwid = i*width;
	for (unsigned int j=0;j<width;j++) {
	  dat[iwid+j] = row_pointers[i][j]*256;
	}
      }
    } else {
      if ((bpp==16&&color_type==PNG_COLOR_TYPE_GRAY)||(bpp==8&&color_type==PNG_COLOR_TYPE_GRAY_ALPHA)) {
	for (unsigned int i=0;i<height;i++) {
	  memcpy (&dat[i*width],&row_pointers[i], sizeof (unsigned short)*width);
	}
      } else {
	//type is RGBA32 or GrayA32
	for (unsigned int i=0;i<height;i++) {
	  unsigned int iwid = i*width;
	  for (unsigned int j=0;j<width;j++) {
	    dat[iwid+j]= (((unsigned short *)row_pointers[i])[j*2]);
	  }
	}
      }
    }
  } else {
    if (color_type==PNG_COLOR_TYPE_RGB) {
      unsigned int coloffset = (bpp==8)?3:6;
      for (unsigned int i=0;i<height;i++) {
	unsigned int iwid = i*width;
	for (unsigned int j=0;j<width;j++) {
	  dat[iwid+j]= * ((unsigned short *)(&(row_pointers[i][j*coloffset])));
	}
      }
      
    }else if (color_type== PNG_COLOR_TYPE_RGB_ALPHA) {///16 bit colors...take Red
      for (unsigned int i=0;i<height;i++) {
	unsigned int iwid = i*width;
	for (unsigned int j=0;j<width;j++) {
	  dat[iwid+j]= (((unsigned short *)row_pointers[i])[j*4]);
	}
      }	
    }
  }
  return (unsigned char *)dat;
}


unsigned char * readTexture (const char * name, int & bpp, int &color_type, unsigned int &width, unsigned int &height, unsigned char * &palette, textureTransform * tt) {
  png_structp png_ptr;
  unsigned char ** row_pointers;
  png_infop info_ptr;
  unsigned int sig_read = 0;
  int bit_depth, interlace_type;
   FILE *fp;
   if ((fp = fopen(name, "rb")) == NULL)
      return NULL;
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
   if (png_ptr == NULL)
   {
      fclose(fp);
      return NULL;
   }
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      return NULL;
   }
   if (setjmp(png_jmpbuf(png_ptr))) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      return NULL;
   }
   png_init_io(png_ptr, fp);
   png_set_sig_bytes(png_ptr, 0);
   
   png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND , NULL);
row_pointers = png_get_rows(png_ptr, info_ptr);
   png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *)&width, (png_uint_32 *)&height, &bpp, &color_type,
       &interlace_type, NULL, NULL);
   unsigned char * result = (*tt) (bpp,color_type,width,height,row_pointers);
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

   /* close the file */
   fclose(fp);
   return result;
}


int main () {
  const char nam []="test.png"; int  bpp; int channels; unsigned int width; unsigned int height; unsigned char * palette;
  unsigned char * tmp = readTexture(nam,bpp,channels,width,height,palette, &heightmapTransform);



  free (tmp);
  

}
