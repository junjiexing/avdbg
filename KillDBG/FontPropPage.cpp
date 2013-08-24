// FontPropPage.cpp : 实现文件
//

#include "stdafx.h"
#include "FontPropPage.h"
#include "afxdialogex.h"


// CFontPropPage 对话框

IMPLEMENT_DYNAMIC(CFontPropPage, CDialogEx)

CFontPropPage::CFontPropPage(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFontPropPage::IDD, pParent)
{

}

CFontPropPage::~CFontPropPage()
{
}

void CFontPropPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FONT, m_FontList);
}


BEGIN_MESSAGE_MAP(CFontPropPage, CDialogEx)
END_MESSAGE_MAP()


// CFontPropPage 消息处理程序


BOOL CFontPropPage::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CDC* pDC = GetDC();
	LOGFONT lf = {0};
	EnumFontFamiliesEx(pDC->GetSafeHdc(),&lf,&CFontPropPage::FontCallback,(LPARAM)this,0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}

int CALLBACK CFontPropPage::FontCallback( CONST LOGFONT* pFont, CONST TEXTMETRIC *, DWORD, LPARAM lParam)
{
	((CFontPropPage*)lParam)->FontCallback(pFont);
	return 1;
}

void CFontPropPage::FontCallback( const LOGFONT* pFont )
{
	if (!(pFont->lfPitchAndFamily & FIXED_PITCH) || pFont->lfFaceName[0] == '@')
	{
		return;
	}
	m_FontList.AddString(pFont->lfFaceName);
}
