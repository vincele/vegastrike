#include "audiolib.h"
#ifdef HAVE_AL
#include <AL/al.h>
#endif
#include <stdio.h>
#include <vector>
#include "al_globals.h"
#include "vs_globals.h"
using std::vector;
struct Listener {
  Vector pos;
  Vector vel;
  Vector p,q,r;
  float gain;
  float rsize;
  Listener():pos(0,0,0),vel(0,0,0),p(1,0,0),q(0,1,0),r(0,0,1),gain(1),rsize(1){}
} mylistener;
unsigned int totalplaying=0;
const int hashsize = 47;
struct ApproxSound {
  int soundname;
};
typedef std::vector<ApproxSound> ApproxSoundVec;
static ApproxSoundVec playingbuffers [hashsize];
int hash_sound (const int buffer) {
  return buffer%hashsize;
}

char AUDQueryAudability (const int sound, const Vector &pos, const Vector & vel, const float gain) {
#ifdef HAVE_AL
  if (sounds[sound].buffer==(ALuint)0) 
    return 0;
  sounds[sound].pos = pos;
  Vector t = pos-mylistener.pos;
  float mag = t.Dot(t);
  int hashed = hash_sound (sounds[sound].buffer);

  if ((!unusedsrcs.empty())&&playingbuffers[hashed].size()<maxallowedsingle) return 1;
  ///could theoretically "steal" buffer from playing sound at this point

  if (playingbuffers[hashed].empty()) 
    return 1;
  //int target = rand()%playingbuffers[hashed].size();
  for (int rr=0;rr<3;rr++) {
    int target = rand()%playingbuffers[hashed].size();
    int target1 =playingbuffers[hashed][target].soundname;
    t = sounds[target1].pos-mylistener.pos;
      //steal sound!
    if (sounds[target1].buffer==sounds[sound].buffer) {
      if (t.Dot(t)>mag) {	
	ALuint tmpsrc = sounds[target1].source;
	//	fprintf (stderr,"stole sound %d %f\n", target1,mag);
	sounds[target1].source = sounds[sound].source;
	sounds[sound].source = tmpsrc;
	playingbuffers[hashed][target].soundname = sound;
	return 2;
      }
    }
  }
  if (playingbuffers[hashed].size()>maxallowedsingle)
    return 0;
  if (totalplaying>maxallowedtotal)
    return 0;
#endif
  return 1;
}
void AUDAddWatchedPlayed (const int sound, const Vector &pos) {
#ifdef HAVE_AL
  totalplaying++;
  if (sounds[sound].buffer!=(ALuint)0) {
    int h= hash_sound(sounds[sound].buffer);
    playingbuffers[h].push_back (ApproxSound());
    playingbuffers[h].back().soundname = sound;
    //    fprintf (stderr,"pushingback %f",(pos-mylistener.pos).Magnitude());
  }
#endif
}
typedef std::vector<int> vecint;
vecint soundstodelete;

void AUDRefreshSounds () {
#ifdef HAVE_AL
  for (int i=0;i<hashsize;i++) {
    for (unsigned int j=0;j<playingbuffers[i].size();j++) {
      if (!AUDIsPlaying (playingbuffers[i][j].soundname)) {
	totalplaying--;
	if (sounds[playingbuffers[i][j].soundname].source!=(ALuint)0) {
	  unusedsrcs.push_back (sounds[playingbuffers[i][j].soundname].source);
	  sounds[playingbuffers[i][j].soundname].source=(ALuint)0;
	}
	ApproxSoundVec::iterator k = playingbuffers[i].begin();
	k+=j;
	playingbuffers[i].erase (k);
	j--;
      }
    }
  } 
  for (int j=soundstodelete.size()-1;j>=0;j--) {//might not get every one every time
    int tmp = soundstodelete[j];
    if (!AUDIsPlaying(tmp)) {
      soundstodelete.erase (soundstodelete.begin()+j);
      AUDDeleteSound (tmp,false);
    }
  }
#endif
}
void AUDListener (const QVector & pos, const Vector & vel) {
#ifdef HAVE_AL
  mylistener.pos = pos.Cast();
  mylistener.vel = vel;
  if (g_game.sound_enabled) {

  if (usepositional)
	alListener3f (AL_POSITION, scalepos*pos.i,scalepos*pos.j,scalepos*pos.k);
  if (usedoppler)
	alListener3f (AL_VELOCITY, scalevel*vel.i,scalevel*vel.j,scalevel*vel.k);
  }
  //  printf ("(%f,%f,%f) <%f %f %f>\n",pos.i,pos.j,pos.k,vel.i,vel.j,vel.k);
#endif
}
void AUDListenerSize (const float rSize) {
#ifdef HAVE_AL
  mylistener.rsize = rSize*rSize;
#endif  
}
void AUDListenerOrientation (const Vector & p, const Vector & q, const Vector & r) {
#ifdef HAVE_AL  
  mylistener.p=p;
  mylistener.q=q;
  mylistener.r=r;
  ALfloat orient [] = {r.i,r.j,r.k,q.i,q.j,q.k};
  //  printf ("R%f,%f,%f>Q<%f %f %f>",r.i,r.j,r.k,q.i,q.j,q.k);
  if (g_game.sound_enabled) {
      alListenerfv (AL_ORIENTATION,orient);
  }
#endif
}
void AUDListenerGain (const float gain) {
#ifdef HAVE_AL
  mylistener.gain = gain;
  if (g_game.sound_enabled) {
    alListenerf (AL_GAIN,gain);	
  }
#endif
}
