// ConfigDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ConfigDlg.h"
#include "afxdialogex.h"
#include "MainFrm.h"

extern CMainFrame* main_frame;

// CConfigDlg 对话框

IMPLEMENT_DYNAMIC(CConfigDlg, CDialogEx)

CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CConfigDlg::IDD, pParent)
{

}

CConfigDlg::~CConfigDlg()
{
}

void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_CONFIG, m_Tab);
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CConfigDlg::OnBnClickedOk)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_CONFIG, &CConfigDlg::OnTcnSelchangeTabConfig)
END_MESSAGE_MAP()


// CConfigDlg 消息处理程序


void CConfigDlg::OnBnClickedOk()
{
	main_frame->m_wndMemView.SetPaintFont(m_FontPage.m_MemViewFont);
	main_frame->m_wndAsmView.SetPaintFont(m_FontPage.m_AsmViewFont);
	CDialogEx::OnOK();
}


void CConfigDlg::OnTcnSelchangeTabConfig(NMHDR *pNMHDR, LRESULT *pResult)
{
	int nSel = m_Tab.GetCurSel();
	switch (nSel)
	{
	case 0:
		m_FontPage.ShowWindow(SW_SHOW);
		break;
	case 1:
		m_FontPage.ShowWindow(SW_HIDE);
		break;
	default:
		break;
	}
	*pResult = 0;
}


BOOL CConfigDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_Tab.InsertItem(0,"字体");
	m_Tab.InsertItem(1,"颜色");

	m_FontPage.Create(CFontPropPage::IDD,&m_Tab);

	RECT rc;
	m_Tab.GetClientRect(&rc);
	rc.top += 22;
	m_FontPage.MoveWindow(&rc);
	m_FontPage.ShowWindow(SW_SHOW);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

