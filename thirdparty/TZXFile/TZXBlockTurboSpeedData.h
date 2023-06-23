#pragma once
#include "TZXBlock.h"

class TZXBlockTurboSpeedData :
	public TZXBlock
{
public:
	unsigned short m_nPilotPulse; // Length of pulse
	unsigned short m_nFirstSyncPulse;
	unsigned short m_nSecondSyncPulse;
	unsigned short m_nZeroBitPulse;
	unsigned short m_nOneBitPulse;
	unsigned short m_nPilotCount; // Repititions of pulse
	unsigned char m_nBitsInLastByte;
	unsigned short m_nDelayAfterBlock;
	unsigned int m_nDataLength;
	unsigned char *m_pData;


public:
	TZXBlockTurboSpeedData();
	~TZXBlockTurboSpeedData();

	char *GetDescription();
	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
};

