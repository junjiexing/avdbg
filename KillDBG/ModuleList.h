#pragma once


// CModuleList

class CModuleList : public CListCtrl
{
	DECLARE_DYNAMIC(CModuleList)

public:
	CModuleList();
	virtual ~CModuleList();

public:
	BOOL Create(const RECT& rect,CWnd* pParentWnd,UINT nID);


protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRefresh();
};


