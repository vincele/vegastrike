#include "cmd/unit.h"
#include "hashtable.h"
#include <float.h>
#include "vs_path.h"
#include <vector>
#include <string>
#include "config_xml.h"
#include "vs_globals.h"
#include "config_xml.h"
#include "savegame.h"
#include <algorithm>
using namespace std;
 std::string GetHelperPlayerSaveGame (int num) {
  static string *res=NULL;
  if (res==NULL) {
    res = new std::string;
    changehome(false);
    char c[2]={'\0','\0'};
    FILE * fp = fopen (("save.txt"),"r");
    if (fp) {
      fseek (fp,0,SEEK_END);
      int length = ftell (fp);
      if (length>0) {
      char * temp = (char *)malloc (length+1);
      temp[length]='\0';
      fseek (fp,0,SEEK_SET);
      fread (temp,length,1,fp);
      bool end=true;
      for (int i=length-1;i>=0;i--) {
        if (temp[i]=='\r'||temp[i]=='\n') {
          temp[i]=(end?'\0':'_');
        }else if (temp[i]=='\0'||temp[i]==' '||temp[i]=='\t') {
          temp[i]=(end?'\0':'_');
        }else {
          end=false;
        }
      }
      *res = (temp);
      free (temp);
      }
      fclose (fp);

    }
#if 0
    if (fp) {
    while (!feof (fp)) {
      c[0]=fgetc (fp);
      if (!feof(fp)) {
        if (c[0]!='\r'&&c[0]!='\n'&&c[0]!='\0') {
          if (c[0]==' ') {
            c[0]='_';
            if (feof(fp)) {
              continue;
            }
          }
          (*res)+=c;
        }
      }else {
        break;
      }
    }
    fclose (fp);
#endif
    if (!res->empty()) {
      if (*res->begin()=='~') {
	fp = fopen (("save.txt"),"w");
	if (fp) {
	  for (unsigned int i=1;i<res->length();i++) {
	    fputc (*(res->begin()+i),fp);
	  }
	  fputc ('\0',fp);
	  fclose (fp);
	}
      }
    }

    
#if 0
    fp = fopen (("save.txt"),"w");
    if (fp) {
      fputc('\0',fp);
      fclose (fp);
    }
#endif
    returnfromhome();
  }
  if (num==0||res->empty()) {
    return (*res);  
  }
  return (*res)+XMLSupport::tostring(num);
 }

std::string GetWritePlayerSaveGame(int num) {
  string ret = GetHelperPlayerSaveGame(num);
  if (!ret.empty()) {
    if (*ret.begin()=='~') {
      return ret.substr (1,ret.length());
    }
  }
  return ret;
}

std::string GetReadPlayerSaveGame(int num) {
  string ret = GetHelperPlayerSaveGame(num);
  if (!ret.empty()) {
    if (*ret.begin()=='~') {
      return "";
    }
  }
  return ret;
}

void FileCopy (const char * src, const char * dst) {
  if (dst[0]!='\0'&&src[0]!='\0') {

  FILE * fp = fopen (src,"r");
  if (fp) {
      fseek(fp,0,SEEK_END);
      long length = ftell (fp);
      fseek(fp,0,SEEK_SET);
      char * info = new char [length];
      fread(info,length,sizeof(char),fp);
      fclose (fp);
      fp = fopen (dst,"w");
      if (fp) {
        fwrite (info,length,sizeof(char),fp);
        fclose(fp);
      }
      delete [] info;
  }
  }
}
QVector LaunchUnitNear (QVector pos) {
  static double def_un_size = XMLSupport::parse_float (vs_config->getVariable ("physics","respawn_unit_size","400"));
  for (unsigned int k=0;k<10;k++) {
    Unit * un;
    bool collision=false;
    for (un_iter i=_Universe->activeStarSystem()->getUnitList().createIterator();(un=*i)!=NULL;++i) {
      if (un->isUnit()==ASTEROIDPTR||un->isUnit()==NEBULAPTR) {
	continue;
      }
      double dist = (pos-un->Position()).Magnitude()-un->rSize()-def_un_size;
      if (dist<0) {
	QVector delta  = pos-un->Position();
        double mag = delta.Magnitude();
        if (mag>.01){
	  delta=delta/mag;
        }else {
          delta.Set(0,0,1);
        }
	delta = delta.Scale ( dist+def_un_size);
	if (k<5) {
	  pos = pos+delta;
	  collision=true;
	}else {
	  QVector r(.5,.5,.5);
	  pos+=un->rSize()*r;
	  collision=true;
	}
	
      }
    }
    if (collision==false)
      break;
  }
  return pos;
}

SaveGame::SaveGame(const std::string &pilot) {
  callsign=pilot;
  ForceStarSystem=string("");
  PlayerLocation.Set(FLT_MAX, FLT_MAX,FLT_MAX);
}

void SaveGame::SetStarSystem (string sys) {
  ForceStarSystem = sys;
}
string SaveGame::GetStarSystem () {
  return ForceStarSystem;
}

void SaveGame::SetPlayerLocation (const QVector &v) {
  fprintf (stderr,"Set Location %lf %lf %lf",v.i,v.j,v.k);
  if ((FINITE (v.i)&&FINITE(v.j)&&FINITE(v.k))) {
    PlayerLocation =v;
  }else {
    fprintf (stderr,"ERROR saving unit");
    PlayerLocation.Set(1,1,1);
  }
}
QVector SaveGame::GetPlayerLocation () {
  return PlayerLocation;
}

Hashtable<int,SavedUnits,char[47]> SaveGame::savedunits;
void SaveGame::RemoveUnitFromSave (int address) {
  SavedUnits *tmp;
  if (NULL!=(tmp =savedunits.Get (address))) {
    savedunits.Delete (address);
    delete tmp;
  }
}
void SaveGame::WriteNewsData (FILE * fp) {
  gameMessage * last;
  vector <gameMessage *> tmp;
  int i=0;
  vector <string> newsvec;
  newsvec.push_back ("news");
  while (NULL!=(last=mission->msgcenter->last(i++,newsvec))) {
    tmp.push_back (last);
  }
  fprintf (fp,"%d\n",i);
  for (int j=tmp.size()-1;j>=0;j--) {
    char * msg = strdup (tmp[j]->message.c_str());
    int k=0;
    while (msg[k]) {
      if (msg[k]=='\n'||msg[k]=='\r')
	msg[k]=' ';
      k++;
    }
    fprintf (fp,"%s\n",msg);
    free (msg);
  }
}
void WriteSaveGame (Cockpit * cp,bool auto_save) {
  int player_num= cp-_Universe->AccessCockpit(0);
  Unit * un = cp->GetSaveParent();
  if (!un) {
    return;
  }
  if (un->GetHull()>0) {
    cp->savegame->WriteSaveGame (cp->activeStarSystem->getFileName().c_str(),un->LocalPosition(),cp->credits,cp->GetUnitFileName().c_str(),auto_save?-1:player_num);
    un->WriteUnit(cp->GetUnitModifications().c_str());
    if (GetWritePlayerSaveGame(player_num).length()&&!auto_save) {
      cp->savegame->SetSavedCredits (_Universe->AccessCockpit()->credits);
      cp->savegame->SetStarSystem(cp->activeStarSystem->getFileName());
      un->WriteUnit(GetWritePlayerSaveGame(player_num).c_str());
      cp->savegame->SetPlayerLocation(un->LocalPosition());    
    }
  }

}
void SaveGame::ReadNewsData (FILE * fp) {
  int numnews;

  char news [1024];
  fgets (news,1023,fp);
  sscanf (news,"%d\n",&numnews);
  for (unsigned int i=0;i<numnews;i++) {
    fgets (news,1023,fp);
    news[1023]='\0';
    if (news[0]!='\r'&&news[0]!='\n') {
      mission->msgcenter->add ("game","news",news);
    }
  }
}
void SaveGame::AddUnitToSave (const char * filename, enum clsptr type, const char * faction, int address) {
  string s = vs_config->getVariable ("physics","Drone","drone");
  if (0==strcmp (s.c_str(),filename)/*||type==ENHANCEMENTPTR*/) {
    RemoveUnitFromSave (address);
    savedunits.Put (address,new SavedUnits (filename,type,faction));
  }
}
olist_t &SaveGame::getMissionData(const std::string &magic_number) {
  unsigned int i=std::find (mission_data.begin(),mission_data.end(),magic_number)-mission_data.begin();
  if (i==mission_data.size()) {
    mission_data.push_back(MissionDat(magic_number));
  }
  return mission_data[i].dat;
}
void SaveGame::WriteMissionData (FILE * fp) {
  fprintf (fp," %d ",mission_data.size());
  for( unsigned int i=0;i<mission_data.size();i++) {
    fprintf (fp,"\n%s ",mission_data[i].magic_number.c_str());
    fprintf (fp,"%d ",mission_data[i].dat.size());
    for (unsigned int j=0;j<mission_data[i].dat.size();j++) {
      fprintf (fp,"%s ",varToString(mission_data[i].dat[j]).c_str());
    }
  }
}
std::string scanInString (FILE * fp) {
  std::string str;
  char c[2]={'\n','\0'};
  while (isspace (c[0])) {
    if (1!=fscanf (fp,"%c",&c[0])) {
      break;
    }
  }
  while (!isspace (c[0])) {
    str+=c;
    if (1!=fscanf (fp,"%c",&c[0])) {
      break;
    }
  }
  return str;
}
void SaveGame::ReadMissionData (FILE * fp) {
  int mdsize;
  fscanf (fp," %d ",&mdsize);
  for( unsigned int i=0;i<mdsize;i++) {
    int md_i_size;
    string mag_num(scanInString (fp));
    fscanf (fp,"%d ",&md_i_size);
    mission_data.push_back (MissionDat(mag_num));
    for (unsigned int j=0;j<md_i_size;j++) {
      varInst * vi = new varInst (VI_IN_OBJECT);//not belong to a mission...not sure should inc counter
      vi->type = VAR_FLOAT;
      fscanf (fp,"%lf ",&vi->float_val);
      mission_data[i].dat.push_back (vi);
    }
  }
}
vector <SavedUnits> SaveGame::ReadSavedUnits (FILE * fp) {
  vector <SavedUnits> su;
  int a;
  char unitname[1024];
  char factname[1024];
  while (3==fscanf (fp,"%d %s %s",&a,unitname,factname)) {
    if (a==0&&0==strcmp(unitname,"factions")&&0==strcmp(factname,"begin")) {
      _Universe->LoadSerializedFaction(fp);
    }else if (a==0&&0==strcmp(unitname,"mission")&&0==strcmp(factname,"data")) {
      ReadMissionData(fp);
    }else if (a==0&&0==strcmp(unitname,"news")&&0==strcmp(factname,"data")) {
      ReadNewsData(fp);
    }else {
      su.push_back (SavedUnits (unitname,(clsptr)a,factname));
    }
  }
  return su;
}
void SaveGame::WriteSavedUnit (FILE * fp, SavedUnits* su) {
  fprintf (fp,"\n%d %s %s",su->type, su->filename.c_str(),su->faction.c_str());
}
void SaveGame::WriteSaveGame (const char *systemname, const QVector &FP, float credits, std::string unitname, int player_num) {
  vector<SavedUnits *> myvec = savedunits.GetAll();
  if (outputsavegame.length()!=0) {
    printf ("Writing Save Game %s",outputsavegame.c_str());
    changehome();
    vschdir ("save");
    FILE * fp = fopen (outputsavegame.c_str(),"w");
    QVector FighterPos= PlayerLocation-FP;
//    if (originalsystem!=systemname) {
      FighterPos=FP;
//    }
    fprintf (fp,"%s^%f^%s %lf %lf %lf",systemname,credits,unitname.c_str(),FighterPos.i,FighterPos.j,FighterPos.k);
    SetSavedCredits (credits);
    while (myvec.empty()==false) {
      WriteSavedUnit (fp,myvec.back());
      delete myvec.back();
      myvec.pop_back();
    }
    fprintf (fp,"\n%d %s %s",0,"mission","data ");
    WriteMissionData(fp);
    fprintf (fp,"\n%d %s %s",0,"news","data ");
    WriteNewsData(fp);
    fprintf (fp,"\n%d %s %s",0,"factions","begin ");
    _Universe->SerializeFaction(fp);
    fclose (fp);
    if (player_num!=-1) {
      FileCopy (outputsavegame.c_str(),GetWritePlayerSaveGame(player_num).c_str());
    }
    vscdup();
    returnfromhome();

  }
}
static float savedcredits=0;
float SaveGame::GetSavedCredits () {
  return savedcredits;
}
void SaveGame::SetSavedCredits (float c) {
  savedcredits = c;
}


vector<SavedUnits> SaveGame::ParseSaveGame (string filename, string &FSS, string originalstarsystem, QVector &PP, bool & shouldupdatepos,float &credits, string &savedstarship, int player_num) {
  if (filename.length()>0)
    filename=callsign+filename;
  vector <SavedUnits> mysav;
  shouldupdatepos=!(PlayerLocation.i==FLT_MAX||PlayerLocation.j==FLT_MAX||PlayerLocation.k==FLT_MAX);
  outputsavegame=filename;
  changehome();
  vschdir ("save");
  FILE * fp = NULL;
  if (filename.length()>0) {
    if (GetReadPlayerSaveGame(player_num).length()) {
          fp = fopen (GetReadPlayerSaveGame(player_num).c_str(),"r");
    }else {
	  fp = fopen (filename.c_str(),"r");
    }
  }
  vscdup();
  returnfromhome();
  if (fp) {
    char tmp2[10000];
    QVector tmppos;
    if (4==fscanf (fp,"%s %lf %lf %lf\n",tmp2,&tmppos.i,&tmppos.j,&tmppos.k)) {
      for (int j=0;'\0'!=tmp2[j];j++) {
	if (tmp2[j]=='^') {
	  sscanf (tmp2+j+1,"%f",&credits);
	  tmp2[j]='\0';
	  for (int k=j+1;tmp2[k]!='\0';k++) {
	    if (tmp2[k]=='^') {
	      tmp2[k]='\0';
	      savedstarship=string(tmp2+k+1);
	      break;
	    }
	  }
	  break;
	}
      }
      if (ForceStarSystem.length()==0) 
	ForceStarSystem=string(tmp2);
      if (PlayerLocation.i==FLT_MAX||PlayerLocation.j==FLT_MAX||PlayerLocation.k==FLT_MAX) {
	shouldupdatepos=true;
	PlayerLocation=tmppos;//LaunchUnitNear(tmppos);
      }
      mysav=ReadSavedUnits (fp);
    }
    fclose (fp);
  }
  if (PlayerLocation.i==FLT_MAX||PlayerLocation.j==FLT_MAX||PlayerLocation.k==FLT_MAX) {
    shouldupdatepos=false;
    PlayerLocation=PP;
  }else {
    PP = PlayerLocation;
    shouldupdatepos=true;
  }
  if (ForceStarSystem.length()==0) {
    ForceStarSystem = FSS;
    originalsystem=FSS;
  } else {
    originalsystem = ForceStarSystem;
    FSS = ForceStarSystem;
  }
  SetSavedCredits(credits);
  return mysav;
}

