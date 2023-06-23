#pragma once
#include "TZXBlock.h"
class TZXBlockMessage :
	public TZXBlock
{
public:
	unsigned char m_nDisplayTimeInSeconds;
	unsigned char m_nMessageLength;
	char m_szMessageString[256];

public:
	TZXBlockMessage();
	~TZXBlockMessage();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

