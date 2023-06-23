//#include "stdafx.h"
#include "TZXBlockPulseSequence.h"
#include "TZXFile.h"

TZXBlockPulseSequence::TZXBlockPulseSequence()
{
	m_nBlockID = TZX_BLOCKID_PULSE_SEQUENCE;
}


TZXBlockPulseSequence::~TZXBlockPulseSequence()
{
}


void TZXBlockPulseSequence::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

	for (int i = 0; i < m_nPulseCount; i++)
	{
		pAudioGenerator->GeneratePulse(m_nPulseLength[i]);
	}
}

char *TZXBlockPulseSequence::GetDescription()
{
	snprintf(m_szToStringDescription, MAX_STRING_LENGTH, "Pulse Sequence Block - %d pulses in sequence.", m_nPulseCount);
	return m_szToStringDescription;
}
