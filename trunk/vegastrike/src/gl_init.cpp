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

#include "vs_globals.h"
//#include "gl_globals.h"

#ifndef WIN32
#   include <GL/glx.h>
#include <stdlib.h>
#include "gfxlib.h"
#else
#include <windows.h>
#endif
#include <GL/gl.h>
#   include <GL/glext.h>
#include <stdio.h>
#include "gl_init.h"
#define WINDOW_TITLE "Vega Strike "VERSION
static int glutWindow;



#ifdef PFNGLLOCKARRAYSEXTPROC
PFNGLLOCKARRAYSEXTPROC glLockArraysEXT_p;
PFNGLUNLOCKARRAYSEXTPROC glUnlockArraysEXT_p;
#endif
#ifdef WIN32
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB=0;
PFNGLCLIENTACTIVETEXTUREARBPROC glActiveTextureARB=0;
PFNGLCOLORTABLEEXTPROC glColorTable=0;
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = 0;
//PFNGLSELECTTEXTURESGISPROC glSelectTextureSGIS ;
//PFNGLMULTITEXCOORD2FSGISPROC glMultiTexCoord2fSGIS ;
//PFNGLMTEXCOORDPOINTERSGISPROC glMTexCoordPointerSGIS ;
#include "gfxlib.h"

#endif
typedef void (*(*get_gl_proc_fptr_t)(const GLubyte *))(); 
#ifdef WIN32
    typedef char * GET_GL_PTR_TYP;
#define GET_GL_PROC wglGetProcAddress
    //    get_gl_proc = (get_gl_proc_fptr_t) wglGetProcAddress;
#else
    typedef GLubyte * GET_GL_PTR_TYP;
#define GET_GL_PROC glXGetProcAddressARB
    //    get_gl_proc = (get_gl_proc_fptr_t) glXGetProcAddressARB;
#endif
#include <GL/glut.h>
void init_opengl_extensions()
{
  //    get_gl_proc_fptr_t get_gl_proc;


#ifdef PFNGLLOCKARRAYSEXTPROC
    if ( glutExtensionSupported( "GL_EXT_compiled_vertex_array" ) ) {

	printf( "GL_EXT_compiled_vertex_array extension "
		     "supported" );

	glLockArraysEXT_p = (PFNGLLOCKARRAYSEXTPROC) 
	    GET_GL_PROC( (GET_GL_PTR_TYP) "glLockArraysEXT" );
	glUnlockArraysEXT_p = (PFNGLUNLOCKARRAYSEXTPROC) 
	    GET_GL_PROC( (GET_GL_PTR_TYP) "glUnlockArraysEXT" );

    } else {
	printf(  "GL_EXT_compiled_vertex_array extension "
		     "NOT supported" );
	//exit(1);  //recoverable error..I was wrong
	glLockArraysEXT_p = NULL;
	glUnlockArraysEXT_p = NULL;

    }
#endif
    if (glutExtensionSupported ("GL_SGIS_multitexture")) {
      g_game.Multitexture =1;
      //glMTexCoordPointerSGIS=(PFNGLMTEXCOORDPOINTERSGISPROC)glSelectTextureSGIS=get_gl_proc ((GLubyte*) "glMTexCoordPointerSGIS");
      //glSelectTextureSGIS = (PFNGLSELECTTEXTURESGISPROC) get_gl_proc ((GLubyte*) "glSelectTextureSGIS")
      //glMultiTexCoord2fSGIS =  (PFNGLMULTITEXCOORD2FSGISPROC)get_gl_proc ((GLubyte*) "glMTexCoord2fSGIS")
    } else {
      g_game.Multitexture = 0;
    }
    g_game.mipmap = 2;
#ifdef WIN32
    glColorTable = (PFNGLCOLORTABLEEXTPROC ) GET_GL_PROC((GET_GL_PTR_TYP)"glColorTableEXT");
    glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC) GET_GL_PROC((GET_GL_PTR_TYP)"glMultiTexCoord2fARB");
    glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC) GET_GL_PROC((GET_GL_PTR_TYP)"glClientActiveTextureARB");
    glActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC) GET_GL_PROC((GET_GL_PTR_TYP)"glActiveTextureARB");
#endif
    if (glutExtensionSupported ("GL_ARB_multitexture")) {
      g_game.Multitexture = 1;
	  printf ("Multitexture supported");
    } else {
      g_game.Multitexture =0;
	  printf ("Multitexture unsupported");
    }
    if ( glutExtensionSupported( "GL_ARB_texture_cube_map" ) ) {
      printf ("Texture Cube Map Ext Supported");
      g_game.cubemap =1;

    }
}

 static void initfov () {

    g_game.fov = 78;

    g_game.aspect = 1.33F;

    g_game.znear = 1.00F;

    g_game.zfar = 100000.00F;

    FILE * fp = fopen ("glsetup.txt","r");
    if (fp) {
      fscanf (fp,"fov %f\n",&g_game.fov);
      fscanf (fp,"aspect %f\n",&g_game.aspect);
      fscanf (fp,"znear %f\n",&g_game.znear);
      fscanf (fp,"zfar %f\n",&g_game.zfar);
      fclose (fp);
    }
 }

extern void GFXInitTextureManager();
void GFXInit (int argc, char ** argv){
    glutInit( &argc, argv );
     
#ifdef USE_STENCIL_BUFFER
    glutInitDisplayMode( GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE/* | GLUT_STENCIL*/ );
#else
    glutInitDisplayMode( GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE );
#endif

      char str [15];
      sprintf (str, "%dx%d:%d",g_game.x_resolution,g_game.y_resolution,g_game.color_depth); 
      glutGameModeString(str);

    /* Create a window */
      if (g_game.fullscreen &&glutGameModeGet(GLUT_GAME_MODE_POSSIBLE)) {
	glutInitWindowPosition( 0, 0 );
	glutEnterGameMode();
    } else {
	/* Set the initial window size */
	glutInitWindowSize(g_game.x_resolution, g_game.y_resolution );
	glutInitWindowPosition( 0, 0 );
	glutWindow = glutCreateWindow( "Vegastrike " );
	if ( glutWindow == 0 ) {
	    fprintf( stderr, "Couldn't create a window.\n" );
	    exit(1);
	} 
    }
    /* Ingore key-repeat messages */
    glutIgnoreKeyRepeat(1);
    glViewport (0, 0, g_game.x_resolution,g_game.y_resolution);
    glClearColor ((float)0.0, (float)0.0, (float)1.0, (float)0);
    initfov();
    glShadeModel (GL_SMOOTH);
    glEnable (GL_CULL_FACE);
	//glDisable (GL_CULL_FACE);
    glCullFace (GL_BACK);
	//glCullFace (GL_FRONT);
    glShadeModel (GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc (GL_LEQUAL);
	//glDepthFunc (GL_LESS);

    //glEnable(TEXTURE0_SGIS);
    //glEnable(TEXTURE1_SGIS);

    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0);


    init_opengl_extensions();
    GFXInitTextureManager();
    
    if (g_game.Multitexture)
      GFXActiveTexture(0);
    glEnable(GL_TEXTURE_2D);
        glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glEnable(GL_SHARED_TEXTURE_PALETTE_EXT);
    //int retval= glGetError();
    if ( !glutExtensionSupported( "GL_EXT_color_table" ) ) {
      g_game.PaletteExt = 0;
      printf ("Palette Not Supported");
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    if (g_game.Multitexture){
      GFXActiveTexture(1);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#ifdef NV_CUBE_MAP

#else
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif

      //glDisable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_SWAP_BYTES, 0);
      glPixelStorei(GL_PACK_ROW_LENGTH, 256);

      // Spherical texture coordinate generation
#ifdef NV_CUBE_MAP
      glEnable(GL_TEXTURE_CUBE_MAP_EXT);
      glTexGeni(GL_S,GL_TEXTURE_GEN_MODE,GL_REFLECTION_MAP_NV);
      glTexGeni(GL_T,GL_TEXTURE_GEN_MODE,GL_REFLECTION_MAP_NV);
      glTexGeni(GL_R,GL_TEXTURE_GEN_MODE,GL_REFLECTION_MAP_NV);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      glEnable(GL_TEXTURE_GEN_R);

#else
      glEnable(GL_TEXTURE_2D);
      glTexGenf(GL_S,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
      glTexGenf(GL_T,GL_TEXTURE_GEN_MODE,GL_SPHERE_MAP);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
#endif
    }
    glClearDepth(1);
    glEnable (GL_BLEND);
    glDisable (GL_ALPHA_TEST);
    //glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //GFXBlendMode (SRCALPHA, INVSRCALPHA);
    GFXBlendMode (ONE, ZERO);
    
    glColor3f(0,0,0);
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode (GL_TEXTURE);
    glLoadIdentity(); //set all matricies to identity
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity();
//	gluPerspective (78,1.33,0.5,20); //set perspective to 78 degree FOV
//	glPushMatrix();

    glEnable(GL_LIGHTING);
    
    glDisable(GL_NORMALIZE);
    int con;
    GFXCreateLightContext (con);

    //FIXME VEGASTRIKE //GFXLoadIdentity(MODEL);
    //FIXME VEGASTRIKE //GFXLoadIdentity(VIEW);
    //FIXME VEGASTRIKE //GFXLoadIdentity(PROJECTION);


    glutSetCursor(GLUT_CURSOR_NONE );
    //glutSetCursor(GLUT_CURSOR_INHERIT );
    //GFXPerspective(78,1.33,0.5,20);

}

void GFXLoop(void main_loop()) {
  glutDisplayFunc(main_loop);
  glutIdleFunc (main_loop);
  glutMainLoop();
  //never make it here;

}
extern void GFXDestroyAllLights();

void GFXShutdown () {
  GFXDestroyAllTextures();
  GFXDestroyAllLights();
  if ( g_game.fullscreen ) {
    glutLeaveGameMode();
  }
}

