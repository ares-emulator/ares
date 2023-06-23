#pragma once
#include "TZXBlock.h"
#include "TZXFile.h"

class TZXBlockStopTheTape48K :
	public TZXBlock
{

public:
	TZXBlockStopTheTape48K();
	~TZXBlockStopTheTape48K();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

