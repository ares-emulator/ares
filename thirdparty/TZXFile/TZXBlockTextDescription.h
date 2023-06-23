#pragma once
#include "TZXBlock.h"


class TZXBlockTextDescription :
	public TZXBlock
{
public:
	unsigned char *m_szDescription;

public:
	TZXBlockTextDescription();
	~TZXBlockTextDescription();

	char *GetDescription();
	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
};

