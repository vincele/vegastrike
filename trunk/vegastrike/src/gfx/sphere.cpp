#include "sphere.h"
#include "ani_texture.h"
#include "vegastrike.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "vs_path.h"
#include "xml_support.h"
#ifndef M_PI
#define M_PI 3.1415926536F
#endif

using XMLSupport::tostring;
int pixelscalesize=30;
float SphereMesh::GetT (float rho, float rho_min, float rho_max) {
 return 1-(rho-rho_min)/ (rho_max-rho_min);  
}
float SphereMesh::GetS (float theta, float theta_min, float theta_max) {
  return 1-(theta-theta_min)/ (theta_max-theta_min);
}
float CityLights::GetT (float rho, float rho_min, float rho_max) {
  return wrapy*SphereMesh::GetT(rho,rho_min,rho_max);
}

float CityLights::GetS (float theta, float theta_min, float theta_max) {
  
  return wrapx * SphereMesh::GetS(theta,theta_min,theta_max);
}
void SphereMesh::InitSphere(float radius, int stacks, int slices, const char *texture, const char *alpha,bool Insideout,  const BLENDFUNC a, const BLENDFUNC b, bool envMapping, float rho_min, float rho_max, float theta_min, float theta_max, FILTER mipmap){
  int numspheres = (stacks+slices)/8;
  if (numspheres<1)
    numspheres =1;
  Mesh *oldmesh;
  char ab[3];
  ab[2]='\0';
  ab[1]=b+'0';
  ab[0]=a+'0';
  hash_name = string("@@Sphere") + "#" + texture + "#" + XMLSupport::tostring(stacks) + "#" + XMLSupport::tostring(slices) +  ab + "#" + XMLSupport::tostring(rho_min) + "#" + XMLSupport::tostring(rho_max);
  if (LoadExistant (hash_name,radius)) {
    return;
  } else {

  }
  oldmesh = AllocNewMeshesEachInSizeofMeshSpace(numspheres);//FIXME::RISKY::MIGHT HAVE DIFFERENT SIZES!! DON"T YOU DARE ADD XTRA VARS TO SphereMesh calsshave to!
  numlods=numspheres;
  
  meshHashTable.Put (hash_name=GetSharedMeshHashName(hash_name,radius), oldmesh);
  //  fprintf (stderr,"\nput %s\n",hash_name.c_str());
  this->orig = oldmesh;
  radialSize = radius;//MAKE SURE FRUSTUM CLIPPING IS DONE CORRECTLY!!!!!
  mn = Vector (-radialSize,-radialSize,-radialSize);
  mx = Vector (radialSize,radialSize,radialSize);
  vector <MeshDrawContext> *odq=NULL;
  for (int l=0;l<numspheres;l++) {
    
    draw_queue = new vector<MeshDrawContext>;
    if (!odq)
      odq = draw_queue;
    //    stacks = origst/(l+1);
    //slices = origsl/(l+1);
    if (stacks>12) {
      stacks -=4;
      slices-=4;
    } else {
      stacks-=2;
      slices-=2;
    }
    float rho, drho, theta, dtheta;
    float x, y, z;
    float s, t, ds, dt;
    int i, j, imin, imax;
    float nsign = Insideout?-1.0:1.0;
    int fir=0;//Insideout?1:0;
    int sec=1;//Insideout?0:1;
    vlist = NULL;
    /* Code below adapted from gluSphere */
    drho = (rho_max-rho_min)/ (GLfloat) stacks;
    dtheta = (theta_max-theta_min)/ (GLfloat) slices;
    
    ds = 1.0 / slices;
    dt = 1.0 / stacks;
      t = 1.0;			/* because loop now runs from 0 */
      
      imin = 0;
      imax = stacks;
      
      int numQuadstrips = stacks;
      //      numQuadstrips = 0;
      int *QSOffsets = new int [numQuadstrips];
      
      // draw intermediate stacks as quad strips 
      int numvertex=stacks*(slices+1)*2;
      GFXVertex *vertexlist = new GFXVertex[numvertex];
    
      GFXVertex *vl = vertexlist;
      enum POLYTYPE *modes= new enum POLYTYPE [numQuadstrips];   
      /*   SetOrientation(Vector(1,0,0),
	   Vector(0,0,-1),
	   Vector(0,1,0));//that's the way prop*///taken care of in loading
      
      
      for (i = imin; i < imax; i++) {
	GFXVertex *vertexlist = vl + (i * (slices+1)*2);
	rho = i * drho + rho_min;
	
	s = 0.0;
	for (j = 0; j <= slices; j++) {
	  theta = j*dtheta;//(j == slices) ? theta_min * 2 * M_PI : j * dtheta;
	  x = -sin(theta) * sin(rho);
	  y = cos(theta) * sin(rho);
	  z = nsign * cos(rho);
	
	  vertexlist[j*2+fir].i = x ;
	  vertexlist[j*2+fir].k = -y;
	  vertexlist[j*2+fir].j = z;
	  vertexlist[j*2+fir].s = GetS(theta,theta_min,theta_max);//1-s;//insideout?1-s:s;
	  vertexlist[j*2+fir].t = GetT(rho,rho_min,rho_max);//t;
	  vertexlist[j*2+fir].x = x * radius;
	  vertexlist[j*2+fir].z = -y * radius;
	  vertexlist[j*2+fir].y = z * radius;

	  
	  x = -sin(theta) * sin(rho + drho);
	  y = cos(theta) * sin(rho + drho);
	  z = nsign * cos(rho + drho);

	  vertexlist[j*2+sec].i = x ;
	  vertexlist[j*2+sec].k = -y;
	  vertexlist[j*2+sec].j = z;//double negative 
	  vertexlist[j*2+sec].s = GetS (theta,theta_min,theta_max);//1-s;//insideout?1-s:s;
	  vertexlist[j*2+sec].t = GetT(rho+drho,rho_min,rho_max);//t - dt;
	  vertexlist[j*2+sec].x = x * radius;
	  vertexlist[j*2+sec].z = -y * radius;
	  vertexlist[j*2+sec].y = z * radius;
	
	  s += ds;
	}
	
	t -= dt;
	QSOffsets[i]= (slices+1)*2;
	modes[i]=GFXQUADSTRIP;
      }
      
      vlist = new GFXVertexList(modes,numvertex, vertexlist, numQuadstrips ,QSOffsets);
      delete [] vertexlist;
      delete [] modes;
      delete [] QSOffsets;
      SetBlendMode (a,b);
      int texlen = strlen (texture);
      bool found_texture=false;
      if (texlen>3) {
	if (texture[texlen-1]=='i'&&texture[texlen-2]=='n'&&
	    texture[texlen-3]=='a'&&texture[texlen-4]=='.') {
	  found_texture = true;
	  Decal = new AnimatedTexture (texture,0,mipmap);
	}

      }
      if (!found_texture) {
	if (alpha) {
	  Decal = new Texture(texture, alpha,0,mipmap,TEXTURE2D,TEXTURE_2D,1,0,(Insideout||g_game.use_planet_textures)?GFXTRUE:GFXFALSE);
	  
	}else {
	  Decal = new Texture (texture,0,mipmap,TEXTURE2D,TEXTURE_2D,(Insideout||g_game.use_planet_textures)?GFXTRUE:GFXFALSE);
	}
      }
      Insideout?setEnvMap(GFXFALSE):setEnvMap(envMapping);
      
      if(Insideout) {
      draw_sequence=0;
      }
      
      Mesh * oldorig = orig;
      refcount=1;
      orig=NULL;
      if (l>=1) {
	lodsize=(numspheres+1-l)*pixelscalesize;
	if (l==1) {
	  lodsize*=2;
	}else if (l==2) {
	  lodsize*=1.75;
	} else if (l==3) {
	  lodsize*=1.5;
	}
      }
      oldmesh[l]=*this;
      refcount =0;
      orig = oldorig;
      lodsize = FLT_MAX;
  }
  draw_queue = odq;
}
void SphereMesh::Draw(float lod,  bool centered, const Matrix &m) {
  if (centered) {
    Matrix m1(m);
    //float m1[16];
    //memcpy (m1,m,sizeof (float)*16);
    m1.p = QVector(_Universe.AccessCamera()->GetPosition().Transform(m1));
    Mesh::Draw (lod,m1);
  } else {	
    Mesh::Draw(lod,m);
  } 
}
static GFXColor getSphereColor () {
  float color[4];
  vs_config->getColor ("planet_ambient",color);
  GFXColor tmp (color[0],color[1],color[2],color[3]);
  return tmp;
}
void SphereMesh::ProcessDrawQueue(int whichdrawqueue) {
  static GFXColor spherecol (getSphereColor ());
  if (blendSrc!=ONE||blendDst!=ZERO) {
    GFXPolygonOffset (0,-2);
  }
  GFXColor tmpcol (0,0,0,1);
  GFXGetLightContextAmbient(tmpcol);
  GFXLightContextAmbient(spherecol);
  Mesh::ProcessDrawQueue (whichdrawqueue);
  GFXLightContextAmbient(tmpcol);
  GFXPolygonOffset (0,0);
    

}

void CityLights::ProcessDrawQueue(int whichdrawqueue) {
  GFXPolygonOffset (0,-1);
  const GFXColor citycol (1,1,1,1);
  GFXColor tmpcol (0,0,0,1);
  GFXGetLightContextAmbient(tmpcol);
  GFXLightContextAmbient(citycol);
  Mesh::ProcessDrawQueue (whichdrawqueue);
  GFXLightContextAmbient(tmpcol);
  GFXPolygonOffset (0,0);
}
float CityLights::wrapx=1;
float CityLights::wrapy=1;

CityLights::CityLights (float radius, int stacks, int slices, const char *texture, int zzwrapx, int zzwrapy,  bool insideout, const BLENDFUNC a, const BLENDFUNC b, bool envMap, float rho_min, float rho_max, float theta_min, float theta_max):SphereMesh() { 
  wrapx = zzwrapx;
  wrapy = zzwrapy;
  /*    if (texture!=NULL) {
      string wrap = string(texture);
      int pos =wrap.find ("wrapx");
      if (pos!=string::npos) {
	string Wrapx = wrap.substr (pos+5,wrap.length());
	sscanf(Wrapx.c_str(),"%f",&wrapx);
	pos = Wrapx.find ("wrapy");
	if (pos!=string::npos) {
	  string Wrapy = Wrapx.substr (pos+5,Wrapx.length());
	  sscanf (Wrapy.c_str(),"%f",&wrapy);
	}
      }
    }*/
    FILTER filter = (FILTER)XMLSupport::parse_int(vs_config->getVariable ("graphics","CityLightFilter",XMLSupport::tostring(((int)TRILINEAR))));    
    InitSphere(radius,stacks,slices,texture,NULL,insideout,a,b,envMap,rho_min,rho_max,theta_min,theta_max,filter);

}
