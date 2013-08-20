// ModuleList.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "ModuleList.h"


// CModuleList

IMPLEMENT_DYNAMIC(CModuleList, CListCtrl)

CModuleList::CModuleList()
{

}

CModuleList::~CModuleList()
{
}


BEGIN_MESSAGE_MAP(CModuleList, CListCtrl)
	ON_WM_RBUTTONDOWN()
	ON_COMMAND(IDR_REFRESH, &CModuleList::OnRefresh)
END_MESSAGE_MAP()

BOOL CModuleList::Create( const RECT& rect,CWnd* pParentWnd,UINT nID )
{
	if (!__super::Create(LVS_REPORT|LVS_NOSORTHEADER,rect,pParentWnd,nID))
	{
		return FALSE;
	}
	
	SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	InsertColumn(0,"基地址",0,70);
	InsertColumn(1,"大小",0,70);
	InsertColumn(2,"入口",0,70);
	InsertColumn(3,"名称",0,70);
	InsertColumn(4,"文件版本",0,100);
	InsertColumn(5,"路径",0,200);
	return TRUE;
}





void CModuleList::OnRButtonDown(UINT nFlags, CPoint point)
{
	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_ENABLED,IDR_REFRESH,"刷新");
	POINT pt = {point.x,point.y};
	ClientToScreen(&pt);
	menu.TrackPopupMenu(NULL,pt.x,pt.y,this);

	//CListCtrl::OnRButtonDown(nFlags, point);
}

void CModuleList::OnRefresh()
{
	MessageBox(NULL);
}
