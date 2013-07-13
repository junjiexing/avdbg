#include "TextView.h"
#include <cassert>

#ifdef max
#undef max
#endif // max
#ifdef min
#undef min
#endif // min


#include <algorithm>

const char* const TextView::wnd_class_name = "TextViewWndClass";

TextView::TextView(void)
	:edit_wnd_(NULL),line_height_(0),
	margin_widrh_(0),margin_color_(0),
	font_(NULL),font_width_(0),h_scroll_pos_(0),
	v_scroll_pos_(0),h_scroll_max_pos(0),page_lines_(0),
	page_columns_(0),caret_lines_(1),caret_columns_(0),caret_is_hide_(false)
{
	InitializeCriticalSection(&critical_sections_);
}


TextView::~TextView(void)
{
	DeleteCriticalSection(&critical_sections_);
}

bool TextView::reg_class()
{
	WNDCLASSEX	wnd_class;

	//Window class for the main application parent window
	wnd_class.cbSize			= sizeof(wnd_class);
	wnd_class.style			= 0;
	wnd_class.lpfnWndProc	= TextView::TextViewWndProc;
	wnd_class.cbClsExtra	= 0;
	wnd_class.cbWndExtra	= sizeof(TextView *);
	wnd_class.hInstance		= GetModuleHandle(NULL);
	wnd_class.hIcon			= 0;
	wnd_class.hCursor		= LoadCursor (NULL, IDC_IBEAM);
	wnd_class.hbrBackground	= (HBRUSH)0;
	wnd_class.lpszMenuName	= 0;
	wnd_class.lpszClassName	= wnd_class_name;	
	wnd_class.hIconSm			= 0;

	return RegisterClassEx(&wnd_class) ? TRUE : FALSE;
}

LRESULT WINAPI TextView::TextViewWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	if (msg == WM_CREATE)
	{
		CREATESTRUCT* create_struct = (CREATESTRUCTA*)lParam;
		assert(create_struct);

		TextView* view = (TextView*)create_struct->lpCreateParams;
		assert(view);

		view->edit_wnd_ = hwnd;

		SetWindowLongPtr(hwnd, 0, (LONG)view);

		view->on_create();
		return TRUE;
	}

	TextView* view = (TextView*)GetWindowLongPtr(hwnd, 0);
	if (!view)
	{
		return DefWindowProc(hwnd,msg,wParam,lParam);
	}

	return view->WndProc(msg,wParam,lParam);
}

LRESULT TextView::WndProc( UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch (msg)
	{
	case WM_PAINT:
		on_paint();
		return TRUE;
	case WM_LBUTTONDOWN:
		on_left_button_down(wParam, LOWORD(lParam), HIWORD(lParam));
		return TRUE;
	case WM_VSCROLL:
		on_v_scroll(LOWORD(wParam), HIWORD(wParam));
		return 0;
	case WM_HSCROLL:
		on_h_scroll(LOWORD(wParam), HIWORD(wParam));
		return 0;
	case WM_SIZE:
		on_size(wParam, LOWORD(lParam), HIWORD(lParam));
		return TRUE;
	}
	return DefWindowProc(edit_wnd_,msg,wParam,lParam);
}

bool TextView::Create( HWND parent_wnd,int x,int y,int width,int height )
{
	return CreateWindowEx(NULL, 
		wnd_class_name, "", 
		WS_VSCROLL |WS_HSCROLL | WS_CHILD | WS_VISIBLE,
		0, 0, width, height, 
		parent_wnd, 
		0, 
		GetModuleHandle(NULL), 
		this) != NULL;
}

void TextView::on_paint()
{
	PAINTSTRUCT	paint_struct = {0};
	HDC dc = BeginPaint(edit_wnd_,&paint_struct);

 	RECT client_rect = {0};
 	GetClientRect(edit_wnd_,&client_rect);

	// 绘制margin
	if (margin_widrh_)
	{
		if (margin_color_ == NULL)
		{
			margin_color_ = GetSysColor(COLOR_BTNSHADOW);
		}

		RECT margin_rect;
		margin_rect.left = margin_rect.top = 0;
		margin_rect.right = margin_widrh_;
		margin_rect.bottom = client_rect.bottom;

		COLORREF color_tmp = SetBkColor(dc, margin_color_);
		ExtTextOut(dc, 0, 0, ETO_OPAQUE, &margin_rect, 0, 0, 0);
		SetBkColor(dc, color_tmp);
	}

	if (line_height_ == 0)
	{
		TEXTMETRIC	text_metrit = {0};
		GetTextMetrics(dc,&text_metrit);
		line_height_ = text_metrit.tmHeight + text_metrit.tmExternalLeading + 5 ;
	}

	for (int line_num = 0;;)
	{
		int x = margin_widrh_+5;
		int y = line_height_ * line_num;
		RECT line_rect;
		line_rect.left = x;
		line_rect.top = y;
		line_rect.right = client_rect.right;
		line_rect.bottom = line_rect.top + line_height_;

		if (y > client_rect.bottom)
		{
			break;
		}

		if (line_num + v_scroll_pos_ >= line_buffer_.size())
		{
			ExtTextOut(dc, x, y, ETO_OPAQUE, &line_rect, 0, 0, 0);
		}
		else
		{
			const std::string& line = line_buffer_[line_num + v_scroll_pos_];
			if (line.size() > h_scroll_pos_)
			{
				const char* pchar = line.c_str() + h_scroll_pos_;
				int size = line.size() - h_scroll_pos_;
				BOOL ret = ExtTextOut(dc,x,y,ETO_OPAQUE,&line_rect,pchar,size,NULL);
			}
			else
			{
				ExtTextOut(dc, x, y, ETO_OPAQUE, &line_rect, 0, 0, 0);
			}
		}


		++line_num;
	}

	EndPaint(edit_wnd_,&paint_struct);
}

void TextView::on_left_button_down( UINT flags,int x,int y )
{
	if (line_buffer_.size() == 0)
	{
		set_caret_status();
		return;
	}

	int index = v_scroll_pos_ + y / line_height_ + 1;
	if (index>=line_buffer_.size())
	{
		index = line_buffer_.size();
	}
	caret_lines_ = index;

	EnterCriticalSection(&critical_sections_);
	const std::string& line = line_buffer_.at(index-1);
	LeaveCriticalSection(&critical_sections_);

	int last_x = 0;

	if (line.size()<=h_scroll_pos_)
	{
		caret_columns_ = line.size()-1;
	}
	else
	{
		const char* line_ptr = line.c_str() + h_scroll_pos_;
		HDC hdc = GetDC(edit_wnd_);
		for (int i=0;i<=line.size()-h_scroll_pos_;++i)
		{
			SIZE sz;
			caret_columns_ = i + h_scroll_pos_;
			GetTextExtentPoint32(hdc, line_ptr, i, &sz);
			if (sz.cx>=x+(sz.cx-last_x)/2-margin_widrh_-5)
			{
				break;
			}
			last_x = sz.cx;
		}
		ReleaseDC(edit_wnd_,hdc);
	}

	set_caret_status();
}

void TextView::on_size( UINT flags, int width, int height )
{
	RECT client_rect;
	GetClientRect(edit_wnd_,&client_rect);

	page_lines_ = client_rect.bottom / line_height_;
	page_columns_ = (client_rect.right - margin_widrh_) / font_width_;

	refresh();
}

void TextView::on_v_scroll( UINT code, UINT pos )
{
	switch(code)
	{
	case SB_TOP:
		v_scroll_pos_ = 0;
		break;

	case SB_BOTTOM:
		v_scroll_pos_ = line_buffer_.size();
		break;

	case SB_LINEUP:
		v_scroll_pos_ = v_scroll_pos_ == 0?0:v_scroll_pos_-1;
		break;

	case SB_LINEDOWN:
		v_scroll_pos_ = v_scroll_pos_<line_buffer_.size()-1?v_scroll_pos_+1:v_scroll_pos_;
		break;

	case SB_PAGEDOWN:
		v_scroll_pos_ += page_lines_;
		break;

	case SB_PAGEUP:
		v_scroll_pos_ -= page_lines_;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		{
			SCROLLINFO scroll_info = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			GetScrollInfo(edit_wnd_, SB_VERT, &scroll_info);
			v_scroll_pos_ = scroll_info.nTrackPos;
		}
		break;
	}

	refresh();
	set_caret_status();
}

void TextView::on_h_scroll( UINT code, UINT pos )
{
	switch(code)
	{
	case SB_TOP:
		h_scroll_pos_ = 0;
		break;

	case SB_BOTTOM:
		h_scroll_pos_ = h_scroll_max_pos;
		break;

	case SB_LINEUP:
		h_scroll_pos_ = h_scroll_pos_ == 0?0:h_scroll_pos_-1;
		break;

	case SB_LINEDOWN:
		h_scroll_pos_ = h_scroll_pos_<h_scroll_max_pos-1?h_scroll_pos_+1:h_scroll_pos_;
		break;

	case SB_PAGEDOWN:
		h_scroll_pos_ += page_columns_;
		break;

	case SB_PAGEUP:
		h_scroll_pos_ -= page_columns_;
		break;

	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		{
			SCROLLINFO scroll_info = { sizeof(SCROLLINFO), SIF_TRACKPOS };
			GetScrollInfo(edit_wnd_, SB_HORZ, &scroll_info);
			h_scroll_pos_ = scroll_info.nTrackPos;
		}
		break;
	}

	refresh();
	set_caret_status();
}

void TextView::set_scroll_status()
{

	SCROLLINFO scroll_info = {sizeof(SCROLLINFO)};
	scroll_info.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;

	scroll_info.nMin = 0;
	scroll_info.nMax = line_buffer_.size();
	scroll_info.nPos = v_scroll_pos_;
	scroll_info.nPage = page_lines_;

	SetScrollInfo(edit_wnd_,SB_VERT,&scroll_info,TRUE);

	scroll_info.nMin = 0;
	scroll_info.nMax = h_scroll_max_pos;
	scroll_info.nPos = h_scroll_pos_;
	scroll_info.nPage = page_columns_;

	SetScrollInfo(edit_wnd_,SB_HORZ,&scroll_info,TRUE);

}

void TextView::on_create()
{
	HDC dc = GetDC(edit_wnd_);
	TEXTMETRIC	text_metrit = {0};
	GetTextMetrics(dc,&text_metrit);
	// 默认行高
	line_height_ = text_metrit.tmHeight + text_metrit.tmExternalLeading + 5 ;
	// 默认字体宽度
	font_width_ = text_metrit.tmAveCharWidth;
	ReleaseDC(edit_wnd_,dc);

	CreateCaret(edit_wnd_,NULL,1,line_height_-2);
	SetCaretPos(margin_widrh_,0);
	ShowCaret(edit_wnd_);
	refresh();
}

void TextView::set_caret_status()
{
	if (caret_columns_<h_scroll_pos_ || caret_lines_<v_scroll_pos_ && !caret_is_hide_)
	{
		SetCaretPos(-1,-1);
		return;
	}

	if (line_buffer_.size() == 0)
	{
		SetCaretPos(margin_widrh_+5,0);
		return;
	}

	int y = (caret_lines_ - v_scroll_pos_ -1)*line_height_;
	EnterCriticalSection(&critical_sections_);
	const std::string& line = line_buffer_.at(caret_lines_-1);
	LeaveCriticalSection(&critical_sections_);
	SIZE sz;
	HDC hdc = GetDC(edit_wnd_);
	GetTextExtentPoint32(hdc,line.c_str()+h_scroll_pos_,caret_columns_-h_scroll_pos_,&sz);
	ReleaseDC(edit_wnd_,hdc);
	SetCaretPos(sz.cx+margin_widrh_+5,y);

}

