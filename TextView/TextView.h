#pragma once

#include <string>
#include <vector>
#include <windows.h>

class TextView
{
public:
	TextView(void);
	~TextView(void);

	// 类名
	static const char* const wnd_class_name;
	// 注册窗口类
	static bool reg_class();
	// 窗口过程
	static LRESULT WINAPI TextViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// 创建编辑控件
	bool Create(HWND parent_wnd,int x,int y,int width,int height);

	HWND get_hwnd()
	{
		return edit_wnd_;
	}

	// 获取行高
	int get_line_height()
	{
		return line_height_;
	}
	// 设置行高
	void set_line_height(int line_height)
	{
		line_height_ = line_height;
	}
	// 设置字体
	HFONT set_font(HFONT font)
	{
		HFONT font_tmp = font_;
		font_ = font;
		return font_tmp;
	}
	// 获取边宽
	int get_margin_width()
	{
		return margin_widrh_;
	}
	// 设置边宽
	void set_margin_width(int margin_width)
	{
		margin_widrh_ = margin_width;
		refresh();
	}

private:
	// 消息分发函数
	LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam);

	void on_create();
	void on_size(UINT flags, int width, int height);
	void on_paint();
	void on_left_button_down(UINT flags,int x,int y);
	void on_v_scroll(UINT code, UINT pos);
	void on_h_scroll(UINT code, UINT pos);
	void set_scroll_status();
	void set_caret_status();

public:
	// 在最后添加一行
	void add_line(const std::string& line, bool invalid = false)
	{
		EnterCriticalSection(&critical_sections_);
		line_buffer_.push_back(line);
		LeaveCriticalSection(&critical_sections_);
		
		if (h_scroll_max_pos<line.size())
		{
			h_scroll_max_pos = line.size();
		}

		if (invalid)
		{
			refresh();
		}
	}

	// 在指定行之后添插入一行
	void insert_line(const std::string& line,int index,bool invalid = false)
	{
		EnterCriticalSection(&critical_sections_);
		line_buffer_.insert(line_buffer_.begin() + index,line);
		LeaveCriticalSection(&critical_sections_);

		if (h_scroll_max_pos<line.size())
		{
			h_scroll_max_pos = line.size();
		}

		if (invalid)
		{
			refresh();
		}
	}

	// 删除指定行
	void eraser_line(int index,bool invalid = false)
	{
		EnterCriticalSection(&critical_sections_);
		line_buffer_.erase(line_buffer_.begin() + index);

		std::string line = line_buffer_.at(index);
		if (h_scroll_max_pos == line.size())
		{
			h_scroll_max_pos = 0;
			for each (line in line_buffer_)
			{
				if (h_scroll_max_pos<line.size())
				{
					h_scroll_max_pos = line.size();
				}
			}
		}
		LeaveCriticalSection(&critical_sections_);

		if (invalid)
		{
			refresh();
		}
	}

	// 刷新窗口
	bool refresh(const RECT* rect_ptr = NULL,BOOL erase_bkg = FALSE)
	{
		set_scroll_status();
		return InvalidateRect(edit_wnd_,rect_ptr,erase_bkg);
	}

private:
// 	// 父窗口句柄
// 	HWND parent_wnd_;
	// 编辑控件的窗口句柄
	HWND edit_wnd_;
	// 保存每一行文字信息
	std::vector<std::string> line_buffer_;

	// 行高
	int line_height_;
	// 边宽度
	int margin_widrh_;
	// 边颜色
	COLORREF margin_color_;
	// 字体
	HFONT font_;
	// 字体宽度
	int font_width_;

	// 水平滚动条位置
	unsigned long h_scroll_pos_;
	// 垂直滚动条位置
	unsigned long v_scroll_pos_;
	// 水平滚动条最大值
	unsigned long h_scroll_max_pos;

	// 一屏有几行
	int page_lines_;
	// 一屏有几列
	int page_columns_;

	// 光标位置
	int caret_lines_;
	int caret_columns_;
	bool caret_is_hide_;

	CRITICAL_SECTION critical_sections_;
};

