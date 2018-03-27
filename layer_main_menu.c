#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "doorbell.h"


static ITUBackground* BGBackground;
static ITUBackground* setting_Background;

uint8_t* mainMenuWallpaperData;
int mainMenuWallpaperSize;
static char mainMenuWallpaperFilePath[PATH_MAX];
static bool mainMenuWallpaperChanged;

/*bool MainMenuColorPickerOnSelection(ITUWidget* widget, char* param)
{
    uint32_t value = (uint32_t)atoi(param);

    if (mainMenuWallpaperData)
    {
        ituIconReleaseSurface((ITUIcon*)mainMenuBackgroundButton);
        free(mainMenuWallpaperData);
        mainMenuWallpaperData = NULL;
        mainMenuWallpaperSize = 0;
        mainMenuWallpaperFilePath[0] = '\0';
    }
    ituSetColor(&((ITUWidget*)mainMenuBackgroundButton)->color, 255, (value & 0x00FF0000) >> 16, (value & 0x0000FF00) >> 8, value & 0x000000FF);

    mainMenuWallpaperChanged = true;

    return true;
}

bool MainMenuWallpaperButtonOnPress(ITUWidget* widget, char* param)
{
    FILE* f;
    ITUIcon* icon = (ITUIcon*)widget;
    char* filePath = icon->filePath;
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
        ituIconLoadJpegData((ITUIcon*)mainMenuBackgroundButton, mainMenuWallpaperData, mainMenuWallpaperSize);
        strcpy(mainMenuWallpaperFilePath, filePath);
    }
    mainMenuWallpaperChanged = true;

    return true;
}

bool MainMenuResetButtonOnPress(ITUWidget* widget, char* param)
{
    if (mainMenuWallpaperData)
    {
        ituIconReleaseSurface((ITUIcon*)mainMenuBackgroundButton);
        free(mainMenuWallpaperData);
        mainMenuWallpaperData = NULL;
        mainMenuWallpaperSize = 0;
        mainMenuWallpaperFilePath[0] = '\0';
    }

    ituSetColor(&((ITUWidget*)mainMenuBackgroundButton)->color, 255, theConfig.bgcolor_red, theConfig.bgcolor_green, theConfig.bgcolor_blue);

    if (theConfig.wallpaper[0] && !mainMenuWallpaperData)
    {
        FILE* f;

        // try to load wallpaper jpeg file if exists
        f = fopen(theConfig.wallpaper, "rb");
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
    }
    if (mainMenuWallpaperData)
    {
        ituIconLoadJpegData((ITUIcon*)mainMenuBackgroundButton, mainMenuWallpaperData, mainMenuWallpaperSize);
    }
    mainMenuWallpaperChanged = false;
    return true;
}

bool MainMenuSaveButtonOnPress(ITUWidget* widget, char* param)
{
    theConfig.bgcolor_red = ((ITUWidget*)mainMenuBackgroundButton)->color.red;
    theConfig.bgcolor_green = ((ITUWidget*)mainMenuBackgroundButton)->color.green;
    theConfig.bgcolor_blue = ((ITUWidget*)mainMenuBackgroundButton)->color.blue;

    strcpy(theConfig.wallpaper, mainMenuWallpaperFilePath);

    ConfigSave();

    ituWidgetUpdate(mainMenuBackgroundButton, ITU_EVENT_LAYOUT, 0, 0, 0);
    mainMenuWallpaperChanged = false;
    return true;
}

bool MainMenuCancelButtonOnPress(ITUWidget* widget, char* param)
{
    ituWidgetUpdate(mainMenuBackgroundButton, ITU_EVENT_LAYOUT, 0, 0, 0);
    return MainMenuResetButtonOnPress(widget, param);
}

static void DismissElevator(int arg)
{
    ituWidgetSetVisible(mainMenuElevatorBackground, false);
    ituWidgetEnable(mainMenuBackgroundButton);
}

bool MainMenuElevatorDownButtonOnPress(ITUWidget* widget, char* param)
{
    RemoteCallElevator(false);
    ituSceneExecuteCommand(&theScene, MAINMENU_ELEVATOR_DELAY, DismissElevator, 0);
	return true;
}

bool MainMenuElevatorUpButtonOnPress(ITUWidget* widget, char* param)
{
    RemoteCallElevator(true);
    ituSceneExecuteCommand(&theScene, MAINMENU_ELEVATOR_DELAY, DismissElevator, 0);
	return true;
}

bool MainMenuOnTimer(ITUWidget* widget, char* param)
{
    if (mainMenuWeatherDelayCount++ >= MAINMENU_WEATHER_INTERVAL)
    {
        if (WeatherIsAvailable())
        {
            float average;
            char buf[16];

            ituWidgetSetVisible(mainMenuWeatherSprite, true);
            ituSpriteGoto(mainMenuWeatherSprite, WeatherGetSymbolIndex(1));

            // day 1
            average = (WeatherGetTemperatureMin(1) + WeatherGetTemperatureMax(1)) / 2.0f;
            sprintf(buf, "%d°C", (int)average);
            ituTextSetString(mainMenuTemperatureText, buf);

			mainMenuWeatherDelayCount = 0;
            return true;
		}
    }
    return false;
}*/

bool MainMenuOnEnter(ITUWidget* widget, char* param)
{
    if (!BGBackground)
    {
        BGBackground = ituSceneFindWidget(&theScene, "Background2");
        assert(BGBackground);

		setting_Background = ituSceneFindWidget(&theScene,"settingBackground");
		assert(setting_Background);

        if (theConfig.wallpaper[0] && !mainMenuWallpaperData)
        {
            FILE* f;

            // try to load wallpaper jpeg file if exists
            f = fopen(theConfig.wallpaper, "rb");
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
                        strcpy(mainMenuWallpaperFilePath, theConfig.wallpaper);
                    }
                }
                fclose(f);
            }
        }
        SceneSetReady(true);
        ituSceneExecuteCommand(&theScene, 3, ScenePredraw, 0);
    }

    if (mainMenuWallpaperData)
    {
        ituIconLoadJpegData((ITUIcon*)BGBackground, mainMenuWallpaperData, mainMenuWallpaperSize);
		ituIconLoadJpegData((ITUIcon*)setting_Background, mainMenuWallpaperData, mainMenuWallpaperSize);
    }
    mainMenuWallpaperChanged = false;

    //SceneResetLongPressTime();

    return true;
}

bool MainMenuOnLeave(ITUWidget* widget, char* param)
{
   
    return true;
}

void MainMenuReset(void)
{

    free(mainMenuWallpaperData);
    mainMenuWallpaperData = NULL;
    mainMenuWallpaperSize = 0;
}