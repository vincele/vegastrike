#include "vs_path.h"
#include "config_xml.h"
#include "vs_globals.h"
#include "xml_support.h"
#ifdef _WIN32

#else
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#endif

#define CONFIGFILE "vegastrike.config"
#define HOMESUBDIR ".vegastrike"
#define DELIM '/'
std::vector <std::string> savedpwd;
std::string sharedtextures;
std::string sharedunits;
std::string sharedsounds;
std::string sharedmeshes;
std::string datadir;
std::vector <std::string> curdir;//current dir starting from datadir
void changehome() {
#ifndef _WIN32
  struct passwd *pwent;
  pwent = getpwuid (getuid());
  chdir (pwent->pw_dir);
  chdir (HOMESUBDIR);
#endif
}

char pwd[8192];
void initpaths () {

  getcwd (pwd,8191);
  datadir= string (pwd);
  sharedsounds = datadir;
  FILE *fp= fopen (CONFIGFILE,"r");
  changehome();
  FILE *fp1= fopen (CONFIGFILE,"r");
  if (fp1) {
    fclose (fp1);
    vs_config=new VegaConfig(CONFIGFILE); // move config to global or some other struct
  }else if (fp) {
    chdir (pwd);
    fclose (fp);
    fp =NULL;
    vs_config = new VegaConfig (CONFIGFILE);
  } else {
    fprintf (stderr,"Could not open config file in either %s/%s\nOr in ~/.vegastrike/%s\n",pwd,CONFIGFILE,CONFIGFILE);
    exit (-1);
  }
  if (fp)
    fclose (fp);
  datadir = vs_config->getVariable ("data","directory",sharedsounds);
  chdir (datadir.c_str());
  chdir (vs_config->getVariable ("data","sharedtextures","textures").c_str());
  getcwd (pwd,8191);
  sharedtextures = string (pwd);
  chdir (datadir.c_str());
  chdir (vs_config->getVariable ("data","sharedsounds","sounds").c_str());
  getcwd (pwd,8191);
  sharedsounds = string (pwd);
  chdir (datadir.c_str());
  chdir (vs_config->getVariable ("data","sharedmeshes","meshes").c_str());
  getcwd (pwd,8191);
  sharedmeshes = string (pwd);
  chdir (datadir.c_str());
  chdir (vs_config->getVariable ("data","sharedunits","units").c_str());
  getcwd (pwd,8191);
  sharedunits = string (pwd);


  chdir (datadir.c_str());
  if (datadir.end()!=datadir.begin()) {
    if (*(datadir.end()-1)!='/'&&*(datadir.end()-1)!='\\') {
      datadir+=DELIM;
    }
  }
  if (sharedtextures.end()!=sharedtextures.begin()) {
    if (*(sharedtextures.end()-1)!='/'&&*(sharedtextures.end()-1)!='\\') {
      sharedtextures+=DELIM;
    }
  }
  if (sharedmeshes.end()!=sharedmeshes.begin()) {
    if (*(sharedmeshes.end()-1)!='/'&&*(sharedmeshes.end()-1)!='\\') {
      sharedmeshes+=DELIM;
    }
  }
  if (sharedunits.end()!=sharedunits.begin()) {
    if (*(sharedunits.end()-1)!='/'&&*(sharedunits.end()-1)!='\\') {
      sharedunits+=DELIM;
    }
  }
  if (sharedsounds.end()!=sharedsounds.begin()) {
    if (*(sharedsounds.end()-1)!='/'&&*(sharedsounds.end()-1)!='\\') {
      sharedsounds+=DELIM;
    }
  }
}
std::string GetHashName (const std::string &name) {
  std::string result("");
  for (unsigned int i=0;i<curdir.size();i++) {
    result+=curdir[i];
  }
  result+=name;
  return result;
}
std::string GetSharedMeshPath (const std::string &name) {
  return sharedmeshes+name;
}
std::string GetSharedUnitPath () {
  return sharedunits;
}
std::string GetSharedMeshHashName (const std::string &name) {
  return (string ("#smsh#")+name);
}

std::string GetSharedTexturePath (const std::string &name) {
  return sharedtextures+name;
}
std::string GetSharedTextureHashName (const std::string &name) {
  return (string ("#stex#")+name);
}
std::string GetSharedSoundPath (const std::string &name) {
  return sharedsounds+name;
}
std::string GetSharedSoundHashName (const std::string &name) {
  return (string ("#ssnd#")+name);
}

void vschdir (const char *path) {
  if (path[0]!='\0') {
    if (path[0]=='.'&&path[1]=='.') {
      vscdup();
      return;
    }
  }
  if (chdir (path)!=-1) {
    std::string tpath = path;
    if (tpath.end()!=tpath.begin())
      if ((*(tpath.end()-1)!='/')&&((*tpath.end()-1)!='\\'))
	tpath+='/';
    curdir.push_back (tpath);
  } else {
    curdir.push_back (string("~"));
  }
}
void vssetdir (const char * path) {
  getcwd (pwd,8191);
  savedpwd.push_back (string (pwd));
  chdir (path);
}
void vsresetdir () {
  chdir (savedpwd.back().c_str());
  savedpwd.pop_back();
}

void vscdup() {
  if (!curdir.empty()) {
    if ((*curdir.back().begin())!='~') {
      chdir ("..");
    }
    curdir.pop_back ();
  }
}
