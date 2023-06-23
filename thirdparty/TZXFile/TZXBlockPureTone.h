#pragma once
#include "TZXBlock.h"
class TZXBlockPureTone :
	public TZXBlock
{
public:
	unsigned short m_nPulseLengthInTStates;
	unsigned short m_nNumberOfPulses;

	TZXBlockPureTone();
	~TZXBlockPureTone();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

