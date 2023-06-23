#pragma once
#include "TZXBlock.h"
class TZXBlockLoopStart :
	public TZXBlock
{
public:
	unsigned short m_nRepitions;

public:
	TZXBlockLoopStart();
	~TZXBlockLoopStart();

	char *GetDescription();
	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
};

