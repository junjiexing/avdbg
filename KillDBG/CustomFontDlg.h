#pragma once
#include "afxwin.h"


// CCustomFontDlg 对话框

class CCustomFontDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCustomFontDlg)

public:
	CCustomFontDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CCustomFontDlg();

// 对话框数据
	enum { IDD = IDD_FONT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	static int CALLBACK FontCallback( CONST LOGFONT* pFont, CONST TEXTMETRIC *, DWORD, LPARAM lParam);
	void FontCallback(const LOGFONT* pFont);
	std::vector<LOGFONT> m_vecFont;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListBox m_FontList;
	LOGFONT m_SelFont;

	afx_msg void OnBnClickedOk();
};
