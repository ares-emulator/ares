//#include "stdafx.h"
#include "TZXBlockPause.h"
#include "TZXFile.h"

TZXBlockPause::TZXBlockPause()
{
	m_nBlockID = TZX_BLOCKID_PAUSE;
}


TZXBlockPause::~TZXBlockPause()
{
}

void TZXBlockPause::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

	if (m_nPauseInMS)
	{
		pAudioGenerator->AddSilence(m_nPauseInMS);
	}
	else {
		pAudioGenerator->AddSilence(10000);
	}
}

char *TZXBlockPause::GetDescription()
{
	snprintf(m_szToStringDescription, MAX_STRING_LENGTH, "Pause Block = %d ms", m_nPauseInMS);
	return m_szToStringDescription;
}
