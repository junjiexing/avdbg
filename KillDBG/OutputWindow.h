#pragma once

#include <TextView.h>

// COutputWindow

class COutputWindow : public CWnd
{
	DECLARE_DYNAMIC(COutputWindow)

public:
	COutputWindow();
	virtual ~COutputWindow();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

private:
	TextView m_TextView;
public:
	void AddLine(const std::string& line)
	{
		m_TextView.add_line(line,true);
	}
};


