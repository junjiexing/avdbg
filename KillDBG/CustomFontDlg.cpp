// CustomFontDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CustomFontDlg.h"
#include "afxdialogex.h"


// CCustomFontDlg 对话框

IMPLEMENT_DYNAMIC(CCustomFontDlg, CDialogEx)

CCustomFontDlg::CCustomFontDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCustomFontDlg::IDD, pParent)
{

}

CCustomFontDlg::~CCustomFontDlg()
{
}

void CCustomFontDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FONT, m_FontList);
}


BEGIN_MESSAGE_MAP(CCustomFontDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CCustomFontDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CCustomFontDlg 消息处理程序


BOOL CCustomFontDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CDC* pDC = GetDC();
	LOGFONT lf = {0};
	EnumFontFamiliesEx(pDC->GetSafeHdc(),&lf,&CCustomFontDlg::FontCallback,(LPARAM)this,0);

	m_FontList.SelectString(-1,m_SelFont.lfFaceName);
	HDC hdc      = ::GetDC(0);
	int nFontSize = MulDiv(-m_SelFont.lfHeight,72,GetDeviceCaps(hdc, LOGPIXELSY));
	::ReleaseDC(0, hdc);
	SetDlgItemInt(IDC_EDIT_SIZE,nFontSize);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


int CALLBACK CCustomFontDlg::FontCallback( CONST LOGFONT* pFont, CONST TEXTMETRIC *, DWORD, LPARAM lParam )
{
	((CCustomFontDlg*)lParam)->FontCallback(pFont);
	return 1;
}

void CCustomFontDlg::FontCallback( const LOGFONT* pFont )
{
	if (!(pFont->lfPitchAndFamily & FIXED_PITCH) || pFont->lfFaceName[0] == '@')
	{
		return;
	}

	m_vecFont.push_back(*pFont);
	m_FontList.AddString(pFont->lfFaceName);
}


void CCustomFontDlg::OnBnClickedOk()
{
	int index = m_FontList.GetCurSel();
	if (index<0 || index>= m_vecFont.size())
	{
		index = 0;
	}

	m_SelFont = m_vecFont[index];

	HDC hdc      = ::GetDC(0);
	m_SelFont.lfHeight = -MulDiv(GetDlgItemInt(IDC_EDIT_SIZE), GetDeviceCaps(hdc, LOGPIXELSY), 72);
	::ReleaseDC(0, hdc);

	m_SelFont.lfWidth = 0;

	CDialogEx::OnOK();
}
