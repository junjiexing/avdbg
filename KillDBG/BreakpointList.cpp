// BreakpointList.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "BreakpointList.h"
#include "DebugKernel.h"

extern std::shared_ptr<debug_kernel> debug_kernel_ptr;

// CBreakpointList

IMPLEMENT_DYNAMIC(CBreakpointList, CListCtrl)

CBreakpointList::CBreakpointList()
{

}

CBreakpointList::~CBreakpointList()
{
}


BEGIN_MESSAGE_MAP(CBreakpointList, CListCtrl)
END_MESSAGE_MAP()

BOOL CBreakpointList::Create( const RECT& rect,CWnd* pParentWnd,UINT nID )
{
	if (!__super::Create(LVS_REPORT|LVS_NOSORTHEADER,rect,pParentWnd,nID))
	{
		return FALSE;
	}

	SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	InsertColumn(0,"地址",0,70);
	InsertColumn(1,"激活",0,70);
	InsertColumn(2,"一次性断点",0,70);
	InsertColumn(3,"命中次数",0,70);

	return TRUE;
}

void CBreakpointList::Refresh()
{
	DeleteAllItems();

	int index = 0;
	for each (auto bp in debug_kernel_ptr->get_bp_list())
	{
		char buffer[20];
		sprintf(buffer,"%08X",bp.address);
		InsertItem(index,buffer);
		SetItemText(index,1,bp.user_enable?"是":"否");
		SetItemText(index,2,bp.is_once?"是":"否");
		sprintf(buffer,"%d",bp.hits);
		SetItemText(index,3,buffer);
	}
}



// CBreakpointList 消息处理程序


