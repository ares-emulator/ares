//#include "stdafx.h"
#include "TZXBlockPureTone.h"
#include "TZXFile.h"
#include <stdio.h>

TZXBlockPureTone::TZXBlockPureTone()
{
	m_nBlockID = TZX_BLOCKID_PURE_TONE;
}


TZXBlockPureTone::~TZXBlockPureTone()
{
}


void TZXBlockPureTone::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

	for (int i = 0; i < m_nNumberOfPulses; i++)
	{
		pAudioGenerator->GeneratePulse(m_nPulseLengthInTStates);
	}
}

char *TZXBlockPureTone::GetDescription()
{
	snprintf(m_szToStringDescription, MAX_STRING_LENGTH, "Pure Tone Block - %d repitions of %d T-States.", m_nNumberOfPulses, m_nPulseLengthInTStates);
	return m_szToStringDescription;
}
