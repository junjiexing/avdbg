#pragma once
#include "afxwin.h"


// CFontPropPage 对话框

class CFontPropPage : public CDialogEx
{
	DECLARE_DYNAMIC(CFontPropPage)

public:
	CFontPropPage(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CFontPropPage();

// 对话框数据
	enum { IDD = IDD_FORMVIEW_FONT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	static int CALLBACK FontCallback( CONST LOGFONT* pFont, CONST TEXTMETRIC *, DWORD, LPARAM lParam);
	void FontCallback(const LOGFONT* pFont);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CListBox m_FontList;
};
