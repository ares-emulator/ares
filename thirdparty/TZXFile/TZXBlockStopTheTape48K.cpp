//#include "stdafx.h"
#include "TZXBlockStopTheTape48K.h"


TZXBlockStopTheTape48K::TZXBlockStopTheTape48K()
{
}


TZXBlockStopTheTape48K::~TZXBlockStopTheTape48K()
{
}

void TZXBlockStopTheTape48K::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}

char *TZXBlockStopTheTape48K::GetDescription()
{
	return (char *)"Stop The Tape 48K Block";
}
