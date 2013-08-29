#pragma once


// CStackView

class CStackView : public CWnd
{
	DECLARE_DYNAMIC(CStackView)

public:
	CStackView();
	virtual ~CStackView();

	BOOL Create(const RECT& rect, CWnd* pParentWnd, UINT nID);

protected:

	CFont m_Font;

	DWORD m_dwStartAddr;

	int m_nLineHight;
	int m_nFontWidth;

	DWORD m_dwSelStart;
	DWORD m_dwSelEnd;
	bool m_bLBtnDwn;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();


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
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
};


