#pragma once
#include "scintillawnd.h"
class COutputWnd :
	public CScintillaWnd
{
	DECLARE_DYNAMIC(COutputWnd)
public:
	COutputWnd(void);
	virtual ~COutputWnd(void);

	BOOL Create( const RECT& rect, CWnd* pParentWnd, UINT nID);

protected:
	DECLARE_MESSAGE_MAP()

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
			line = "INFO:";
			break;
		case OUT_WARNING:
			line = "WARNING:";
			break;
		default:
			line = "ERROR:";
			break;
		}

		line += str;
		line += "\n";
		AddSring(line);
	}

	void AddSring(const std::string& line)
	{
		SendMessage(SCI_SETREADONLY,FALSE);
		SendMessage(SCI_APPENDTEXT,line.size(),reinterpret_cast<LPARAM>(line.c_str()));
		SendMessage(SCI_SETREADONLY,TRUE);
	}
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

