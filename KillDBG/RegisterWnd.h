#pragma once

#define WM_USER_SETCONTEXT WM_USER+2

// CRegisterWnd

class CRegisterWnd : public CMFCPropertyGridCtrl
{
	DECLARE_DYNAMIC(CRegisterWnd)

public:
	CRegisterWnd();
	virtual ~CRegisterWnd();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void SetContextToShow(const CONTEXT* context);

private:

	enum item_type
	{
		//DEBUG_REG,
		GENERAL_REG,
		SEGMENT_REG,
		EFLAGS_REG,
		EFLAGS_BIT,
	};

	class CRegItem : public CMFCPropertyGridProperty
	{
	public:
		CRegItem(char* szName, item_type type);
		~CRegItem();

		void SetValue(DWORD dwValue)
		{
			m_dwValue = dwValue;
			
			char buffer[10];
			switch (m_type)
			{
			case CRegisterWnd::GENERAL_REG:
			case CRegisterWnd::EFLAGS_REG:
				sprintf(buffer,"%08X",m_dwValue);
				break;
			case CRegisterWnd::SEGMENT_REG:
				sprintf(buffer,"%X",m_dwValue);
				break;
			case CRegisterWnd::EFLAGS_BIT:
				strcpy(buffer,m_dwValue>0?"1":"0");
				break;
			default:
				break;
			}

			__super::SetValue((_variant_t)buffer);
		}

		virtual void OnRClickValue( CPoint point, BOOL b);

	private:
		DWORD m_dwValue;
		item_type m_type;
	};

// 	CRegItem* pDr0;
// 	CRegItem* pDr1;
// 	CRegItem* pDr2;
// 	CRegItem* pDr3;
// 	CRegItem* pDr6;
// 	CRegItem* pDr7;

	CRegItem* pGs;
	CRegItem* pFs;
	CRegItem* pEs;
	CRegItem* pDs;
	CRegItem* pSs;
	CRegItem* pCs;

	CRegItem* pEax;
	CRegItem* pEbx;
	CRegItem* pEcx;
	CRegItem* pEdx;
	CRegItem* pEdi;
	CRegItem* pEsi;
	CRegItem* pEbp;
	CRegItem* pEsp;
	CRegItem* pEip;

	CRegItem* pEflag;
	CRegItem* pCf;
	CRegItem* pPf;
	CRegItem* pAf;
	CRegItem* pZf;
	CRegItem* pSf;
	CRegItem* pTf;
	CRegItem* pDf;
	CRegItem* pOf;
protected:
	afx_msg LRESULT OnUserSetcontext(WPARAM wParam, LPARAM lParam);
};


