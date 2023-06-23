#pragma once
#include "TZXBlock.h"
#include "TZXFile.h"

class TZXBlockGroupEnd :
	public TZXBlock
{
public:
	TZXBlockGroupEnd();
	~TZXBlockGroupEnd();


	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

