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

#ifndef JVOIPNORMALMIXER_H

#define JVOIPNORMALMIXER_H

#include "jvoipmixer.h"
#include "jvoipsampleconverter.h"
#include <list>

class JVOIPNormalMixer : public JVOIPMixer
{
public:
	JVOIPNormalMixer(JVOIPSession *sess);
	~JVOIPNormalMixer();
	int Init(int sampinterval,int outputsamprate,int outputbytespersample,bool stereo,const JVOIPComponentParams *componentparams);
	int Cleanup();
	int AddBlock(VoIPFramework::VoiceBlock *vb,VoIPFramework::VOIPuint64 sourceid);
	int GetSampleBlock(VoIPFramework::VoiceBlock *vb);
	int GetSampleOffset(VoIPFramework::VOIPdouble *offset);
	bool SupportsSampleInterval(int ival);
	int SetSampleInterval(int ival);
	bool SupportsOutputSamplingRate(int orate);
	int SetOutputSamplingRate(int orate);
	bool SupportsOutputBytesPerSample(int outputbytespersample);
	int SetOutputBytesPerSample(int outputbytespersample);
	int SetStereo(bool s);

	int GetComponentState(JVOIPComponentState **compstate);
	int SetComponentState(JVOIPComponentState *compstate);
	
	std::string GetComponentName();
	std::string GetComponentDescription();
	std::vector<JVOIPCompParamInfo> *GetComponentParameters() throw (JVOIPException);
private:
	bool init;	
	JVOIPSampleConverter sampconv;
	
	// current settings
	int sampleinterval,samplerate,bytespersample;
	bool isstereo;
	
	class MixerBlock
	{
	public:
		MixerBlock(VoIPFramework::VOIPuint64 sampnum) { data = NULL; datalen = 0; numsamples = 0; samplenum = sampnum; }
		~MixerBlock() { if (data) delete [] data; }
		
		unsigned char *data;
		int datalen;
		int numsamples;
		VoIPFramework::VOIPuint64 samplenum;
	};
	
	void DeleteMixerBlocks();
	std::list<MixerBlock *> mixerblocks;
	
	VoIPFramework::VOIPdouble sampleoffset;

	inline VoIPFramework::VOIPuint64 CalculateSampleNumber(VoIPFramework::VOIPdouble offset);
	int AdjustSamples(VoIPFramework::VoiceBlock *vb);
	inline bool IsTooLate(VoIPFramework::VoiceBlock *vb);
	int GetStartMixerBlock(VoIPFramework::VOIPuint64 samplenum,std::list<MixerBlock*>::iterator *i);
	int CalculateNewNumSamples();
	int CreateAllMemoryBlocks(std::list<MixerBlock*>::iterator it,VoIPFramework::VOIPuint64 endsamplenum);
	void SetSilence(unsigned char *data,int numsamples);
	void FillInSamples(std::list<MixerBlock*>::iterator it,VoIPFramework::VoiceBlock *vb,VoIPFramework::VOIPuint64 startsamplenum);
	
	double realtotaltime,wantedtotaltime;
};

#endif // JVOIPNORMALMIXER_H
