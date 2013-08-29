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

	int m_nLineHight;
	int m_nFontWidth;

	CMenu m_Menu;
public:
	afx_msg void OnPaint();
	afx_msg void OnHdnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnEndtrack(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnFollowAddr();

	BOOL Create(LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID);

	void SetAddrToView(DWORD address)
	{
		m_dwStartAddr = address;
		Invalidate();
	}

	BOOL SetPaintFont(const LOGFONT& font)
	{
		m_Font.Detach();

		if (! m_Font.CreateFontIndirect(&font))
		{
			return FALSE;
		}

		CDC* pDC = GetDC();
		CFont* pFont = pDC->SelectObject(&m_Font);
		pFont->DeleteObject();
		TEXTMETRIC	text_metrit = {0};
		if (!pDC->GetTextMetrics(&text_metrit))
		{
			return FALSE;
		}
		;
		// 默认行高
		m_nLineHight = text_metrit.tmHeight + text_metrit.tmExternalLeading + 5 ;
		// 默认字体宽度
		m_nFontWidth = text_metrit.tmAveCharWidth;
		ReleaseDC(pDC);

		return TRUE;
	}

	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};


