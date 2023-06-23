//#include "stdafx.h"
#include "TZXBlockPureData.h"
#include "TZXFile.h"
#include <stdlib.h>

TZXBlockPureData::TZXBlockPureData()
{
	m_nBlockID = TZX_BLOCKID_PURE_DATA;
	m_pData = NULL;
}


TZXBlockPureData::~TZXBlockPureData()
{
	if (m_pData) free(m_pData);
}



void TZXBlockPureData::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

	// Generate the actual data
	for (int i = 0; i < m_nDataLength; i++)
	{
		int bits = 8;
		if (i == (m_nDataLength - 1)) bits = m_nBitsInLastByte;

		for (int n = 0; n < bits; n++)
		{
			char mask = 1 << (7-n);
			char bitset = m_pData[i] & mask;
			if (bitset)
			{
				// Write a one bit
				pAudioGenerator->GeneratePulse(m_nLengthOfOnePulse);
				pAudioGenerator->GeneratePulse(m_nLengthOfOnePulse);
			}
			else {
				// Write a zero bit
				pAudioGenerator->GeneratePulse(m_nLengthOfZeroPulse);
				pAudioGenerator->GeneratePulse(m_nLengthOfZeroPulse);
			}
		}
	}

	// Add any delay at the end of the block
	if (m_nPauseAfterBlock) pAudioGenerator->AddSilence(m_nPauseAfterBlock);
}

char *TZXBlockPureData::GetDescription()
{
	snprintf((char *)m_szToStringDescription, MAX_STRING_LENGTH, "Pure Data Block: %d bytes.  Delay = %d ms", m_nDataLength, m_nPauseAfterBlock);
	return m_szToStringDescription;
}
