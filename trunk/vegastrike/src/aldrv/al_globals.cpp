#include "gfx/vec.h"
#include "al_globals.h"
#include "hashtable.h"
#ifdef HAVE_AL
mp3Loader *alutLoadMP3p = 0;
Hashtable<std::string, ALuint,char [127]> soundHash;
unsigned int maxallowedsingle=10;
unsigned int maxallowedtotal=40;
float scalepos;
bool usedoppler=false;
bool usepositional=true;
float scalevel;
std::vector <ALuint> unusedsrcs;
#endif
