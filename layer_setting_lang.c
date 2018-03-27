#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "doorbell.h"

//#pragma execution_character_set("utf-8")

static ITUBackground* settingLangBackground;
static ITURadioBox* settingLangChtRadioBox;
static ITURadioBox* settingLangChsRadioBox;
static ITURadioBox* settingLangEngRadioBox;
static ITUButton* paperBGButton;
static ITUButton* screenSaveTypeButton;
static ITUButton* screenSaveTimeButton;

static void SettingLangChangeCommand(int arg)
{
    ituSceneUpdate(&theScene, ITU_EVENT_LANGUAGE, theConfig.lang, 0, 0);
    ituSceneUpdate(&theScene, ITU_EVENT_LAYOUT, 0, 0, 0);
    ConfigSave();
}


bool SettingLangChtRadioBoxOnPress(ITUWidget* widget, char* param)
{
    if (theConfig.lang == LANG_CHT)
        return false;

    theConfig.lang = LANG_CHT;
    ituSceneExecuteCommand(&theScene, 6, SettingLangChangeCommand, 0);
    return true;
}

bool SettingLangChsRadioBoxOnPress(ITUWidget* widget, char* param)
{
    if (theConfig.lang == LANG_CHS)
        return false;

    theConfig.lang = LANG_CHS;
    ituSceneExecuteCommand(&theScene, 6, SettingLangChangeCommand, 0);

    return true;
}

bool SettingLangEngRadioBoxOnPress(ITUWidget* widget, char* param)
{
    if (theConfig.lang == LANG_ENG)
        return false;

    theConfig.lang = LANG_ENG;
    ituSceneExecuteCommand(&theScene, 6, SettingLangChangeCommand, 0);

    return true;
}

bool SettingLangOnEnter(ITUWidget* widget, char* param)
{
    if (!settingLangBackground)
    {
        settingLangBackground = ituSceneFindWidget(&theScene, "settingLangBackground");
        assert(settingLangBackground);

        settingLangChtRadioBox = ituSceneFindWidget(&theScene, "settingLangChtRadioBox");
        assert(settingLangChtRadioBox);

        settingLangChsRadioBox = ituSceneFindWidget(&theScene, "settingLangChsRadioBox");
        assert(settingLangChsRadioBox);

        settingLangEngRadioBox = ituSceneFindWidget(&theScene, "settingLangEngRadioBox");
        assert(settingLangEngRadioBox);

		paperBGButton = ituSceneFindWidget(&theScene, "paperBGButton");
		assert(paperBGButton);

		screenSaveTypeButton = ituSceneFindWidget(&theScene, "screenSaveTypeButton");
		assert(screenSaveTypeButton);

		screenSaveTimeButton = ituSceneFindWidget(&theScene, "screenSaveTimeButton");
		assert(screenSaveTimeButton);
    }

    // current settings
    if (theConfig.lang == LANG_CHT)
        ituRadioBoxSetChecked(settingLangChtRadioBox, true);
    else if (theConfig.lang == LANG_CHS)
        ituRadioBoxSetChecked(settingLangChsRadioBox, true);
    else
        ituRadioBoxSetChecked(settingLangEngRadioBox, true);

    return true;
}

bool SettingLangOnLeave(ITUWidget* widget, char* param)
{
    return true;
}

void SettingLangReset(void)
{
    settingLangBackground = NULL;
}
