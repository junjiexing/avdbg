#pragma once
#include "x86Analysis.h"

// CAsmView

class CAsmView : public CWnd
{
	DECLARE_DYNAMIC(CAsmView)

public:
	CAsmView();
	virtual ~CAsmView();

	BOOL Create(LPCTSTR lpszWindowName, const RECT& rect, CWnd* pParentWnd, UINT nID);

protected:
	DECLARE_MESSAGE_MAP()

private:
	DWORD	dwFirstShownAddr;
public:
	afx_msg void OnPaint();

	std::shared_ptr<x86Analysis>	m_pAnalysiser;
	void SetAnalysiser(std::shared_ptr<x86Analysis> pAnalysiser)
	{
		m_pAnalysiser = pAnalysiser;
		Invalidate(TRUE);
	}
};


