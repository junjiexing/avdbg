// AsmView.cpp : 实现文件
//

#include "stdafx.h"
#include "KillDBG.h"
#include "DebugKernel.h"
#include "AsmView.h"


// CAsmView

IMPLEMENT_DYNAMIC(CAsmView, CWnd)

CAsmView::CAsmView()
:dwFirstShownAddr(0)
{

}

CAsmView::~CAsmView()
{
}


BEGIN_MESSAGE_MAP(CAsmView, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CAsmView::Create( LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
// 	m_wndOutputWnd.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
// 		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL);
	return __super::Create(NULL,lpszWindowName,AFX_WS_DEFAULT_VIEW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
		rect,pParentWnd,nID);
}



// CAsmView 消息处理程序




void CAsmView::OnPaint()
{
	//__super::OnPaint();
	CPaintDC dc(this); // device context for painting
	RECT	rc;
	GetClientRect(&rc);
	dc.FillRect(&rc,&CBrush(0x611C87));

	if (/*dwFirstShownAddr == 0 ||*/ m_pAnalysiser == NULL)
	{
		return;
	}

	std::vector<std::string> asmcode;
	uint32 start_va = m_pAnalysiser->code_start_va_;
	char	szBuffer[100]; // FIXME：可能缓冲区溢出
	for each (auto block in m_pAnalysiser->blocks_vect_)
	{
		if (start_va != block.start)
		{
			for (uint32 i = start_va; i < block.start; ++i)
			{
				sprintf(szBuffer,"%08X  DB %02X",i,*(m_pAnalysiser->va_to_code_offset(i)));
// 				asmcode.push_back(std::string(szBuffer));
			}
		}

		m_pAnalysiser->disasm_block(block,asmcode);
		start_va = block.end;
		int y = 0;
		for each (std::string s in asmcode)
		{
			y+=20;
			dc.ExtTextOut(0,y,NULL,NULL,s.c_str(),s.size(),NULL);
		}
		break;
	}
	//pDbgKrnl->get_memory_info_by_addr()
	
}
