#pragma once


// CFileOpenDlg 对话框

class CFileOpenDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFileOpenDlg)

public:
	CFileOpenDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFileOpenDlg();

// 对话框数据
	enum { IDD = IDD_OPEN };

	CString	m_strPath;
	CString	m_strParam;
	CString m_strRunDir;

	std::string get_path()
	{
		return std::string(m_strPath.GetBuffer());
	}
	std::string get_param()
	{
		return std::string(m_strParam.GetBuffer());
	}
	std::string get_run_dir()
	{
		return std::string(m_strRunDir.GetBuffer());
	}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonbrowse();
};
