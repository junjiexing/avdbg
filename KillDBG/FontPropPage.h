#pragma once
#include "afxwin.h"


// CFontPropPage 对话框

class CFontPropPage : public CDialogEx
{
	DECLARE_DYNAMIC(CFontPropPage)

public:
	CFontPropPage(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFontPropPage();

// 对话框数据
	enum { IDD = IDD_FORMVIEW_FONT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonChoosememviewFont();

	BOOL m_bAsmViewFontChanged;
	LOGFONT m_AsmViewFont;
	BOOL m_bMemViewFontChanged;
	LOGFONT m_MemViewFont;
	BOOL m_bStkViewFontChanged;
	LOGFONT m_StkViewFont;

	afx_msg void OnBnClickedButtonChooseDasmFont();
	afx_msg void OnBnClickedButtonChooseStkviewFont();
};
