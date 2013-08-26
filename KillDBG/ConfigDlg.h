#pragma once
#include "afxcmn.h"
#include "FontPropPage.h"
#include "AppConfig.h"


// CConfigDlg 对话框

class CConfigDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConfigDlg)

public:
	CConfigDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CConfigDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
	CTabCtrl m_Tab;
	afx_msg void OnBnClickedOk();
	afx_msg void OnTcnSelchangeTabConfig(NMHDR *pNMHDR, LRESULT *pResult);
	CFontPropPage m_FontPage;

public:
	virtual BOOL OnInitDialog();
};
