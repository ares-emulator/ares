#pragma once
#include "TZXBlock.h"
class TZXBlockGroupStart :
	public TZXBlock
{
public:
	char *m_szName;

public:
	TZXBlockGroupStart();
	~TZXBlockGroupStart();

	char *GetDescription();
	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
};

