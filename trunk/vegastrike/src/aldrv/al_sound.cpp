#include "audiolib.h"
#include "hashtable.h"
#include "vs_path.h"
#include <string>
#include "al_globals.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifdef HAVE_AL
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/alut.h>
#endif
#include <vector>
#include "vs_globals.h"
#include <algorithm>
#ifdef HAVE_AL
std::vector <unsigned int> dirtysounds;
std::vector <OurSound> sounds;
std::vector <ALuint> buffers;

static int LoadSound (ALuint buffer, bool looping) {
  unsigned int i;
  if (!dirtysounds.empty()) {
    i = dirtysounds.back();
    dirtysounds.pop_back();
    assert (sounds[i].buffer==(ALuint)0);
    sounds[i].buffer= buffer;
  } else {
    i=sounds.size();
    sounds.push_back (OurSound (0,buffer));
  }
  sounds[i].source = (ALuint)0;
  sounds[i].looping = looping?AL_TRUE:AL_FALSE;
  //limited number of sources
  //  alGenSources( 1, &sounds[i].source);
  //alSourcei(sounds[i].source, AL_BUFFER, buffer );
  //alSourcei(sounds[i].source, AL_LOOPING, looping ?AL_TRUE:AL_FALSE);
  return i;

}
#endif
int AUDCreateSoundWAV (const std::string &s, const bool music, const bool LOOP){
#ifdef HAVE_AL
  if ((g_game.sound_enabled&&!music)||(g_game.music_enabled&&music)) {
    FILE * fp = fopen (s.c_str(),"rb");
    bool shared=false;
    std::string nam (s);
    if (fp) {
      fclose (fp);
    }else {
      nam = GetSharedSoundPath (s);
      shared=true;

    }
    ALuint * wavbuf =NULL;
    std::string hashname;
    if (!music) {
      hashname = shared?GetSharedSoundHashName(s):GetHashName (s);
      wavbuf = soundHash.Get(hashname);
    }
    if (wavbuf==NULL) {
      wavbuf = (ALuint *) malloc (sizeof (ALuint));
      alGenBuffers (1,wavbuf);
      ALsizei size;	
      ALsizei bits;	
      ALsizei freq;
      signed char * filename = (signed char *)strdup (nam.c_str());
      void *wave;
      ALboolean err=AL_TRUE;
#ifndef WIN32
#ifdef MACOS
ALint format;
	  alutLoadWAVFile(filename, &format, &wave, &size, &freq);
#else
	
      ALsizei format;
      err = alutLoadWAV((char *)filename, &wave, &format, &size, &bits, &freq);
#endif
#else
	  ALboolean looping;
	  ALint format;

  	  fp = fopen ((const char *)filename,"rb");

	  if (fp) {

		fclose (fp);

	  } else {

		free (filename);

        alDeleteBuffers (1,wavbuf);

		free (wavbuf);

		return -1;

	  }
      alutLoadWAVFile((char *)filename, (int*)&format, &wave, &size, &freq, &looping);


#endif
      if(err == AL_FALSE) {
		return -1;
      }
      alBufferData( *wavbuf, format, wave, size, freq );
	  free (filename);
      free(wave);
      if (!music) {
	soundHash.Put (hashname,wavbuf);
	buffers.push_back (*wavbuf);
      }
    }
    return LoadSound (*wavbuf,LOOP);  
  }
#endif
  return -1;
}
int AUDCreateSoundWAV (const std::string &s, const bool LOOP) {
  return AUDCreateSoundWAV (s,false,LOOP);
}
int AUDCreateMusicWAV (const std::string &s, const bool LOOP) {
  return AUDCreateSoundWAV (s,true,LOOP);
}

int AUDCreateSoundMP3 (const std::string &s, const bool music, const bool LOOP){
#ifdef HAVE_AL
  if ((g_game.sound_enabled&&!music)||(g_game.music_enabled&&music)) {
    FILE * fp = fopen (s.c_str(),"rb");
    bool shared=false;
    std::string nam (s);
    if (fp) {
      fclose (fp);
    }else {
      nam = GetSharedSoundPath (s);
      shared=true;
    }
    ALuint * mp3buf=NULL;
    std::string hashname;
    if (!music) {
      hashname = shared?GetSharedSoundHashName(s):GetHashName (s);
      mp3buf = soundHash.Get (hashname);
    }
#ifdef _WIN32
	return -1;
#endif
    if (mp3buf==NULL) {
      FILE * fp = fopen (nam.c_str(),"rb");
      if (!fp)
	return -1;
      fseek (fp,0,SEEK_END);
      long length = ftell (fp);
      rewind (fp);
      char *data = (char *)malloc (length);
      fread (data,1,length,fp);
      fclose (fp);
      mp3buf = (ALuint *) malloc (sizeof (ALuint));
      alGenBuffers (1,mp3buf);
      if ((*alutLoadMP3p)(*mp3buf,data,length)!=AL_TRUE) {
	free (data);
	return -1;
      }
      free (data);
      if (!music) {
	soundHash.Put (hashname,mp3buf);
	buffers.push_back (*mp3buf);
      }
    }
    return LoadSound (*mp3buf,LOOP);
  }
#endif
  return -1;
}

int AUDCreateSoundMP3 (const std::string &s, const bool LOOP) {
  return AUDCreateSoundMP3 (s,false,LOOP);
}
int AUDCreateMusicMP3 (const std::string &s, const bool LOOP) {
  return AUDCreateSoundMP3 (s,true,LOOP);
}
int AUDCreateSound (const std::string &s,const bool LOOP) {
  if (s.end()-1>=s.begin()){
    if (*(s.end()-1)=='3') {
      return AUDCreateSoundMP3 (s,LOOP);
    } else {
      return AUDCreateSoundWAV (s,LOOP);
    }
  }
  return -1;
}
int AUDCreateMusic (const std::string &s,const bool LOOP) {
  if (s.end()-1>=s.begin()){
    if (*(s.end()-1)=='v') {
      return AUDCreateMusicWAV (s,LOOP);
    } else {
      return AUDCreateMusicMP3 (s,LOOP);
    }
  }
  return -1;
}

///copies other sound loaded through AUDCreateSound
int AUDCreateSound (int sound,const bool LOOP/*=false*/){
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size())
    return LoadSound (sounds[sound].buffer,LOOP);
#endif
  return -1;
}
extern std::vector <int> soundstodelete;
void AUDDeleteSound (int sound, bool music){
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size()) {
    if (AUDIsPlaying (sound)) {
      if (!music) {
	soundstodelete.push_back(sound);
	return;
      } else
	AUDStopPlaying (sound);
    }
    if (sounds[sound].source){
      unusedsrcs.push_back (sounds[sound].source);
      sounds[sound].source=(ALuint)0;
    }
    if (std::find (dirtysounds.begin(),dirtysounds.end(),sound)==dirtysounds.end()) {
      dirtysounds.push_back (sound);
    }else {
      fprintf (stderr,"double delete of sound");
      return;
    }
    //FIXME??
    //    alDeleteSources(1,&sounds[sound].source);
    if (music) {
      alDeleteBuffers (1,&sounds[sound].buffer);
    }

    sounds[sound].buffer=(ALuint)0;
  }
#endif
}
void AUDAdjustSound (const int sound, const QVector &pos, const Vector &vel){
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size()) {
    float p []= {scalepos*pos.i,scalepos*pos.j,scalepos*pos.k};
    float v []= {scalevel*vel.i,scalevel*vel.j,scalevel*vel.k};
    sounds[sound].pos = pos.Cast();
	if (usepositional)
	    alSourcefv(sounds[sound].source,AL_POSITION,p);
  if (usedoppler)
    alSourcefv(sounds[sound].source,AL_VELOCITY,v);
  }
#endif
}
void AUDSoundGain (const int sound, const float gain) {
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size()) {
    alSourcef(sounds[sound].source,AL_GAIN,gain);
    //    alSourcefv(sounds[sound].source,AL_VELOCITY,v);
  }
#endif
}

bool AUDIsPlaying (const int sound){
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size()) {
	if (!sounds[sound].source) 
		return false;
    ALint state;
#ifdef _WIN32
    alGetSourcei(sounds[sound].source,AL_SOURCE_STATE, &state);  //Obtiene el estado de la fuente para windows
#else
    alGetSourceiv(sounds[sound].source, AL_SOURCE_STATE, &state);
#endif

    return (state==AL_PLAYING);
  }
#endif
  return false;
}
void AUDStopPlaying (const int sound){
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size()) {
	if (sounds[sound].source!=0) {
	  alSourceStop(sounds[sound].source);
      unusedsrcs.push_back (sounds[sound].source);
	}
    sounds[sound].source=(ALuint)0;
  }
#endif
}
static bool AUDReclaimSource (const int sound) {
#ifdef HAVE_AL
  if (sounds[sound].source==(ALuint)0) {
    if (unusedsrcs.empty())
      return false;
    sounds[sound].source = unusedsrcs.back();
    unusedsrcs.pop_back();
    alSourcei(sounds[sound].source, AL_BUFFER, sounds[sound].buffer );
    alSourcei(sounds[sound].source, AL_LOOPING, sounds[sound].looping);    
  }
  return true;
#endif		
  return false;//silly
}
void AUDStartPlaying (const int sound){
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size()) {
    if (AUDReclaimSource (sound)) {
      alSourcePlay( sounds[sound].source );
    }
  }
#endif
}

void AUDPlay (const int sound, const QVector &pos, const Vector & vel, const float gain) {
#ifdef HAVE_AL
  char tmp;
  if (sound<0)
    return;
  if (sounds[sound].buffer==0) {
	return;
  }
  if ((tmp=AUDQueryAudability (sound,pos.Cast(),vel,gain))!=0) {
    if (AUDReclaimSource (sound)) {
      ALfloat p [3] = {pos.i,pos.j,pos.k};
      AUDAdjustSound (sound,pos,vel);
      alSourcef(sounds[sound].source,AL_GAIN,gain);    
      if (tmp!=2){
		AUDAddWatchedPlayed (sound,pos.Cast());
		alSourcePlay( sounds[sound].source );
      }

    }
  }
#endif
}

void AUDPausePlaying (const int sound){
#ifdef HAVE_AL
  if (sound>=0&&sound<(int)sounds.size()) {

    //    alSourcePlay( sounds[sound].source() );
  }
#endif
}
