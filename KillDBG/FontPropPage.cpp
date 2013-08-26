// FontPropPage.cpp : 实现文件
//

#include "stdafx.h"
#include "FontPropPage.h"
#include "afxdialogex.h"
#include "CustomFontDlg.h"


// CFontPropPage 对话框

IMPLEMENT_DYNAMIC(CFontPropPage, CDialogEx)

CFontPropPage::CFontPropPage(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFontPropPage::IDD, pParent),
	m_bAsmViewFontChanged(FALSE),m_bMemViewFontChanged(FALSE)
{

}

CFontPropPage::~CFontPropPage()
{
}

void CFontPropPage::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFontPropPage, CDialogEx)
//	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_CHOOSEMEMVIEW_FONT, &CFontPropPage::OnBnClickedButtonChoosememviewFont)
	ON_BN_CLICKED(IDC_BUTTON_CHOOSE_DASM_FONT, &CFontPropPage::OnBnClickedButtonChooseDasmFont)
END_MESSAGE_MAP()





void CFontPropPage::OnBnClickedButtonChoosememviewFont()
{
	CCustomFontDlg dlg;

	dlg.m_SelFont = m_MemViewFont;
	if (dlg.DoModal()!=IDOK)
	{
		return;
	}

	m_MemViewFont = dlg.m_SelFont;
	m_bMemViewFontChanged = TRUE;
}


void CFontPropPage::OnBnClickedButtonChooseDasmFont()
{
	CCustomFontDlg dlg;

	dlg.m_SelFont = m_AsmViewFont;
	if (dlg.DoModal()!=IDOK)
	{
		return;
	}

	m_AsmViewFont = dlg.m_SelFont;
	m_bAsmViewFontChanged = TRUE;
}
