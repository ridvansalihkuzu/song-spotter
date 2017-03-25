// StringList.h: interface for the StringList class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRINGLIST_H__2D932394_A995_4CC9_BD07_ECFA582CA408__INCLUDED_)
#define AFX_STRINGLIST_H__2D932394_A995_4CC9_BD07_ECFA582CA408__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>
using namespace std;

class StringList  
{
public:
	string *names;
	int len;

	void Allocate(int n);
	void Destroy();

	StringList();
	virtual ~StringList();

};

#endif // !defined(AFX_STRINGLIST_H__2D932394_A995_4CC9_BD07_ECFA582CA408__INCLUDED_)
