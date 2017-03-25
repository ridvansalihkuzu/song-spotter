// Songs.h: interface for the Songs class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SONGS_H__CD868F32_8A15_4ACD_8C04_0DC91B851C8B__INCLUDED_)
#define AFX_SONGS_H__CD868F32_8A15_4ACD_8C04_0DC91B851C8B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "EnergyContour.h"

class Songs  
{
public:
	void Destroy();
	void Allocate(int len);

	void Reset(int ind=-1) {if (ind>=numSongs) ind = 0; songInd = ind;};

	void PointTo(EnergyContour *newSong, int index, float corrVal);

	EnergyContour **songs;
	int *inds;
	float *corrVals;

	int numSongs;
	int songInd;

	Songs();
	virtual ~Songs();

};

#endif // !defined(AFX_SONGS_H__CD868F32_8A15_4ACD_8C04_0DC91B851C8B__INCLUDED_)
