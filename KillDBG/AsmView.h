#pragma once
#include "x86Analysis.h"

extern std::shared_ptr<debug_kernel> debug_kernel_ptr;
// CAsmView

class CAsmView : public CWnd
{
	DECLARE_DYNAMIC(CAsmView)

public:
	CAsmView();
	virtual ~CAsmView();

	BOOL Create(LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID);

protected:
	DECLARE_MESSAGE_MAP()

public:
	void SetEIP(DWORD eip);
	void SetTopAddr(DWORD addr)
	{
		m_AddrToShow = addr;
		UpdateScrollInfo();
	}

	DWORD m_AddrToShow;
	DWORD m_Eip;
	x86dis m_Decoder;
	DWORD m_dwSelAddrStart;
	DWORD m_dwSelAddrEnd;

private:
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
	void PreviousCode(DWORD TargetAddr,DWORD* PreInsn);
	void UpdateScrollInfo();

	std::vector<DWORD> m_vecAddress;
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};


