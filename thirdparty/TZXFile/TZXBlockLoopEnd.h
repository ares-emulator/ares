#pragma once
#include "TZXBlock.h"
class TZXBlockLoopEnd :
	public TZXBlock
{
public:
	TZXBlockLoopEnd();
	~TZXBlockLoopEnd();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

