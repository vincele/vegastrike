#include "gfx_mesh.h"

#include <iostream.h>
#include <fstream.h>
#include <expat.h>
#include <values.h>
#include "xml_support.h"
#include "gfx_transform_vector.h"
#ifdef max
#undefine max
#endif

static inline float max(float x, float y) {
  if(x>y) return x;
  else return y;
}

#ifdef min
#undefine min
#endif

static inline float min(float x, float y) {
  if(x<y) return x;
  else return y;
}

const float scale=.06;

using XMLSupport::EnumMap;
using XMLSupport::Attribute;
using XMLSupport::AttributeList;
using XMLSupport::parse_float;
using XMLSupport::parse_int;

const EnumMap::Pair Mesh::XML::element_names[] = {
  {"UNKNOWN", XML::UNKNOWN},
  {"Mesh", XML::MESH},
  {"Points", XML::POINTS},
  {"Point", XML::POINT},
  {"Location", XML::LOCATION},
  {"Normal", XML::NORMAL},
  {"Polygons", XML::POLYGONS},
  {"Line", XML::LINE},
  {"Tri", XML::TRI},
  {"Quad", XML::QUAD},
  {"Linestrip",XML::LINESTRIP},
  {"Tristrip", XML::TRISTRIP},
  {"Trifan", XML::TRIFAN},
  {"Quadstrip", XML::QUADSTRIP},
  {"Vertex", XML::VERTEX}
};

const EnumMap::Pair Mesh::XML::attribute_names[] = {
  {"UNKNOWN", XML::UNKNOWN},
  {"texture", XML::TEXTURE},
  {"x", XML::X},
  {"y", XML::Y},
  {"z", XML::Z},
  {"i", XML::I},
  {"j", XML::J},
  {"k", XML::K},
  {"Shade", XML::FLATSHADE},
  {"point", XML::POINT},
  {"s", XML::S},
  {"t", XML::T}};

const EnumMap Mesh::XML::element_map(XML::element_names, 15);
const EnumMap Mesh::XML::attribute_map(XML::attribute_names, 12);

void Mesh::beginElement(void *userData, const XML_Char *name, const XML_Char **atts) {
  ((Mesh*)userData)->beginElement(name, AttributeList(atts));
}

void Mesh::endElement(void *userData, const XML_Char *name) {
  ((Mesh*)userData)->endElement(name);
}

/* Load stages:
0 - no tags seen
1 - waiting for points
2 - processing points 
3 - done processing points, waiting for face data

Note that this is only a debugging aid. Once DTD is written, there
will be no need for this sort of checking
 */

void Mesh::beginElement(const string &name, const AttributeList &attributes) {
  //cerr << "Start tag: " << name << endl;
  //static bool flatshadeit=false;
  XML::Names elem = (XML::Names)XML::element_map.lookup(name);
  XML::Names top;
  if(xml->state_stack.size()>0) top = *xml->state_stack.rbegin();
  xml->state_stack.push_back(elem);

  bool texture_found = false;
  switch(elem) {
  case XML::UNKNOWN:
    cerr << "Unknown element start tag '" << name << "' detected " << endl;
    break;
  case XML::MESH:
    assert(xml->load_stage == 0);
    assert(xml->state_stack.size()==1);

    xml->load_stage = 1;
    // Read in texture attribute
    
    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::TEXTURE:
	xml->decal_name = (*iter).value;
	texture_found = true;
	goto texture_done;
      }
    }
  texture_done:
    assert(texture_found);
    break;
  case XML::POINTS:
    assert(top==XML::MESH);
    assert(xml->load_stage == 1);

    xml->load_stage = 2;
    break;
  case XML::POINT:
    assert(top==XML::POINTS);
    
    memset(&xml->vertex, 0, sizeof(xml->vertex));
    xml->point_state = 0; // Point state is used to check that all necessary attributes are recorded
    break;
  case XML::LOCATION:
    assert(top==XML::POINT);

    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Location tag" << endl;
	break;
      case XML::X:
	assert(!(xml->point_state & XML::P_X));
	xml->vertex.x = parse_float((*iter).value);
	xml->vertex.i = 0;
	xml->point_state |= XML::P_X;
	break;
      case XML::Y:
	assert(!(xml->point_state & XML::P_Y));
	xml->vertex.y = parse_float((*iter).value);
	xml->vertex.j = 0;
	xml->point_state |= XML::P_Y;
	break;
      case XML::Z:
	assert(!(xml->point_state & XML::P_Z));
	xml->vertex.z = parse_float((*iter).value);
	xml->vertex.k = 0;
	xml->point_state |= XML::P_Z;
	break;
      default:
	assert(0);
      }
    }
    assert(xml->point_state & (XML::P_X |
			       XML::P_Y |
			       XML::P_Z) == 
	   (XML::P_X |
	    XML::P_Y |
	    XML::P_Z) );
    break;
  case XML::NORMAL:
    assert(top==XML::POINT);

    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Normal tag" << endl;
	break;
      case XML::I:
	assert(!(xml->point_state & XML::P_I));
	xml->vertex.i = parse_float((*iter).value);
	xml->point_state |= XML::P_I;
	break;
      case XML::J:
	assert(!(xml->point_state & XML::P_J));
	xml->vertex.j = parse_float((*iter).value);
	xml->point_state |= XML::P_J;
	break;
      case XML::K:
	assert(!(xml->point_state & XML::P_K));
	xml->vertex.k = parse_float((*iter).value);
	xml->point_state |= XML::P_K;
	break;
      default:
	assert(0);
      }
    }
    if (xml->point_state & (XML::P_I |
			       XML::P_J |
			       XML::P_K) != 
	   (XML::P_I |
	    XML::P_J |
	    XML::P_K) ) {
      if (!xml->recalc_norm) {
	cerr.form ("Invalid Normal Data for point: <%f,%f,%f>\n",xml->vertex.x,xml->vertex.y, xml->vertex.z);
	xml->vertex.i=xml->vertex.j=xml->vertex.k=0;
	xml->recalc_norm=true;
      }
    }
    break;
  case XML::POLYGONS:
    assert(top==XML::MESH);
    assert(xml->load_stage==3);

    xml->load_stage = 4;
    break;
  case XML::TRI:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=3;
    xml->active_list = &xml->tris;
    xml->active_ind = &xml->triind;
    xml->trishade.push_back (0);
    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Vertex tag" << endl;
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  xml->trishade[xml->trishade.size()-1]=1;
	}else {
	  if ((*iter).value=="Smooth") {
	    xml->trishade[xml->trishade.size()-1]=0;
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::TRISTRIP:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=3;//minimum number vertices
    xml->tristrips.push_back (vector<GFXVertex>());
    xml->active_list = &(xml->tristrips[xml->tristrips.size()-1]);
    xml->tstrcnt = xml->tristripind.size();
    xml->active_ind = &xml->tristripind;
    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Vertex tag" << endl;
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  cerr << "Cannot Flatshade Tristrips" << endl;
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::TRIFAN:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=3;//minimum number vertices
    xml->trifans.push_back (vector<GFXVertex>());
    xml->active_list = &(xml->trifans[xml->trifans.size()-1]);
    xml->tfancnt = xml->trifanind.size();
    xml->active_ind = &xml->trifanind;
    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Vertex tag" << endl;
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  cerr << "Cannot Flatshade Trifans" << endl;
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;

  case XML::QUADSTRIP:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=4;//minimum number vertices
    xml->quadstrips.push_back (vector<GFXVertex>());
    xml->active_list = &(xml->quadstrips[xml->quadstrips.size()-1]);
    xml->qstrcnt = xml->quadstripind.size();
    xml->active_ind = &xml->quadstripind;
    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Vertex tag" << endl;
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  cerr << "Cannot Flatshade Quadstrips" << endl;
	}else {
	  if ((*iter).value=="Smooth") {
	    //ignored -- already done
	  }
	}
	break;
      default:
	assert (0);
      }
    }
    break;
   
  case XML::QUAD:
    assert(top==XML::POLYGONS);
    assert(xml->load_stage==4);
    xml->num_vertices=4;
    xml->active_list = &xml->quads;
    xml->active_ind = &xml->quadind;
    xml->quadshade.push_back (0);
    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      cerr << "another shade bites the dust";
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Vertex tag" << endl;
	break;
      case XML::FLATSHADE:
	if ((*iter).value=="Flat") {
	  xml->quadshade[xml->quadshade.size()-1]=1;
	}else {
	  if ((*iter).value=="Smooth") {
	    xml->quadshade[xml->quadshade.size()-1]=0;
	  }
	}
	break;
      default:
	assert(0);
      }
    }
    break;
  case XML::VERTEX:
    assert(top==XML::TRI || top==XML::QUAD || top ==XML::TRISTRIP || top ==XML::TRIFAN||top ==XML::QUADSTRIP);
    assert(xml->load_stage==4);

    xml->vertex_state = 0;
    unsigned int index;
    float s, t;
    for(AttributeList::const_iterator iter = attributes.begin(); iter!=attributes.end(); iter++) {
      switch(XML::attribute_map.lookup((*iter).name)) {
      case XML::UNKNOWN:
	cerr << "Unknown attribute '" << (*iter).name << "' encountered in Vertex tag" << endl;
	break;
      case XML::POINT:
	assert(!(xml->vertex_state & XML::V_POINT));
	xml->vertex_state |= XML::V_POINT;
	index = parse_int((*iter).value);
	break;
      case XML::S:
	assert(!(xml->vertex_state & XML::V_S));
	xml->vertex_state |= XML::V_S;
	s = parse_float((*iter).value);
	break;
      case XML::T:
	assert(!(xml->vertex_state & XML::V_T));
	xml->vertex_state |= XML::V_T;
	t = parse_float((*iter).value);
	break;
      default:
	assert(0);
      }
    }
    assert(xml->vertex_state & (XML::V_POINT|
				XML::V_S|
				XML::V_T) == 
	   (XML::V_POINT|
	    XML::V_S|
	    XML::V_T) );
    assert(index < xml->vertices.size());

    memset(&xml->vertex, 0, sizeof(xml->vertex));
    xml->vertex = xml->vertices[index];
    xml->vertexcount[index]+=1;
    if ((!xml->vertex.i)&&(!xml->vertex.j)&&(!xml->vertex.k)) {
      if (!xml->recalc_norm) {
	cerr.form ("Invalid Normal Data for point: <%f,%f,%f>\n",xml->vertex.x,xml->vertex.y, xml->vertex.z);
	
	xml->recalc_norm=true;
      }
    }
    xml->vertex.x *= scale;
    xml->vertex.y *= scale;
    xml->vertex.z *= scale;
    xml->vertex.s = s;
    xml->vertex.t = t;
    xml->active_list->push_back(xml->vertex);
    xml->active_ind->push_back(index);
    xml->num_vertices--;
    break;
  default:
    assert(0);
  }
}

void Mesh::endElement(const string &name) {
  //cerr << "End tag: " << name << endl;

  XML::Names elem = (XML::Names)XML::element_map.lookup(name);
  assert(*xml->state_stack.rbegin() == elem);
  xml->state_stack.pop_back();
  unsigned int i;
  switch(elem) {
  case XML::UNKNOWN:
    cerr << "Unknown element end tag '" << name << "' detected " << endl;
    break;
  case XML::POINT:
    assert(xml->point_state & (XML::P_X | 
			       XML::P_Y | 
			       XML::P_Z |
			       XML::P_I |
			       XML::P_J |
			       XML::P_K) == 
	   (XML::P_X | 
	    XML::P_Y | 
	    XML::P_Z |
	    XML::P_I |
	    XML::P_J |
	    XML::P_K) );
    xml->vertices.push_back(xml->vertex);
    xml->vertexcount.push_back(0);
    break;
  case XML::POINTS:
    xml->load_stage = 3;

    /*
    cerr << xml->vertices.size() << " vertices\n";
    for(int a=0; a<xml->vertices.size(); a++) {
      clog << "Point: (" << xml->vertices[a].x << ", " << xml->vertices[a].y << ", " << xml->vertices[a].z << ") (" << xml->vertices[a].i << ", " << xml->vertices[a].j << ", " << xml->vertices[a].k << ")\n";
    }
    clog << endl;
    */
    break;
  case XML::TRI:
    assert(xml->num_vertices==0);
    break;
  case XML::QUAD:
    assert(xml->num_vertices==0);
    break;
  case XML::TRISTRIP:
    assert(xml->num_vertices<=0);   
    for (i=xml->tstrcnt+2;i<xml->tristripind.size();i++) {
      if ((i-xml->tstrcnt)%2) {
	//normal order
	xml->nrmltristrip.push_back (xml->tristripind[i-2]);
	xml->nrmltristrip.push_back (xml->tristripind[i-1]);
	xml->nrmltristrip.push_back (xml->tristripind[i]);
      } else {
	//reverse order
	xml->nrmltristrip.push_back (xml->tristripind[i-1]);
	xml->nrmltristrip.push_back (xml->tristripind[i-2]);
	xml->nrmltristrip.push_back (xml->tristripind[i]);
      }
    }
    break;
  case XML::TRIFAN:
    assert (xml->num_vertices<=0);
    for (i=xml->tfancnt+2;i<xml->trifanind.size();i++) {
      xml->nrmltrifan.push_back (xml->trifanind[xml->tfancnt]);
      xml->nrmltrifan.push_back (xml->trifanind[i-1]);
      xml->nrmltrifan.push_back (xml->trifanind[i]);
    }
    break;
  case XML::QUADSTRIP://have to fix up nrmlquadstrip so that it 'looks' like a quad list for smooth shading
    assert(xml->num_vertices<=0);
    for (i=xml->qstrcnt+3;i<xml->quadstripind.size();i+=2) {
      xml->nrmlquadstrip.push_back (xml->quadstripind[i-3]);
      xml->nrmlquadstrip.push_back (xml->quadstripind[i-2]);
      xml->nrmlquadstrip.push_back (xml->quadstripind[i]);
      xml->nrmlquadstrip.push_back (xml->quadstripind[i-1]);
    }
    break;
  case XML::POLYGONS:
    assert(xml->tris.size()%3==0);
    assert(xml->quads.size()%4==0);
    
    /*
    cerr << xml->tris.size()/3 << " triangles\n";
    cerr << xml->quads.size()/4 << " quadrilaterals\n";
    for(int a=0; a<xml->tris.size(); a++) {
      if(a%3==0) {
	clog << "Triangle\n";
      }
      clog << "    (" << xml->tris[a].x << ", " << xml->tris[a].y << ", " << xml->tris[a].z << ") (" << xml->tris[a].i << ", " << xml->tris[a].j << ", " << xml->tris[a].k << ") (" << xml->tris[a].s << ", " << xml->tris[a].t << ")\n";
    }
    clog << "** ** ** Quadrilaterals ** ** **\n";
    for(int a=0; a<xml->quads.size(); a++) {
      if(a%4==0) {
	clog << "Quadrilateral\n";
      }
      clog << "    (" << xml->quads[a].x << ", " << xml->quads[a].y << ", " << xml->quads[a].z << ") (" << xml->quads[a].i << ", " << xml->quads[a].j << ", " << xml->quads[a].k << ") (" << xml->quads[a].s << ", " << xml->quads[a].t << ")\n";
    }
    clog << endl;
*/
    break;
  case XML::MESH:
    assert(xml->load_stage==4);

    xml->load_stage=5;
    break;
  default:
    ;
  }
}



void SumNormals (int trimax, int t3vert, 
		 vector <GFXVertex> &vertices,
		 vector <int> &triind,
		 vector <int> &vertexcount,
		 bool * vertrw ) {
  int a=0;
  int i=0;
  int j=0;
  for (i=0;i<trimax;i++,a+=t3vert) {
    for (j=0;j<t3vert;j++) {
      if (vertrw[triind[a+j]]) {
	Vector Cur (vertices[triind[a+j]].x,
		    vertices[triind[a+j]].y,
		    vertices[triind[a+j]].z);
	Cur = (Vector (vertices[triind[a+((j+2)%t3vert)]].x,
		       vertices[triind[a+((j+2)%t3vert)]].y,
		       vertices[triind[a+((j+2)%t3vert)]].z)-Cur)
	  .Cross(Vector (vertices[triind[a+((j+1)%t3vert)]].x,
			 vertices[triind[a+((j+1)%t3vert)]].y,
			 vertices[triind[a+((j+1)%t3vert)]].z)-Cur);
	Normalize(Cur);
	//Cur = Cur*(1.00F/xml->vertexcount[a+j]);
	vertices[triind[a+j]].i+=Cur.i/vertexcount[triind[a+j]];
	vertices[triind[a+j]].j+=Cur.j/vertexcount[triind[a+j]];
	vertices[triind[a+j]].k+=Cur.k/vertexcount[triind[a+j]];
      }
    }
  }
}



void Mesh::LoadXML(const char *filename, Mesh *oldmesh) {
  const int chunk_size = 16384;
  
  ifstream inFile(filename, ios::in | ios::binary);
  if(!inFile) {
    assert(0);
    return;
  }

  xml = new XML;
  xml->load_stage = 0;
  xml->recalc_norm=false;
  XML_Parser parser = XML_ParserCreate(NULL);
  XML_SetUserData(parser, this);
  XML_SetElementHandler(parser, &Mesh::beginElement, &Mesh::endElement);
  
  do {
    char *buf = (XML_Char*)XML_GetBuffer(parser, chunk_size);
    int length;
    
    inFile.read(buf, chunk_size);
    length = inFile.gcount();
    XML_ParseBuffer(parser, length, inFile.eof());
  } while(!inFile.eof());

  // Now, copy everything into the mesh data structures
  assert(xml->load_stage==5);
  //begin vertex normal calculations if necessary
  if (xml->recalc_norm) {
    unsigned int i; unsigned int a=0;
    unsigned int j;

    bool *vertrw = new bool [xml->vertices.size()]; 
    for (i=0;i<xml->vertices.size();i++) {
      if (xml->vertices[i].i==0&&
	  xml->vertices[i].j==0&&
	  xml->vertices[i].k==0) {
	vertrw[i]=true;
      }else {
	vertrw[i]=false;
      }
    }
    SumNormals (xml->tris.size()/3,3,xml->vertices, xml->triind,xml->vertexcount, vertrw);
    SumNormals (xml->quads.size()/4,4,xml->vertices, xml->quadind,xml->vertexcount, vertrw);
    SumNormals (xml->nrmltristrip.size()/3,3,xml->vertices, xml->nrmltristrip,xml->vertexcount, vertrw);
    SumNormals (xml->nrmltrifan.size()/3,3,xml->vertices, xml->nrmltrifan,xml->vertexcount, vertrw);
    SumNormals (xml->nrmlquadstrip.size()/4,4,xml->vertices, xml->nrmlquadstrip,xml->vertexcount, vertrw);

    delete []vertrw;
    for (i=0;i<xml->vertices.size();i++) {
      float dis = sqrtf (xml->vertices[i].i*xml->vertices[i].i +
			xml->vertices[i].j*xml->vertices[i].j +
			xml->vertices[i].k*xml->vertices[i].k);
      if (dis!=0) {
	xml->vertices[i].i/=dis;//renormalize
	xml->vertices[i].j/=dis;
	xml->vertices[i].k/=dis;
	fprintf (stderr, "Vertex %d, (%f,%f,%f) <%f,%f,%f>\n",i,
		 xml->vertices[i].x,
		 xml->vertices[i].y,
		 xml->vertices[i].z,
		 xml->vertices[i].i,
		 xml->vertices[i].j,
		 xml->vertices[i].k);
      }else {
	xml->vertices[i].i=xml->vertices[i].x;
	xml->vertices[i].j=xml->vertices[i].y;
	xml->vertices[i].k=xml->vertices[i].z;
	dis = sqrtf (xml->vertices[i].i*xml->vertices[i].i +
		     xml->vertices[i].j*xml->vertices[i].j +
		     xml->vertices[i].k*xml->vertices[i].k);
	if (dis!=0) {
	  xml->vertices[i].i/=dis;//renormalize
	  xml->vertices[i].j/=dis;
	  xml->vertices[i].k/=dis;	  
	}else {
	  xml->vertices[i].i=0;
	  xml->vertices[i].j=0;
	  xml->vertices[i].k=1;	  
	}
      } 
    }

    a=0;
    for (a=0;a<xml->tris.size();a+=3) {
      for (j=0;j<3;j++) {
	xml->tris[a+j].i = xml->vertices[xml->triind[a+j]].i;
	xml->tris[a+j].j = xml->vertices[xml->triind[a+j]].j;
	xml->tris[a+j].k = xml->vertices[xml->triind[a+j]].k;
      }
    }
    a=0;
    for (a=0;a<xml->quads.size();a+=4) {
      for (j=0;j<4;j++) {
	xml->quads[a+j].i=xml->vertices[xml->quadind[a+j]].i;
	xml->quads[a+j].j=xml->vertices[xml->quadind[a+j]].j;
	xml->quads[a+j].k=xml->vertices[xml->quadind[a+j]].k;
      }
    }
    a=0;
    unsigned int k=0;
    unsigned int l=0;
    for (l=a=0;a<xml->tristrips.size();a++) {
      for (k=0;k<xml->tristrips[a].size();k++,l++) {
	xml->tristrips[a][k].i = xml->vertices[xml->tristripind[l]].i;
	xml->tristrips[a][k].j = xml->vertices[xml->tristripind[l]].j;
	xml->tristrips[a][k].k = xml->vertices[xml->tristripind[l]].k;
      }
    }
    for (l=a=0;a<xml->trifans.size();a++) {
      for (k=0;k<xml->trifans[a].size();k++,l++) {
	xml->trifans[a][k].i = xml->vertices[xml->trifanind[l]].i;
	xml->trifans[a][k].j = xml->vertices[xml->trifanind[l]].j;
	xml->trifans[a][k].k = xml->vertices[xml->trifanind[l]].k;
      }
    }
    for (l=a=0;a<xml->quadstrips.size();a++) {
      for (k=0;k<xml->quadstrips[a].size();k++,l++) {
	xml->quadstrips[a][k].i = xml->vertices[xml->quadstripind[l]].i;
	xml->quadstrips[a][k].j = xml->vertices[xml->quadstripind[l]].j;
	xml->quadstrips[a][k].k = xml->vertices[xml->quadstripind[l]].k;
      }
    }

  }
  
  // TODO: add alpha handling

   //check for per-polygon flat shading
  unsigned int trimax = xml->tris.size()/3;
  unsigned int a=0;
  unsigned int i=0;
  unsigned int j=0;
  for (i=0;i<trimax;i++,a+=3) {
    if (xml->trishade[i]==1) {
      for (j=0;j<3;j++) {
	Vector Cur (xml->vertices[xml->triind[a+j]].x,
		    xml->vertices[xml->triind[a+j]].y,
		    xml->vertices[xml->triind[a+j]].z);
	Cur = (Vector (xml->vertices[xml->triind[a+((j+2)%3)]].x,
		       xml->vertices[xml->triind[a+((j+2)%3)]].y,
		       xml->vertices[xml->triind[a+((j+2)%3)]].z)-Cur)
	  .Cross(Vector (xml->vertices[xml->triind[a+((j+1)%3)]].x,
			 xml->vertices[xml->triind[a+((j+1)%3)]].y,
			 xml->vertices[xml->triind[a+((j+1)%3)]].z)-Cur);
	Normalize(Cur);
	//Cur = Cur*(1.00F/xml->vertexcount[a+j]);
	xml->tris[a+j].i=Cur.i/xml->vertexcount[xml->triind[a+j]];
	xml->tris[a+j].j=Cur.j/xml->vertexcount[xml->triind[a+j]];
	xml->tris[a+j].k=Cur.k/xml->vertexcount[xml->triind[a+j]];
      }
    }
  }
  a=0;
  trimax = xml->quads.size()/4;
  for (i=0;i<trimax;i++,a+=4) {
    if (xml->quadshade[i]==1) {
      for (j=0;j<4;j++) {
	Vector Cur (xml->vertices[xml->quadind[a+j]].x,
		    xml->vertices[xml->quadind[a+j]].y,
		    xml->vertices[xml->quadind[a+j]].z);
	Cur = (Vector (xml->vertices[xml->quadind[a+((j+2)%4)]].x,
		       xml->vertices[xml->quadind[a+((j+2)%4)]].y,
		       xml->vertices[xml->quadind[a+((j+2)%4)]].z)-Cur)
	  .Cross(Vector (xml->vertices[xml->quadind[a+((j+1)%4)]].x,
			 xml->vertices[xml->quadind[a+((j+1)%4)]].y,
			 xml->vertices[xml->quadind[a+((j+1)%4)]].z)-Cur);
	Normalize(Cur);
	//Cur = Cur*(1.00F/xml->vertexcount[a+j]);
	xml->quads[a+j].i=Cur.i/xml->vertexcount[xml->quadind[a+j]];
	xml->quads[a+j].j=Cur.j/xml->vertexcount[xml->quadind[a+j]];
	xml->quads[a+j].k=Cur.k/xml->vertexcount[xml->quadind[a+j]];
      }
    }
  }
  Decal = new Texture(xml->decal_name.c_str(), 0);

  int index = 0;

  int totalvertexsize = xml->tris.size()+xml->quads.size();
  for (index=0;index<xml->tristrips.size();index++) {
    totalvertexsize += xml->tristrips[index].size();
  }
  for (index=0;index<xml->trifans.size();index++) {
    totalvertexsize += xml->trifans[index].size();
  }
  for (index=0;index<xml->quadstrips.size();index++) {
    totalvertexsize += xml->quadstrips[index].size();
  }

  index =0;
  vertexlist = new GFXVertex[totalvertexsize];

  minSizeX = minSizeY = minSizeZ = FLT_MAX;
  maxSizeX = maxSizeY = maxSizeZ = -FLT_MAX;
  if (xml->tris.size()==0&&xml->quads.size()==0) {
    minSizeX = minSizeY = minSizeZ = 0;
    maxSizeX = maxSizeY = maxSizeZ = 0;
    fprintf (stderr, "uhoh");
  }
  radialSize = 0;
  for(a=0; a<xml->tris.size(); a++, index++) {
    vertexlist[index] = xml->tris[a];
    minSizeX = min(vertexlist[index].x, minSizeX);
    maxSizeX = max(vertexlist[index].x, maxSizeX);
    minSizeY = min(vertexlist[index].y, minSizeY);
    maxSizeY = max(vertexlist[index].y, maxSizeY);
    minSizeZ = min(vertexlist[index].z, minSizeZ);
    maxSizeZ = max(vertexlist[index].z, maxSizeZ);
  }
  for(a=0; a<xml->quads.size(); a++, index++) {
    vertexlist[index] = xml->quads[a];
    minSizeX = min(vertexlist[index].x, minSizeX);
    maxSizeX = max(vertexlist[index].x, maxSizeX);
    minSizeY = min(vertexlist[index].y, minSizeY);
    maxSizeY = max(vertexlist[index].y, maxSizeY);
    minSizeZ = min(vertexlist[index].z, minSizeZ);
    maxSizeZ = max(vertexlist[index].z, maxSizeZ);
  }
  for (a=0;a<xml->tristrips.size();a++) {
    for (int m=0;m<xml->tristrips[a].size();m++,index++) {
      vertexlist[index] = xml->tristrips[a][m];
      minSizeX = min(vertexlist[index].x, minSizeX);
      maxSizeX = max(vertexlist[index].x, maxSizeX);
      minSizeY = min(vertexlist[index].y, minSizeY);
      maxSizeY = max(vertexlist[index].y, maxSizeY);
      minSizeZ = min(vertexlist[index].z, minSizeZ);
      maxSizeZ = max(vertexlist[index].z, maxSizeZ);
    }
  }
  for (a=0;a<xml->trifans.size();a++) {
    for (int m=0;m<xml->trifans[a].size();m++,index++) {
      vertexlist[index] = xml->trifans[a][m];
      minSizeX = min(vertexlist[index].x, minSizeX);
      maxSizeX = max(vertexlist[index].x, maxSizeX);
      minSizeY = min(vertexlist[index].y, minSizeY);
      maxSizeY = max(vertexlist[index].y, maxSizeY);
      minSizeZ = min(vertexlist[index].z, minSizeZ);
      maxSizeZ = max(vertexlist[index].z, maxSizeZ);
    }
  }
  for (a=0;a<xml->quadstrips.size();a++) {
    for (int m=0;m<xml->quadstrips[a].size();m++,index++) {
      vertexlist[index] = xml->quadstrips[a][m];
      minSizeX = min(vertexlist[index].x, minSizeX);
      maxSizeX = max(vertexlist[index].x, maxSizeX);
      minSizeY = min(vertexlist[index].y, minSizeY);
      maxSizeY = max(vertexlist[index].y, maxSizeY);
      minSizeZ = min(vertexlist[index].z, minSizeZ);
      maxSizeZ = max(vertexlist[index].z, maxSizeZ);
    }
  }

  float x_center = (minSizeX + maxSizeX)/2.0,
    y_center = (minSizeY + maxSizeY)/2.0,
    z_center = (minSizeZ + maxSizeZ)/2.0;
  SetPosition(x_center, y_center, z_center);
  for(a=0; a<totalvertexsize; a++) {
    vertexlist[a].x -= x_center;
    vertexlist[a].y -= y_center;
    vertexlist[a].z -= z_center;
  }

  minSizeX -= x_center;
  maxSizeX -= x_center;
  minSizeY -= y_center;
  maxSizeY -= y_center;
  minSizeZ -= z_center;
  maxSizeZ -= z_center;
  
  radialSize = .5*sqrtf ((maxSizeX-minSizeX)*(maxSizeX-minSizeX)+(maxSizeY-minSizeY)*(maxSizeY-minSizeY)+(maxSizeX-minSizeZ)*(maxSizeX-minSizeZ));

  vlist[GFXTRI] = new GFXVertexList(GFXTRI,xml->tris.size(),vertexlist); 
  vlist[GFXQUAD]= new GFXVertexList(GFXQUAD,xml->quads.size(),vertexlist+xml->tris.size());
  index = xml->tris.size()+xml->quads.size();
  numQuadstrips = xml->tristrips.size()+xml->trifans.size()+xml->quadstrips.size();
  quadstrips = new GFXVertexList* [numQuadstrips];
  unsigned int tmpind =0;
  for (a=0;a<xml->tristrips.size();a++,tmpind++) {
    quadstrips[tmpind]= new GFXVertexList (GFXTRISTRIP,xml->tristrips[a].size(),vertexlist+index);
    index+= xml->tristrips[a].size();
  }
  for (a=0;a<xml->trifans.size();a++,tmpind++) {
    quadstrips[tmpind]= new GFXVertexList (GFXTRIFAN,xml->trifans[a].size(),vertexlist+index);
    index+= xml->trifans[a].size();
  }
  for (a=0;a<xml->quadstrips.size();a++,tmpind++) {
    quadstrips[tmpind]= new GFXVertexList (GFXQUADSTRIP,xml->quadstrips[a].size(),vertexlist+index);
    index+= xml->quadstrips[a].size();
  }


  
  //TODO: add force handling


  // Calculate bounding sphere

  this->orig = oldmesh;
  *oldmesh=*this;
  oldmesh->orig = NULL;
  oldmesh->refcount++;
  fprintf (stderr, "Minx %f maxx %f, miny %f maxy %fminz %fmaxz %f, radsiz %f\n",minSizeX, maxSizeX,  minSizeY, maxSizeY,  minSizeZ, maxSizeZ,radialSize);  
  delete [] vertexlist;
  delete xml;
}
