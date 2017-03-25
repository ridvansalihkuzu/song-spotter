// StringList.cpp: implementation of the StringList class.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "StringList.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

StringList::StringList()
{
	names = NULL;
	len = 0;
}

StringList::~StringList()
{
	Destroy();
}

void StringList::Destroy()
{
	if (names != NULL)
	{
		delete [] names;
		names = NULL;
	}

	len = 0;
}

void StringList::Allocate(int n)
{
	Destroy();

	if (n>0)
	{
		len = n;
		names = new string[len];
	}
}