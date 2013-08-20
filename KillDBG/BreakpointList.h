#pragma once
#include "DebugKernel.h"


// CBreakpointList

class CBreakpointList : public CListCtrl
{
	DECLARE_DYNAMIC(CBreakpointList)

public:
	CBreakpointList();
	virtual ~CBreakpointList();

	BOOL Create(const RECT& rect,CWnd* pParentWnd,UINT nID);
	void Refresh();

protected:
	DECLARE_MESSAGE_MAP()
};


