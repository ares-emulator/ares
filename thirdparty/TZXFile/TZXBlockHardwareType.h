#pragma once
#include "TZXBlock.h"

#define HWTYPE_COMPUTERS				0x00
#define HWTYPE_EXTERNAL_STORAGE			0x01
#define HWTYPE_ROM_RAM_ADD_ON			0x02
#define HWTYPE_SOUND_DEVICES			0x03
#define HWTYPE_JOYSTICKS				0x04
#define HWTYPE_MICE						0x05
#define HWTYPE_OTHER_CONTROLLERS		0x06
#define HWTYPE_SERIAL_PORTS				0x07
#define HWTYPE_PARALLEL_PORTS			0x08
#define HWTYPE_PRINTERS					0x09
#define HWTYPE_MODEMS					0x0A
#define HWTYPE_DIGITIZERS				0x0B
#define HWTYPE_NETWORK_ADAPTERS			0x0C
#define HWTYPE_KEYBOARDS_KEYPADS		0x0D
#define HWTYPE_AD_DA_CONVERTORS			0x0E
#define HWTYPE_EPROM_PROGRAMMERS		0x0F
#define HWTYPE_GRAPHICS					0x0G

#define HWID_ZX_SPECTRUM_16K			0x00
/*

Hardware type	Hardware ID
00 - Computers	00 - ZX Spectrum 16k
01 - ZX Spectrum 48k, Plus
02 - ZX Spectrum 48k ISSUE 1
03 - ZX Spectrum 128k +(Sinclair)
04 - ZX Spectrum 128k +2 (grey case)
05 - ZX Spectrum 128k +2A, +3
06 - Timex Sinclair TC-2048
07 - Timex Sinclair TS-2068
08 - Pentagon 128
09 - Sam Coupe
0A - Didaktik M
0B - Didaktik Gama
0C - ZX-80
0D - ZX-81
0E - ZX Spectrum 128k, Spanish version
0F - ZX Spectrum, Arabic version
10 - Microdigital TK 90-X
11 - Microdigital TK 95
12 - Byte
13 - Elwro 800-3
14 - ZS Scorpion 256
15 - Amstrad CPC 464
16 - Amstrad CPC 664
17 - Amstrad CPC 6128
18 - Amstrad CPC 464+
19 - Amstrad CPC 6128+
1A - Jupiter ACE
1B - Enterprise
1C - Commodore 64
1D - Commodore 128
1E - Inves Spectrum+
1F - Profi
20 - GrandRomMax
21 - Kay 1024
22 - Ice Felix HC 91
23 - Ice Felix HC 2000
24 - Amaterske RADIO Mistrum
25 - Quorum 128
26 - MicroART ATM
27 - MicroART ATM Turbo 2
28 - Chrome
29 - ZX Badaloc
2A - TS-1500
2B - Lambda
2C - TK-65
2D - ZX-97
01 - External storage	00 - ZX Microdrive
01 - Opus Discovery
02 - MGT Disciple
03 - MGT Plus-D
04 - Rotronics Wafadrive
05 - TR-DOS (BetaDisk)
06 - Byte Drive
07 - Watsford
08 - FIZ
09 - Radofin
0A - Didaktik disk drives
0B - BS-DOS (MB-02)
0C - ZX Spectrum +3 disk drive
0D - JLO (Oliger) disk interface
0E - Timex FDD3000
0F - Zebra disk drive
10 - Ramex Millenia
11 - Larken
12 - Kempston disk interface
13 - Sandy
14 - ZX Spectrum +3e hard disk
15 - ZXATASP
16 - DivIDE
17 - ZXCF
02 - ROM/RAM type add-ons	00 - Sam Ram
01 - Multiface ONE
02 - Multiface 128k
03 - Multiface +3
04 - MultiPrint
05 - MB-02 ROM/RAM expansion
06 - SoftROM
07 - 1k
08 - 16k
09 - 48k
0A - Memory in 8-16k used
03 - Sound devices	00 - Classic AY hardware (compatible with 128k ZXs)
01 - Fuller Box AY sound hardware
02 - Currah microSpeech
03 - SpecDrum
04 - AY ACB stereo (A+C=left, B+C=right); Melodik
05 - AY ABC stereo (A+B=left, B+C=right)
06 - RAM Music Machine
07 - Covox
08 - General Sound
09 - Intec Electronics Digital Interface B8001
0A - Zon-X AY
0B - QuickSilva AY
0C - Jupiter ACE
04 - Joysticks	00 - Kempston
01 - Cursor, Protek, AGF
02 - Sinclair 2 Left (12345)
03 - Sinclair 1 Right (67890)
04 - Fuller
05 - Mice	00 - AMX mouse
01 - Kempston mouse
06 - Other controllers	00 - Trickstick
01 - ZX Light Gun
02 - Zebra Graphics Tablet
03 - Defender Light Gun
07 - Serial ports	00 - ZX Interface 1
01 - ZX Spectrum 128k
08 - Parallel ports	00 - Kempston S
01 - Kempston E
02 - ZX Spectrum +3
03 - Tasman
04 - DK'Tronics
05 - Hilderbay
06 - INES Printerface
07 - ZX LPrint Interface 3
08 - MultiPrint
09 - Opus Discovery
0A - Standard 8255 chip with ports 31,63,95
09 - Printers	00 - ZX Printer, Alphacom 32 & compatibles
01 - Generic printer
02 - EPSON compatible
0A - Modems	00 - Prism VTX 5000
01 - T/S 2050 or Westridge 2050
0B - Digitizers	00 - RD Digital Tracer
01 - DK'Tronics Light Pen
02 - British MicroGraph Pad
03 - Romantic Robot Videoface
0C - Network adapters	00 - ZX Interface 1
0D - Keyboards & keypads	00 - Keypad for ZX Spectrum 128k
0E - AD/DA converters	00 - Harley Systems ADC 8.2
01 - Blackboard Electronics
0F - EPROM programmers	00 - Orme Electronics
10 - Graphics	00 - WRX Hi-Res
01 - G007
02 - Memotech
03 - Lambda Colour


*/

#define HWINFO_TAPE_RUNS_ON_THIS_MACHINE									0x00
#define HWINFO_TAPE_USES_HARDWARE_OR_SPECIAL_FEATURES_OF_THIS_MACHINE		0x01
#define HWINFO_TAPE_RUNS_BUT_DOES_NOT_USE_SPECIAL_FEATURES_OF_THIS_MACHIEN	0x02
#define HWINFO_TAPE_DOESNT_RUN_ON_THIS_MACHINE_WITH_THIS_HARDWARE			0x03

typedef struct
{
	unsigned char nHardwareType;
	unsigned char nHardwareID;
	unsigned char nHardwareInfo;
} THWINFO;


class TZXBlockHardwareType :
	public TZXBlock
{
public:
	unsigned char m_nHwInfoCount;
	THWINFO *m_pHWInfo;


	TZXBlockHardwareType();
	~TZXBlockHardwareType();

	void GenerateAudio(TZXAudioGenerator *pAudioGenerator, TZXFile *pTZXFile);
	char *GetDescription();
};

