//#include "stdafx.h"
#include "TZXBlockLoopStart.h"
#include "TZXFile.h"


TZXBlockLoopStart::TZXBlockLoopStart()
{
	m_nBlockID = TZX_BLOCKID_LOOP_START;
}


TZXBlockLoopStart::~TZXBlockLoopStart()
{
}

char *TZXBlockLoopStart::GetDescription()
{
	snprintf(m_szToStringDescription, MAX_STRING_LENGTH, "Loop Start Block - %d repitions", m_nRepitions);
	return m_szToStringDescription;
}

void TZXBlockLoopStart::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

}
