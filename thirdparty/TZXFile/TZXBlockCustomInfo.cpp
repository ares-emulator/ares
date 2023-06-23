//#include "stdafx.h"
#include "TZXBlockCustomInfo.h"
#include "TZXFile.h"
#include <stdlib.h>
#include <stdio.h>


TZXBlockCustomInfo::TZXBlockCustomInfo()
{
	m_nBlockID = TZX_BLOCKID_CUSTOM_INFO;
	m_pCustomData = NULL;
}


TZXBlockCustomInfo::~TZXBlockCustomInfo()
{
	if (m_pCustomData) free(m_pCustomData);
}

void TZXBlockCustomInfo::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}

char *TZXBlockCustomInfo::GetDescription()
{
	snprintf(m_szToStringDescription, MAX_STRING_LENGTH, "Custom Info Block, ID = %s, Length = %d", m_szIDString, m_nCustomInfoLength);
	return m_szToStringDescription;
}
