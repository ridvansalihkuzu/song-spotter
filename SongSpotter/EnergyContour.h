// EnergyContour.h: interface for the EnergyContour class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENERGYCONTOUR_H__8C84CF9F_FE9B_41D9_A4E0_59F740191702__INCLUDED_)
#define AFX_ENERGYCONTOUR_H__8C84CF9F_FE9B_41D9_A4E0_59F740191702__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MIN_ENERGY_VALUE -5.0f

#define ENERGY_CONTOUR 1
#define SEGMENT 2

class EnergyContour  
{
public:
	int isEnergy; //Write energy to segment file?
	int isDelta; //Write delta energy to segment file?
	
	//Energy
	float *en;
	int enLen;
	//

	//Segment
	float *seg;
	int segLen;
	//

	//Delta segment
	float *deltaSeg;
	int deltaSegLen;
	//

	float startTime; //segment start time in seconds
	float endTime; //segment end time in seconds
	float songDuration; //total duration of song in seconds
	//

	float ws;
	float ss;

	void Destroy();
	void DestroyEn();
	void DestroySeg();
	void DestroyDeltaSeg();

	void AllocateEn(int newlen);
	void AllocateSeg(int newlen);
	void AllocateDeltaSeg(int newlen);

	void read_energy_file(const char *enFile);
	void write_energy_file(const char *enFile);
	void Extract(const char *wavFile, float winsize, float skipsize, int isSmooth);
	int SelectSegment(float inputDur, float ws, float ss, 
		               float segmentStartExclude, float segmentEndExclude, float uvSegDur, bool selectOnlyEnd, float inputLength);
	void write_segment_file(const char* segFile);
	void read_segment_file(const char* segFile);
	void Normalize(int which);
	void GetDeltaSegment();

	EnergyContour();
	virtual ~EnergyContour();

};

#endif // !defined(AFX_ENERGYCONTOUR_H__8C84CF9F_FE9B_41D9_A4E0_59F740191702__INCLUDED_)
