#pragma once

class TZXAudioGenerator
{
private:
	char *m_pAudioData;
	int m_nAudioBlocks;
	bool m_bAmp;
public:
    int m_nAudioDataLengthinSamples;
    
	TZXAudioGenerator();
	~TZXAudioGenerator();

	void ToggleAmp();
	void SetAmp(bool val);
	void AddByte(char byte);
	void GeneratePulse(int lengthInTStates, bool toggleAmp = true);
	void ExtendAudioBuffer();
	void ResetBuffer();
	int GetCurrentLength();
	void AddSilence(int lengthInMS);
	char *GetAudioBufferPtr();
	bool SaveAsUncompressedWav(const char *szFilename);
};

