#pragma once
#include "afxwin.h"


// CSymPathDlg 对话框

class CSymPathDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSymPathDlg)

public:
	CSymPathDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CSymPathDlg();

// 对话框数据
	enum { IDD = IDD_SYMBOL_SEARCH_PATHS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	CButton m_Reload;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonAddPath();

	CString m_strSymPaths;
	BOOL m_bReload;
};
