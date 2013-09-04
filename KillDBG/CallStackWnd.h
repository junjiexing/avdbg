#pragma once


// CCallStackWnd

class CCallStackWnd : public CWnd
{
	DECLARE_DYNAMIC(CCallStackWnd)

public:
	CCallStackWnd();
	virtual ~CCallStackWnd();

protected:
	DECLARE_MESSAGE_MAP()

	CHeaderCtrl m_Header;
	int m_nAddrWidth;
	int m_nRetAddrWidth;
	int m_nFrameWidth;
	int m_nStackWidth;

	CFont m_Font;
	int m_nLineHight;
	int m_nFontWidth;

public:
	afx_msg void OnPaint();
	BOOL Create( const RECT& rect, CWnd* pParentWnd, UINT nID);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnHdnEndtrack( NMHDR *pNMHDR, LRESULT *pResult );

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

		Invalidate(FALSE);
		return TRUE;
	}

};


