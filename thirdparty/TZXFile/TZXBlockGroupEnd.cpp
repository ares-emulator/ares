//#include "stdafx.h"
#include "TZXBlockGroupEnd.h"


TZXBlockGroupEnd::TZXBlockGroupEnd()
{
	m_nBlockID = TZX_BLOCKID_GROUP_END;
}


TZXBlockGroupEnd::~TZXBlockGroupEnd()
{
}


char *TZXBlockGroupEnd::GetDescription()
{
	return (char *)"Group End Block";
}

void TZXBlockGroupEnd::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}

