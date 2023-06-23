#pragma once
#include "TZXBlock.h"


class TZXBlockPause :
	public TZXBlock
{
public:
	unsigned short m_nPauseInMS;

	TZXBlockPause();
	~TZXBlockPause();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

