#pragma once

#define MAX_STRING_LENGTH 8192

#include <stdio.h>

#include "TZXAudioGenerator.h"

class TZXFile;

class TZXBlock
{
public:
	unsigned char m_nBlockID;
	char m_szToStringDescription[MAX_STRING_LENGTH];
	int m_nAudioBufferOffsetLocation;

public:
	TZXBlock();
	virtual ~TZXBlock();

	virtual char *GetDescription() = 0;
	virtual void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile) = 0;
    float GetAudioBufferOffsetLocationInSeconds();
};

