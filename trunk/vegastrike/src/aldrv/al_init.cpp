#include "audiolib.h"
#include "config_xml.h"
#include "xml_support.h"
#include "vs_globals.h"

#ifdef HAVE_AL
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alut.h>
#include <stdlib.h>
#include <stdio.h>
#include "al_globals.h"
#include <vector>
static void fixup_function_pointers(void) {
  alutLoadMP3p = (mp3Loader *) alGetProcAddress((ALubyte *)"alutLoadMP3_LOKI");
  if(alutLoadMP3p == NULL) {
    fprintf(stderr, "Could not GetProc %s\n","alutLoadMP3_LOKI");
  }


  /* UNCOMMENT AS NEEDED
	talcGetAudioChannel = (ALfloat (*)(ALuint channel))
				GP("alcGetAudioChannel_LOKI");
	talcSetAudioChannel = (void (*)(ALuint channel, ALfloat volume))
				GP("alcSetAudioChannel_LOKI");

	talMute   = (void (*)(ALvoid)) GP("alMute_LOKI");
	talUnMute = (void (*)(ALvoid)) GP("alUnMute_LOKI");

	talReverbScale = (void (*)(ALuint sid, ALfloat param))
		GP("alReverbScale_LOKI");
	talReverbDelay = (void (*)(ALuint sid, ALfloat param))
		GP("alReverbDelay_LOKI");

	talBombOnError = (void (*)(void))
		GP("alBombOnError_LOKI");

	if(talBombOnError == NULL) {
		fprintf(stderr,
			"Could not GetProcAddress alBombOnError_LOKI\n");
		exit(1);
	}
	
	talBufferi = (void (*)(ALuint, ALenum, ALint ))
		GP("alBufferi_LOKI");

	if(talBufferi == NULL) {
		fprintf(stderr,
			"Could not GetProcAddress alBufferi_LOKI\n");
		exit(1);
	}

	alCaptureInit    = (ALboolean (*)( ALenum, ALuint, ALsizei )) GP("alCaptureInit_EXT");
	alCaptureDestroy = (ALboolean (*)( ALvoid )) GP("alCaptureDestroy_EXT");
	alCaptureStart   = (ALboolean (*)( ALvoid )) GP("alCaptureStart_EXT");
	alCaptureStop    = (ALboolean (*)( ALvoid )) GP("alCaptureStop_EXT");
	alCaptureGetData = (ALsizei (*)( ALvoid*, ALsizei, ALenum, ALuint )) GP("alCaptureGetData_EXT");

	talBufferAppendData = (ALuint (*)(ALuint, ALenum, ALvoid *, ALint, ALint)) GP("alBufferAppendData_LOKI");
	talBufferAppendWriteData = (ALuint (*)(ALuint, ALenum, ALvoid *, ALint, ALint, ALenum)) GP("alBufferAppendWriteData_LOKI");

	talGenStreamingBuffers = (void (*)(ALsizei n, ALuint *bids )) GP("alGenStreamingBuffers_LOKI");
	if( talGenStreamingBuffers == NULL ) {
		fprintf( stderr, "Could not GP alGenStreamingBuffers_LOKI\n");
		exit(1);
	}
	
	talutLoadRAW_ADPCMData = (ALboolean (*)(ALuint bid,ALvoid *data, ALuint size, ALuint freq,ALenum format)) GP("alutLoadRAW_ADPCMData_LOKI");
	if( talutLoadRAW_ADPCMData == NULL ) {
		fprintf( stderr, "Could not GP alutLoadRAW_ADPCMData_LOKI\n");
		exit(1);
	}

	talutLoadIMA_ADPCMData = (ALboolean (*)(ALuint bid,ALvoid *data, ALuint size, ALuint freq,ALenum format)) GP("alutLoadIMA_ADPCMData_LOKI");
	if( talutLoadIMA_ADPCMData == NULL ) {
		fprintf( stderr, "Could not GP alutLoadIMA_ADPCMData_LOKI\n");
		exit(1);
	}

	talutLoadMS_ADPCMData = (ALboolean (*)(ALuint bid,ALvoid *data, ALuint size, ALuint freq,ALenum format)) GP("alutLoadMS_ADPCMData_LOKI");
	if( talutLoadMS_ADPCMData == NULL ) {
		fprintf( stderr, "Could not GP alutLoadMS_ADPCMData_LOKI\n");
		exit(1);
	}

  */
	return;
}

void AUDChangeVolume (float volume) {
	if (volume==0) {
		usepositional=false;
		return;
	} else {
		usepositional=true;
	}
	scalepos= 1./volume;
}
float AUDGetVolume () {
	return 1./scalepos;
}
void AUDChangeDoppler (float doppler) {
	if (doppler<=0) {
		usedoppler=false;
	}else {
		usedoppler=true;
	}
	scalevel = doppler;
}
float AUDGetDoppler () {
	return scalevel;
}


///I don't think we'll need to switch contexts or devices in vegastrike
static ALCdevice *dev=NULL;

#ifndef _WIN32
static ALvoid *context_id=NULL;
#else
static ALCcontext * context_id=NULL;
#endif
#endif
bool AUDInit () {
  usedoppler = XMLSupport::parse_bool (vs_config->getVariable ("audio","Doppler","false"));
  usepositional = XMLSupport::parse_bool (vs_config->getVariable ("audio","Positional","true"));

  scalepos = 1/XMLSupport::parse_float (vs_config->getVariable ("audio","Volume","100"));
  scalevel=XMLSupport::parse_float (vs_config->getVariable ("audio","DopplerScale","1"));
  //  enabled = XMLSupport::parse_bool (vs_config->getVariable ("audio","enabled","true"));
  g_game.audio_frequency_mode = XMLSupport::parse_int (vs_config->getVariable ("audio","frequency","22050"));
  g_game.sound_enabled = false;
  g_game.music_enabled = false;
#ifdef HAVE_AL
  maxallowedsingle = XMLSupport::parse_int (vs_config->getVariable ("audio","MaxSingleSounds","8"));
  maxallowedtotal = XMLSupport::parse_int (vs_config->getVariable ("audio","MaxTotalSounds","20"));
  g_game.sound_enabled = XMLSupport::parse_bool (vs_config->getVariable ("audio","Sound","true"));
  g_game.music_enabled = XMLSupport::parse_bool (vs_config->getVariable ("audio","Music","true"));
	int attrlist[] = { ALC_FREQUENCY, g_game.audio_frequency_mode, 0 };
#ifdef _WIN32
	dev = alcOpenDevice ((ALubyte*)"DirectSound3D");
#else
	dev = alcOpenDevice( NULL );
#endif
	if( dev == NULL ) {
		return false;
	}

	/* Initialize ALUT. */
	context_id = alcCreateContext( dev, attrlist );
	if(context_id == NULL) {
		alcCloseDevice( dev );
		return false;
	}

	alcMakeContextCurrent( context_id );

	fixup_function_pointers();
	ALenum alGetEr = 0;
	ALuint cursrc;
	alGetError();
	alGenSources (1,&cursrc);
	alGetEr = alGetError();
	while (alGetEr==0) {
	  unusedsrcs.push_back (cursrc);
	  if (unusedsrcs.size()>=maxallowedtotal) {
	    break;
	  }
	  alGenSources (1,&cursrc);
	  alGetEr = alGetError();

	}
	return true;
#endif
	return false;
}


void AUDDestroy() {
#ifdef HAVE_AL
  //Go through and delete all loaded wavs
  unsigned int i;
  for (i=0;i<sounds.size();i++) {
    if (sounds[i].buffer!=0)
      AUDStopPlaying (i);
      AUDDeleteSound (i);
  }
  for (i=0;i<unusedsrcs.size();i++) {
    alDeleteSources (1,&unusedsrcs[i]);
  }
  for (i=0;i<buffers.size();i++) {
    alDeleteBuffers (1,&buffers[i]);
  }
  buffers.clear();
  if (context_id)
    alcDestroyContext(context_id);
  if (dev)
    alcCloseDevice( dev );
#endif
}
