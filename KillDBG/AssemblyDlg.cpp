// AssemblyDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "AssemblyDlg.h"
#include "afxdialogex.h"
#include <x86dis.h>
#include <x86asm.h>


// CAssemblyDlg 对话框

IMPLEMENT_DYNAMIC(CAssemblyDlg, CDialogEx)

CAssemblyDlg::CAssemblyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAssemblyDlg::IDD, pParent)
{

}

CAssemblyDlg::~CAssemblyDlg()
{
}

void CAssemblyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_OPCODE, m_cmbOpcode);
}


BEGIN_MESSAGE_MAP(CAssemblyDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CAssemblyDlg::OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT_ASM, &CAssemblyDlg::OnEnChangeEditAsm)
END_MESSAGE_MAP()


// CAssemblyDlg 消息处理程序

bool IsHex( char c )
{
	return (c>='0' && c<='9')
		|| (c>='a' && c<='f')
		|| (c>='A' && c<='F');
}

byte ConvertHexChar(byte ch)
{
	if((ch>='0')&&(ch<='9'))
		return ch-0x30;
	else if((ch>='A')&&(ch<='F'))
		return ch-'A'+10;
	else if((ch>='a')&&(ch<='f'))
		return ch-'a'+10;        
	else
		return -1;
}

void CAssemblyDlg::OnBnClickedOk()
{
	char buffer[105] = {0};
	int size = GetDlgItemText(IDC_COMBO_OPCODE,buffer,100);
	if (size == 0)
	{
		MessageBox("无效的字节码","错误",MB_OK|MB_ICONERROR);
		return;
	}
	
	strcat(buffer,"  ");
	int j = 0;
	for (int i=0; i<size;)
	{
		if (buffer[i] == ' ')
		{
			++i;
			continue;
		}

		if (IsHex(buffer[i]) && IsHex(buffer[i+1]) && buffer[i+2] == ' ')
		{
			m_NewOpcode[j] = (ConvertHexChar(buffer[i])<<4) + ConvertHexChar(buffer[i+1]);
			++j;
			i+=2;
			continue;
		}
		MessageBox("无效的字节码","错误",MB_OK|MB_ICONERROR);
		return;
	}

	m_NewOpcSize = j;

	CDialogEx::OnOK();
}


BOOL CAssemblyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	x86dis decoder(X86_OPSIZE32,X86_ADDRSIZE32);
	CPU_ADDR addr = {0};
	addr.addr32.offset = m_dwAddress;
	x86dis_insn* insn = (x86dis_insn*)decoder.decode(m_OrgOpcode,15,addr);
	
	SetDlgItemText(IDC_EDIT_ASM,decoder.str(insn,DIS_STYLE_HEX_ASMSTYLE | DIS_STYLE_HEX_UPPERCASE | DIS_STYLE_HEX_NOZEROPAD));

	GetDlgItem(IDC_EDIT_ASM)->SetFocus();

	return FALSE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CAssemblyDlg::OnEnChangeEditAsm()
{
	m_cmbOpcode.ResetContent();

	x86asm encoder(X86_OPSIZE32,X86_ADDRSIZE32);

	x86asm_insn insn = {0};
	char buffer[50];
	if (GetDlgItemText(IDC_EDIT_ASM,buffer,50) == 0)
	{
		return;
	}

	if (!encoder.translate_str(&insn,buffer))
	{
		m_cmbOpcode.SetWindowText(encoder.get_error_msg());
		return;
	}

	CPU_ADDR addr = {0};
	addr.addr32.offset = m_dwAddress;
	asm_code* code = encoder.encode(&insn,X86ASM_NULL,addr);

	if (!code)
	{
		return;
	}

	int i = 0;
	while (code)
	{
	 	char s[50], *tmp = s;
	 	for (int j=0; j < code->size; j++)
	 	{
	 		tmp += sprintf(tmp, "%02X ", code->data[j]);
	 	}
	 	code = code->next;
	 	++i;
	 	m_cmbOpcode.AddString(s);
	}

	m_cmbOpcode.SetCurSel(0);
}

