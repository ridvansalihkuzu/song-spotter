// Songs.cpp: implementation of the Songs class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Songs.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Songs::Songs()
{
	songs = NULL;
	inds = NULL;
	corrVals = NULL;
	numSongs = 0;
	songInd = -1;
}

Songs::~Songs()
{
	Destroy();
}

void Songs::Allocate(int len)
{
	Destroy();

	if (len>0)
	{	
		numSongs = len;
		songs = new EnergyContour*[numSongs];
		inds = new int[numSongs];
		corrVals = new float[numSongs];
	}

	Reset();
}

void Songs::Destroy()
{
	if (songs != NULL)
	{
		delete [] songs;
		songs = NULL;
	}

	if (inds != NULL)
	{
		delete [] inds;
		inds = NULL;
	}

	if (corrVals != NULL)
	{
		delete [] corrVals;
		corrVals = NULL;
	}

	numSongs = 0;
	songInd = -1;
}

void Songs::PointTo(EnergyContour *newSong, int index, float corrVal)
{
	if (songInd+1<numSongs)
	{
		songInd++;
		songs[songInd] = newSong;
		inds[songInd] = index;	
		corrVals[songInd] = corrVal;
	}
}