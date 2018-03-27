﻿#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/ip.h"
#include "SDL/SDL.h"
#include "scene.h"
#include "doorbell.h"
#include "linphone/linphone_castor3.h"

#define ERROR_COUNTDOWN (3.999f)

static ITULayer*        mainMenuLayer;
static ITUText*         callingSourceText;
static ITUBackground*   callingRemoteBackground;
static ITUSprite*       callingRemoteSprite;
static ITUProgressBar*  callingVolProgressBar;
static ITUTrackBar*     callingVolTrackBar;
static ITUText*         callingCountDownText;
static ITUBackground*   callingErrorBackground;
static ITUText*         callingErrorCountDownText;
static ITUBackground*   callingWaitingBackground;
static ITUText*         callingWaitingTypeText;
static ITUText*         callingWaitingRoomText;
static ITUButton*       callingUnlockButton;
static ITUButton*       callingSnapshotButton;
static ITUButton*       callingRecordButton;
static ITUButton*       callingStopRecordButton;
static ITUButton*       callingAnswerButton;
static ITUButton*       callingButtonContainer;

// status
static bool callingInVideoState;
static bool callingConnected;
static float callingCountDown, callingErrorCountDown, dialCountDown, spotCountDown, LockRecordButtonCountDown;
static uint32_t callingLastTick, callingErrorLastTick, dialLastTick, spotLastTick, LockRecordButtonLastTick;
static int callingID, callingPendingID;
static bool callingPendingVideo;
static char callingPendingAddr[32];
static DeviceInfo callingDeviceInfo;
static int callingVoiceLevelOld, callingRingLevelOld;
static bool callingHangUp;
static int snapshotCountDown;
static bool callingVideoRecordEnable;
static DeviceInfo callingManagerInfoArray[MAX_MANAGER_COUNT];
static int callingManagerIndex;
static int callingManagerCount;
static char callingManagerIPAddr[16];
static bool LockRecordButton;

extern SPOTTED_STATUS callSpottedStatus;

#define SNAPSHOT_COUNT (1 * 1000 / MS_PER_FRAME) // 1 sec

static bool CallingWaitingConfirmButtonOnSimulatePressing()
{
    char* desc;

    //LinphoneTerminate(callingPendingID);
    //LinphoneAnswer(callingPendingID);
    CaptureStopRecord();
    if (callingPendingVideo)
    {
        ituWidgetSetVisible(callingRemoteBackground, true);
        ituWidgetSetVisible(callingRemoteSprite, false);
        ituWidgetSetVisible(callingSnapshotButton, true);
        ituWidgetSetVisible(callingAnswerButton, true);
        ituWidgetSetVisible(callingStopRecordButton, false);
        if (callingVideoRecordEnable)
            ituWidgetSetVisible(callingRecordButton, true);
        else
            ituWidgetSetVisible(callingRecordButton, false);    
    }
    else
    {
        ituWidgetSetVisible(callingRemoteBackground, false);
        ituWidgetSetVisible(callingRemoteSprite, true);
        ituWidgetSetVisible(callingSnapshotButton, false);
        ituWidgetSetVisible(callingAnswerButton, true);
        ituWidgetSetVisible(callingStopRecordButton, false);
        ituWidgetSetVisible(callingRecordButton, false);
    }

    AddressBookGetDeviceInfo(&callingDeviceInfo, callingPendingAddr);

    ituWidgetSetVisible(callingUnlockButton, (callingDeviceInfo.type >= DEVICE_OUTDOOR && callingDeviceInfo.type <= DEVICE_WALL) ? true : false);
    
    desc = StringGetRoomAddress(callingDeviceInfo.area, callingDeviceInfo.building, callingDeviceInfo.unit, callingDeviceInfo.floor, callingDeviceInfo.room, NULL);
    ituTextSetString(callingSourceText, desc);
    free(desc);

    callingInVideoState = callingPendingVideo;
    callingID = callingPendingID;
    callingPendingID = 0;
    ituWidgetEnable(callingButtonContainer);
    callSpottedStatus = SPOTTED_ALLOW;
	return true;
}

bool CallingVolTrackBarOnChanged(ITUWidget* widget, char* param)
{
    int level = atoi(param);

    if (callingConnected)
    {
        LinphoneSetVoiceLevel(level);
    }
    else
    {
        LinphoneSetRingLevel(level, true);
    }
	return true;
}

bool CallingRecordButtonOnPress(ITUWidget* widget, char* param)
{
    if(LockRecordButton) return false;
    if(callingConnected && callingCountDown < 2.0f) return false;
    if (CaptureStartRecord(callingInVideoState ? MEDIA_VIDEO : MEDIA_AUDIO, callingDeviceInfo.ip))
        return false;

    LockRecordButton = true;
    LockRecordButtonCountDown = 1.0f;
    LockRecordButtonLastTick = SDL_GetTicks();

	return true;
}

bool CallingStopRecordButtonOnPress(ITUWidget* widget, char* param)
{
    if(LockRecordButton) return false;
    CaptureStopRecord();

    LockRecordButton = true;
    LockRecordButtonCountDown = 1.0f;
    LockRecordButtonLastTick = SDL_GetTicks();
    
	return true;
}

bool CallingAnswerButtonOnPress(ITUWidget* widget, char* param)
{
    if (callingConnected)
    {
        LinphoneTerminate(0);
        SceneHangUp();
        ituLayerGoto(mainMenuLayer);
    }
    
	return true;
}

bool CallingUnlockButtonOnPress(ITUWidget* widget, char* param)
{
    SceneOpenDoor(NULL);
	return true;
}

bool CallingSnapshotButtonOnPress(ITUWidget* widget, char* param)
{
    if (snapshotCountDown)
        return false;

    CaptureSnapshot(callingDeviceInfo.ip, 0);
    snapshotCountDown = SNAPSHOT_COUNT;
	return true;
}

bool CallingHangUpButtonOnPress(ITUWidget* widget, char* param)
{
    ITUButton* btn = (ITUButton*)widget;

    ituButtonSetPressed(btn, false);

    LinphoneTerminate(0);
    SceneHangUp();
    callingHangUp = true;
	return true;
}

bool CallingErrorConfirmButtonOnPress(ITUWidget* widget, char* param)
{
    callingErrorCountDown = 0;
	return true;
}

bool CallingWaitingCancelButtonOnPress(ITUWidget* widget, char* param)
{
    ITUButton* btn = (ITUButton*)widget;

    ituButtonSetPressed(btn, false);

    //LinphoneTerminate(callingPendingID);
    callingPendingID = 0;
    ituWidgetEnable(callingButtonContainer);
    callSpottedStatus = SPOTTED_REJECT;
	return true;
}

bool CallingWaitingConfirmButtonOnPress(ITUWidget* widget, char* param)
{
    char* desc;

    //LinphoneTerminate(callingPendingID);
    //LinphoneAnswer(callingPendingID);
    CaptureStopRecord();
    if (callingPendingVideo)
    {
        ituWidgetSetVisible(callingRemoteBackground, true);
        ituWidgetSetVisible(callingRemoteSprite, false);
        ituWidgetSetVisible(callingSnapshotButton, true);
        ituWidgetSetVisible(callingAnswerButton, true);
        ituWidgetSetVisible(callingStopRecordButton, false);
        if (callingVideoRecordEnable)
            ituWidgetSetVisible(callingRecordButton, true);
        else
            ituWidgetSetVisible(callingRecordButton, false);    
    }
    else
    {
        ituWidgetSetVisible(callingRemoteBackground, false);
        ituWidgetSetVisible(callingRemoteSprite, true);
        ituWidgetSetVisible(callingSnapshotButton, false);
        ituWidgetSetVisible(callingAnswerButton, true);
        ituWidgetSetVisible(callingStopRecordButton, false);
        ituWidgetSetVisible(callingRecordButton, false);
    }

    AddressBookGetDeviceInfo(&callingDeviceInfo, callingPendingAddr);

    ituWidgetSetVisible(callingUnlockButton, (callingDeviceInfo.type >= DEVICE_OUTDOOR && callingDeviceInfo.type <= DEVICE_WALL) ? true : false);
    
    desc = StringGetRoomAddress(callingDeviceInfo.area, callingDeviceInfo.building, callingDeviceInfo.unit, callingDeviceInfo.floor, callingDeviceInfo.room, NULL);
    ituTextSetString(callingSourceText, desc);
    free(desc);

    callingInVideoState = callingPendingVideo;
    callingID = callingPendingID;
    callingPendingID = 0;
    ituWidgetEnable(callingButtonContainer);
    callSpottedStatus = SPOTTED_ALLOW;
	return true;
}

#if (CFG_CHIP_FAMILY == 9850) && (CFG_VIDEO_ENABLE)
static void callingRemoteBackgroundDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    ITURectangle* rect = (ITURectangle*) &widget->rect;

    destx = rect->x + x;
    desty = rect->y + y;
    
    ituDrawVideoSurface(dest, destx, desty, rect->width, rect->height);
    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}
#endif

int CallingCall(char* area, char* building, char* unit, char* floor, char* room, char* ext)
{
    char* ip;
    if (!callingRemoteBackground)
    {        
        callingRemoteBackground = ituSceneFindWidget(&theScene, "callingRemoteBackground");
        assert(callingRemoteBackground);
#if (CFG_CHIP_FAMILY == 9850) && (CFG_VIDEO_ENABLE)        
        ituWidgetSetDraw(callingRemoteBackground, callingRemoteBackgroundDraw);
#endif        

        callingRemoteSprite = ituSceneFindWidget(&theScene, "callingRemoteSprite");
        assert(callingRemoteSprite);

        callingSourceText = ituSceneFindWidget(&theScene, "callingSourceText");
        assert(callingSourceText);

        callingUnlockButton = ituSceneFindWidget(&theScene, "callingUnlockButton");
        assert(callingUnlockButton);

        callingSnapshotButton = ituSceneFindWidget(&theScene, "callingSnapshotButton");
        assert(callingSnapshotButton);    

        callingRecordButton = ituSceneFindWidget(&theScene, "callingRecordButton");
        assert(callingRecordButton);    
    }

    callingManagerIndex = -1;
    callingVideoRecordEnable = false;
	printf("theDeviceInfo.type=%s",theDeviceInfo.type);

    if (theDeviceInfo.type == DEVICE_INDOOR2 && CaptureGetPath())
        callingVideoRecordEnable = true;

    if (ext && strlen(ext) > 0)
    {
        ip = AddressBookGetDeviceIP(area, building, unit, floor, room, ext);
        if (ip)
        {
            char* desc;
            callingInVideoState = false;

            AddressBookGetDeviceInfo(&callingDeviceInfo, ip);
            switch (callingDeviceInfo.type)
            {
            case DEVICE_CENTER:
            case DEVICE_OUTDOOR:
            case DEVICE_ENTRANCE:
            case DEVICE_BUILDING:
            case DEVICE_WALL:
            //case DEVICE_MANAGER:
            case DEVICE_INTERCOM:
                callingInVideoState = true;
                break;
            }

            LinphoneCall(ip, callingInVideoState, false);
            free(ip);

            dialCountDown = (float)theConfig.dial_time;
            dialLastTick = SDL_GetTicks();

            desc = StringGetRoomAddress(area, building, unit, floor, room, NULL);
            ituTextSetString(callingSourceText, desc);
            free(desc);

            if (callingInVideoState)
            {
                ituWidgetSetVisible(callingRemoteBackground, true);
                ituWidgetSetVisible(callingRemoteSprite, false);
                ituWidgetSetVisible(callingSnapshotButton, true);

                if (callingVideoRecordEnable)
                    ituWidgetSetVisible(callingRecordButton, true);
                else
                    ituWidgetSetVisible(callingRecordButton, false);    
            }
            else
            {
                ituWidgetSetVisible(callingRemoteBackground, false);
                ituWidgetSetVisible(callingRemoteSprite, true);
                ituWidgetSetVisible(callingSnapshotButton, false);
                ituWidgetSetVisible(callingRecordButton, false);
            }
            ituWidgetSetVisible(callingUnlockButton, (callingDeviceInfo.type >= DEVICE_OUTDOOR && callingDeviceInfo.type <= DEVICE_WALL) ? true : false);
            return 0;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        DeviceInfo infos[LINPHONE_CASTOR3_MAX_IP_COUNT];

        int len = AddressBookGetDeviceInfoArray(infos, LINPHONE_CASTOR3_MAX_IP_COUNT, area, building, unit, floor, room);
		printf("len=%d",len);
        if (len > 0)
        {
            char* desc;
            callingInVideoState = false;
            memcpy(&callingDeviceInfo, &infos[0], sizeof (callingDeviceInfo));
			printf("infos=%d", infos[0].area);
			printf("infos[0].type=%d",infos[0].type);

            switch (infos[0].type)
            {
            case DEVICE_CENTER:
            case DEVICE_OUTDOOR:
            case DEVICE_ENTRANCE:
            case DEVICE_BUILDING:
            case DEVICE_WALL:
            //case DEVICE_MANAGER:
            case DEVICE_INTERCOM:
                callingInVideoState = true;
                break;
            }
            LinphoneCallDevices(infos, len, callingInVideoState, false);

            dialCountDown = (float)theConfig.dial_time;
            dialLastTick = SDL_GetTicks();

            desc = StringGetRoomAddress(area, building, unit, floor, room, NULL);
            ituTextSetString(callingSourceText, desc);
            free(desc);

            if (callingInVideoState)
            {
                ituWidgetSetVisible(callingRemoteBackground, true);
                ituWidgetSetVisible(callingRemoteSprite, false);
                ituWidgetSetVisible(callingSnapshotButton, true);
                
                if (callingVideoRecordEnable)
                    ituWidgetSetVisible(callingRecordButton, true);
                else
                    ituWidgetSetVisible(callingRecordButton, false);
            }
            else
            {
                ituWidgetSetVisible(callingRemoteBackground, false);
                ituWidgetSetVisible(callingRemoteSprite, true);
                ituWidgetSetVisible(callingSnapshotButton, false);
                ituWidgetSetVisible(callingRecordButton, false);
            }
            ituWidgetSetVisible(callingUnlockButton, (callingDeviceInfo.type >= DEVICE_OUTDOOR && callingDeviceInfo.type <= DEVICE_WALL) ? true : false);
            return 0;
        }
        else
        {
            return -1;
        }
    }
}

int CallingCallByIP(char* ip)
{
    if (!callingRemoteBackground)
    {        
        callingRemoteBackground = ituSceneFindWidget(&theScene, "callingRemoteBackground");
        assert(callingRemoteBackground);
#if (CFG_CHIP_FAMILY == 9850) && (CFG_VIDEO_ENABLE)        
        ituWidgetSetDraw(callingRemoteBackground, callingRemoteBackgroundDraw);
#endif        

        callingRemoteSprite = ituSceneFindWidget(&theScene, "callingRemoteSprite");
        assert(callingRemoteSprite);

        callingSourceText = ituSceneFindWidget(&theScene, "callingSourceText");
        assert(callingSourceText);

        callingSnapshotButton = ituSceneFindWidget(&theScene, "callingSnapshotButton");
        assert(callingSnapshotButton);

        callingRecordButton = ituSceneFindWidget(&theScene, "callingRecordButton");
        assert(callingRecordButton);

        callingUnlockButton = ituSceneFindWidget(&theScene, "callingUnlockButton");
        assert(callingUnlockButton);
    }

    callingManagerIndex = -1;

    if (ip)
    {
        char* desc;
        int n = ipaddr_addr(ip);
        if (n == -1)
            return -1;

        callingInVideoState = false;

        AddressBookGetDeviceInfo(&callingDeviceInfo, ip);
        desc = StringGetRoomAddress(callingDeviceInfo.area, callingDeviceInfo.building, callingDeviceInfo.unit, callingDeviceInfo.floor, callingDeviceInfo.room, NULL);
        ituTextSetString(callingSourceText, desc);
        free(desc);

        switch (callingDeviceInfo.type)
        {
        case DEVICE_CENTER:
        case DEVICE_OUTDOOR:
        case DEVICE_ENTRANCE:
        case DEVICE_BUILDING:
        case DEVICE_WALL:
        //case DEVICE_MANAGER:
        case DEVICE_INTERCOM:
            ituWidgetSetVisible(callingRemoteBackground, false);
            callingInVideoState = true;
            break;
        }
        LinphoneCall(ip, callingInVideoState, false);

        dialCountDown = (float)theConfig.dial_time;
        dialLastTick = SDL_GetTicks();

        if (callingInVideoState)
        {
            ituWidgetSetVisible(callingRemoteBackground, true);
            ituWidgetSetVisible(callingRemoteSprite, false);
            ituWidgetSetVisible(callingSnapshotButton, true);

            if (callingVideoRecordEnable)
                ituWidgetSetVisible(callingRecordButton, true);
            else
                ituWidgetSetVisible(callingRecordButton, false);
        }
        else
        {
            ituWidgetSetVisible(callingRemoteBackground, false);
            ituWidgetSetVisible(callingRemoteSprite, true);
            ituWidgetSetVisible(callingSnapshotButton, false);
            ituWidgetSetVisible(callingRecordButton, false);
        }
        ituWidgetSetVisible(callingUnlockButton, (callingDeviceInfo.type >= DEVICE_OUTDOOR && callingDeviceInfo.type <= DEVICE_WALL) ? true : false);
        return 0;
    }
    else
    {
        return -1;
    }
}

int CallingCallManager(char* ip)
{
    char* desc;

    if (!callingRemoteBackground)
    {      
        callingRemoteBackground = ituSceneFindWidget(&theScene, "callingRemoteBackground");
        assert(callingRemoteBackground);
#if (CFG_CHIP_FAMILY == 9850) && (CFG_VIDEO_ENABLE)        
        ituWidgetSetDraw(callingRemoteBackground, callingRemoteBackgroundDraw);
#endif        

        callingRemoteSprite = ituSceneFindWidget(&theScene, "callingRemoteSprite");
        assert(callingRemoteSprite);

        callingSourceText = ituSceneFindWidget(&theScene, "callingSourceText");
        assert(callingSourceText);

        callingSnapshotButton = ituSceneFindWidget(&theScene, "callingSnapshotButton");
        assert(callingSnapshotButton);

        callingRecordButton = ituSceneFindWidget(&theScene, "callingRecordButton");
        assert(callingRecordButton);

        callingUnlockButton = ituSceneFindWidget(&theScene, "callingUnlockButton");
        assert(callingUnlockButton);
    }

    callingInVideoState = false;
    callingManagerIndex = -1;
    callingManagerCount = AddressBookGetManagerInfoArray(callingManagerInfoArray, MAX_MANAGER_COUNT);

    if (ip)
    {
        strcpy(callingManagerIPAddr, ip);
        AddressBookGetDeviceInfo(&callingDeviceInfo, ip);
        callingManagerIndex = callingManagerCount;
    }
    else
    {
        callingManagerIPAddr[0] = '\0';

        if (callingManagerCount > 0)
        {
            callingManagerIndex = 0;
            memcpy(&callingDeviceInfo, &callingManagerInfoArray[callingManagerIndex], sizeof (DeviceInfo));
        }
        else if (theCenterInfo.ip[0] != '\0')
        {
            // call to center
            memcpy(&callingDeviceInfo, &theCenterInfo, sizeof (DeviceInfo));
        }
        else
        {
            return -1;
        }
    }

    LinphoneCall(callingDeviceInfo.ip, callingInVideoState, true);
    SceneSetRemoteIP(callingDeviceInfo.ip);

    if (callingDeviceInfo.alias[0] == '\0')
    {
        desc = StringGetRoomAddress(callingDeviceInfo.area, callingDeviceInfo.building, callingDeviceInfo.unit, callingDeviceInfo.floor, callingDeviceInfo.room, NULL);
        ituTextSetString(callingSourceText, desc);
        free(desc);
    }
    else
    {
        ituTextSetString(callingSourceText, callingDeviceInfo.alias);
    }

    dialCountDown = (float)theConfig.dial_time;
    dialLastTick = SDL_GetTicks();

    ituWidgetSetVisible(callingRemoteBackground, false);
    ituWidgetSetVisible(callingRemoteSprite, true);
    ituWidgetSetVisible(callingSnapshotButton, false);
    ituWidgetSetVisible(callingRecordButton, false);

    ituWidgetSetVisible(callingUnlockButton, false);
    return 0;
}

bool CallingConnectedOnCustom(ITUWidget* widget, char* param)
{
    char buf[32];
#if 1    
    int level = LinphoneGetVoiceLevel();

    if (level > 100)
        level = 100;
    else if (level < 0)
        level = 0;

    ituProgressBarSetValue(callingVolProgressBar, level);
    ituTrackBarSetValue(callingVolTrackBar, level);
#endif    
    callingConnected = true;

    dialCountDown       = 0.0f;
    callingCountDown    = ((theConfig.calling_time > 0) ? (float)theConfig.calling_time + 0.5f : (float)theConfig.calling_time);
    callingLastTick     = SDL_GetTicks();
    
    sprintf(buf, "%d", (int)callingCountDown);
    ituTextSetString(callingCountDownText, buf);

    callingID = atoi(param);
    
    return true;
}

bool CallingErrorOnCustom(ITUWidget* widget, char* param)
{
    char buf[4];
    int code = atoi(param);

    if (callingManagerIndex >= 0)
    {
        if (callingManagerIPAddr[0] != '\0')
        {
            if (callingManagerIndex == callingManagerCount)
                callingManagerIndex = 0;
            else
                callingManagerIndex++;

            for (; callingManagerIndex < callingManagerCount; callingManagerIndex++)
            {
                if (strcmp(callingManagerInfoArray[callingManagerIndex].ip, callingManagerIPAddr))
                    break;
            }
        }
        else
        {
            callingManagerIndex++;
        }

        if (callingManagerIndex == callingManagerCount)
        {
            if (theCenterInfo.ip[0] == '\0')
            {
               ituWidgetSetVisible(callingErrorBackground, true);
           
               dialCountDown            = 0.0f;
               callingErrorCountDown    = ERROR_COUNTDOWN;
               callingErrorLastTick     = SDL_GetTicks();
               sprintf(buf, "%d", (int)callingErrorCountDown);
               ituTextSetString(callingErrorCountDownText, buf);
            }
            else
            {
                // call to center
                memcpy(&callingDeviceInfo, &theCenterInfo, sizeof (DeviceInfo));
                callingManagerIndex = -1;
            }
        }
        else
        {
            memcpy(&callingDeviceInfo, &callingManagerInfoArray[callingManagerIndex], sizeof (DeviceInfo));
        }
        LinphoneCall(callingDeviceInfo.ip, callingInVideoState, true);
		SceneSetRemoteIP(callingDeviceInfo.ip);
        dialCountDown = (float)theConfig.dial_time;
        dialLastTick = SDL_GetTicks();
    }
    else
    {
        ituWidgetSetVisible(callingErrorBackground, true);

        dialCountDown            = 0.0f;
        callingErrorCountDown    = ERROR_COUNTDOWN;
        callingErrorLastTick     = SDL_GetTicks();
        sprintf(buf, "%d", (int)callingErrorCountDown);
        ituTextSetString(callingErrorCountDownText, buf);
    }
    return true;
}

bool CallingIncomingOnCustom(ITUWidget* widget, char* param)
{
    DeviceInfo info;
    char* desc;
	char addr[32];
	int id = 0, video = 0;

    if(callingErrorCountDown > 0)
    {
        ituWidgetSetVisible(callingErrorBackground, false);
        callingErrorCountDown = 0;
        LinphoneTerminate(0);
        SceneHangUp();
	    ituLayerGoto(mainMenuLayer);
        return true;
    }
	sscanf(param, "%s %d %d", addr, &id, &video);
    AddressBookGetDeviceInfo(&info, addr);

    ituTextSetString(callingWaitingTypeText, StringGetDeviceType(info.type));
    desc = StringGetRoomAddress(info.area, info.building, info.unit, info.floor, info.room, NULL);
    ituTextSetString(callingWaitingRoomText, desc);
    free(desc);

    if(callSpottedStatus == SPOTTED_INIT)
    {
        ituWidgetDisable(callingButtonContainer);
    	ituWidgetSetVisible(callingWaitingBackground, true);
        spotCountDown = (float)theConfig.dial_time;
        spotLastTick = SDL_GetTicks();
    }
    callingPendingID = id;
    strcpy(callingPendingAddr, addr);
    callingPendingVideo = video;
	return true;
}

bool CallingOnTimer(ITUWidget* widget, char* param)
{
    if (LockRecordButtonCountDown > 0.0f && LockRecordButton)
	{
        uint32_t tick = SDL_GetTicks();
        uint32_t diff = tick - LockRecordButtonLastTick;
        if (diff >= 1000)
        {
            LockRecordButtonCountDown -= (float)diff / 1000.0f;
            LockRecordButtonLastTick = tick;
        }
        if (LockRecordButtonCountDown <= 0.0f)
        {
            LockRecordButton = false;
        }
	}
    if (spotCountDown > 0.0f && callSpottedStatus == SPOTTED_INIT)
	{
        uint32_t tick = SDL_GetTicks();
        uint32_t diff = tick - spotLastTick;
        if (diff >= 1000)
        {
            spotCountDown -= (float)diff / 1000.0f;
            spotLastTick = tick;
        }
        if (spotCountDown <= 0.0f)
        {
            callingPendingID = 0;
            ituWidgetEnable(callingButtonContainer);
            ituWidgetSetVisible(callingWaitingBackground, false);
            callSpottedStatus = SPOTTED_REJECT;
        }
	}
	if (dialCountDown > 0.0f)
	{
        uint32_t tick = SDL_GetTicks();
        uint32_t diff = tick - dialLastTick;
        if (diff >= 1000)
        {
            dialCountDown -= (float)diff / 1000.0f;
            dialLastTick = tick;
        }
        if (dialCountDown <= 0.0f)
        {
            if (callingManagerIndex >= 0)
            {
                if (callingManagerIPAddr[0] != '\0')
                {
                    if (callingManagerIndex == callingManagerCount)
                        callingManagerIndex = 0;
                    else
                        callingManagerIndex++;

                    for (; callingManagerIndex < callingManagerCount; callingManagerIndex++)
                    {
                        if (strcmp(callingManagerInfoArray[callingManagerIndex].ip, callingManagerIPAddr))
                            break;
                    }
                }

                if (callingManagerIndex == callingManagerCount)
                {
                    if (theCenterInfo.ip[0] == '\0')
                    {
                        LinphoneTerminate(0);
                        SceneHangUp();
                        ituLayerGoto(mainMenuLayer);
                    }
                    else
                    {
						LinphoneTerminate(0);
                        // call to center
                        memcpy(&callingDeviceInfo, &theCenterInfo, sizeof (DeviceInfo));
                        callingManagerIndex = -1;
                        LinphoneCall(callingDeviceInfo.ip, callingInVideoState, true);
		                SceneSetRemoteIP(callingDeviceInfo.ip);
                        dialCountDown = (float)theConfig.dial_time;
                        dialLastTick = SDL_GetTicks();
                    }
                }
                else if(callSpottedStatus == SPOTTED_INIT)
                {
                    ituWidgetSetVisible(callingWaitingBackground, false);
                    CallingWaitingConfirmButtonOnSimulatePressing();
                }
                else
                {
					LinphoneTerminate(0);
                    memcpy(&callingDeviceInfo, &callingManagerInfoArray[callingManagerIndex], sizeof (DeviceInfo));
                    LinphoneCall(callingDeviceInfo.ip, callingInVideoState, true);
		            SceneSetRemoteIP(callingDeviceInfo.ip);
                    dialCountDown = (float)theConfig.dial_time;
                    dialLastTick = SDL_GetTicks();
                }
            }
            else
            {
                LinphoneTerminate(0);
                SceneHangUp();
    		    ituLayerGoto(mainMenuLayer);
            }    
        }
	}
    else if (callingCountDown > 0.0f)
    {
        uint32_t diff, tick = SDL_GetTicks();

        if (tick >= callingLastTick)
        {
            diff = tick - callingLastTick;
        }
        else
        {
            diff = 0xFFFFFFFF - callingLastTick + tick;
        }

        if (diff >= 1000)
        {
            char buf[4];

            callingCountDown -= (float)diff / 1000.0f;
            callingLastTick = tick;

            if (callingCountDown >= 0.0f)
            {
                sprintf(buf, "%d", (int)callingCountDown);
                ituTextSetString(callingCountDownText, buf);
            }
        }
        if (callingCountDown <= 0.0f)
        {
            if(callSpottedStatus == SPOTTED_INIT)
            {
                ituWidgetSetVisible(callingWaitingBackground, false);
                CallingWaitingConfirmButtonOnSimulatePressing();
            }
            else
            {
                LinphoneTerminate(0);
                SceneHangUp();
    		    ituLayerGoto(mainMenuLayer);
            }
        }
    }
    else if (callingErrorCountDown > 0.0f)
    {
        uint32_t diff, tick = SDL_GetTicks();

        if (tick >= callingErrorLastTick)
        {
            diff = tick - callingErrorLastTick;
        }
        else
        {
            diff = 0xFFFFFFFF - callingErrorLastTick + tick;
        }

        if (diff >= 1000)
        {
            char buf[4];

            callingErrorCountDown -= (float)diff / 1000.0f;
            callingErrorLastTick = tick;

            sprintf(buf, "%d", (int)callingErrorCountDown);
            ituTextSetString(callingErrorCountDownText, buf);
        }
        if (callingErrorCountDown <= 0.0f)
        {
			callingHangUp = true;
		    ituLayerGoto(mainMenuLayer);
        }
    }
    if (snapshotCountDown > 0)
    {
        snapshotCountDown--;
    }
	return false;
}

bool CallingOnEnter(ITUWidget* widget, char* param)
{
    if (!mainMenuLayer)
    {
        mainMenuLayer = ituSceneFindWidget(&theScene, "mainMenuLayer");
        assert(mainMenuLayer);

        if (!callingRemoteBackground)
        {
            
            callingRemoteBackground = ituSceneFindWidget(&theScene, "callingRemoteBackground");
            assert(callingRemoteBackground);
#if (CFG_CHIP_FAMILY == 9850) && (CFG_VIDEO_ENABLE)        
            ituWidgetSetDraw(callingRemoteBackground, callingRemoteBackgroundDraw);
#endif

            callingSourceText = ituSceneFindWidget(&theScene, "callingSourceText");
            assert(callingSourceText);
        }
        callingVolProgressBar = ituSceneFindWidget(&theScene, "callingVolProgressBar");
        assert(callingVolProgressBar);

        callingVolTrackBar = ituSceneFindWidget(&theScene, "callingVolTrackBar");
        assert(callingVolTrackBar);

        callingCountDownText = ituSceneFindWidget(&theScene, "callingCountDownText");
        assert(callingCountDownText);

        callingErrorBackground = ituSceneFindWidget(&theScene, "callingErrorBackground");
        assert(callingErrorBackground);

        callingErrorCountDownText = ituSceneFindWidget(&theScene, "callingErrorCountDownText");
        assert(callingErrorCountDownText);

        callingWaitingBackground = ituSceneFindWidget(&theScene, "callingWaitingBackground");
        assert(callingWaitingBackground);

        callingWaitingTypeText = ituSceneFindWidget(&theScene, "callingWaitingTypeText");
        assert(callingWaitingTypeText);

        callingWaitingRoomText = ituSceneFindWidget(&theScene, "callingWaitingRoomText");
        assert(callingWaitingRoomText);

        callingRecordButton = ituSceneFindWidget(&theScene, "callingRecordButton");
        assert(callingRecordButton);    

        callingStopRecordButton = ituSceneFindWidget(&theScene, "callingStopRecordButton");
        assert(callingStopRecordButton);

        callingAnswerButton = ituSceneFindWidget(&theScene, "callingAnswerButton");
        assert(callingAnswerButton);

        callingUnlockButton = ituSceneFindWidget(&theScene, "callingUnlockButton");
        assert(callingUnlockButton);

        callingButtonContainer = ituSceneFindWidget(&theScene, "callingButtonContainer");
        assert(callingButtonContainer);
    }

    // volume
    callingConnected = false;
    ituProgressBarSetValue(callingVolProgressBar, LinphoneGetRingLevel());
    ituTrackBarSetValue(callingVolTrackBar, LinphoneGetRingLevel());
    callingVoiceLevelOld = LinphoneGetVoiceLevel();
    callingRingLevelOld = LinphoneGetRingLevel();

    // count down
    ituTextSetString(callingCountDownText, NULL);
    callingCountDown = callingErrorCountDown = spotCountDown = 0.0f;

    // snapshot
    snapshotCountDown = 0;

    // video
    SceneEnterVideoState(33);
    if (callingInVideoState)
    {
        LinphoneSetWindowID(callingRemoteBackground);
    }

    // waiting
    ituWidgetSetVisible(callingWaitingBackground, false);
    ituWidgetSetVisible(callingAnswerButton, false);

    callingHangUp = false;

    return true;
}

bool CallingOnLeave(ITUWidget* widget, char* param)
{
    if (!callingHangUp)
    {
        LinphoneTerminate(0);
        SceneHangUp();
        callingHangUp = false;
    }

    CaptureStopRecord();

    if (callingVoiceLevelOld != LinphoneGetVoiceLevel() ||
        callingRingLevelOld != LinphoneGetRingLevel())
        ConfigSave();

    ituWidgetSetVisible(callingErrorBackground, false);

    callingInVideoState = false;
    callingID = callingPendingID = 0;
    SceneLeaveVideoState();
    
	return true;
}

void CallingReset(void)
{
    mainMenuLayer         = NULL;
    callingRemoteBackground    = NULL;
}
