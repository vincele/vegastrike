/*

    This file is a part of JVOIPLIB, a library designed to facilitate
    the use of Voice over IP (VoIP).

    Copyright (C) 2000-2002  Jori Liesenborgs (jori@lumumba.luc.ac.be)

    This library (JVOIPLIB) is based upon work done for my thesis at
    the School for Knowledge Technology (Belgium/The Netherlands)

    This file was developed at the 'Expertise Centre for Digital
    Media' (EDM) in Diepenbeek, Belgium (http://www.edm.luc.ac.be).
    The EDM is a research institute of the 'Limburgs Universitair
    Centrum' (LUC) (http://www.luc.ac.be).

    The full GNU Library General Public License can be found in the
    file LICENSE.LGPL which is included in the source code archive.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
    USA

*/

#ifndef JVOIPSOUNDCARDINPUT_H

#define JVOIPSOUNDCARDINPUT_H

#include "jvoipvoiceinput.h"
#include "jvoipsamplingtimer.h"
#include "jvoipsigwait.h"
#include "jvoipsampleconverter.h"
#include "jvoipmixerstatus.h"
#include "voiceblock.h"
#include <windows.h>
#include <mmsystem.h>

class JVOIPSoundcardInputParams : public JVOIPComponentParams
{
public:
	JVOIPSoundcardInputParams(bool resetbugfix = true)				{ nastyresetbug = resetbugfix; }
	JVOIPComponentParams *CreateCopy() const					{ return new JVOIPSoundcardInputParams(nastyresetbug); }
	bool GetNastyResetBugFix() const						{ return nastyresetbug; }
	void SetNastyResetBugFix(bool resetbugfix)					{ nastyresetbug = resetbugfix; }
private:
	bool nastyresetbug;
};

class JVOIPSoundcardInput : public JVOIPVoiceInput,public JVOIPSamplingTimer
{
public:
	JVOIPSoundcardInput(JVOIPSession *sess) throw (JVOIPException);
	~JVOIPSoundcardInput();
	int Init(int sampinterval,int inputsamprate,int inputbytespersample,const JVOIPComponentParams *componentparams);
	int Cleanup();
	int StartSampling();
	void Reset();
	int GetSampleBlock(VoIPFramework::VoiceBlock *vb);
	JVOIPSamplingTimer *GetSamplingTimer() 						{ return this; }
	bool SupportsSampleInterval(int ival);
	bool SupportsInputSamplingRate(int irate);
	bool SupportsInputBytesPerSample(int inputbytespersample);
	int SetSampleInterval(int ival);
	int SetInputSamplingRate(int irate);
	int SetInputBytesPerSample(int inputbytespersample);

	// sampling timer stuff
	bool HasTimeOut();
	int RestartTimer();
	JVOIPSignalWait *GetSignalWaiter()						{ return &sigwait; }
		
	int GetComponentState(JVOIPComponentState **compstate);
	int SetComponentState(JVOIPComponentState *compstate);
	
	std::string GetComponentName();
	std::string GetComponentDescription();
	std::vector<JVOIPCompParamInfo> *GetComponentParameters() throw (JVOIPException);
private:
	int InitBuffers();
	void ClearBuffers();
	
	static void CALLBACK InputCallback(HWAVEIN hwi,UINT uMsg,DWORD dwInstance,DWORD dwParam1,DWORD dwParam2);
	static void BlockDeleteHandler(unsigned char **data,void *param);
	void InitSampleConverter();
	int OpenSoundDevice(int samplerate);
	int CalculateDriveSampleRate(int requestedrate);
	int RequestSamplingRate(int rate);

	JVOIPSignalWait sigwait;
	
	HWAVEIN sounddev;
	JVOIPSampleConverter sampconvert;
	bool init,firsttime;
	
	bool resetting;
	bool nastyresetbug;
	JVOIPMixerStatus mixstat;
	
	int drvsamprate,drvbytespersample,drvstereo;
	int sampleinterval,bytespersample,samprate;
	
	WAVEHDR *sampleblocks;
	int numblocks,blocksize;
	int blockpos;
	unsigned char *convertbuffer;
	int convertbufsize;
	
	double srcsampletime,wantedsrcsampletime;
	double dstsampletime,wanteddstsampletime;
};

#endif // JVOIPSOUNDCARDINPUT_H
