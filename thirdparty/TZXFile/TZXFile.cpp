//#include "stdafx.h"
#include "TZXFile.h"
#include <stdlib.h>
#include <string.h>


TZXFile::TZXFile()
{
	m_bDecodeSuccesful = false;
	m_nFileOffset = 0;
	m_pSourceFile = NULL;
	m_nFileLength = 0;
	m_nMajorRev = 0;
	m_nMinorRev = 0;
	m_nBlockCount = 0;
	memset(m_pBlocks, 0, sizeof(TZXBlock *) * MAX_TZX_BLOCKS);
	m_pAudioGenerator = new TZXAudioGenerator();
    m_eFileType = FileTypeUndetermined;
}

TZXFile::~TZXFile()
{
	for (int i = 0; i < m_nBlockCount; i++)
	{
		delete m_pBlocks[i];
	}
	delete m_pAudioGenerator;
}

bool TZXFile::AddToBlockList(TZXBlock *pBlock)
{
	if (m_nBlockCount >= MAX_TZX_BLOCKS)
	{
		return false;
	}
	m_pBlocks[m_nBlockCount] = pBlock;
	m_nBlockCount++;
	return true;
}

int TZXFile::GetBlockCount()
{
    return m_nBlockCount;
}

TZXBlock *TZXFile::GetBlockPtr(int nBlockNum)
{
    return m_pBlocks[nBlockNum];
}

int TZXFile::GetAudioBufferLengthInSamples()
{
    return m_pAudioGenerator->m_nAudioDataLengthinSamples;
}

bool TZXFile::ReadBytes(unsigned char *pDest, int nCount)
{
    if(nCount < 0) return false;
	if ((m_nFileOffset + nCount) > m_nFileLength) return false;
	memcpy(pDest, m_pSourceFile+m_nFileOffset, nCount);
	m_nFileOffset += nCount;
	return true;
}

int TZXFile::DisplayError(int nError, void *param)
{
	switch (nError)
	{
	case TZX_UNEXPECTED_EOF:
		printf("Unexpected end of file, reading read beyond end of file.\n");
		break;
	case TZX_BAD_TZX_SIGNATURE:
		printf("Bad TZX Signature in file, expected ZXTape!, but got %s\n", (char *)param);
		break;
	case TZX_UNSUPPORTED_REVISION:
		printf("Unsupported TZX revision: %d.%d\n", m_nMajorRev, m_nMinorRev);
		break;
	case TZX_UNHANDLED_BLOCK_TYPE:
		printf("The block type 0x%02x is not handled by this utility.\n", *(unsigned char *)param);
		break;
	case TZX_BLOCK_LIST_OVERFLOW:
		printf("Block list overflow, the current build can only support a maximum of %d blocks in a TZX file.\n", MAX_TZX_BLOCKS);
		break;
	default:
		printf("Unknown error code: %d\n", nError);

	}
	return nError;
}


int TZXFile::DecodeTextDescriptionBlock()
{
	unsigned char nTextLength = 0;
	if (!ReadBytes(&nTextLength, 1)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);

	TZXBlockTextDescription *tdb = new TZXBlockTextDescription();
	tdb->m_szDescription = (unsigned char *)malloc(nTextLength + 1);

	if (!ReadBytes(tdb->m_szDescription, nTextLength))
	{
		delete tdb;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	tdb->m_szDescription[nTextLength] = 0; // Add null termination

	if (!AddToBlockList(tdb))
	{
		delete tdb;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeStandardSpeedDataBlock()
{
	TZXBlockStandardSpeedData *ssd = new TZXBlockStandardSpeedData();
	if (!ReadBytes((unsigned char *)&ssd->m_nPauseAfterBlockInMS, 2))
	{
		delete ssd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&ssd->m_nLengthOfData, 2))
	{
		delete ssd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	ssd->m_pData = (unsigned char *)malloc(ssd->m_nLengthOfData);
	if (!ReadBytes(ssd->m_pData, ssd->m_nLengthOfData))
	{
		delete ssd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(ssd))
	{
		delete ssd;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeArchiveInfoBlock()
{
	TZXBlockArchiveInfo *ai = new TZXBlockArchiveInfo();

	unsigned short blockLength = 0;
	if (!ReadBytes((unsigned char *)(&blockLength), 2))
	{
		delete ai;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes(&ai->m_nTextStringCount, 1))
	{
		delete ai;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	for (int i = 0; i < ai->m_nTextStringCount; i++)
	{
		if (!ReadBytes(&ai->m_nTextStringIdentificationByte[i], 1))
		{
			delete ai;
			return DisplayError(TZX_UNEXPECTED_EOF, NULL);
		}

		unsigned char strLen = 0;
		if (!ReadBytes(&strLen, 1))
		{
			delete ai;
			return DisplayError(TZX_UNEXPECTED_EOF, NULL);
		}

		ai->m_szTextStrings[i] = (char *)malloc(strLen + 1);
		if (!ReadBytes((unsigned char *)(ai->m_szTextStrings[i]), strLen))
		{
			delete ai;
			return DisplayError(TZX_UNEXPECTED_EOF, NULL);
		}
		ai->m_szTextStrings[i][strLen] = 0;
	}

	if (!AddToBlockList(ai))
	{
		delete ai;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}

	return TZX_SUCCESS;
}

int TZXFile::DecodeGroupStartBlock()
{
	unsigned char nTextLength = 0;
	if (!ReadBytes(&nTextLength, 1)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);

	TZXBlockGroupStart *gs = new TZXBlockGroupStart();
	gs->m_szName = (char *)malloc(nTextLength + 1);

	if (!ReadBytes((unsigned char *)gs->m_szName, nTextLength))
	{
		delete gs;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	gs->m_szName[nTextLength] = 0; // Add null termination

	if (!AddToBlockList(gs))
	{
		delete gs;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeTurboSpeedDataBlock()
{
	TZXBlockTurboSpeedData *tsd = new TZXBlockTurboSpeedData();

	if (!ReadBytes((unsigned char *)&tsd->m_nPilotPulse, 2))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&tsd->m_nFirstSyncPulse, 2))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&tsd->m_nSecondSyncPulse, 2))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&tsd->m_nZeroBitPulse, 2))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&tsd->m_nOneBitPulse, 2))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&tsd->m_nPilotCount, 2))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&tsd->m_nBitsInLastByte, 1))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&tsd->m_nDelayAfterBlock, 2))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	unsigned char temp[3];
	if (!ReadBytes((unsigned char *)temp, 3))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}
	tsd->m_nDataLength = 0;
	tsd->m_nDataLength = temp[0] | (temp[1] << 8) | (temp[2] << 16);

	tsd->m_pData = (unsigned char *)malloc(tsd->m_nDataLength);
	if (!ReadBytes((unsigned char *)tsd->m_pData, tsd->m_nDataLength))
	{
		delete tsd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(tsd))
	{
		delete tsd;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeLoopStartBlock()
{
	TZXBlockLoopStart *ls = new TZXBlockLoopStart();

	if (!ReadBytes((unsigned char *)&ls->m_nRepitions, 2))
	{
		delete ls;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(ls))
	{
		delete ls;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodePureToneBlock()
{
	TZXBlockPureTone *pt = new TZXBlockPureTone();

	if (!ReadBytes((unsigned char *)&pt->m_nPulseLengthInTStates, 2))
	{
		delete pt;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&pt->m_nNumberOfPulses, 2))
	{
		delete pt;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(pt))
	{
		delete pt;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodePulseSequenceBlock()
{
	TZXBlockPulseSequence *ps = new TZXBlockPulseSequence();

	if (!ReadBytes((unsigned char *)&ps->m_nPulseCount, 1))
	{
		delete ps;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	for (int i = 0; i < ps->m_nPulseCount; i++)
	{
		if (!ReadBytes((unsigned char *)&(ps->m_nPulseLength[i]), 2))
		{
			delete ps;
			return DisplayError(TZX_UNEXPECTED_EOF, NULL);
		}
	}

	if (!AddToBlockList(ps))
	{
		delete ps;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeLoopEndBlock()
{
	TZXBlockLoopEnd *le = new TZXBlockLoopEnd();
	if (!AddToBlockList(le))
	{
		delete le;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;

}


int TZXFile::DecodeGroupEndBlock()
{
	TZXBlockGroupEnd *le = new TZXBlockGroupEnd();
	if (!AddToBlockList(le))
	{
		delete le;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;

}

int TZXFile::DecodePauseBlock()
{
	TZXBlockPause *p = new TZXBlockPause();

	if (!ReadBytes((unsigned char *)&p->m_nPauseInMS, 2))
	{
		delete p;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(p))
	{
		delete p;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodePureDataBlock()
{
	TZXBlockPureData *pd = new TZXBlockPureData();

	if (!ReadBytes((unsigned char *)&pd->m_nLengthOfZeroPulse, 2))
	{
		delete pd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&pd->m_nLengthOfOnePulse, 2))
	{
		delete pd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&pd->m_nBitsInLastByte,1))
	{
		delete pd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&pd->m_nPauseAfterBlock,2))
	{
		delete pd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	unsigned char temp[3];
	if (!ReadBytes((unsigned char *)temp, 3))
	{
		delete pd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}
	pd->m_nDataLength = 0;
	pd->m_nDataLength = temp[0] | (temp[1] << 8) | (temp[2] << 16);

	pd->m_pData = (unsigned char *)malloc(pd->m_nDataLength);
	if (!ReadBytes((unsigned char *)pd->m_pData, pd->m_nDataLength))
	{
		delete pd;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(pd))
	{
		delete pd;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeHardwareTypeBlock()
{
	TZXBlockHardwareType *ht = new TZXBlockHardwareType();

	if (!ReadBytes((unsigned char *)&ht->m_nHwInfoCount, 1))
	{
		delete ht;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	ht->m_pHWInfo = (THWINFO *)malloc(sizeof(THWINFO) * ht->m_nHwInfoCount);
	for (int i = 0; i < ht->m_nHwInfoCount; i++)
	{
		THWINFO *phwInfo = &(ht->m_pHWInfo[i]);

		if (!ReadBytes((unsigned char *)&phwInfo->nHardwareType, 1))
		{
			delete ht;
			return DisplayError(TZX_UNEXPECTED_EOF, NULL);
		}
		if (!ReadBytes((unsigned char *)&phwInfo->nHardwareID, 1))
		{
			delete ht;
			return DisplayError(TZX_UNEXPECTED_EOF, NULL);
		}
		if (!ReadBytes((unsigned char *)&phwInfo->nHardwareInfo, 1))
		{
			delete ht;
			return DisplayError(TZX_UNEXPECTED_EOF, NULL);
		}
	}

	if (!AddToBlockList(ht))
	{
		delete ht;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeStopTheTape48KBlock()
{
	int temp;

	TZXBlockStopTheTape48K *st = new TZXBlockStopTheTape48K();

	if (!ReadBytes((unsigned char *)&temp, 4))
	{
		delete st;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(st))
	{
		delete st;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeCustomInfoBlock()
{
	TZXBlockCustomInfo *ci = new TZXBlockCustomInfo();

	if (!ReadBytes((unsigned char *)&ci->m_szIDString[0], 16))
	{
		delete ci;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}
	ci->m_szIDString[16] = 0;

	if (!ReadBytes((unsigned char *)&ci->m_nCustomInfoLength, 4))
	{
		delete ci;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	ci->m_pCustomData = (unsigned char *)malloc(ci->m_nCustomInfoLength);
	if (!ReadBytes((unsigned char *)ci->m_pCustomData, ci->m_nCustomInfoLength))
	{
		delete ci;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!AddToBlockList(ci))
	{
		delete ci;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}

int TZXFile::DecodeMessageBlock()
{
	TZXBlockMessage *m = new TZXBlockMessage();

	if (!ReadBytes((unsigned char *)&m->m_nDisplayTimeInSeconds, 1))
	{
		delete m;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)&m->m_nMessageLength, 1))
	{
		delete m;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}

	if (!ReadBytes((unsigned char *)m->m_szMessageString, m->m_nMessageLength))
	{
		delete m;
		return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	}
	m->m_szMessageString[m->m_nMessageLength] = 0;

	if (!AddToBlockList(m))
	{
		delete m;
		return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
	}
	return TZX_SUCCESS;
}


EFileType TZXFile::DecodeFile(unsigned char *pData, int nFileLength)
{
    // Try to decode the file as a Tzx file and if that doesn't work try to decode it as a Tap File
    
    int result = DecodeTzxFile(pData, nFileLength);
    if(result == TZX_SUCCESS)
    {
        m_eFileType = FileTypeTzx;
        return FileTypeTzx;
    }
    
    result = DecodeTapFileData(pData, nFileLength);
    if(result == TZX_SUCCESS)
    {
        m_eFileType = FileTypeTap;
        return FileTypeTap;
    }
    
    m_eFileType = FileTypeUndetermined;
    return FileTypeUndetermined;
}


int TZXFile::DecodeTapFileData(unsigned char *pData, int nFileLength)
{
    // Tap file support is kind of an after thought hack whereby the tap file will be loaded and converted into a TZX containing standard loading blocks using the data in the tap file
    
    m_bDecodeSuccesful = false;
    m_nFileOffset = 0;
    m_nFileLength = nFileLength;
    m_pSourceFile = pData;
    
    while(m_nFileOffset < m_nFileLength)
    {
        // Decode the next chunk of the tap file
        // Read 2 bytes, these are the length of the chunk
        unsigned short nChunkLength;
        if(!ReadBytes((unsigned char *)&nChunkLength, 2)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);
        
        unsigned char nFlag;
        if(!ReadBytes(&nFlag,1))return DisplayError(TZX_UNEXPECTED_EOF, NULL);
        
        if(nFlag == 0x00) // Header
        {
            TZXBlockStandardSpeedData *ssd = new TZXBlockStandardSpeedData();
            ssd->m_nPauseAfterBlockInMS = 1000;
            ssd->m_nLengthOfData = nChunkLength;
            ssd->m_pData = (unsigned char *)malloc(ssd->m_nLengthOfData);
            *ssd->m_pData = nFlag;
            if (!ReadBytes((ssd->m_pData)+1, ssd->m_nLengthOfData-1))
            {
                delete ssd;
                return DisplayError(TZX_UNEXPECTED_EOF, NULL);
            }
            
            char name[11];
            memcpy(name, ssd->m_pData+1, 10);
            name[10] = 0;
            
            printf("Header Name = %s.\n", name);
            
            if (!AddToBlockList(ssd))
            {
                delete ssd;
                return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
            }
        } else if(nFlag==0xFF){
            TZXBlockStandardSpeedData *ssd = new TZXBlockStandardSpeedData();
            ssd->m_nPauseAfterBlockInMS = 3000;
            ssd->m_nLengthOfData = nChunkLength;
            ssd->m_pData = (unsigned char *)malloc(ssd->m_nLengthOfData);
            *ssd->m_pData = nFlag;
            if (!ReadBytes(ssd->m_pData+1, ssd->m_nLengthOfData-1))
            {
                delete ssd;
                return DisplayError(TZX_UNEXPECTED_EOF, NULL);
            }
            
            printf("Code Length = %d.\n", nChunkLength-2);
            
            if (!AddToBlockList(ssd))
            {
                delete ssd;
                return DisplayError(TZX_BLOCK_LIST_OVERFLOW, NULL);
            }
        } else {
            return DisplayError(TZX_UNHANDLED_BLOCK_TYPE, NULL);
        }
    }
    
    m_bDecodeSuccesful = true;
    return TZX_SUCCESS;
}
                   
int TZXFile::DecodeTzxFile(unsigned char *pData, int nFileLength)
{
	m_bDecodeSuccesful = false;
	m_nFileOffset = 0;
	m_nFileLength = nFileLength;
	m_pSourceFile = pData;

	// Read the TZX Header
	// First TZX Signature
	unsigned char szTemp[1024];
	if (!ReadBytes(szTemp, 7)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	szTemp[7] = 0;
	if (strcmp((char *)szTemp, "ZXTape!") != 0) return DisplayError(TZX_BAD_TZX_SIGNATURE, szTemp);

	// end of text marker
	if (!ReadBytes(szTemp, 1)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	if (szTemp[0] != 0x1a) return DisplayError(TZX_BAD_TZX_SIGNATURE, szTemp);

	// Major / minor revision
	if (!ReadBytes(&m_nMajorRev, 1)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);
	if (!ReadBytes(&m_nMinorRev, 1)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);

	if (m_nMajorRev > 1) return DisplayError(TZX_UNSUPPORTED_REVISION, NULL);
	if((m_nMajorRev == 1) && (m_nMinorRev > 20)) return DisplayError(TZX_UNSUPPORTED_REVISION, NULL);


	// Read the blocks...
	while (m_nFileOffset < m_nFileLength)
	{
		// Read the next block
		// Read the block type identifier
		unsigned char nBlockID = 0;
		if (!ReadBytes(&nBlockID, 1)) return DisplayError(TZX_UNEXPECTED_EOF, NULL);

		int result = 0;
		switch (nBlockID)
		{
		case TZX_BLOCKID_STANDARD_SPEED_DATA: // 0x10
			result = DecodeStandardSpeedDataBlock();
			break;
		case TZX_BLOCKID_TURBO_SPEED_DATA: // 0x11
			result = DecodeTurboSpeedDataBlock();
			break;
		case TZX_BLOCKID_PURE_TONE: // 0x12
			result = DecodePureToneBlock();
			break;
		case TZX_BLOCKID_PULSE_SEQUENCE: // 0x13
			result = DecodePulseSequenceBlock();
			break;
		case TZX_BLOCKID_PURE_DATA: // 0x14
			result = DecodePureDataBlock();
			break;
		case TZX_BLOCKID_PAUSE: // 0x20
			result = DecodePauseBlock();
			break;
		case TZX_BLOCKID_GROUP_START: // 0x21
			result = DecodeGroupStartBlock();
			break;
		case TZX_BLOCKID_GROUP_END: // 0x22
			result = DecodeGroupEndBlock();
			break;
		case TZX_BLOCKID_LOOP_START: //0x24
			result = DecodeLoopStartBlock();
			break;
		case TZX_BLOCKID_LOOP_END: // 0x25
			result = DecodeLoopEndBlock();
			break;
		case TZX_BLOCKID_STOP_THE_TAPE_48K:
			result = DecodeStopTheTape48KBlock();
			break;
		case TZX_BLOCKID_TEXT_DESCRIPTION: // 0x30
			result = DecodeTextDescriptionBlock();
			break;
		case TZX_BLOCKID_MESSAGE_BLOCK: // 0x31
			result = DecodeMessageBlock();
			break;
		case TZX_BLOCKID_ARCHIVE_INFO: // 0x32
			result = DecodeArchiveInfoBlock();
			break;
		case TZX_BLOCKID_HARDWARE_TYPE: // 0x33
			result = DecodeHardwareTypeBlock();
			break;
		case TZX_BLOCKID_CUSTOM_INFO: // 0x35
			result = DecodeCustomInfoBlock();
			break;
		default:
			return DisplayError(TZX_UNHANDLED_BLOCK_TYPE, &nBlockID);
		}
		if (result != TZX_SUCCESS) return result;

		printf("Block %2d: %s\n", m_nBlockCount - 1, m_pBlocks[m_nBlockCount - 1]->GetDescription());
	}

	m_bDecodeSuccesful = true;
	return TZX_SUCCESS;
}

char *TZXFile::GetAudioBufferPtr()
{
	return m_pAudioGenerator->GetAudioBufferPtr();
}

int TZXFile::GetAudioBufferLength()
{
	return m_pAudioGenerator->GetCurrentLength();
}


int TZXFile::GenerateAudioData()
{
	// Clear any previously generated audio data in the buffer
	m_pAudioGenerator->ResetBuffer();

	// Go through each block and generate the audio
	for (int i = 0; i < m_nBlockCount; i++)
	{
		TZXBlock *bl = m_pBlocks[i];
		//printf("Generating audio for block %d - type %02x\n", i, bl->m_nBlockID);
		bl->GenerateAudio(m_pAudioGenerator, this);

		// Any additional special processing required for this block?
		switch (bl->m_nBlockID)
		{
		case TZX_BLOCKID_LOOP_START:
			m_nLoopRepitionsRemaining = ((TZXBlockLoopStart *)bl)->m_nRepitions;
			m_nLoopFromBlockNum = i;
			break;
		case TZX_BLOCKID_LOOP_END:
			m_nLoopRepitionsRemaining--;
			if (m_nLoopRepitionsRemaining) i = m_nLoopFromBlockNum;
			break;
		default:
			// No special processing
			break;
		}
	}


	return 0;
}

bool TZXFile::WriteAudioToUncompressedWavFile(const char *filename)
{
	if (m_pAudioGenerator->GetAudioBufferPtr() == NULL) return false;

	return m_pAudioGenerator->SaveAsUncompressedWav(filename);
}



