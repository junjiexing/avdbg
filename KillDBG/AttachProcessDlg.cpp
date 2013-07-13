// AttachProcessDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "AttachProcessDlg.h"
#include "afxdialogex.h"


// CAttachProcessDlg 对话框

IMPLEMENT_DYNAMIC(CAttachProcessDlg, CDialogEx)

CAttachProcessDlg::CAttachProcessDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAttachProcessDlg::IDD, pParent),m_dwPID(0)
{

}

CAttachProcessDlg::~CAttachProcessDlg()
{
}

void CAttachProcessDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTPROCESS, m_ListProcess);
	DDX_Control(pDX, IDOK, m_BtnOK);
	DDX_Control(pDX, IDCANCEL, m_BtnCancel);
}


BEGIN_MESSAGE_MAP(CAttachProcessDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAttachProcessDlg::OnBnClickedOk)
	ON_WM_SIZE()
	ON_NOTIFY(NM_DBLCLK, IDC_LISTPROCESS, &CAttachProcessDlg::OnNMDblclkListprocess)
END_MESSAGE_MAP()


// CAttachProcessDlg 消息处理程序


BOOL CAttachProcessDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	RECT	rc;
	GetClientRect(&rc);
	
	int cx = rc.right-rc.left;
	int cy = rc.bottom-rc.top;
	m_ListProcess.MoveWindow(5,5,cx-10,cy-50,TRUE);
	m_BtnOK.MoveWindow(cx-200,cy-35,80,25,TRUE);
	m_BtnCancel.MoveWindow(cx-100,cy-35,80,25,TRUE);

	m_ListProcess.SetExtendedStyle(LVS_EX_GRIDLINES |LVS_EX_FULLROWSELECT); 

	m_ListProcess.InsertColumn(0, _T("PID"), LVCFMT_LEFT, 50);
	m_ListProcess.InsertColumn(1, _T("文件名"),LVCFMT_LEFT,50);
	m_ListProcess.InsertColumn(2, _T("文件路径"), LVCFMT_LEFT, 500);
	HANDLE hToolhelp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hToolhelp == INVALID_HANDLE_VALUE)
	{
		this->MessageBox(_T("获取进程快照失败"));
		return	TRUE;
	}
	PROCESSENTRY32	stProcess = {0};
	stProcess.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hToolhelp, &stProcess);
	for (int i=0;Process32Next(hToolhelp, &stProcess);i++)
	{
		TCHAR	pszPid[10];
		_itot_s(stProcess.th32ProcessID, pszPid,10);
		//m_ListProcess.SetItemText(i,0,pszPid);
		m_ListProcess.InsertItem(i, pszPid, NULL);
		m_ListProcess.SetItemText(i,1,stProcess.szExeFile);
		HANDLE	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, stProcess.th32ProcessID);
		TCHAR	pszProcessPath[MAX_PATH+2];
		if (GetModuleFileNameEx(hProcess, NULL, pszProcessPath, MAX_PATH+2))
		{
			m_ListProcess.SetItemText(i, 2, pszProcessPath);
		}
		CloseHandle(hProcess);
	}
	CloseHandle(hToolhelp);


	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CAttachProcessDlg::OnBnClickedOk()
{
	UINT index = m_ListProcess.GetSelectionMark();
	if(index == -1)
	{
		this->MessageBox(_T("请选择一个进程"));
		return;
	}
	TCHAR	pszPid[10];
	m_ListProcess.GetItemText(index, 0, pszPid, 10);
	m_dwPID = _ttoi(pszPid);

	CDialogEx::OnOK();
}


void CAttachProcessDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	if (m_ListProcess.m_hWnd)
	{
		m_ListProcess.MoveWindow(5,5,cx-10,cy-50,TRUE);
		m_BtnOK.MoveWindow(cx-200,cy-35,80,25,TRUE);
		m_BtnCancel.MoveWindow(cx-100,cy-35,80,25,TRUE);
	}
}


void CAttachProcessDlg::OnNMDblclkListprocess(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	OnBnClickedOk();
	*pResult = 0;
}
