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
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	CEdit m_editSymPath;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonAddPath();

	CString m_strSymPaths;
};
