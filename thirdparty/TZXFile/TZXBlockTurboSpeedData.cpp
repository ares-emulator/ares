//#include "stdafx.h"
#include "TZXBlockTurboSpeedData.h"
#include "TZXFile.h"
#include <stdlib.h>
#include <stdio.h>

TZXBlockTurboSpeedData::TZXBlockTurboSpeedData()
{
	m_nBlockID = TZX_BLOCKID_TURBO_SPEED_DATA;
	m_pData = NULL;
}


TZXBlockTurboSpeedData::~TZXBlockTurboSpeedData()
{
	if (m_pData) free(m_pData);
}

char *TZXBlockTurboSpeedData::GetDescription()
{
	snprintf((char *)m_szToStringDescription, MAX_STRING_LENGTH, "Turbo Speed Data Block: %d bytes.  Delay = %d ms", m_nDataLength, m_nDelayAfterBlock);
	return m_szToStringDescription;
}

void TZXBlockTurboSpeedData::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	//printf("Block starts at time %0.3f\n", (float)(pAudioGenerator->GetCurrentLength()) / 44100.0f);
	
    // Store the offset location for the current block
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

	pAudioGenerator->SetAmp(false); // Make sure the pilot always starts on a false.
	for (int i = 0; i <= m_nPilotCount; i++)
	{
		pAudioGenerator->GeneratePulse(m_nPilotPulse);
	}

	// Generate the two sync pulses
	pAudioGenerator->GeneratePulse(m_nFirstSyncPulse);
	pAudioGenerator->GeneratePulse(m_nSecondSyncPulse);

	// Generate the actual data
	for (int i = 0; i < m_nDataLength; i++)
	{
		int bits = 8;
		if (i == (m_nDataLength - 1)) bits = m_nBitsInLastByte;
		

		for (int n = 0; n<bits; n++)
		{
			char mask = 1 << (7-n);
			char bitset = m_pData[i] & mask;
			if (bitset)
			{
				// Write a one bit
				pAudioGenerator->GeneratePulse(m_nOneBitPulse);
				pAudioGenerator->GeneratePulse(m_nOneBitPulse);
			}
			else {
				// Write a zero bit
				pAudioGenerator->GeneratePulse(m_nZeroBitPulse);
				pAudioGenerator->GeneratePulse(m_nZeroBitPulse);
			}
		}
	}

	// Add any delay at the end of the block
	if (m_nDelayAfterBlock) pAudioGenerator->AddSilence(m_nDelayAfterBlock);
}
