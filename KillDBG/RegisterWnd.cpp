// RegisterWnd.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "RegisterWnd.h"


// CRegisterWnd

IMPLEMENT_DYNAMIC(CRegisterWnd, CMFCPropertyGridCtrl)

CRegisterWnd::CRegisterWnd()
{

}

CRegisterWnd::~CRegisterWnd()
{
}


BEGIN_MESSAGE_MAP(CRegisterWnd, CMFCPropertyGridCtrl)
	ON_WM_CREATE()
	ON_MESSAGE(WM_USER_SETCONTEXT, &CRegisterWnd::OnUserSetcontext)
END_MESSAGE_MAP()



// CRegisterWnd 消息处理程序




int CRegisterWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMFCPropertyGridCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	HDITEM	item = {0};
	item.mask = HDI_WIDTH | HDI_TEXT;
	item.cxy = 120;
	item.pszText = "寄存器";
	this->GetHeaderCtrl().SetItem(0,&item);

// 	CMFCPropertyGridProperty* pDbgRegGroup = new CMFCPropertyGridProperty("调试寄存器");
// 	pDr0 = new CRegItem("Dr0",DEBUG_REG);
// 	pDr0->AllowEdit(FALSE);
// 	pDbgRegGroup->AddSubItem(pDr0);
// 	pDr1 = new CRegItem("Dr1",DEBUG_REG);
// 	pDr1->AllowEdit(FALSE);
// 	pDbgRegGroup->AddSubItem(pDr1);
// 	pDr2 = new CRegItem("Dr2",DEBUG_REG);
// 	pDr2->AllowEdit(FALSE);
// 	pDbgRegGroup->AddSubItem(pDr2);
// 	pDr3 = new CRegItem("Dr3",DEBUG_REG);
// 	pDr3->AllowEdit(FALSE);
// 	pDbgRegGroup->AddSubItem(pDr3);
// 	pDr6 = new CRegItem("Dr6",DEBUG_REG);
// 	pDr6->AllowEdit(FALSE);
// 	pDbgRegGroup->AddSubItem(pDr6);
// 	pDr7 = new CRegItem("Dr7",DEBUG_REG);
// 	pDr7->AllowEdit(FALSE);
// 	pDbgRegGroup->AddSubItem(pDr7);
// 	AddProperty(pDbgRegGroup);
// 
	CMFCPropertyGridProperty* pGeneralRegGroup = new CMFCPropertyGridProperty("通用存器");
	pEax = new CRegItem("EAX",GENERAL_REG);
	pEax->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEax);
	pEbx = new CRegItem("EBX",GENERAL_REG);
	pEbx->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEbx);
	pEcx = new CRegItem("ECX",GENERAL_REG);
	pEcx->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEcx);
	pEdx = new CRegItem("EDX",GENERAL_REG);
	pEdx->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEdx);
	pEdi = new CRegItem("EDI",GENERAL_REG);
	pEdi->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEdi);
	pEsi = new CRegItem("ESI",GENERAL_REG);
	pEsi->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEsi);
	pEbp = new CRegItem("EBP",GENERAL_REG);
	pEbp->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEbp);
	pEsp = new CRegItem("ESP",GENERAL_REG);
	pEsp->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEsp);
	pEip = new CRegItem("EIP",GENERAL_REG);
	pEip->AllowEdit(FALSE);
	pGeneralRegGroup->AddSubItem(pEip);
	AddProperty(pGeneralRegGroup);

	CMFCPropertyGridProperty* pFlagRegGroup = new CMFCPropertyGridProperty("标志存器");
	pEflag = new CRegItem("EFlags",EFLAGS_REG);
	pEflag->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pEflag);
	pCf = new CRegItem("CF",EFLAGS_BIT);
	pCf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pCf);
	pPf = new CRegItem("PF",EFLAGS_BIT);
	pPf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pPf);
	pAf = new CRegItem("AF",EFLAGS_BIT);
	pAf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pAf);
	pZf = new CRegItem("ZF",EFLAGS_BIT);
	pZf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pZf);
	pSf = new CRegItem("SF",EFLAGS_BIT);
	pSf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pSf);
	pTf = new CRegItem("TF",EFLAGS_BIT);
	pTf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pTf);
	pDf = new CRegItem("DF",EFLAGS_BIT);
	pDf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pDf);
	pOf = new CRegItem("OF",EFLAGS_BIT);
	pOf->AllowEdit(FALSE);
	pFlagRegGroup->AddSubItem(pOf);
	AddProperty(pFlagRegGroup);

	CMFCPropertyGridProperty* pSegRegGroup = new CMFCPropertyGridProperty("段寄存器");
	pGs = new CRegItem("GS",SEGMENT_REG);
	pGs->AllowEdit(FALSE);
	pSegRegGroup->AddSubItem(pGs);
	pFs = new CRegItem("FS",SEGMENT_REG);
	pFs->AllowEdit(FALSE);
	pSegRegGroup->AddSubItem(pFs);
	pEs = new CRegItem("ES",SEGMENT_REG);
	pEs->AllowEdit(FALSE);
	pSegRegGroup->AddSubItem(pEs);
	pDs = new CRegItem("DS",SEGMENT_REG);
	pDs->AllowEdit(FALSE);
	pSegRegGroup->AddSubItem(pDs);
	pSs = new CRegItem("SS",SEGMENT_REG);
	pSs->AllowEdit(FALSE);
	pSegRegGroup->AddSubItem(pSs);
	pCs = new CRegItem("CS",SEGMENT_REG);
	pCs->AllowEdit(FALSE);
	pSegRegGroup->AddSubItem(pCs);
	AddProperty(pSegRegGroup);

	return 0;
}

void CRegisterWnd::SetContextToShow( const CONTEXT* context )
{
// 	pDr0->SetValue(context->Dr0);
// 	pDr1->SetValue(context->Dr1);
// 	pDr2->SetValue(context->Dr2);
// 	pDr3->SetValue(context->Dr3);
// 	pDr6->SetValue(context->Dr6);
// 	pDr7->SetValue(context->Dr7);

	pGs->SetValue(context->SegGs);
	pFs->SetValue(context->SegFs);
	pEs->SetValue(context->SegEs);
	pDs->SetValue(context->SegDs);
	pSs->SetValue(context->SegSs);
	pCs->SetValue(context->SegCs);

	pEax->SetValue(context->Eax);
	pEbx->SetValue(context->Ebx);
	pEcx->SetValue(context->Ecx);
	pEdx->SetValue(context->Edx);
	pEdi->SetValue(context->Edi);
	pEsi->SetValue(context->Esi);
	pEbp->SetValue(context->Ebp);
	pEsp->SetValue(context->Esp);
	pEip->SetValue(context->Eip);

	pEflag->SetValue(context->EFlags);

	pCf->SetValue(context->EFlags & 1);
	pPf->SetValue(context->EFlags & (1<<1));
	pAf->SetValue(context->EFlags & (1<<2));
	pZf->SetValue(context->EFlags & (1<<3));
	pSf->SetValue(context->EFlags & (1<<4));
	pTf->SetValue(context->EFlags & (1<<5));
	pDf->SetValue(context->EFlags & (1<<7));
	pOf->SetValue(context->EFlags & (1<<8));
}

CRegisterWnd::CRegItem::CRegItem( char* szName, item_type type )
	:CMFCPropertyGridProperty(szName,(_variant_t)"0",""),
	m_dwValue(NULL),m_type(type)
{
}

CRegisterWnd::CRegItem::~CRegItem()
{

}

void CRegisterWnd::CRegItem::OnRClickValue( CPoint point, BOOL b )
{
	__super::OnRClickValue(point,b);
}


afx_msg LRESULT CRegisterWnd::OnUserSetcontext(WPARAM wParam, LPARAM lParam)
{
	// 操蛋...在调试线程里面不能直接调用SetContextToShow，所以弄了个消息转发
	SetContextToShow((const CONTEXT*)wParam);
	return 0;
}
