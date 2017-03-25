// Wave.h: interface for the Wave class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVE_H__3E2C901E_6D0E_488C_8D02_92130C504E99__INCLUDED_)
#define AFX_WAVE_H__3E2C901E_6D0E_488C_8D02_92130C504E99__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <stdio.h>

#ifndef WAVE_FORMAT_PCM
#define	WAVE_FORMAT_PCM			0x0001 
#endif
#ifndef	WAVE_FORMAT_UNKNOWN
#define	WAVE_FORMAT_UNKNOWN		0x0000
#endif
#ifndef	WAVE_FORMAT_ADPCM
#define	WAVE_FORMAT_ADPCM		0x0002
#endif
#ifndef	WAVE_FORMAT_ALAW
#define	WAVE_FORMAT_ALAW		0x0006
#endif
#ifndef	WAVE_FORMAT_MULAW
#define	WAVE_FORMAT_MULAW		0x0007
#endif
#ifndef	WAVE_FORMAT_OKI_ADPCM
#define	WAVE_FORMAT_OKI_ADPCM	0x0010
#endif
#ifndef	WAVE_FORMAT_DIGISTD
#define	WAVE_FORMAT_DIGISTD		0x0015
#endif
#ifndef	WAVE_FORMAT_DIGIFIX
#define	WAVE_FORMAT_DIGIFIX		0x0016
#endif
#ifndef	IBM_FORMAT_MULAW
#define	IBM_FORMAT_MULAW        0x0101
#endif
#ifndef	IBM_FORMAT_ALAW
#define	IBM_FORMAT_ALAW			0x0102
#endif
#ifndef	IBM_FORMAT_ADPCM
#define	IBM_FORMAT_ADPCM        0x0103
#endif

struct RiffChunkHeader
{
	unsigned long chunkID;
	long chunkSize;

	static const unsigned long idRIFF, idWAVE, idfmt_, iddata;
};

class Wave  
{
public:
	void lin2pcmu(short *x, unsigned char *out, long len, float s = 4004.189931f);
	void pcmu2lin(unsigned char *p, short *buff_pcm, long len, float s);
	void lin2pcma(short *x, unsigned char *y, long len, short m = 85, float s = 2017.396342f);
	void pcma2lin(unsigned char *p, short *buff_pcm, long len, unsigned char m, float s);

	//creates a reader object
	//opens the given file, fails if the file does not exist
	//attributes are initialized according to the file given
	static Wave *openForRead(const char *fileName);

	//creates a writer object
	//creates or overwrites the given file
	static Wave *createForWrite(const char *fileName, unsigned long fs, unsigned long numBits, short numChannels, int waveFormatTag);
	static Wave *createForWrite(const char *fileName, Wave* wIn);
	static Wave *createForWrite(const char *fileName, Wave* wIn, int waveFormatTag);
	static Wave *createForWrite(const char *fileName, unsigned long fs, short numBits, short numChannels, int waveFormatTag);

	//creates a writer object
	//appends the given file, fails if the file does not exist
	//attributes are initialized according to the file given
	static Wave *openForAppend(const char *fileName);

	//creates a reader object
	//opens WaveIn with the given attributes
	//static Wave *openWaveIn(DWORD fs, short numBits, short numChannels);

	//creates a writer object
	//opens WaveOut with the given attributes
	//static Wave *openWaveOut(DWORD fs, short numBits, short numChannels);

	//closes the file, writes out the sizes (for a write file), does nothing on an unopened object
	void close();

	//reads numSamples data from each channel
	//these guys cast the data into the desired form
	//the return value is that of fread, or -1 for failure
	//the read data will be placed into buf[channelNo][sampleNo]
	int readData8(char **buf, int numSamples);
	int readData16(short **buf, int numSamples);
	//reads all channels and mixes down to one channel
	int readMonoData8(char *buf, int numSamples);
	int readMonoData16(short *buf, int numSamples);
	
	//seeking functions
	bool rewind() { return seek(0); };
	bool seek(int sample);
	
	//the write functions work the exact same way as the read functions
	int writeData8(char **buf, int numSamples);
	int writeData16(short **buf, int numSamples);
	//writes the same data to all channels
	int writeMonoData8(char *buf, int numSamples);
	int writeMonoData16(short *buf, int numSamples);
	//writes in correct file size and data size info into the headers
	bool writeSize();

	//the following are in samples
	int getFileLength() { return numSamples; };
	int getFilePosition() { return (ftell(fp)-dataOffset) / blockSize; };

	enum STATE {
		UNINITED,
		OPENFOR_READ,
		OPENFOR_WRITE,
		OPENFOR_APPEND,
		FAILED_TO_OPEN_FILE,
		NOT_RIFF_FILE,
		NOT_WAVE_FILE,
		CANNOT_FIND_FMT_,
		UNRECOGNIZED_ENCODING,
		CONFLICTING_FORMAT_DATA,
		CANNOT_FIND_DATA,
		FILE_SIZE_MISMATCH,
		UNEXPECTED_EOF,
		FILE_WRITE_ERROR,
		FSEEK_ERROR,
		CLOSED,
	} state;

	//can be used to get meaningful error strings
	static char* stateString(STATE state);
	char *stateString() {stateString(state);};

	FILE *fp;

	int fileSize;
	int numSamples;

	short formatTag;
	short numChannels;
	unsigned long fs;					//sampling rate samples per second
	unsigned long avgBytesPerSecond;	//for noncompressed = numChannels*fs*numBits/8
								//non trivial for compressed
	short blockSize;				//for PCM numChannels*bytesPerSample
	short numBits;				//bits per sample, it doesnt have to be a multiple of 8 for compressed formats

	virtual ~Wave();
private:
	Wave();
	
	//returns true if ok
	bool checkFormat();

	long fileSizeOffset, dataSizeOffset, dataOffset;
};


#endif // !defined(AFX_WAVE_H__3E2C901E_6D0E_488C_8D02_92130C504E99__INCLUDED_)
