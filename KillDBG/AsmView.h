#pragma once
#include "x86Analysis.h"

extern std::shared_ptr<debug_kernel> debug_kernel_ptr;
// CAsmView

class CAsmWnd : public CWnd
{
	DECLARE_DYNAMIC(CAsmWnd)

public:
	CAsmWnd();
	virtual ~CAsmWnd();

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
	DWORD GetSelAddrStart()
	{
		return m_dwSelAddrStart;
	}

private:
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
	void PreviousCode(DWORD dwTargetAddr,DWORD* pdwPreInsn);
	void UpdateScrollInfo();

	std::vector<DWORD> m_vecAddress;
	std::vector<std::string> m_vecAsm;
	CFont m_Font;
	int m_nLineHight;
	int m_nFontWidth;
	int m_nMargenWidth;

	DWORD m_AddrToShow;
	DWORD m_Eip;
	x86dis m_Decoder;
	DWORD m_dwSelAddrStart;
	DWORD m_dwSelAddrEnd;
	BOOL m_bLButtonDown;
	std::string m_strSelWord;

	CHeaderCtrl m_Header;

	int m_nAddrWidth;
	int m_nHexWidth;
	int m_nDisasmWidth;
	int m_nCommicWidth;

	BOOL ExtTextOutWithSelection(CDC& dc, int x, int y, LPCRECT lpRect, LPCTSTR lpszString, UINT nCount);

public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	BOOL SetPaintFont(const LOGFONT& font);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnHdnEndtrack(NMHDR *pNMHDR, LRESULT *pResult);
};


