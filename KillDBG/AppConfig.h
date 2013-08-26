#pragma once

struct app_config_t 
{
	LOGFONT asm_view_font;
	LOGFONT mem_view_font;
};

extern app_config_t app_cfg;

bool load_app_confgi();
bool save_app_config();
void load_default_config();