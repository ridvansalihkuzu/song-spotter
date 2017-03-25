// Wave.cpp: implementation of the Wave class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Wave.h"
#include <io.h>
#include <math.h>
#include "L_encoding.h"
#include "FuncDef.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Static stuff
//////////////////////////////////////////////////////////////////////////
const unsigned long RiffChunkHeader::idRIFF = 0x46464952; //0x52494646;	//"RIFF"
const unsigned long RiffChunkHeader::idWAVE = 0x45564157; //0x57415645;	//"WAVE"
const unsigned long RiffChunkHeader::idfmt_ = 0x20746d66; //0x666d7420;	//"fmt "
const unsigned long RiffChunkHeader::iddata = 0x61746164; //0x64617461;	//"data"

Wave::Wave()
{
	state = UNINITED;

	fp = NULL;

	fileSize = 0;
	numSamples = 0;

	formatTag = 0;
	numChannels = 0;
	fs = 0;
	avgBytesPerSecond = 0;
	blockSize = 0;
	numBits = 0;

	fileSizeOffset = 0;
	dataSizeOffset = 0;
	dataOffset = 0;
}

Wave::~Wave()
{
	close();
}

Wave *Wave::openForRead(const char *fileName)
{
	Wave *ret = new Wave;
	RiffChunkHeader chunk;
	int remainingSize = 0;
	long fileLength = 0;
	
	if (fileName == "")
		goto openfor_read_fail;

	ret->fp = fopen(fileName, "rb");
	if(ret->fp == NULL)
	{
		ret->state = FAILED_TO_OPEN_FILE;
		return ret;
	}

	//now we will read our first header, the actual RIFF header
	if(fread(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
	{
		ret->state = UNEXPECTED_EOF;
		goto openfor_read_fail;
	}

	//the id of this first chunk should be equal to RIFF, with the size giving the 
	//filesize - 8
	if(chunk.chunkID != RiffChunkHeader::idRIFF)
	{
		ret->state = NOT_RIFF_FILE;
		goto openfor_read_fail;
	}
	//ill check the file size reported by the riff header and compare it
	//with the actual file size reported by the system, in the event of a
	//mismatch ill go with the system reported size
	//actually this check is redundant, i can simply ignore the file size
	//reported in the riff header and go with the system size, but im leaving
	//this check here so i might later add something useful here
	fileLength = filelength(fileno(ret->fp));
	if(fileLength != chunk.chunkSize+8)
		ret->fileSize = fileLength;
	else
		ret->fileSize = chunk.chunkSize + 8;
	ret->fileSizeOffset = ftell(ret->fp) - 4;
	remainingSize = ret->fileSize - sizeof(RiffChunkHeader);

	//here we expect the "WAVE" indicator
	if(remainingSize == 0)
	{
		ret->state = NOT_WAVE_FILE;
		goto openfor_read_fail;
	}
	unsigned long wave;
	if(fread(&wave, sizeof(unsigned long), 1, ret->fp) != 1)
	{
		ret->state = UNEXPECTED_EOF;
		goto openfor_read_fail;
	}
	remainingSize -= sizeof(unsigned long);
	if(wave != RiffChunkHeader::idWAVE)
	{
		ret->state = NOT_WAVE_FILE;
		goto openfor_read_fail;
	}

	//now we expect the fmt_ header, but we are not guaranteed to get it right away
	while (true)
	{
		if(remainingSize == 0)
		{
			ret->state = CANNOT_FIND_FMT_;
			goto openfor_read_fail;
		}
		if(fread(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		remainingSize -= sizeof(RiffChunkHeader);
		if(remainingSize < chunk.chunkSize)
		{
			ret->state = FILE_SIZE_MISMATCH;
			goto openfor_read_fail;
		}
		if(chunk.chunkID != RiffChunkHeader::idfmt_)
		{
			//not fmt_ chunk, well skip it
			if(fseek(ret->fp, chunk.chunkSize, SEEK_CUR))
			{
				ret->state = UNEXPECTED_EOF;
				goto openfor_read_fail;
			}
			remainingSize -= chunk.chunkSize;
			continue;
		}
		//we have found the fmt chunk
		//now we will read the following parameters as long as the
		//fmt_ chunk is reported to hold more, if any variables
		//remain, they will assume default values, if any more
		//information remains in the fmt_ chunk it will be ignored
		if(chunk.chunkSize < sizeof(short))
			goto fmt_end;
		if(fread(&ret->formatTag, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);
		if(chunk.chunkSize < sizeof(short))
			goto fmt_end;
		if(fread(&ret->numChannels, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);
		if(chunk.chunkSize < sizeof(unsigned long))
			goto fmt_end;
		if(fread(&ret->fs, sizeof(unsigned long), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		chunk.chunkSize -= sizeof(unsigned long);
		remainingSize -= sizeof(unsigned long);
		if(chunk.chunkSize < sizeof(unsigned long))
			goto fmt_end;
		if(fread(&ret->avgBytesPerSecond, sizeof(unsigned long), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		chunk.chunkSize -= sizeof(unsigned long);
		remainingSize -= sizeof(unsigned long);
		if(chunk.chunkSize < sizeof(unsigned long))
			goto fmt_end;
		if(fread(&ret->blockSize, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);
		if(chunk.chunkSize < sizeof(short))
			goto fmt_end;
		if(fread(&ret->numBits, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);

fmt_end:
		//now we will skip any other info in the fmt_ chunk
		if(fseek(ret->fp, chunk.chunkSize, SEEK_CUR))
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		remainingSize -= chunk.chunkSize;
		if(!ret->checkFormat())
		{
			ret->state = CONFLICTING_FORMAT_DATA;
			goto openfor_read_fail;
		}
		break;
	}

	//now we need to search for the data chunk, just as we did for the format chunk
	//now we expect the fmt_ header, but we are not guaranteed to get it right away
	while (true)
	{
		if(remainingSize == 0)
		{
			ret->state = CANNOT_FIND_DATA;
			goto openfor_read_fail;
		}
		if(fread(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_read_fail;
		}
		remainingSize -= sizeof(RiffChunkHeader);
		if(remainingSize < chunk.chunkSize)
		{
			//i need to implement a fix for this situation later
			//currently the reported file sizes are ignored and
			//the file size returned from the system is used
			//ret->state = FILE_SIZE_MISMATCH;
			//goto openfor_read_fail;
		}
		if(chunk.chunkID != RiffChunkHeader::iddata)
		{
			//not data chunk, well skip it
			if(remainingSize < chunk.chunkSize)
			{
				//there is appearantly a problem with the reported file sizes
				//and we will run into the end of the file before we see the
				//end of this chunk :(
				ret->state = CANNOT_FIND_DATA;
				goto openfor_read_fail;
			}
			if(fseek(ret->fp, chunk.chunkSize, SEEK_CUR))
			{
				ret->state = UNEXPECTED_EOF;
				goto openfor_read_fail;
			}
			remainingSize -= chunk.chunkSize;
			continue;
		}
		//we have found the data chunk
		if(remainingSize < chunk.chunkSize)
			ret->numSamples = remainingSize / ret->blockSize;
		else
			ret->numSamples = chunk.chunkSize / ret->blockSize;
		long fpos = ftell(ret->fp);
		ret->dataSizeOffset = fpos - 4;
		ret->dataOffset = fpos;
		break;
	}

	//maybe i should record the file position of ret->fp at this point

	ret->state = OPENFOR_READ;
	return ret;

openfor_read_fail:
	if (ret->fp != NULL)
		fclose(ret->fp);
	ret->fp = NULL;
	return ret;
}

bool Wave::checkFormat()
{
	//we can currently read only pcm and mulaw
	if(formatTag != WAVE_FORMAT_PCM && formatTag != WAVE_FORMAT_MULAW)
	{
		state = UNRECOGNIZED_ENCODING;
		return false;
	}

	//if undefined, assign a default value
	if(numChannels == 0)
		numChannels = 1;

	if(fs == 0)
		fs = 16000;
	
	if(numBits == 0)
	{
		if(formatTag == WAVE_FORMAT_MULAW)
			numBits = 8;
		else
			numBits = 16;
	}
	else
	{
		if(formatTag == WAVE_FORMAT_MULAW && numBits != 8)
		{
			state = CONFLICTING_FORMAT_DATA;
			return false;
		}
		else if(numBits != 8 && numBits != 16)
		{
			state = UNRECOGNIZED_ENCODING;
			return false;
		}
	}

	if(avgBytesPerSecond == 0)
		avgBytesPerSecond = numChannels * fs * numBits / 8;

	if(blockSize == 0)
		blockSize = numChannels * numBits / 8;

	if(avgBytesPerSecond != numChannels * fs * numBits / 8
	|| blockSize		 != numChannels * numBits / 8)
	{
		//correct these values
		avgBytesPerSecond = numChannels * fs * numBits / 8;
		blockSize = numChannels * numBits / 8;
	}

	return true;
}

int Wave::readData8(char **buf, int numSamples)
{
	if(state != OPENFOR_READ)
		return -1;

	int remaining = this->numSamples - ((ftell(fp) - dataOffset) / blockSize);
	numSamples = MIN(numSamples, remaining);
	if(numSamples <= 0)
		return 0;

	int ret, i;
	if(numBits == 8)
	{
		unsigned char *tempBuf = new unsigned char[numSamples*numChannels];

		ret = (int)fread(tempBuf, 1, numSamples*numChannels, fp);

		//now we will place the data into the input buffer
		int channel = 0;
		
		for(i=0; i<ret; ++i)
			buf[(channel++)%numChannels][i/numChannels] = tempBuf[i];

		delete [] tempBuf;

		return ret;
	}
	else if(numBits == 16)
	{
		short *tempBuf = new short[numSamples*numChannels];

		ret = (int)fread(tempBuf, 2, numSamples*numChannels, fp);

		//now we will place the data into the input buffer
		int channel = 0;
		for(i=0; i<ret; ++i)
			buf[(channel++)%numChannels][i/numChannels] = (char)((tempBuf[i]>>8)+128);

		delete [] tempBuf;
		
		return ret;
	}
	else
	{
		//how the hell did i get here, only 8bits and 16bits are allowed
		return -1;
	}
}

int Wave::readData16(short **buf, int numSamples)
{
	if(state != OPENFOR_READ)
		return -1;
	
	int remaining = this->numSamples - ((ftell(fp) - dataOffset) / blockSize);
	numSamples = MIN(numSamples, remaining);
	if(numSamples <= 0)
		return 0;

	int ret, i;
	if(numBits == 8)
	{
		unsigned char *tempBuf = new unsigned char[numSamples*numChannels];
		
		ret = (int)fread(tempBuf, 1, numSamples*numChannels, fp);
		
		//now we will place the data into the input buffer
		int channel = 0;
		if(formatTag == WAVE_FORMAT_MULAW)
		{
			for(i=0; i<ret; ++i)
				pcmu2lin(&(tempBuf[i]), &(buf[(channel++)%numChannels][i/numChannels]), 1, 1.0);
		}
		else
		{
			for(i=0; i<ret; ++i)
				buf[(channel++)%numChannels][i/numChannels] = ((short)tempBuf[i]-128)<<8;
		}

		delete [] tempBuf;
		
		return ret;
	}
	else if(numBits == 16)
	{
		short *tempBuf = new short[numChannels*numSamples];
		
		ret = (int)fread(tempBuf, 2, numSamples*numChannels, fp);

		//now we will place the data into the input buffer
		int channel = 0;
		for(i=0; i<ret; ++i)
			buf[(channel++)%numChannels][i/numChannels] = tempBuf[i];

		delete [] tempBuf;
		
		return ret;
	}
	else
	{
		//how the hell did i get here, only 8bits and 16bits are allowed
		return -1;
	}
}

int Wave::readMonoData8(char *buf, int numSamples)
{
	if(state != OPENFOR_READ)
		return -1;

	int ret;
	int i;

	if(numChannels == 1)
		ret = readData8(&buf, numSamples);
	else
	{
		char **tempBuf = new char*[numChannels];
		for(int i=0; i<numChannels; ++i)
			tempBuf[i] = new char[numSamples];
	
		ret = readData8(tempBuf, numSamples)/numChannels;

		for(i=0; i<ret; ++i)
		{
			buf[i] = 0;
			for(int j=0; j<numChannels; ++j)
				buf[i] += (char)((float)tempBuf[j][i] / numChannels);
		}
	
		for(i=0; i<numChannels; ++i)
			delete [] tempBuf[i];
		delete [] tempBuf;
	}

	return ret;
}

int Wave::readMonoData16(short *buf, int numSamples)
{
	if(state != OPENFOR_READ)
		return -1;

	int ret;
	int i;

	if(numChannels == 1)
		ret = readData16(&buf, numSamples);
	else
	{
		short **tempBuf = new short*[numChannels];
		for(int i=0; i<numChannels; ++i)
			tempBuf[i] = new short[numSamples];
	
		ret = readData16(tempBuf, numSamples)/numChannels;
	
		for(i=0; i<ret; ++i)
		{
			buf[i] = 0;
			for(int j=0; j<numChannels; ++j)
				buf[i] += (short)((float)tempBuf[j][i] / numChannels);
		}
	
		for(i=0; i<numChannels; ++i)
			delete [] tempBuf[i];
		delete [] tempBuf;
	}

	return ret;
}

void Wave::pcma2lin(unsigned char *p, short *buff_pcm, long len, unsigned char m, float s)
{
	float t, g, e, f;
	unsigned char q, k;
	
	t=1.0f/s;
	for (long i=0; i<len; i++)
	{
		if (m)
			q = p[i] ^ m;
		else
			q = p[i];

		k = q % 16;
		g = (float)floor(q/128.0);
		e = (q-k-128.0f*g)/16.0f;
		f = (float)((fabs(e-1.0f)-e+1.0f)/2.0f);
		buff_pcm[i] = (short)((2.0f*g-1.0f)*((k+16.5f)*pow(2.0f,e)+f*(k-15.5f))*t);
	}
}

void Wave::lin2pcma(short *x, unsigned char *out, long len, short m, float s)
{
	float y, q, a;
	int d, e;

	for (long i=0; i<len; i++)
	{
		y = (float)(x[i]*ldexp(s,-6));
		y = (float)((fabs(y+63.0f)-fabs(y-63.0f))/2.0f);
		q = (float)(floor((y+64.0f)/64.0f));
		a = (float)frexp(fabs(y),&e);
		d = (int)((e+fabs((float)e))/2.0f);
		out[i] = (unsigned char)(128*q+16*d+floor(ldexp(a,e-d+5)));
		if (m) 
			out[i]= out[i] ^ m;
	}
}

void Wave::pcmu2lin(unsigned char *p, short *buff_pcm, long len, float s)
{
	for (long i=0; i<len; i++)
        buff_pcm[i] = st_ulaw_to_linear(p[i]);
}

void Wave::lin2pcmu(short *x, unsigned char *out, long len, float s)
{
	for (long i=0; i<len; i++)
        out[i] = st_linear_to_ulaw(x[i]);
}

Wave *Wave::createForWrite(const char *fileName, Wave* wIn)
{
	Wave *ret = createForWrite(fileName, wIn->fs, wIn->numBits, wIn->numChannels, wIn->formatTag);

	return ret;
}

Wave *Wave::createForWrite(const char *fileName, Wave* wIn, int waveFormatTag)
{
	Wave *ret = createForWrite(fileName, wIn->fs, wIn->numBits, wIn->numChannels, waveFormatTag);

	return ret;
}

Wave *Wave::createForWrite(const char *fileName, unsigned long fs, short numBits, short numChannels, int waveFormatTag)
{
	Wave *ret = new Wave;
	RiffChunkHeader chunk;

	ret->formatTag = waveFormatTag;
	ret->numChannels = numChannels;
	ret->fs = fs;
	ret->numBits = numBits;
	ret->avgBytesPerSecond = numChannels * fs * numBits / 8;
	ret->blockSize = numChannels * numBits / 8;
	ret->fileSize = 0;

	ret->fp = fopen(fileName, "wb");
	if(ret->fp == NULL)
	{
		ret->state = FAILED_TO_OPEN_FILE;
		return ret;
	}

	//put in the riff header
	chunk.chunkID = RiffChunkHeader::idRIFF;
	chunk.chunkSize = 0;	//we have to come back here and remedy this
	if(fwrite(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
	{
		ret->state = FILE_WRITE_ERROR;
		goto openfor_write_fail;
	}
	ret->fileSize += sizeof(RiffChunkHeader);
	ret->fileSizeOffset = ret->fileSize - 4;

	//indicate that this is a wave file
	unsigned long wave;
	wave = RiffChunkHeader::idWAVE;
	if(fwrite(&wave, sizeof(unsigned long), 1, ret->fp) != 1)
	{
		ret->state = FILE_WRITE_ERROR;
		goto openfor_write_fail;
	}
	ret->fileSize += sizeof(unsigned long);

	//put in the fmt_ header
	chunk.chunkID = RiffChunkHeader::idfmt_;
	chunk.chunkSize = 16;	//size of a simple fmt_ chunk
	if(fwrite(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
	{
		ret->state = FILE_WRITE_ERROR;
		goto openfor_write_fail;
	}
	ret->fileSize += sizeof(RiffChunkHeader);

	//now put in the fmt_ fields
	if(fwrite(&ret->formatTag, sizeof(short), 1, ret->fp) != 1
	|| fwrite(&ret->numChannels, sizeof(short), 1, ret->fp) != 1
	|| fwrite(&ret->fs, sizeof(unsigned long), 1, ret->fp) != 1
	|| fwrite(&ret->avgBytesPerSecond, sizeof(unsigned long), 1, ret->fp) != 1
	|| fwrite(&ret->blockSize, sizeof(short), 1, ret->fp) != 1
	|| fwrite(&ret->numBits, sizeof(short), 1, ret->fp) != 1)
	{
		ret->state = FILE_WRITE_ERROR;
		goto openfor_write_fail;
	}
	ret->fileSize += 16;

	//put in the data header
	chunk.chunkID = RiffChunkHeader::iddata;
	chunk.chunkSize = 0;	//we have also gotto remedy this value when closing
	if(fwrite(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
	{
		ret->state = FILE_WRITE_ERROR;
		goto openfor_write_fail;
	}
	ret->fileSize += sizeof(RiffChunkHeader);
	ret->dataSizeOffset = ret->fileSize - 4;
	ret->dataOffset = ret->fileSize;
	//from here on every call to writeData will append sample data to the file

	ret->state = OPENFOR_WRITE;
	return ret;

openfor_write_fail:
	fclose(ret->fp);
	ret->fp = NULL;
	return ret;
}

Wave *Wave::openForAppend(const char *fileName)
{
	Wave *ret = new Wave;
	RiffChunkHeader chunk;
	int remainingSize = 0;
	long fileLength = 0;

	ret->fp = fopen(fileName, "r+b");
	if(ret->fp == NULL)
	{
		ret->state = FAILED_TO_OPEN_FILE;
		return ret;
	}

	//now we will read our first header, the actual RIFF header
	if(fread(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
	{
		ret->state = UNEXPECTED_EOF;
		goto openfor_append_fail;
	}

	//the id of this first chunk should be equal to RIFF, with the size giving the 
	//filesize - 8
	if(chunk.chunkID != RiffChunkHeader::idRIFF)
	{
		ret->state = NOT_RIFF_FILE;
		goto openfor_append_fail;
	}
	//ill check the file size reported by the riff header and compare it
	//with the actual file size reported by the system, in the event of a
	//mismatch ill go with the system reported size
	//actually this check is redundant, i can simply ignore the file size
	//reported in the riff header and go with the system size, but im leaving
	//this check here so i might later add something useful here
	fileLength = filelength(fileno(ret->fp));
	if(fileLength != chunk.chunkSize+8)
		ret->fileSize = fileLength;
	else
		ret->fileSize = chunk.chunkSize + 8;
	ret->fileSizeOffset = ftell(ret->fp) - 4;
	remainingSize = ret->fileSize - sizeof(RiffChunkHeader);

	//here we expect the "WAVE" indicator
	if(remainingSize == 0)
	{
		ret->state = NOT_WAVE_FILE;
		goto openfor_append_fail;
	}
	unsigned long wave;
	if(fread(&wave, sizeof(unsigned long), 1, ret->fp) != 1)
	{
		ret->state = UNEXPECTED_EOF;
		goto openfor_append_fail;
	}
	remainingSize -= sizeof(unsigned long);
	if(wave != RiffChunkHeader::idWAVE)
	{
		ret->state = NOT_WAVE_FILE;
		goto openfor_append_fail;
	}

	//now we expect the fmt_ header, but we are not guaranteed to get it right away
	while (true)
	{
		if(remainingSize == 0)
		{
			ret->state = CANNOT_FIND_FMT_;
			goto openfor_append_fail;
		}
		if(fread(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		remainingSize -= sizeof(RiffChunkHeader);
		if(remainingSize < chunk.chunkSize)
		{
			ret->state = FILE_SIZE_MISMATCH;
			goto openfor_append_fail;
		}
		if(chunk.chunkID != RiffChunkHeader::idfmt_)
		{
			//not fmt_ chunk, well skip it
			if(fseek(ret->fp, chunk.chunkSize, SEEK_CUR))
			{
				ret->state = UNEXPECTED_EOF;
				goto openfor_append_fail;
			}
			remainingSize -= chunk.chunkSize;
			continue;
		}
		//we have found the fmt chunk
		//now we will read the following parameters as long as the
		//fmt_ chunk is reported to hold more, if any variables
		//remain, they will assume default values, if any more
		//information remains in the fmt_ chunk it will be ignored
		if(chunk.chunkSize < sizeof(short))
			goto fmt_end;
		if(fread(&ret->formatTag, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);
		if(chunk.chunkSize < sizeof(short))
			goto fmt_end;
		if(fread(&ret->numChannels, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);
		if(chunk.chunkSize < sizeof(unsigned long))
			goto fmt_end;
		if(fread(&ret->fs, sizeof(unsigned long), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		chunk.chunkSize -= sizeof(unsigned long);
		remainingSize -= sizeof(unsigned long);
		if(chunk.chunkSize < sizeof(unsigned long))
			goto fmt_end;
		if(fread(&ret->avgBytesPerSecond, sizeof(unsigned long), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		chunk.chunkSize -= sizeof(unsigned long);
		remainingSize -= sizeof(unsigned long);
		if(chunk.chunkSize < sizeof(short))
			goto fmt_end;
		if(fread(&ret->blockSize, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);
		if(chunk.chunkSize < sizeof(short))
			goto fmt_end;
		if(fread(&ret->numBits, sizeof(short), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		chunk.chunkSize -= sizeof(short);
		remainingSize -= sizeof(short);

fmt_end:
		//now we will skip any other info in the fmt_ chunk
		if(fseek(ret->fp, chunk.chunkSize, SEEK_CUR))
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		remainingSize -= chunk.chunkSize;
		if(!ret->checkFormat())
		{
			ret->state = CONFLICTING_FORMAT_DATA;
			goto openfor_append_fail;
		}
		break;
	}

	//now we need to search for the data chunk, just as we did for the format chunk
	//now we expect the data header, but we are not guaranteed to get it right away
	while (true)
	{
		if(remainingSize == 0)
		{
			ret->state = CANNOT_FIND_DATA;
			goto openfor_append_fail;
		}
		if(fread(&chunk, sizeof(RiffChunkHeader), 1, ret->fp) != 1)
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		remainingSize -= sizeof(RiffChunkHeader);
		if(remainingSize < chunk.chunkSize)
		{
			//i need to implement a fix for this situation later
			//currently the reported file sizes are ignored and
			//the file size returned from the system is used
			//ret->state = FILE_SIZE_MISMATCH;
			//goto openfor_append_fail;
		}
		if(chunk.chunkID != RiffChunkHeader::iddata)
		{
			//not data chunk, well skip it
			if(remainingSize < chunk.chunkSize)
			{
				//there is appearantly a problem with the reported file sizes
				//and we will run into the end of the file before we see the
				//end of this chunk :(
				ret->state = CANNOT_FIND_DATA;
				goto openfor_append_fail;
			}
			if(fseek(ret->fp, chunk.chunkSize, SEEK_CUR))
			{
				ret->state = UNEXPECTED_EOF;
				goto openfor_append_fail;
			}
			remainingSize -= chunk.chunkSize;
			continue;
		}
		//we have found the data chunk
		if(remainingSize < chunk.chunkSize)
			ret->numSamples = remainingSize / ret->blockSize;
		else
			ret->numSamples = chunk.chunkSize / ret->blockSize;
		long fpos = ftell(ret->fp);
		ret->dataSizeOffset = fpos - 4;
		ret->dataOffset = fpos;
		//note that the file pointer should be positioned to the end of the
		//data chunk, so that subsequent write operations will write to the 
		//end of the data
		if(fseek(ret->fp, chunk.chunkSize, SEEK_CUR))
		{
			ret->state = UNEXPECTED_EOF;
			goto openfor_append_fail;
		}
		break;
	}

	ret->state = OPENFOR_APPEND;
	return ret;

openfor_append_fail:
	fclose(ret->fp);
	ret->fp = NULL;
	return ret;
}

int Wave::writeData8(char **buf, int numSamples)
{
	if(!(state == OPENFOR_WRITE || state == OPENFOR_APPEND))
		return -1;

	int ret, i;
	if(numBits == 8)
	{
		//ill first move the data to a single dimensional array
		//and then write it (so that i can write whole chunks of data
		char *tempBuf = new char[numChannels*numSamples];

		for(i=0; i<numChannels*numSamples; ++i)
			tempBuf[i] = buf[i%numChannels][i/numChannels];

		ret = (int)fwrite(tempBuf, 1, numSamples*numChannels, fp);

		delete [] tempBuf;
	}
	else if(numBits == 16)
	{
		//ill first move the data to a single dimensional array
		//and then write it (so that i can write whole chunks of data
		short *tempBuf = new short[numChannels*numSamples];
		
		for(i=0; i<numChannels*numSamples; ++i)
			tempBuf[i] = ((short)buf[i%numChannels][i/numChannels]-128) << 8;
		
		ret = (int)fwrite(tempBuf, 2, numSamples*numChannels, fp);
		
		delete [] tempBuf;
	}
	else
		return -1;

	this->numSamples += ret / numChannels;
	fileSize += ret * numBits / 8;

	return ret;
}

int Wave::writeData16(short **buf, int numSamples)
{
	if(!(state == OPENFOR_WRITE || state == OPENFOR_APPEND))
		return -1;
	
	int ret, i;
	if(numBits == 8)
	{
		//ill first move the data to a single dimensional array
		//and then write it (so that i can write whole chunks of data
		unsigned char *tempBuf = new unsigned char[numChannels*numSamples];
		
		for(i=0; i<numChannels*numSamples; ++i)
		{
			if(formatTag == WAVE_FORMAT_MULAW)
				lin2pcmu(&(buf[i%numChannels][i/numChannels]), &(tempBuf[i]), 1, 1.0);
			else
				tempBuf[i] = (char)((buf[i%numChannels][i/numChannels]>>8) + 128 );
		}
		
		ret = (int)fwrite(tempBuf, 1, numSamples*numChannels, fp);
		
		delete [] tempBuf;
	}
	else if(numBits == 16)
	{
		//ill first move the data to a single dimensional array
		//and then write it (so that i can write whole chunks of data
		short *tempBuf = new short[numChannels*numSamples];
		
		for(i=0; i<numChannels*numSamples; ++i)
			tempBuf[i] = buf[i%numChannels][i/numChannels];
		
		ret = (int)fwrite(tempBuf, 2, numSamples*numChannels, fp);
		
		delete [] tempBuf;
	}
	else
		return -1;

	this->numSamples += ret / numChannels;
	fileSize += ret * numBits / 8;

	return ret;
}

int Wave::writeMonoData8(char *buf, int numSamples)
{
	if(!(state == OPENFOR_WRITE || state == OPENFOR_APPEND))
		return -1;

	int ret, i, j;

	if(numChannels == 1)
		ret = writeData8(&buf, numSamples);
	else
	{
		char **tempBuf = new char*[numChannels];
		for(i=0; i<numChannels; ++i)
		{
			tempBuf[i] = new char[numSamples];
			for(j=0; j<numSamples; ++j)
				tempBuf[i][j] = buf[j];
		}

		ret = writeData8(tempBuf, numSamples)/numChannels;

		for(i=0; i<numChannels; ++i)
			delete [] tempBuf[i];
		delete [] tempBuf;
	}

	return ret;
}

int Wave::writeMonoData16(short *buf, int numSamples)
{
	if(!(state == OPENFOR_WRITE || state == OPENFOR_APPEND))
		return -1;
	
	int ret, i, j;
	
	if(numChannels == 1)
		ret = writeData16(&buf, numSamples);
	else
	{
		short **tempBuf = new short*[numChannels];
		for(i=0; i<numChannels; ++i)
		{
			tempBuf[i] = new short[numSamples];
			for(j=0; j<numSamples; ++j)
				tempBuf[i][j] = buf[j];
		}
	
		ret = writeData16(tempBuf, numSamples)/numChannels;
	
		for(i=0; i<numChannels; ++i)
			delete [] tempBuf[i];
		delete [] tempBuf;
	}
	
	return ret;
}

bool Wave::seek(int sample)
{
	if(state != OPENFOR_READ)
		return false;

	if(sample >= numSamples)
		return false;

	if(fseek(fp, dataOffset + sample*blockSize, SEEK_SET) != 0)
	{
		fclose(fp);
		state = FSEEK_ERROR;
		return false;
	}
	else
		return true;
}

bool Wave::writeSize()
{
	if(!(state == OPENFOR_WRITE || state == OPENFOR_APPEND))
		return false;

	unsigned long fileSize = this->fileSize;
	unsigned long dataSize = numSamples*numChannels*numBits/8;
	//record correct size in riff header and data header
	if(fseek(fp, fileSizeOffset, SEEK_SET) != 0
		|| fwrite(&fileSize, sizeof(unsigned long), 1, fp) != 1
		|| fseek(fp, dataSizeOffset, SEEK_SET) != 0
		|| fwrite(&dataSize, sizeof(unsigned long), 1, fp) != 1
		|| fseek(fp, 0, SEEK_END) != 0)
	{
		state = FILE_WRITE_ERROR;
		fclose(fp);
		return false;
	}
	else
		return true;
}

void Wave::close()
{
	switch(state) {
	case OPENFOR_READ:
		{
			fclose(fp);
			state = CLOSED;
		}
		break;
	case OPENFOR_WRITE:
	case OPENFOR_APPEND:
		{
			if(writeSize())
			{
				fclose(fp);
				state = CLOSED;
			}
		}
		break;
	default:	//in all other states the file is already closed
				//so there is nothing to do
		break;
	}
}

char *Wave::stateString(STATE state)
{
	switch(state) {
	case UNINITED:
		return "The Wave object has not been initialized yet.";
		break;
	case OPENFOR_READ:
		return "The Wave object is open for reading.";
		break;
	case OPENFOR_WRITE:
		return "The Wave object is open for writing.";
		break;
	case OPENFOR_APPEND:
		return "The Wave object is open for appending.";
		break;
	case FAILED_TO_OPEN_FILE:
		return "Failed to open file.";
		break;
	case NOT_RIFF_FILE:
		return "The file is not a RIFF file.";
		break;
	case NOT_WAVE_FILE:
		return "The file is not a WAVE file.";
		break;
	case CANNOT_FIND_FMT_:
		return "Could not find FMT_ chunk.";
		break;
	case UNRECOGNIZED_ENCODING:
		return "The file has an unrecognized encoding.";
		break;
	case CONFLICTING_FORMAT_DATA:
		return "FMT_ data is corrupt.";
		break;
	case CANNOT_FIND_DATA:
		return "Could not find DATA chunk.";
		break;
	case FILE_SIZE_MISMATCH:
		return "Reported file sizes do not match.";
		break;
	case UNEXPECTED_EOF:
		return "Unexpected end of file.";
		break;
	case FILE_WRITE_ERROR:
		return "Failed to write to file.";
		break;
	case CLOSED:
		return "The Wave object has been closed.";
		break;

	default:
		return "";
		break;
	}
}

