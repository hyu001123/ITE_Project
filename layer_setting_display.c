#include <assert.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "doorbell.h"
#include "ite/itp.h"
#include "ite/itu.h"
#include "../driver//ith/ith_pwm.c"
#include "../driver/itp/itp_cfg.h"
#include <errno.h>
#include <unistd.h>

#pragma execution_character_set("utf-8")

static ITUTrackBar* settingDisplayBrightnessTrackBar;
static ITUProgressBar* settingDisplayBrightnessProgressBar;
static ITUButton* paperBGButton;
static ITUButton* screenSaveTypeButton;
static ITUButton* screenSaveTimeButton;
static ITUWheel* paperBGWheel;
static ITUWheel* screenSaveTypeWheel;
static ITUWheel* screenSaveTimeWheel;
static ITUBackground* BGbackground;
static ITUBackground* setting_BGbackground;
static ITUText* brightnessTextEng;
static ITUText* brightnessTextCh;
static ITUText* wallpaperTextEng;
static ITUText* wallpaperTextChs;
static ITUText* wallpaperTextCht;
static ITUText* screenSaveTextEng;
static ITUText* screenSaveTextCh;
static ITUText* timeTextEng;
static ITUText* timeTextChs;
static ITUText* timeTextCht;

// status
static int settingDisplayScreenMaxBrightness;
static int settingDisplayBrightnessOld;
static int settingDisplayScreensaverTimeOld;
static ScreensaverType settingDisplayScreensaverTypeOld;
static bool mainMenuWallpaperChanged;
uint8_t* mainMenuWallpaperData;
int mainMenuWallpaperSize;
static char mainMenuWallpaperFilePath[PATH_MAX];
static char name[50] = { "" };

bool SettingDisplayBrightnessTrackBarOnChanged(ITUWidget* widget, char* param)
{
	int value = atoi(param)/10;

	if (value < 0)
		value = 0;
	//else if (value > settingDisplayScreenMaxBrightness)
		//value = settingDisplayScreenMaxBrightness;

	theConfig.brightness = value;
	ScreenSetBrightness(value);


	return true;
}


bool SettingDisplayPaperBGWheelOnChanged(ITUWidget* widget, char* param)
{
	ITUText* text = (ITUText*)ituWheelGetFocusItem((ITUWheel*)widget);
	printf("param=%d",(int)param);
	if (strcmp(param, "0") == 0)
	{
		strcpy(mainMenuWallpaperFilePath,"A:/wallpaper/default.jpg");
	}else if (strcmp(param, "1") == 0)
	{
		strcpy(mainMenuWallpaperFilePath, "A:/wallpaper/Desert.jpg");
	}else if (strcmp(param, "2") == 0)
	{
		strcpy(mainMenuWallpaperFilePath, "A:/wallpaper/Hydrangeas.jpg");
	}else if (strcmp(param,"3") == 0 )
	{
		strcpy(mainMenuWallpaperFilePath, "A:/wallpaper/Koala.jpg");
	}else if (strcmp(param, "4") == 0)
	{
		strcpy(mainMenuWallpaperFilePath, "A:/wallpaper/Lighthouse.jpg");
	}
	ituButtonSetString(paperBGButton, ituTextGetString(text));
	if (!ituWidgetIsVisible(paperBGWheel)){
		//printf(ituTextGetString(text));
		ituWidgetSetVisible(paperBGButton, true);
	}

	strcpy(theConfig.wallpaper, mainMenuWallpaperFilePath);//change theConfig
	mainMenuWallpaperChanged=true;

	FILE* f;
	ITUIcon* icon = (ITUIcon*)widget;
	//char* filePath = icon->filePath; 
	char* filePath = mainMenuWallpaperFilePath;
	printf("Id=%s", filePath);
	if (mainMenuWallpaperData)
	{
		free(mainMenuWallpaperData);
		mainMenuWallpaperData = NULL;
		mainMenuWallpaperSize = 0;
	}

	// try to load wallpaper jpeg file if exists
	f = fopen(filePath, "rb");
	if (f)
	{
		struct stat sb;

		if (fstat(fileno(f), &sb) != -1)
		{
			mainMenuWallpaperSize = sb.st_size;
			mainMenuWallpaperData = malloc(mainMenuWallpaperSize);
			if (mainMenuWallpaperData)
			{
				mainMenuWallpaperSize = fread(mainMenuWallpaperData, 1, mainMenuWallpaperSize, f);
			}
		}
		fclose(f);
	}
	if (mainMenuWallpaperData)
	{
		ituIconLoadJpegData((ITUIcon*)BGbackground, mainMenuWallpaperData, mainMenuWallpaperSize);
		ituIconLoadJpegData((ITUIcon*)setting_BGbackground, mainMenuWallpaperData, mainMenuWallpaperSize);
		ituWidgetUpdate(setting_BGbackground, ITU_EVENT_LAYOUT, 0, 0, 0);
		//strcpy(mainMenuWallpaperFilePath, filePath);
	}
	return true;
}

bool SettingDisplaySaveTypeWheelOnChanged(ITUWidget* widget,char* param)
{
	ITUText* text = (ITUText*)ituWheelGetFocusItem((ITUWheel*)widget);
	if (strcmp(param, "0") == 0)
	{
		theConfig.screensaver_type = SCREENSAVER_CLOCK;
	}else if (strcmp(param, "1") == 0)
	{
		theConfig.screensaver_type = SCREENSAVER_BLANK;
	}		
		ituButtonSetString(screenSaveTypeButton, ituTextGetString(text));
		if (!ituWidgetIsVisible(screenSaveTypeWheel)){
		printf(ituTextGetString(text));
	    ituWidgetSetVisible(screenSaveTypeButton, true);
		ituWidgetEnable(screenSaveTypeButton);
		}
		printf(param);
	return true;
}

bool SettingDisplaySaveTimeWheelOnChanged(ITUWidget* widget, char* param)
{
	ITUText* text = (ITUText*)ituWheelGetFocusItem((ITUWheel*)widget);
	if (strcmp(param,"0")==0)
	{
		theConfig.screensaver_time = 1;	
	}else if (strcmp(param, "1") == 0)
	{
		theConfig.screensaver_time = 2;
	}else if (strcmp(param, "1") == 0)
	{
		theConfig.screensaver_time = 5;
	}
	else if (strcmp(param, "1") == 0){
		theConfig.screensaver_time = 10;
	}
	ituButtonSetString(screenSaveTimeButton, ituTextGetString(text));
	if (!ituWidgetIsVisible(screenSaveTimeWheel)){
	ituWidgetSetVisible(screenSaveTimeButton, true);
	}
	printf(param);
	return true;
}

void SettingpaperBGButtonName(char paper[10]){
	printf("paper=%s",paper);
	switch (theConfig.lang)
	{
	case 0: 
		ituWidgetSetVisible(brightnessTextEng,true);
		ituWidgetSetVisible(brightnessTextCh,false);
		ituWidgetSetVisible(wallpaperTextEng, true);
		ituWidgetSetVisible(wallpaperTextChs, false);
		ituWidgetSetVisible(wallpaperTextCht, false);
		ituWidgetSetVisible(screenSaveTextEng, true);
		ituWidgetSetVisible(screenSaveTextCh, false);
		ituWidgetSetVisible(timeTextEng, true);
		ituWidgetSetVisible(timeTextChs, false);
		ituWidgetSetVisible(timeTextCht, false);
		ituButtonSetString(paperBGButton, paper);
		break;
	case 1:
		ituWidgetSetVisible(brightnessTextEng, false);
		ituWidgetSetVisible(brightnessTextCh, true);
		ituWidgetSetVisible(wallpaperTextEng, false);
		ituWidgetSetVisible(wallpaperTextChs, false);
		ituWidgetSetVisible(wallpaperTextCht, true);
		ituWidgetSetVisible(screenSaveTextEng, false);
		ituWidgetSetVisible(screenSaveTextCh, true);
		ituWidgetSetVisible(timeTextEng, false);
		ituWidgetSetVisible(timeTextChs, false);
		ituWidgetSetVisible(timeTextCht, true);
		if (strcmp(paper, "default") == 0){
			ituButtonSetString(paperBGButton, "默認");
		}
		else if (strcmp(paper, "Desert") == 0){
			ituButtonSetString(paperBGButton, "沙漠");
		}
		else if (strcmp(paper, "Hydrangeas") == 0){
			ituButtonSetString(paperBGButton, "繡球花");
		}
		else if (strcmp(paper, "Koala") == 0){
			ituButtonSetString(paperBGButton, "考拉");
		}
		else if (strcmp(paper, "Lighthouse") == 0){
			ituButtonSetString(paperBGButton, "燈塔");
		}
		break;
	case 2:
		ituWidgetSetVisible(brightnessTextEng, false);
		ituWidgetSetVisible(brightnessTextCh, true);
		ituWidgetSetVisible(wallpaperTextEng, false);
		ituWidgetSetVisible(wallpaperTextChs, true);
		ituWidgetSetVisible(wallpaperTextCht, false);
		ituWidgetSetVisible(screenSaveTextEng, false);
		ituWidgetSetVisible(screenSaveTextCh, true);
		ituWidgetSetVisible(timeTextEng, false);
		ituWidgetSetVisible(timeTextChs, true);
		ituWidgetSetVisible(timeTextCht, false);
		if (strcmp(paper, "default") == 0){
			ituButtonSetString(paperBGButton, "默认");
		}
		else if (strcmp(paper, "Desert") == 0){
			ituButtonSetString(paperBGButton, "沙漠");
		}
		else if (strcmp(paper, "Hydrangeas") == 0){
			ituButtonSetString(paperBGButton, "绣球花");
		}
		else if (strcmp(paper, "Koala") == 0){
			ituButtonSetString(paperBGButton, "考拉");
		}
		else if (strcmp(paper, "Lighthouse") == 0){
			ituButtonSetString(paperBGButton, "灯塔");
		}
		break;
	default:
		break;
	}
}

void SettingBGbackgroundWheel(char filename[10]){
	if (strcmp(filename, "Default") == 0){
		ituWheelGoto(paperBGWheel, 0);
	}
	else if (strcmp(filename, "Desert") == 0){
		ituWheelGoto(paperBGWheel, 1);
	}
	else if (strcmp(filename, "Hydrangeas") == 0){
		ituWheelGoto(paperBGWheel, 2);
	}
	else if (strcmp(filename, "Koala") == 0){
		ituWheelGoto(paperBGWheel, 3);
	}
	else if (strcmp(filename, "Lighthouse") == 0){
		ituWheelGoto(paperBGWheel, 4);
	}
}


bool SettingDisplayOnEnter(ITUWidget* widget, char* param)
{
	if (!settingDisplayBrightnessTrackBar)
	{
		settingDisplayBrightnessTrackBar = ituSceneFindWidget(&theScene, "settingBrightnessTrackBar");
		assert(settingDisplayBrightnessTrackBar);

		settingDisplayBrightnessProgressBar = ituSceneFindWidget(&theScene, "settingBrightnessProgressBar");
		assert(settingDisplayBrightnessProgressBar);

		paperBGButton = ituSceneFindWidget(&theScene, "paperBGButton");
		assert(paperBGButton);

		screenSaveTypeButton = ituSceneFindWidget(&theScene, "screenSaveTypeButton");
		assert(screenSaveTypeButton);

		screenSaveTimeButton = ituSceneFindWidget(&theScene, "screenSaveTimeButton");
		assert(screenSaveTimeButton);

		paperBGWheel = ituSceneFindWidget(&theScene, "paperBGWheel");
		assert(paperBGWheel);

		screenSaveTypeWheel = ituSceneFindWidget(&theScene, "screenSaveTypeWheel");
		assert(screenSaveTypeWheel);

		screenSaveTimeWheel = ituSceneFindWidget(&theScene, "screenSaveTimeWheel");
		assert(screenSaveTimeWheel);

		BGbackground = ituSceneFindWidget(&theScene,"Background2");
		assert(BGbackground);

		setting_BGbackground = ituSceneFindWidget(&theScene, "settingBackground");
		assert(setting_BGbackground);

		brightnessTextEng = ituSceneFindWidget(&theScene, "Brightness_Text");
		assert(brightnessTextEng);

		brightnessTextCh = ituSceneFindWidget(&theScene, "Brightness_Text_lang");
		assert(brightnessTextCh);

		wallpaperTextEng = ituSceneFindWidget(&theScene, "Wallpaper_Text");
		assert(wallpaperTextEng);

		wallpaperTextChs = ituSceneFindWidget(&theScene, "Wallpaper_Text_langs");
		assert(wallpaperTextChs);

		wallpaperTextCht = ituSceneFindWidget(&theScene, "Wallpaper_Text_langt");
		assert(wallpaperTextCht);

		screenSaveTextEng = ituSceneFindWidget(&theScene, "ScreenSave_Text");
		assert(screenSaveTextEng);

		screenSaveTextCh = ituSceneFindWidget(&theScene, "ScreenSave_Text_lang");
		assert(screenSaveTextCh);

		timeTextEng = ituSceneFindWidget(&theScene, "Time_Text");
		assert(timeTextEng);

		timeTextChs = ituSceneFindWidget(&theScene, "Time_Text_langs");
		assert(timeTextChs);

		timeTextCht = ituSceneFindWidget(&theScene, "Time_Text_langt");
		assert(timeTextCht);

		// status
		settingDisplayScreenMaxBrightness = ScreenGetMaxBrightness();
	}

	// current settings
	settingDisplayBrightnessOld = theConfig.brightness;
	ituTrackBarSetValue(settingDisplayBrightnessTrackBar, theConfig.brightness*10);
	ituProgressBarSetValue(settingDisplayBrightnessProgressBar, theConfig.brightness*10);
	//setting background
	/**
	get name of the config.wallpaper
	*/
	printf(theConfig.wallpaper);
	char *p = theConfig.wallpaper;
	char *tmp = p;
	tmp=strrchr(tmp, '/') + 1;
	strncpy(name, tmp, strlen(tmp)-4);
	name[strlen(tmp) - 4] = '\0';
	printf("name= %s",name);
	SettingpaperBGButtonName(name);
	//ituButtonUpdate(paperBGButton, ITU_EVENT_LAYOUT, 0, 0, 0);
	//ituButtonSetString(paperBGButton, name);
	SettingBGbackgroundWheel(name);

	//set screensavetime
	switch (theConfig.screensaver_time)
	{
	case 1:
		if (theConfig.lang == 0){
			ituButtonSetString(screenSaveTimeButton, "1 minutes");
		}
		else if (theConfig.lang == 1){
			ituButtonSetString(screenSaveTimeButton, "1分鐘");
		}
		else{
			ituButtonSetString(screenSaveTimeButton, "1分钟");
		}
		ituWheelGoto(screenSaveTimeWheel, 0);
		break;

	case 2:
		if (theConfig.lang == 0){
			ituButtonSetString(screenSaveTimeButton, "2 minutes");
		}
		else if (theConfig.lang == 1){
			ituButtonSetString(screenSaveTimeButton, "2分鐘");
		}
		else{
			ituButtonSetString(screenSaveTimeButton, "2分钟");
		}
		ituWheelGoto(screenSaveTimeWheel, 1);
		break;

	case 5:
		if (theConfig.lang == 0){
			ituButtonSetString(screenSaveTimeButton, "5 minutes");
		}
		else if (theConfig.lang == 1){
			ituButtonSetString(screenSaveTimeButton, "5分鐘");
		}
		else{
			ituButtonSetString(screenSaveTimeButton, "5分钟");
		}
		ituWheelGoto(screenSaveTimeWheel, 2);
		break;

	case 10:
		if (theConfig.lang == 0){
			ituButtonSetString(screenSaveTimeButton, "10 minutes");
		}
		else if (theConfig.lang == 1){
			ituButtonSetString(screenSaveTimeButton, "10分鐘");
		}
		else{
			ituButtonSetString(screenSaveTimeButton, "10分钟");
		}
		ituWheelGoto(screenSaveTimeWheel, 3);
		break;
	}
	settingDisplayScreensaverTimeOld = theConfig.screensaver_time;

	//set screensaver_type
	switch (theConfig.screensaver_type)
	{
	case 1:
		if (theConfig.lang == 0){
			ituButtonSetString(screenSaveTypeButton, "Clock");
		}
		else if (theConfig.lang == 1){
			ituButtonSetString(screenSaveTypeButton, "时鐘");
		}
		else{
			ituButtonSetString(screenSaveTypeButton, "时钟");
		}
		ituWheelGoto(screenSaveTypeWheel, 0);
		break;

	case SCREENSAVER_BLANK:
		if (theConfig.lang == 0){
			ituButtonSetString(screenSaveTypeButton, "Black Screen");
		}
		else{
			ituButtonSetString(screenSaveTypeButton, "黑屏");
		}
		ituWheelGoto(screenSaveTypeWheel, 1);
		break;
	}
	settingDisplayScreensaverTypeOld = theConfig.screensaver_type;

	return true;
}

bool SettingDisplayOnLeave(ITUWidget* widget, char* param)
{
	if (settingDisplayBrightnessOld != theConfig.brightness ||
		settingDisplayScreensaverTimeOld != theConfig.screensaver_time ||
		settingDisplayScreensaverTypeOld != theConfig.screensaver_type ||
		mainMenuWallpaperChanged)
	{
		ConfigSave();
	}
	return true;
}

void SettingDisplayReset(void)
{
	settingDisplayBrightnessTrackBar = NULL;
}