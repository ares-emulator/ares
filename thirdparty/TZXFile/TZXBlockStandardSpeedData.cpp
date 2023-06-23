//#include "stdafx.h"
#include "TZXBlockStandardSpeedData.h"
#include "TZXFile.h"
#include <stdlib.h>
#include <stdio.h>

TZXBlockStandardSpeedData::TZXBlockStandardSpeedData()
{
	m_nBlockID = TZX_BLOCKID_STANDARD_SPEED_DATA;
	m_pData = NULL;
	m_nLengthOfData = 0;
	m_nPauseAfterBlockInMS = 0;
}


TZXBlockStandardSpeedData::~TZXBlockStandardSpeedData()
{
	if (m_pData) free(m_pData);
}

char *TZXBlockStandardSpeedData::GetDescription()
{
	snprintf((char *)m_szToStringDescription, MAX_STRING_LENGTH, "Standard Speed Data Block: %d bytes.  Delay = %d ms", m_nLengthOfData, m_nPauseAfterBlockInMS);
	return m_szToStringDescription;
}

void TZXBlockStandardSpeedData::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	// Store the offset location for the current block
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

	// What type of stadard block are we, header or data?
	// Get the flag byte...
	int dataType = m_pData[0];
	if (dataType == 0)
	{
		// It's a header block...
		// Generate the standard header pilot pulse
		pAudioGenerator->SetAmp(false); // Make sure the pilot always starts on a false.
		for (int i = 0; i <= TZX_PILOT_PULSES_HEADER; i++)
		{
			pAudioGenerator->GeneratePulse(TZX_PILOT_PULSE_LENGTH);
		}
	}
	else {
		// It's a data block
		// Generate the standard data pilot pulse
		pAudioGenerator->SetAmp(false); // Make sure the pilot always starts on a false.
		for (int i = 0; i <= TZX_PILOT_PULSES_DATA; i++)
		{
			pAudioGenerator->GeneratePulse(TZX_PILOT_PULSE_LENGTH);
		}
	}

	// Generate the two sync pulses
	pAudioGenerator->GeneratePulse(TZX_FIRST_SYNC_PULSE_LENGTH);
	pAudioGenerator->GeneratePulse(TZX_SECOND_SYNC_PULSE_LENGTH);

	// Generate the actual data
	for (int i = 0; i < m_nLengthOfData; i++)
	{
		for (int n = 7; n >= 0; n--)
		{

			char mask = 1 << n;
			char bitset = m_pData[i] & mask;
			if (bitset)
			{
				// Write a one bit
				pAudioGenerator->GeneratePulse(TZX_ONE_BIT_LENGTH);
				pAudioGenerator->GeneratePulse(TZX_ONE_BIT_LENGTH);
			}
			else {
				// Write a zero bit
				pAudioGenerator->GeneratePulse(TZX_ZERO_BIT_LENGTH);
				pAudioGenerator->GeneratePulse(TZX_ZERO_BIT_LENGTH);
			}
		}
	}

	// Add any delay at the end of the block
	if (m_nPauseAfterBlockInMS) pAudioGenerator->AddSilence(m_nPauseAfterBlockInMS);
}
