#pragma once
#include "TZXBlock.h"

class TZXBlockPureData :
	public TZXBlock
{
public:
	unsigned short m_nLengthOfZeroPulse;
	unsigned short m_nLengthOfOnePulse;
	unsigned char m_nBitsInLastByte;
	unsigned short m_nPauseAfterBlock;
	int m_nDataLength;
	unsigned char *m_pData;

	TZXBlockPureData();
	~TZXBlockPureData();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

