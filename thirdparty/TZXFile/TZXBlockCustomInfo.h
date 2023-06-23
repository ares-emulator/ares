#pragma once
#include "TZXBlock.h"

class TZXBlockCustomInfo :
	public TZXBlock
{
public:
	char m_szIDString[17];
	int m_nCustomInfoLength;
	unsigned char *m_pCustomData;

	TZXBlockCustomInfo();
	~TZXBlockCustomInfo();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

