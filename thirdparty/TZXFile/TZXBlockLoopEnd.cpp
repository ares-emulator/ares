//#include "stdafx.h"
#include "TZXBlockLoopEnd.h"
#include "TZXFile.h"

TZXBlockLoopEnd::TZXBlockLoopEnd()
{
	m_nBlockID = TZX_BLOCKID_LOOP_END;
}


TZXBlockLoopEnd::~TZXBlockLoopEnd()
{
}


void TZXBlockLoopEnd::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}

char *TZXBlockLoopEnd::GetDescription()
{
	return (char *)"Loop End Block";
}
