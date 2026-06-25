#pragma once
#include "TZXBlock.h"

class TZXBlockKansasCityStandard : public TZXBlock {
public:
	unsigned short m_nPauseAfterBlock;
	unsigned short m_nPilotPulseLength;
	unsigned short m_nPilotPulseCount;
	unsigned short m_nZeroPulseLength;
	unsigned short m_nOnePulseLength;
	unsigned char m_nBitConfig;
	unsigned char m_nByteConfig;
	int m_nDataLength;
	unsigned char *m_pData;

	TZXBlockKansasCityStandard();
	~TZXBlockKansasCityStandard();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};
