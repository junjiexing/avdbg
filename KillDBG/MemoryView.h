#pragma once


// CMemoryView

class CMemoryView : public CWnd
{
	DECLARE_DYNAMIC(CMemoryView)

public:
	CMemoryView();
	virtual ~CMemoryView();

protected:
	DECLARE_MESSAGE_MAP()

	CHeaderCtrl m_Header;

	int m_AddrWidth;
	int m_HexWidth;
	int m_AsciiWidth;
	CFont m_Font;

	DWORD m_dwSelStart;
	DWORD m_dwSelEnd;
	bool m_bLBtnDwn;

	DWORD m_dwStartAddr;

	std::vector<unsigned char> m_vecBuffer;
public:
	afx_msg void OnPaint();
	BOOL Create(LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID);
	afx_msg void OnHdnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnEndtrack(NMHDR *pNMHDR, LRESULT *pResult);
	BOOL SetPaintFont(const LOGFONT& font)
	{
		m_Font.Detach();
		return m_Font.CreateFontIndirect(&font);
	}
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	void SetAddrToView(DWORD address)
	{
		m_dwStartAddr = address;
		Invalidate();
	}
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};


