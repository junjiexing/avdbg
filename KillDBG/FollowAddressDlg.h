#pragma once


// CFollowAddressDlg 对话框

class CFollowAddressDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFollowAddressDlg)

public:
	CFollowAddressDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFollowAddressDlg();

// 对话框数据
	enum { IDD = IDD_DIALOG_FLOADDR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	DWORD m_dwAddr;
	virtual BOOL OnInitDialog();
};
