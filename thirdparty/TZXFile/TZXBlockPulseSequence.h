#pragma once
#include "TZXBlock.h"
class TZXBlockPulseSequence :
	public TZXBlock
{
public:
	unsigned char m_nPulseCount;
	unsigned short m_nPulseLength[255];

	TZXBlockPulseSequence();
	~TZXBlockPulseSequence();


	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

