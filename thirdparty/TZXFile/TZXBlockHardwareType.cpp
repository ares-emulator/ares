//#include "stdafx.h"
#include "TZXBlockHardwareType.h"
#include "TZXFile.h"
#include <stdlib.h>

TZXBlockHardwareType::TZXBlockHardwareType()
{
	m_nBlockID = TZX_BLOCKID_HARDWARE_TYPE;
	m_pHWInfo = NULL;
}


TZXBlockHardwareType::~TZXBlockHardwareType()
{
	if (m_pHWInfo) free(m_pHWInfo);
}

void TZXBlockHardwareType::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}

char *TZXBlockHardwareType::GetDescription()
{
	return (char *)"Hardware Type Block";
}
