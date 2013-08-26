#include "stdafx.h"
#include "AppConfig.h"

app_config_t app_cfg;

bool load_app_confgi()
{
	FILE* f = fopen("Config","rb");
	if (!f)
	{
		return false;
	}

	if (fread(&app_cfg,sizeof(app_cfg),1,f) != 1)
	{
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}

bool save_app_config()
{
	FILE* f = fopen("Config","wb");
	if (!f)
	{
		return false;
	}

	if (fwrite(&app_cfg,sizeof(app_cfg),1,f) != 1)
	{
		fclose(f);
		return false;
	}
	
	fclose(f);
	return true;
}

void load_default_config()
{
	LOGFONT default_font = {-13,0,0,0,400,'\0','\0','\0','\0','\x32','\x2','\x1','1',"ºÚÌå"};
	app_cfg.asm_view_font = app_cfg.mem_view_font = default_font;
}
