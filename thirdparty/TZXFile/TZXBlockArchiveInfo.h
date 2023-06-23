#pragma once
#include "TZXBlock.h"
class TZXBlockArchiveInfo :
	public TZXBlock
{
public:
	unsigned char m_nTextStringIdentificationByte[256];
	char *m_szTextStrings[256];
	unsigned char m_nTextStringCount;

public:
	TZXBlockArchiveInfo();
	~TZXBlockArchiveInfo();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

