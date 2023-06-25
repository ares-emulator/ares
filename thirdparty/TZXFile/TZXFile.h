#pragma once

// TZX File Format specification can be found at
// https://www.worldofspectrum.org/TZXformat.html

#include "TZXBlock.h"
#include "TZXBlockTextDescription.h"
#include "TZXBlockStandardSpeedData.h"
#include "TZXBlockArchiveInfo.h"
#include "TZXAudioGenerator.h"
#include "TZXBlockGroupStart.h"
#include "TZXBlockTurboSpeedData.h"
#include "TZXBlockLoopStart.h"
#include "TZXBlockPureTone.h"
#include "TZXBlockPulseSequence.h"
#include "TZXBlockLoopEnd.h"
#include "TZXBlockGroupEnd.h"
#include "TZXBlockPause.h"
#include "TZXBlockPureData.h"
#include "TZXBlockHardwareType.h"
#include "TZXBlockStopTheTape48K.h"
#include "TZXBlockCustomInfo.h"
#include "TZXBlockMessage.h"

#define MAX_TZX_BLOCKS				512

#define TZX_SUCCESS					0
#define TZX_UNEXPECTED_EOF			1
#define TZX_BAD_TZX_SIGNATURE		2
#define TZX_UNSUPPORTED_REVISION	3
#define TZX_UNHANDLED_BLOCK_TYPE	4
#define TZX_BLOCK_LIST_OVERFLOW		5
#define TZX_INVALID_TZX_DATA		6

// Supported block types
#define TZX_BLOCKID_STANDARD_SPEED_DATA		0x10
#define TZX_BLOCKID_TURBO_SPEED_DATA		0x11
#define TZX_BLOCKID_PURE_TONE				0x12
#define TZX_BLOCKID_PULSE_SEQUENCE			0x13
#define TZX_BLOCKID_PURE_DATA				0x14
#define TZX_BLOCKID_PAUSE					0x20
#define TZX_BLOCKID_GROUP_START				0x21
#define TZX_BLOCKID_GROUP_END				0x22
#define TZX_BLOCKID_LOOP_START				0x24
#define TZX_BLOCKID_LOOP_END				0x25
#define TZX_BLOCKID_STOP_THE_TAPE_48K		0x2A
#define TZX_BLOCKID_TEXT_DESCRIPTION		0x30
#define TZX_BLOCKID_MESSAGE_BLOCK			0x31
#define TZX_BLOCKID_ARCHIVE_INFO			0x32
#define TZX_BLOCKID_HARDWARE_TYPE			0x33
#define TZX_BLOCKID_CUSTOM_INFO				0x35

// Currently unsupported block types
#define TZX_BLOCKID_DIRECT_RECORDING		0x15
#define TZX_BLOCKID_CSW_RECORDING			0x18
#define TZX_BLOCKID_GENERALIZED_DATA		0x19
#define TZX_BLOCKID_JUMP_TO_BLOCK			0x23
#define TZX_BLOCKID_CALL_SEQUENCE			0x26
#define TZX_BLOCKID_RETURN_FROM_SEQUENCE	0x27
#define TZX_BLOCKID_SELECT_BLOCK			0x28
#define TZX_BLOCKID_SET_SIGNAL_LEVEL		0x2B
#define TZX_BLOCKID_GLUE					0x5A

#define TZX_TSTATES_PER_SECOND				3500000
#define TZX_AUDIO_DATARATE					44100

#define TZX_PILOT_PULSE_LENGTH				2168
#define TZX_FIRST_SYNC_PULSE_LENGTH			667
#define TZX_SECOND_SYNC_PULSE_LENGTH		735
#define TZX_ZERO_BIT_LENGTH					885 // Spec says 855 but two different sources for other projects both use 885, suspect typo in spec.
#define TZX_ONE_BIT_LENGTH					1710
#define TZX_PILOT_PULSES_HEADER				8064 // Spec says 8063 but other apps have 8064
#define TZX_PILOT_PULSES_DATA				3220 // Spec says 3223 but other apps have 3220

typedef enum {
    FileTypeUndetermined,
    FileTypeTzx,
    FileTypeTap,
} EFileType;

class TZXFile
{
private:
    EFileType m_eFileType;
	bool m_bDecodeSuccesful;
	int m_nFileOffset;
	int m_nFileLength;
	unsigned char *m_pSourceFile;
	TZXAudioGenerator *m_pAudioGenerator;

	// TZX File Info
	unsigned char m_nMajorRev;
	unsigned char m_nMinorRev;

	// Block List
	int m_nBlockCount;
	TZXBlock *m_pBlocks[MAX_TZX_BLOCKS];

	// Looping
	int m_nLoopRepitionsRemaining;
	int m_nLoopFromBlockNum;

	bool AddToBlockList(TZXBlock *pBlock);
	bool ReadBytes(unsigned char *pDest, int nCount);
	int DisplayError(int nError, void *param);
	int DecodeTextDescriptionBlock();
	int DecodeStandardSpeedDataBlock();
	int DecodeArchiveInfoBlock();
	int DecodeGroupStartBlock();
	int DecodeTurboSpeedDataBlock();
	int DecodeLoopStartBlock();
	int DecodePureToneBlock();
	int DecodePulseSequenceBlock();
	int DecodeLoopEndBlock();
	int DecodeGroupEndBlock();
	int DecodePauseBlock();
	int DecodePureDataBlock();
	int DecodeHardwareTypeBlock();
	int DecodeStopTheTape48KBlock();
	int DecodeCustomInfoBlock();
	int DecodeMessageBlock();
    int DecodeTzxFile(unsigned char *pData, int nFileLength);
    int DecodeTapFileData(unsigned char *pData, int nFileLength);

public:
	TZXFile();
	~TZXFile();
    
    int GetBlockCount();
    TZXBlock *GetBlockPtr(int nBlockNum);
    
    EFileType DecodeFile(unsigned char *pData, int nFileLength);
	int GenerateAudioData();  // This will allocate the memory
	char *GetAudioBufferPtr();
	int GetAudioBufferLength();
    int GetAudioBufferLengthInSamples();
	bool WriteAudioToUncompressedWavFile(const char *filename);

	
};


