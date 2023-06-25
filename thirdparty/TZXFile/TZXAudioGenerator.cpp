//#include "..\stdafx.h"
#include "TZXAudioGenerator.h"
#include <stdlib.h>
#include "TZXFile.h"
#include <stdio.h>
#include <memory.h>


#define SECONDS_PER_BLOCK_CHUNK 60
#define BLOCK_SIZE (TZX_AUDIO_DATARATE * SECONDS_PER_BLOCK_CHUNK)
#define AUDIO_BUFFER_LENGTH (BLOCK_SIZE * m_nAudioBlocks)
#define HI 115
#define LO -115

TZXAudioGenerator::TZXAudioGenerator()
{
	m_pAudioData = (char *)malloc(BLOCK_SIZE);
	m_nAudioDataLengthinSamples = 0;
	m_nAudioBlocks = 1;
	m_bAmp = false;
}

TZXAudioGenerator::~TZXAudioGenerator()
{
}

void TZXAudioGenerator::SetAmp(bool bAmp)
{
	m_bAmp = bAmp;
}

void TZXAudioGenerator::AddByte(char byte)
{
	if (m_nAudioDataLengthinSamples == AUDIO_BUFFER_LENGTH)
	{
		// Need to increas the size of the buffer
		ExtendAudioBuffer();
	}

	m_pAudioData[m_nAudioDataLengthinSamples] = byte;
	m_nAudioDataLengthinSamples++;
}

void TZXAudioGenerator::ExtendAudioBuffer()
{
	//printf("Debug: extending audio buffer to %d bytes\n", AUDIO_BUFFER_LENGTH);
	char *pNewBuffer = (char *)malloc((m_nAudioBlocks + 1)*BLOCK_SIZE);
	char *pOldBuffer = m_pAudioData;
	memcpy(pNewBuffer, pOldBuffer, m_nAudioDataLengthinSamples);
	m_pAudioData = pNewBuffer;
	m_nAudioBlocks++;
	free(pOldBuffer);
}

void TZXAudioGenerator::GeneratePulse(int lengthInTStates, bool toggleAmp)
{
	// Calculate the length of the pulse in terms of samples
	// Note this method is not completely accurate so may need some finessing for very high speed turbo loaders to work accurately...
    double cycle = (double)TZX_AUDIO_DATARATE / (double)TZX_TSTATES_PER_SECOND;
	int samples = ((unsigned int)(0.5 + (cycle*(double)lengthInTStates)));
	for (int i = 0; i < samples; i++)
	{
		if (m_bAmp) AddByte(HI); else AddByte(LO);
	}

	// Automatically toggle the state unless we are specifically told not to
	if (toggleAmp)
	{
		m_bAmp = !m_bAmp;
	}
}

void TZXAudioGenerator::ResetBuffer()
{
	free(m_pAudioData);
	m_pAudioData = (char *)malloc(BLOCK_SIZE);
	m_nAudioDataLengthinSamples = 0;
	m_nAudioBlocks = 1;
	m_bAmp = false;
}

int TZXAudioGenerator::GetCurrentLength()
{
	return m_nAudioDataLengthinSamples;
}

void TZXAudioGenerator::AddSilence(int nLengthInMS)
{
	int samplesToAdd = (nLengthInMS * TZX_AUDIO_DATARATE) / 1000;
	
	if (nLengthInMS > 1  && m_bAmp)
	{
		int nRemainder = nLengthInMS - 1;
		AddSilence(1);
		m_bAmp = false;
		AddSilence(nRemainder);
	}
	else {
		for (int i = 0; i < samplesToAdd; i++)
		{
			if (m_bAmp) AddByte(HI); else AddByte(LO);
		}
	}
}

char *TZXAudioGenerator::GetAudioBufferPtr()
{
	return m_pAudioData;
}

bool TZXAudioGenerator::SaveAsUncompressedWav(const char *szFilename)
{
	FILE *hFile = NULL;
	hFile = fopen(szFilename, "wb");
	if (hFile == NULL) return false;

    // We create an uncompressed wav and manually hack the header together according to the data at the website below
	// http://www.topherlee.com/software/pcm-tut-wavformat.html

	char *riff = (char *)"RIFF";
	if (fwrite(riff, 4, 1, hFile) != 1) { fclose(hFile); return false; }

	int fileLength = m_nAudioDataLengthinSamples + 44;
	if (fwrite(&fileLength, 4, 1, hFile) != 1) { fclose(hFile); return false; }

	char *wave = (char *)"WAVEfmt ";
	if (fwrite(wave, 8, 1, hFile) != 1) { fclose(hFile); return false; }

	int formatDataLength = 16;
	if (fwrite(&formatDataLength, 4, 1, hFile) != 1) { fclose(hFile); return false; }

	short formatType = 1; // PCM
	if (fwrite(&formatType, 2, 1, hFile) != 1) { fclose(hFile); return false; }

	short channels = 1;
	if (fwrite(&channels, 2, 1, hFile) != 1) { fclose(hFile); return false; }

	int sampleRate = TZX_AUDIO_DATARATE;
	if (fwrite(&sampleRate, 4, 1, hFile) != 1) { fclose(hFile); return false; }
	if (fwrite(&sampleRate, 4, 1, hFile) != 1) { fclose(hFile); return false; }

	short bytesPerSample = 1;
	if (fwrite(&bytesPerSample, 2, 1, hFile) != 1) { fclose(hFile); return false; }

	int bitsPerSample = 8;
	if (fwrite(&bitsPerSample, 2, 1, hFile) != 1) { fclose(hFile); return false; }

	char *data = (char *)"data";
	if (fwrite(data, 4, 1, hFile) != 1) { fclose(hFile); return false; }

	int dataSize = m_nAudioDataLengthinSamples;
	if (fwrite(&dataSize, 4, 1, hFile) != 1) { fclose(hFile); return false; }

	for (int i = 0; i < dataSize; i++)
	{
		int n = m_pAudioData[i];
		n = n + 128;
		unsigned char b = (unsigned char) n;

		if (fwrite(&b, 1, 1, hFile) != 1) { fclose(hFile); return false; }
	}
	
	fclose(hFile);
	return true;
}
