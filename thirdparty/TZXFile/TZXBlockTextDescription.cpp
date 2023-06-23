//#include "stdafx.h"
#include "TZXBlockTextDescription.h"
#include "TZXFile.h"
#include <stdlib.h>
#include <stdio.h>


TZXBlockTextDescription::TZXBlockTextDescription()
{
	m_nBlockID = TZX_BLOCKID_TEXT_DESCRIPTION;
	m_szDescription = NULL;
}


TZXBlockTextDescription::~TZXBlockTextDescription()
{
	if (m_szDescription) free(m_szDescription);
}


char *TZXBlockTextDescription::GetDescription()
{
	snprintf((char *)m_szToStringDescription, MAX_STRING_LENGTH, "Text Description Block: %s", m_szDescription);
	return m_szToStringDescription;
}

void TZXBlockTextDescription::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	//printf("Block starts at time %0.3f\n", (float)(pAudioGenerator->GetCurrentLength()) / 44100.0f);
	
    // Nothing to do but log the audio position in the buffer
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}
