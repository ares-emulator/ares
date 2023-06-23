//#include "stdafx.h"
#include "TZXBlockGroupStart.h"
#include "TZXFile.h"
#include <stdlib.h>


TZXBlockGroupStart::TZXBlockGroupStart()
{
	m_nBlockID = TZX_BLOCKID_GROUP_START;
	m_szName = NULL;
}


TZXBlockGroupStart::~TZXBlockGroupStart()
{
	if (m_szName) free(m_szName);
}

char *TZXBlockGroupStart::GetDescription()
{
	snprintf(m_szToStringDescription, 8192, "Group Start Block: %s", m_szName);
	return m_szToStringDescription;
}

void TZXBlockGroupStart::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}
