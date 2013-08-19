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

	enum OutputType
	{
		OUT_INFO,
		OUT_WARNING,
		OUT_ERROR
	};

	void output_string( const std::string& str,OutputType type = OUT_INFO )
	{
		std::string line;

		switch (type)
		{
		case OUT_INFO:
			line = "INFO";
			break;
		case OUT_WARNING:
			line = "WARNING";
			break;
		default:
			line = "ERROR";
			break;
		}

		line += str;
		m_TextView.add_line(line,true);
	}
	void AddLine(const std::string& line)
	{
		m_TextView.add_line(line,true);
	}
};


