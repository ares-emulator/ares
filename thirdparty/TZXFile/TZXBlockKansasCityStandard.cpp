//#include "stdafx.h"
#include "TZXBlockKansasCityStandard.h"
#include "TZXFile.h"
#include <stdlib.h>

TZXBlockKansasCityStandard::TZXBlockKansasCityStandard()
{
	m_nBlockID = TZX_BLOCKID_KANSAS_CITY_STANDARD;
	m_pData = NULL;
}

TZXBlockKansasCityStandard::~TZXBlockKansasCityStandard()
{
	if (m_pData) free(m_pData);
}

void TZXBlockKansasCityStandard::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();

	for (int i = 0; i < m_nPilotPulseCount; i++) {
		pAudioGenerator->GeneratePulse(m_nPilotPulseLength);
	}

	auto pulseCount = [](int count) {
		if (count == 0) return 16;
		if (count & 1) return count - 1;
		return count;
	};

	int zeroPulses = pulseCount((m_nBitConfig >> 4) & 15);
	int onePulses = pulseCount(m_nBitConfig & 15);
	int startBits = (m_nByteConfig >> 6) & 3;
	bool startBit = (m_nByteConfig >> 5) & 1;
	int stopBits = (m_nByteConfig >> 3) & 3;
	bool stopBit = (m_nByteConfig >> 2) & 1;
	bool msbFirst = m_nByteConfig & 1;

	auto generateBit = [&](bool bit) {
		int pulses = bit ? onePulses : zeroPulses;
		int pulseLength = bit ? m_nOnePulseLength : m_nZeroPulseLength;
		for (int pulse = 0; pulse < pulses; pulse++) {
			pAudioGenerator->GeneratePulse(pulseLength);
		}
	};

	for (int i = 0; i < m_nDataLength; i++) {
		for (int bit = 0; bit < startBits; bit++) generateBit(startBit);

		for (int bit = 0; bit < 8; bit++) {
			int shift = msbFirst ? 7 - bit : bit;
			generateBit(m_pData[i] & (1 << shift));
		}

		for (int bit = 0; bit < stopBits; bit++) generateBit(stopBit);
	}

	if (m_nPauseAfterBlock) pAudioGenerator->AddSilence(m_nPauseAfterBlock);
}

char *TZXBlockKansasCityStandard::GetDescription()
{
	snprintf((char *)m_szToStringDescription, MAX_STRING_LENGTH, "Kansas City Standard Block: %d bytes", m_nDataLength);
	return m_szToStringDescription;
}
