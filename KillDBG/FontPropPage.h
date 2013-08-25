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

	LOGFONT m_AsmViewFont;
	LOGFONT m_MemViewFont;

	afx_msg void OnBnClickedButtonChooseDasmFont();
};
