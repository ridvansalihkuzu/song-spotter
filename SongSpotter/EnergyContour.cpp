// EnergyContour.cpp: implementation of the EnergyContour class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "EnergyContour.h"
#include <stdio.h>
#include <math.h>
#include "FuncDef.h"
#include <io.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

EnergyContour::EnergyContour()
{
	en = NULL;
	enLen = 0;
	seg = NULL;
	segLen = 0;
	deltaSeg = NULL;
	deltaSegLen = 0;

	startTime = -1.0f;
	endTime = -1.0f;
	ws = 0.0f;
	ss = 0.0f;
	isEnergy = 1;
	isDelta = 0;
}

EnergyContour::~EnergyContour()
{
	Destroy();
}

void EnergyContour::Destroy()
{
	DestroyEn();
	DestroySeg();
	DestroyDeltaSeg();
}

void EnergyContour::DestroyEn()
{
	if (en!=NULL)
	{
		delete [] en;
		en = NULL;
	}

	enLen = 0;
}

void EnergyContour::DestroySeg()
{
	if (seg!=NULL)
	{
		delete [] seg;
		seg = NULL;
	}

	segLen = 0;
}

void EnergyContour::DestroyDeltaSeg()
{
	if (deltaSeg!=NULL)
	{
		delete [] deltaSeg;
		deltaSeg = NULL;
	}

	deltaSegLen = 0;
}

void EnergyContour::AllocateEn(int newlen)
{
	DestroyEn();

	if (newlen>0)
	{
		enLen = newlen;
		en = new float[enLen];
	}
}

void EnergyContour::AllocateSeg(int newlen)
{
	DestroySeg();

	if (newlen>0)
	{
		segLen = newlen;
		seg = new float[segLen];
		for (int i=0; i<segLen; i++)
			seg[i] = -100.0f;
	}
}

void EnergyContour::AllocateDeltaSeg(int newlen)
{
	DestroyDeltaSeg();

	if (newlen>0)
	{
		deltaSegLen = newlen;
		deltaSeg = new float[deltaSegLen];
		for (int i=0; i<deltaSegLen; i++)
			deltaSeg[i] = 0.0f;
	}
}

void EnergyContour::read_energy_file(const char *enFile)
{
	DestroyEn();
	
	if (access(enFile, 0)!=-1)
	{
		FILE *fp = fopen(enFile, "rb");
		
		if (fp != NULL)
		{
			fread(&ws, sizeof(float), 1, fp);
			fread(&ss, sizeof(float), 1, fp);
			
			fread(&enLen, sizeof(int), 1, fp);
			if (enLen>0)
			{
				AllocateEn(enLen);
				fread(en, sizeof(float), enLen, fp);
			}
			else
				enLen = 0;
		}
		
		fclose(fp);
	}
	else
		printf("Energy file not found!\n");
}

void EnergyContour::write_energy_file(const char *enFile)
{
	FILE *fp = fopen(enFile, "wb");

	if (fp != NULL)
	{
		fwrite(&ws, sizeof(float), 1, fp);
		fwrite(&ss, sizeof(float), 1, fp);
		
		fwrite(&enLen, sizeof(int), 1, fp);

		if (enLen>0)
			fwrite(en, sizeof(float), enLen, fp);
	}

	fclose(fp);
}

void EnergyContour::Extract(const char *wavFile, float winsize, float skipsize, int isSmooth)
{
	DestroyEn();
	ws = winsize;
	ss = skipsize;
	energy(wavFile, ws, ss, &en, enLen, isSmooth);
}

void EnergyContour::read_segment_file(const char *segFile)
{
	DestroySeg();
	DestroyDeltaSeg();

	FILE *fp = fopen(segFile, "rb");
	
	if (fp != NULL)
	{
		fread(&startTime, sizeof(float), 1, fp);
		fread(&endTime, sizeof(float), 1, fp);
		fread(&songDuration, sizeof(float), 1, fp);
		fread(&ws, sizeof(float), 1, fp);
		fread(&ss, sizeof(float), 1, fp);
		fread(&isEnergy, sizeof(int), 1, fp);
		fread(&isDelta, sizeof(int), 1, fp);
		
		if (isEnergy)
		{
			fread(&segLen, sizeof(int), 1, fp);
			if (segLen>0)
			{
				AllocateSeg(segLen);
				fread(seg, sizeof(float), segLen, fp);
			}
		}

		if (isDelta)
		{
			fread(&deltaSegLen, sizeof(int), 1, fp);
			if (deltaSegLen>0)
			{
				AllocateDeltaSeg(deltaSegLen);
				fread(deltaSeg, sizeof(float), deltaSegLen, fp);
			}
		}
	}

	fclose(fp);
}

void EnergyContour::write_segment_file(const char *segFile)
{
	FILE *fp = fopen(segFile, "wb");

	if (fp != NULL)
	{
		fwrite(&startTime, sizeof(float), 1, fp);
		fwrite(&endTime, sizeof(float), 1, fp);
		fwrite(&songDuration, sizeof(float), 1, fp);
		fwrite(&ws, sizeof(float), 1, fp);
		fwrite(&ss, sizeof(float), 1, fp);
		fwrite(&isEnergy, sizeof(int), 1, fp);
		fwrite(&isDelta, sizeof(int), 1, fp);

		if (isEnergy && segLen>0)
		{
			fwrite(&segLen, sizeof(int), 1, fp);
			fwrite(seg, sizeof(float), segLen, fp);
		}

		if (isDelta && deltaSegLen>0)
		{
			fwrite(&deltaSegLen, sizeof(int), 1, fp);
			fwrite(deltaSeg, sizeof(float), deltaSegLen, fp);
		}
	}

	fclose(fp);
}		

int EnergyContour::SelectSegment(float inputDur, float ws, float ss, 
								  float segmentStartExclude, float segmentEndExclude, float uvSegDur, bool selectOnlyEnd, float inputLength)
{


	int i;
	int segmentLen = (int)floor((inputDur-ws/2)/ss+1.5);
	int stInd = 1;        // (int)floor((segmentStartExclude-ws/2)/ss+1.5);
	int enInd = enLen-1;  //-(int)(floor((segmentEndExclude-ws/2)/ss+1.5));

	if (segmentLen>=enInd-stInd+1)
	{
//		stInd = 0;
//		enInd = enLen-1;
//		segmentLen=enInd-stInd+1;
//		AllocateSeg(segmentLen);
//		
//		for (i=0; i<segmentLen; i++)
//			seg[i] = en[i+stInd-1];
//
//		startTime = (stInd-1)*ss+ws/2;
//		endTime = (enInd-1)*ss+ws/2;
		return -1;
	}

	AllocateSeg(segmentLen);
	int numfrm = enInd-segmentLen+1-stInd+1;
	int *frmInds = new int[numfrm+1];
	float *segmentStd = new float[numfrm+1];
	int *inds = new int[numfrm+1];

	if(selectOnlyEnd)
	{		
		float tenPercentTime = inputLength * 0.1;
		endTime = inputLength - tenPercentTime;
		startTime = tenPercentTime;
		songDuration = inputLength;
	}
	else
	{
		for (i=stInd; i<=enInd-segmentLen+1; i++)
			frmInds[i-stInd+1] = i;
		
		for (i=1; i<=numfrm; i++)
		{
			segmentStd[i] = stdev(&(en[frmInds[i]-1]), segmentLen);
			inds[i] = i;
		}

		//Search for the max variable segment that is not silent
		sort(segmentStd, inds, numfrm);

		bool bFound=false;

		int currInd = numfrm;

		float vadThreshold = (float)(20.0f*log10(500.0f));
		int uvSegLenTh = (int)(floor(uvSegDur/ss+0.5));
		if (uvSegLenTh<1)
			uvSegLenTh = 1;

		float m1, m2, mm;
		int halfLen = (int)(floor(segLen*0.5+0.5f));
		if (halfLen<1)
			halfLen = 1;

		while (!bFound)
		{
			for (i=0; i<segLen; i++)
				seg[i] = en[i+frmInds[inds[currInd]]-1];

			m1 = mean(seg, halfLen);
			m2 = mean(&(seg[halfLen]), segLen-halfLen);
			mm = MAX(m1, m2);

			//if (mean(seg, segLen)>20*log10(500))
			if (fabs(m1-m2)<0.001*mm && totalLess(seg, segLen, vadThreshold)<uvSegLenTh)
				bFound=true;
			else
			{	
				currInd--;
				
				if (currInd<1) //No non-silent frame found so select the one with middle variability at least
				{	
					currInd = (int)floor(numfrm/2.0f+0.5);
					if (currInd<0)
						currInd=0;
					if (currInd>numfrm-1)
						currInd=numfrm-1;
					
					for (i=0; i<segLen; i++)
						seg[i] = en[i+frmInds[inds[currInd]]-1];

					bFound = true;
				}
			}
		}

		startTime = (frmInds[inds[currInd]]-1)*ss+ws/2;
		endTime = (frmInds[inds[currInd]]-1+segmentLen)*ss+ws/2;
		songDuration = enLen*ss+ws/2;	
	}

	
	Normalize(SEGMENT);

	if (isDelta)
		GetDeltaSegment();
	
	delete [] frmInds;
	delete [] segmentStd;
	delete [] inds;

	return 0;
}

void EnergyContour::Normalize(int which)
{
	if (which==SEGMENT && seg!=NULL && segLen>0)
		NormalizeEnergy(seg, segLen, 40000.0f);
	else if (which==ENERGY_CONTOUR && en!=NULL && enLen>0)
		NormalizeEnergy(en, enLen, 40000.0f);
}

void EnergyContour::GetDeltaSegment()
{
	if (seg!=NULL && segLen>0)
	{
		AllocateDeltaSeg(segLen);

		for (int i=0; i<segLen; i++)
			deltaSeg[i] = seg[i];

		ExtractDelta(deltaSeg, deltaSegLen);
	}
}