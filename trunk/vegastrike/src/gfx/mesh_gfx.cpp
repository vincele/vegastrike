#include <algorithm>
#include "mesh.h"
#include "aux_texture.h"
#include "aux_logo.h"
#include "lin_time.h"
#include "configxml.h"
#include "vs_globals.h"
#include "cmd/nebula_generic.h"
#include "gfx/camera.h"
#include "gfx/animation.h"

extern vector<Logo*> undrawn_logos;
class OrigMeshContainer {
public:
  float d;
  Mesh * orig;
  OrigMeshContainer(){ orig=NULL; };
  OrigMeshContainer (Mesh * tmp, float d) {
    orig = tmp;
    this->d = d;
  }
  bool operator < (const OrigMeshContainer & b) const {
    if(orig->Decal[0]==NULL || b.orig->Decal[0]==NULL){
      cout << "DEcal is nulll" << endl;
      return b.orig->Decal[0]!=NULL;
    }
    return ((*orig->Decal[0]) < (*b.orig->Decal[0]));
  }
  bool operator == (const OrigMeshContainer &b) const {
    return (*orig->Decal[0])==*b.orig->Decal[0];
  }
};
class MeshCloser { 
public:
  MeshCloser () {}
  ///approximate closness based on center o matrix (which is gonna be center for spheres and convex objects most likely)
  bool operator () (const OrigMeshContainer & a, const OrigMeshContainer & b) {
    //    return a.d+a.orig->rSize() > b.d+b.orig->rSize();//draw from outside in :-)
    return a.d > b.d;//draw from outside in :-)
  }
};

typedef std::vector<OrigMeshContainer> OrigMeshVector;
OrigMeshVector undrawn_meshes[NUM_MESH_SEQUENCE]; // lower priority means draw first

Texture * Mesh::TempGetTexture (int index, std::string factionname)const {
    Texture *tex=NULL;
    assert (index<(int)xml->decals.size());
    XML::ZeTexture * zt = &(xml->decals[index]);
    if (zt->animated_name.length()) {
        string tempani = factionname+"_"+zt->animated_name;
        tex = new AnimatedTexture (tempani.c_str(),0,BILINEAR);
        if (!tex->LoadSuccess()) {
            delete tex;
            tex = new AnimatedTexture (zt->animated_name.c_str(),0,BILINEAR);
        }
    }else if (zt->decal_name.length()==0) {
        tex = NULL;
    } else {
        if (zt->alpha_name.length()==0) {
            string temptex = factionname+"_"+zt->decal_name;
            tex = new Texture(temptex.c_str(),0,MIPMAP,TEXTURE2D,TEXTURE_2D,(g_game.use_ship_textures||xml->force_texture)?GFXTRUE:GFXFALSE);
            if (!tex->LoadSuccess()) {
                delete tex;
                tex = new Texture(zt->decal_name.c_str(),0,MIPMAP,TEXTURE2D,TEXTURE_2D,(g_game.use_ship_textures||xml->force_texture)?GFXTRUE:GFXFALSE);
            }
        }else {
            string temptex = factionname+"_"+zt->decal_name;
            string tempalp = factionname+"_"+zt->alpha_name;
            tex = new Texture(temptex.c_str(), tempalp.c_str(),0,MIPMAP,TEXTURE2D,TEXTURE_2D,1,0,(g_game.use_ship_textures||xml->force_texture)?GFXTRUE:GFXFALSE);
            if (!tex->LoadSuccess()) {
                delete tex;
                tex = new Texture(zt->decal_name.c_str(), zt->alpha_name.c_str(),0,MIPMAP,TEXTURE2D,TEXTURE_2D,1,0,(g_game.use_ship_textures||xml->force_texture)?GFXTRUE:GFXFALSE);
            }

        }
    }
    return tex;
    
}
Texture * createTexture( const char * filename, int stage=0,enum FILTER f1= MIPMAP,enum TEXTURE_TARGET t0=TEXTURE2D,enum TEXTURE_IMAGE_TARGET t=TEXTURE_2D,unsigned char c=GFXFALSE,int i=65536)
{
	return new Texture( filename, stage, f1, t0, t, c, i);
}
Logo * createLogo(int numberlogos,Vector* center, Vector* normal, float* sizes, float* rotations, float offset, Texture * Dec, Vector *Ref)
{
	return new Logo(numberlogos,center,normal,sizes ,rotations, offset, Dec ,Ref);
}
Texture * createTexture( char const * ccc,char const * cc,int k= 0,enum FILTER f1= MIPMAP,enum TEXTURE_TARGET t0=TEXTURE2D,enum TEXTURE_IMAGE_TARGET t=TEXTURE_2D,float f=1,int j=0,unsigned char c=GFXFALSE,int i=65536)
{
	return new Texture( ccc, cc, k, f1, t0, t, f, j, c, i);
}
AnimatedTexture * createAnimatedTexture( char const * c,int i,enum FILTER f)
{
	return new AnimatedTexture( c, i, f);
}

Mesh::~Mesh()
{
	if(!orig||orig==this)
	{
	  for (int j=0;j<NUM_MESH_SEQUENCE;j++) {
	    for (unsigned int i=0;i<undrawn_meshes[j].size();i++) {
	      if (undrawn_meshes[j][i].orig==this) {
		undrawn_meshes[j].erase(undrawn_meshes[j].begin()+i);
		i--;
		fprintf (stderr,"stale mesh found in draw queue--removed!\n");
	      }
	    }
	  }
	  delete vlist;
	  for (unsigned int i=0;i<Decal.size();i++) {
	    if(Decal[i] != NULL) {
	      delete Decal[i];
	      Decal[i] = NULL;
	    }
	  }
	  if (squadlogos!=NULL) {
	    delete squadlogos;
	    squadlogos= NULL;
	  }
	  if (forcelogos!=NULL) {
	    delete forcelogos;
	    forcelogos = NULL;
	  }
	  if (meshHashTable.Get(hash_name)==this)
	    meshHashTable.Delete(hash_name);
	  
	  if(draw_queue!=NULL)
	    delete draw_queue;
	} else {
	  orig->refcount--;
	  //printf ("orig refcount: %d",refcount);
	  if(orig->refcount == 0) {
	    delete [] orig;	      
	  }
	}
}
void Mesh::Draw(float lod, const Matrix &m, float toofar, short cloak, float nebdist)
{
  //  Vector pos (local_pos.Transform(m));
  MeshDrawContext c(m);
  UpdateFX(GetElapsedTime());
  c.SpecialFX = &LocalFX;
  static float too_far_dist = XMLSupport::parse_float (vs_config->getVariable ("graphics","mesh_far_percent",".8"));
  //c.mesh_seq=((toofar+rSize()>too_far_dist*g_game.zfar)/*&&draw_sequence==0*/)?NUM_ZBUF_SEQ:draw_sequence;
  c.mesh_seq=((toofar+((MeshType()==1)?0:rSize())>too_far_dist*g_game.zfar)/*&&draw_sequence==0*/)?NUM_ZBUF_SEQ:draw_sequence;
  c.cloaked=MeshDrawContext::NONE;
  if (nebdist<0) {
    c.cloaked|=MeshDrawContext::FOG;
  }
  if (cloak>=0) {
    c.cloaked|=MeshDrawContext::CLOAK;
    if ((cloak&0x1)) {
      c.cloaked |= MeshDrawContext::GLASSCLOAK;
      c.mesh_seq=MESH_SPECIAL_FX_ONLY;//draw near the end with lights
    } else {
      c.mesh_seq =2;
    }
    if (cloak<16384) {
      c.cloaked|=MeshDrawContext::NEARINVIS;
    }
    float tmp = ((float)cloak)/32767;
    c.CloakFX.r = (c.cloaked&MeshDrawContext::GLASSCLOAK)?tmp:1;
    c.CloakFX.g = (c.cloaked&MeshDrawContext::GLASSCLOAK)?tmp:1;
    c.CloakFX.b = (c.cloaked&MeshDrawContext::GLASSCLOAK)?tmp:1;
    c.CloakFX.a = tmp;
    /*
    c.CloakNebFX.ambient[0]=((float)cloak)/32767;
    c.CloakNebFX.ag=((float)cloak)/32767;
    c.CloakNebFX.ab=((float)cloak)/32767;
    c.CloakNebFX.aa=((float)cloak)/32767;
    */
    ///all else == defaults, only ambient
  } 
  //  c.mat[12]=pos.i;
  //  c.mat[13]=pos.j;
  //  c.mat[14]=pos.k;//to translate to local_pos which is now obsolete!
  Mesh *origmesh = getLOD (lod);
  origmesh->draw_queue->push_back(c);
  if(!(origmesh->will_be_drawn&(1<<c.mesh_seq))) {
    origmesh->will_be_drawn |= (1<<c.mesh_seq);
    //    fprintf (stderr,"origmesh %x",origmesh);
    undrawn_meshes[c.mesh_seq].push_back(OrigMeshContainer(origmesh,toofar-rSize()));//FIXME will not work if many of hte same mesh are blocking each other
  }
  will_be_drawn |= (1<<c.mesh_seq);
}
void Mesh::DrawNow(float lod,  bool centered, const Matrix &m, short cloak, float nebdist) {
  Mesh *o = getLOD (lod);
  //fixme: cloaking not delt with.... not needed for backgroudn anyway
  if (nebdist<0) {
    Nebula * t=_Universe->AccessCamera()->GetNebula();
    if (t) {
      t->SetFogState();
    }
  } else {
    GFXFogMode(FOG_OFF);
  }
  if (centered) {
    //    Matrix m1 (m);
    //Vector pos(_Universe->AccessCamera()->GetPosition().Transform(m1));
    //m1[12]=pos.i;
    //m1[13]=pos.j;
    //m1[14]=pos.k;
    GFXCenterCamera (true);
    GFXLoadMatrixModel (m);    
  } else {	
    if (o->draw_sequence!=MESH_SPECIAL_FX_ONLY) {
      GFXLoadIdentity(MODEL);
      GFXPickLights (Vector (m.p.i,m.p.j,m.p.k),rSize());
    }
    GFXLoadMatrixModel (m);
  } 
  vector <int> specialfxlight;
  unsigned int i;
  for ( i=0;i<LocalFX.size();i++) {
    int ligh;
    GFXCreateLight (ligh,(LocalFX)[i],true);
    specialfxlight.push_back(ligh);
  }
  GFXSelectMaterial(o->myMatNum);
  if (blendSrc!=SRCALPHA&&blendDst!=ZERO) 
    GFXDisable(DEPTHWRITE);
  GFXBlendMode(blendSrc, blendDst);
  if (o->Decal[0])
    o->Decal[0]->MakeActive();
  o->vlist->DrawOnce();
  if (centered) {
    GFXCenterCamera(false);
  }
  for ( i=0;i<specialfxlight.size();i++) {
    GFXDeleteLight (specialfxlight[i]);
  }
  if (cloak>=0&&cloak<32767) {
    GFXEnable (TEXTURE1);
  }
}
static GFXColor getMeshColor () {
   float color[4];
  vs_config->getColor ("unit", "ship_ambient",color);
  GFXColor tmp (color[0],color[1],color[2],color[3]);
  return tmp;
}
void Mesh::ProcessZFarMeshes () {
  static GFXColor meshcolor (getMeshColor());
  GFXLightContextAmbient(meshcolor);
  _Universe->AccessCamera()->UpdateGFX (GFXFALSE, GFXFALSE);
  GFXEnable(LIGHTING);
  GFXEnable(CULLFACE);
  GFXDisable (DEPTHTEST);
  GFXDisable (DEPTHWRITE);
  ///sort meshes  
  //std::sort<OrigMeshVector::iterator,MeshCloser>(undrawn_meshes[NUM_ZBUF_SEQ].begin(),undrawn_meshes[NUM_ZBUF_SEQ].end(),MeshCloser());
  std::sort(undrawn_meshes[NUM_ZBUF_SEQ].begin(),undrawn_meshes[NUM_ZBUF_SEQ].end(),MeshCloser());
  for (OrigMeshVector::iterator i=undrawn_meshes[NUM_ZBUF_SEQ].begin();i!=undrawn_meshes[NUM_ZBUF_SEQ].end();i++) {
    i->orig->ProcessDrawQueue (NUM_ZBUF_SEQ);
    i->orig->will_be_drawn &= (~(1<<NUM_ZBUF_SEQ));
  }
  undrawn_meshes[NUM_ZBUF_SEQ].clear();
  GFXFogMode(FOG_OFF);
  Animation::ProcessFarDrawQueue(-FLT_MAX);
  _Universe->AccessCamera()->UpdateGFX (GFXTRUE, GFXFALSE);
  GFXEnable (DEPTHTEST);
  GFXEnable (DEPTHWRITE);
}

void Mesh::ProcessUndrawnMeshes(bool pushSpecialEffects) {
  static GFXColor meshcolor (getMeshColor());
  GFXLightContextAmbient(meshcolor);
  GFXEnable(DEPTHWRITE);
  GFXEnable(DEPTHTEST);
  GFXEnable(LIGHTING);
  GFXEnable(CULLFACE);

  for(int a=0; a<NUM_ZBUF_SEQ; a++) {
    if (a==MESH_SPECIAL_FX_ONLY) {
      
      GFXPushGlobalEffects();
      GFXDisable(DEPTHWRITE);
    } else {
    }
    if (!undrawn_meshes[a].empty()) {
      // shouldn't the sort - if any - be placed here??
      std::sort(undrawn_meshes[a].begin(),undrawn_meshes[a].end());//sort by texture address
      undrawn_meshes[a].back().orig->vlist->LoadDrawState();
    }
    while(!undrawn_meshes[a].empty()) {
      Mesh *m = undrawn_meshes[a].back().orig;
      undrawn_meshes[a].pop_back();
      m->ProcessDrawQueue(a);
      m->will_be_drawn &= (~(1<<a));
    }
    if (a==MESH_SPECIAL_FX_ONLY) {
      if (!pushSpecialEffects) {
	GFXPopGlobalEffects();
      }
      GFXEnable(DEPTHWRITE);
    }
  
    while(undrawn_logos.size()) {
      Logo *l = undrawn_logos.back();
      undrawn_logos.pop_back();
      l->ProcessDrawQueue();
      l->will_be_drawn = false;
    }
  }
}
void Mesh::RestoreCullFace (int whichdrawqueue) {
  if (blendDst!=ZERO &&whichdrawqueue!=NUM_ZBUF_SEQ||getCullFaceForcedOff()) {
    if (blendSrc!=SRCALPHA) {
      GFXEnable (CULLFACE);
    }
  }
}
void Mesh::SelectCullFace (int whichdrawqueue) {
  if (whichdrawqueue==NUM_ZBUF_SEQ) {
    GFXEnable(CULLFACE);
  }else {
    if (getCullFaceForcedOn()) {
      GFXEnable(CULLFACE);
    }else if (getCullFaceForcedOff()) {
      GFXDisable(CULLFACE);
    }
  }
  if (blendDst!=ZERO&&whichdrawqueue!=NUM_ZBUF_SEQ) {
    //    
    GFXDisable(DEPTHWRITE);
    if (blendSrc!=SRCALPHA||getCullFaceForcedOn()) {
      GFXDisable(CULLFACE);
    }
  }
  
}
void SetupCloakState (char cloaked,const GFXColor & CloakFX, vector <int> &specialfxlight) {
    if (cloaked&MeshDrawContext::CLOAK) {
        GFXPushBlendMode ();
        if (cloaked&MeshDrawContext::GLASSCLOAK) {
            GFXDisable (TEXTURE1);
            int ligh;
            GFXCreateLight (ligh,GFXLight (true,GFXColor(0,0,0,1),GFXColor (0,0,0,1),GFXColor (0,0,0,1),CloakFX,GFXColor(1,0,0)),true);
            specialfxlight.push_back (ligh);
            GFXBlendMode (ONE,ONE);
        }else {
            if (cloaked&MeshDrawContext::NEARINVIS) {      
                //NOT sure I like teh jump this produces	GFXDisable (TEXTURE1);
            }
            GFXBlendMode (SRCALPHA, INVSRCALPHA);
            GFXColorMaterial (AMBIENT|DIFFUSE);
            GFXColorf(CloakFX);
        }
    }
}
static void RestoreCloakState (char cloaked, bool envMap) {
    if (cloaked&MeshDrawContext::CLOAK) {
        GFXColorMaterial (0);
        if (envMap)
            GFXEnable (TEXTURE1);
        GFXPopBlendMode ();
    }
}
static void SetupFogState (char cloaked) {
    if (cloaked&MeshDrawContext::FOG) {
        Nebula *t=_Universe->AccessCamera()->GetNebula();
        if (t) {
            t->SetFogState();
        }
    } else {
        GFXFogMode (FOG_OFF);
    }    
}
bool SetupSpecMapFirstPass (vector <Texture *> &decal, unsigned int mat, bool envMap) {
    bool retval=false;
    if (decal.size()>1) {
        if (decal[1]) {
	  
            GFXSelectMaterialHighlights(mat,
                                        GFXColor(1,1,1,1),
                                        GFXColor(1,1,1,1),
                                        GFXColor(0,0,0,0),
                                        GFXColor(0,0,0,0));
            retval=true;
            if (envMap)
                GFXDisable(TEXTURE1);
            if (decal[0])
                decal[0]->MakeActive();
        }
    }
    return retval;
}
void SetupSpecMapSecondPass(Texture * decal,unsigned int mat,BLENDFUNC blendsrc, bool envMap, const GFXColor &cloakFX) {
    GFXSelectMaterialHighlights(mat,
                                GFXColor(0,0,0,0),
                                GFXColor(0,0,0,0),
				cloakFX,
                                (envMap&&GFXMultiTexAvailable())?GFXColor (1,1,1,1):GFXColor(0,0,0,0));
    GFXBlendMode (blendsrc,ONE);
    decal->MakeActive();
    float a,b;
    GFXGetPolygonOffset(&a,&b);
    GFXPolygonOffset (a, b-1);
    GFXDisable(DEPTHWRITE);
    if (envMap){
      GFXActiveTexture(1);
      GFXTextureAddOrModulate(1,true); 
      GFXEnable(TEXTURE1);
    }
}
void SetupGlowMapThirdPass(Texture * decal,unsigned int mat,BLENDFUNC blendsrc, const GFXColor &cloakFX) {
    GFXSelectMaterialHighlights(mat,
                                GFXColor(0,0,0,0),
                                GFXColor(0,0,0,0),
								GFXColor(0,0,0,0),
                                cloakFX);
    GFXBlendMode (blendsrc,ONE);
    decal->MakeActive();
    float a,b;
    GFXGetPolygonOffset(&a,&b);
    GFXPolygonOffset (a, b-2);
    GFXDisable(DEPTHWRITE);
	GFXDisable(TEXTURE1);
}
void RestoreGlowMapState(bool write_to_depthmap) { 
  float a,b;
    GFXGetPolygonOffset(&a,&b);
    GFXPolygonOffset (a, b+2);
	if (write_to_depthmap) {
		GFXEnable(DEPTHWRITE);
	}
	GFXEnable(TEXTURE1);
}

void RestoreSpecMapState(bool envMap, bool write_to_depthmap) { 
  float a,b;
    GFXGetPolygonOffset(&a,&b);
    GFXPolygonOffset (a, b+1);
    if (envMap) {
      GFXActiveTexture(1);
      GFXTextureAddOrModulate(1,false); //restore modulate
    }
    if (write_to_depthmap) {
        GFXEnable(DEPTHWRITE);
    }
}
void Mesh::ProcessDrawQueue(int whichdrawqueue) {
  //  assert(draw_queue->size());

  if (draw_queue->empty()) {
    fprintf (stderr,"cloaking queues issue! Report to hellcatv@hotmail.com\nn%d\n%s",whichdrawqueue,hash_name.c_str());
    return;
  }
  if (whichdrawqueue==NUM_ZBUF_SEQ) {
	  for (unsigned int i=0;i<draw_queue->size();i++) {
		MeshDrawContext * c = &((*draw_queue)[i]);
	    if (c->mesh_seq==whichdrawqueue) {
	      Animation::ProcessFarDrawQueue ((_Universe->AccessCamera()->GetPosition()-c->mat.p).Magnitude()/*+this->radialSize*/);		
		}
	  }
      GFXEnable(LIGHTING);
  }

  if (getLighting()) {
    GFXSelectMaterial(myMatNum);
    GFXEnable (LIGHTING);
  }else {
    GFXDisable (LIGHTING);
    GFXColor4f(1,1,1,1);
  }
  bool write_to_depthmap=!(blendDst!=ZERO&&whichdrawqueue!=NUM_ZBUF_SEQ);
  if (!write_to_depthmap) {
    GFXDisable(DEPTHWRITE);
  }
  SelectCullFace(whichdrawqueue);
  GFXBlendMode(blendSrc, blendDst);
  GFXEnable(TEXTURE0);
  if(Decal[0])
    Decal[0]->MakeActive();
  GFXSelectTexcoordSet(0, 0);
  if(getEnvMap()) {
    GFXEnable(TEXTURE1);
    _Universe->activateLightMap();
    GFXSelectTexcoordSet(1, 1);
  } else {
    GFXDisable(TEXTURE1);
  }
  vlist->BeginDrawState();	
  vector<MeshDrawContext> tmp_draw_queue;
  while(draw_queue->size()) {	  
    MeshDrawContext c = draw_queue->back();
    draw_queue->pop_back();
    if (c.mesh_seq!=whichdrawqueue) {
      tmp_draw_queue.push_back (c);
      continue;
    }
    if (whichdrawqueue!=MESH_SPECIAL_FX_ONLY) {
      GFXLoadIdentity(MODEL);
      GFXPickLights (Vector (c.mat.p.i,c.mat.p.j,c.mat.p.k),rSize());
    }
    vector <int> specialfxlight;
    GFXLoadMatrixModel ( c.mat);
    SetupCloakState (c.cloaked,c.CloakFX,specialfxlight);
    unsigned int i;
    for ( i=0;i<c.SpecialFX->size();i++) {
      int ligh;
      GFXCreateLight (ligh,(*c.SpecialFX)[i],true);
      specialfxlight.push_back(ligh);
    }
    SetupFogState(c.cloaked);
    const bool SpecMap = SetupSpecMapFirstPass (Decal,myMatNum,getEnvMap());
	bool GlowMap = Decal.size()>2;
	if (GlowMap)
		GlowMap = Decal[2]!=NULL;
    vlist->Draw();
	if (SpecMap||GlowMap) {
		GFXPushBlendMode();
	}
    if (SpecMap) {
        SetupSpecMapSecondPass(Decal[1],myMatNum,blendSrc,getEnvMap(), c.CloakFX);
	        vlist->Draw();
        RestoreSpecMapState(getEnvMap(),write_to_depthmap);

    }
	if (GlowMap) {
		SetupGlowMapThirdPass (Decal[2],myMatNum,ONE,c.CloakFX);
			vlist->Draw();
		RestoreGlowMapState(write_to_depthmap);
	}
	if (SpecMap||GlowMap) {
		GFXPopBlendMode();
	}
    for ( i=0;i<specialfxlight.size();i++) {
      GFXDeleteLight (specialfxlight[i]);
    }
    RestoreCloakState(c.cloaked,getEnvMap());
    
    if(0!=forcelogos&&!(c.cloaked&MeshDrawContext::NEARINVIS)) {
      forcelogos->Draw(c.mat);
    }
    if (0!=squadlogos&&!(c.cloaked&MeshDrawContext::NEARINVIS)){
      squadlogos->Draw(c.mat);
    }
  }
  vlist->EndDrawState();
  if (!getLighting()) {
    GFXEnable(LIGHTING);
  }
  while (tmp_draw_queue.size()) {
    draw_queue->push_back (tmp_draw_queue.back());
    tmp_draw_queue.pop_back();
  }
  if (!write_to_depthmap) {
    GFXEnable(DEPTHWRITE);//risky--for instance logos might be fubar!
  }
  RestoreCullFace(whichdrawqueue);
}
void Mesh::CreateLogos(int faction, Flightgroup * fg) {
  numforcelogo=numsquadlogo =0;
  unsigned int index;
  for (index=0;index<xml->logos.size();index++) {
    if (xml->logos[index].type==0)
      numforcelogo++;
    if (xml->logos[index].type==1)
      numsquadlogo++;
  }
  unsigned int nfl=numforcelogo;
  Logo ** tmplogo=NULL;
  Texture * Dec=NULL;
  for (index=0,nfl=numforcelogo,tmplogo=&forcelogos,Dec=FactionUtil::getForceLogo(faction);index<2;index++,nfl=numsquadlogo,tmplogo=&squadlogos,Dec=(fg==NULL?FactionUtil::getSquadLogo(faction):fg->squadLogo)) {
    if (Dec==NULL) {
      Dec = FactionUtil::getSquadLogo(faction);
    }
    if (nfl==0)
      continue;
    Vector *PolyNormal = new Vector [nfl];
    Vector *center = new Vector [nfl];
    float *sizes = new float [nfl];
    float *rotations = new float [nfl];
    float *offset = new float [nfl];
    Vector *Ref = new Vector [nfl];
    Vector norm1,norm2,norm;
    int ri=0;
    float totoffset=0;
    for (unsigned int ind=0;ind<xml->logos.size();ind++) {
      if (xml->logos[ind].type==index) {
	float weight=0;
	norm1.Set(0,1,0);
	norm2.Set(1,0,0);
	if (xml->logos[ind].refpnt.size()>2) {
	  norm2=Vector (xml->vertices[xml->logos[ind].refpnt[1]].x-
			xml->vertices[xml->logos[ind].refpnt[0]].x,
			xml->vertices[xml->logos[ind].refpnt[1]].y-
			xml->vertices[xml->logos[ind].refpnt[0]].y,
			xml->vertices[xml->logos[ind].refpnt[1]].z-
			xml->vertices[xml->logos[ind].refpnt[0]].z);
	  norm1=Vector (xml->vertices[xml->logos[ind].refpnt[2]].x-
			xml->vertices[xml->logos[ind].refpnt[0]].x,
			xml->vertices[xml->logos[ind].refpnt[2]].y-
			xml->vertices[xml->logos[ind].refpnt[0]].y,
			xml->vertices[xml->logos[ind].refpnt[2]].z-
			xml->vertices[xml->logos[ind].refpnt[0]].z);
	}
	CrossProduct (norm2,norm1,norm);
	
	Normalize(norm);//norm is our normal vect, norm1 is our reference vect
	Vector Cent(0,0,0);
	for (unsigned int rj=0;rj<xml->logos[ind].refpnt.size();rj++) {
	  weight+=xml->logos[ind].refweight[rj];
	  Cent += Vector (xml->vertices[xml->logos[ind].refpnt[rj]].x*xml->logos[ind].refweight[rj],
			  xml->vertices[xml->logos[ind].refpnt[rj]].y*xml->logos[ind].refweight[rj],
			  xml->vertices[xml->logos[ind].refpnt[rj]].z*xml->logos[ind].refweight[rj]);
	}	
	if (weight!=0) {
	  Cent.i/=weight;
	  Cent.j/=weight;
	  Cent.k/=weight;
	}
	//Cent.i-=x_center;
	//Cent.j-=y_center;
	//Cent.k-=z_center;
	Ref[ri]=norm2;
	PolyNormal[ri]=norm;
	center[ri] = Cent;
	sizes[ri]=xml->logos[ind].size*xml->scale;
	rotations[ri]=xml->logos[ind].rotate;
	offset[ri]=xml->logos[ind].offset;
	totoffset+=offset[ri];
	ri++;
      }
    }
    totoffset/=nfl;
    *tmplogo= new Logo (nfl,center,PolyNormal,sizes,rotations, totoffset,Dec,Ref);
    delete [] Ref;
    delete []PolyNormal;
    delete []center;
    delete [] sizes;
    delete [] rotations;
    delete [] offset;
  }
}

