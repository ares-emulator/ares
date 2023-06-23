//#include "stdafx.h"
#include "TZXBlockMessage.h"
#include "TZXFile.h"

TZXBlockMessage::TZXBlockMessage()
{
	m_nBlockID = TZX_BLOCKID_MESSAGE_BLOCK;
}


TZXBlockMessage::~TZXBlockMessage()
{
}

void TZXBlockMessage::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}

char *TZXBlockMessage::GetDescription()
{
	snprintf(m_szToStringDescription, MAX_STRING_LENGTH, "Message Block - Time = %d, Message = %s", m_nDisplayTimeInSeconds, m_szMessageString);
	return m_szToStringDescription;
}
