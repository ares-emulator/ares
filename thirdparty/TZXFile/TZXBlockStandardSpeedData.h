#pragma once
#include "TZXBlock.h"
class TZXBlockStandardSpeedData :
	public TZXBlock
{
public:
	unsigned short m_nPauseAfterBlockInMS;
	unsigned short m_nLengthOfData;
	unsigned char *m_pData;

	TZXBlockStandardSpeedData();
	~TZXBlockStandardSpeedData();
	char *GetDescription();
	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
};

