//#include "stdafx.h"
#include "TZXBlock.h"
#include "TZXFile.h"

TZXBlock::TZXBlock()
{
	m_nAudioBufferOffsetLocation = 0;
}


TZXBlock::~TZXBlock()
{
}

float TZXBlock::GetAudioBufferOffsetLocationInSeconds()
{
    float time = (float)m_nAudioBufferOffsetLocation;
    time = time / (float)TZX_AUDIO_DATARATE;
    return time;
}
