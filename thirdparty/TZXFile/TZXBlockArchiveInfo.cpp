//#include "stdafx.h"
#include "TZXBlockArchiveInfo.h"
#include <stdlib.h>
#include "TZXFile.h"
#include <string.h>


TZXBlockArchiveInfo::TZXBlockArchiveInfo()
{
	memset(m_szTextStrings, 0, sizeof(char *) * 256);
	m_nTextStringCount = 0;
	m_nBlockID = TZX_BLOCKID_ARCHIVE_INFO;
}


TZXBlockArchiveInfo::~TZXBlockArchiveInfo()
{
	for (int i = 0; i < 256; i++)
	{
		if (m_szTextStrings[i]) free(m_szTextStrings[i]);
	}
}

void TZXBlockArchiveInfo::GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile)
{
	m_nAudioBufferOffsetLocation = pAudioGenerator->GetCurrentLength();
}

char *TZXBlockArchiveInfo::GetDescription()
{
	char szTemp[8192];
	char szTemp2[300];
	snprintf(szTemp, 8192, "Archive Info Block:\n");
	for (int i = 0; i < m_nTextStringCount; i++)
	{
		char *szType;
		switch (m_nTextStringIdentificationByte[i])
		{
			case 0: // Full title
				szType = (char *)"Title: ";
				break;
			case 1: // Software house
				szType = (char *)"Software House or Publisher: ";
				break;
			case 2: // Author
				szType = (char *)"Author(s): ";
				break;
			case 3: // Year of publication
				szType = (char *)"Year: ";
				break;
			case 4: // Language
				szType = (char *)"Language: ";
				break;
			case 5: // Game / Utility type
				szType = (char *)"Type: ";
				break;
			case 6: // Price
				szType = (char *)"Price: ";
				break;
			case 7: // Protection scheme / loader
				szType = (char *)"Origin: ";
				break;
			case 0xFF: // Comment
				szType = (char *)"Comment: ";
				break;
			default: // Unknown
				szType = (char *)"? :";
				break;
		}
		snprintf(szTemp2, 300, "%s%s\n", szType, m_szTextStrings[i]);
		strcat(szTemp, szTemp2);
		strcpy(m_szToStringDescription, szTemp);
	}
	return m_szToStringDescription;
}
