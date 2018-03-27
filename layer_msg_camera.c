#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL/SDL.h"
#include "scene.h"
#include "doorbell.h"
#include "castor3player.h"
#include "ite/itv.h"
#include "../driver/itp/itp_cfg.h"

#ifdef _WIN32
#undef CFG_VIDEO_ENABLE
#endif

#define SNAPSHOT_PATH  "E:/capture"
#define CAMERA_TIMEOUT      (30.999f)  // 30 secs
#pragma execution_character_set("utf-8")

static ITUScrollListBox* msgAndCameraScrollListBox;
static ITUBackground* cameraScreenBackground;
static ITUIcon* imageIcon;
static ITUText* cameraCountDownText;
static ITUButton* cameraRecordButton;
static ITUButton* cameraStopRecordButton;
static ITUButton* cameraSnapshotButton;
static ITUScrollListBox* msgAndCameraScrollListBox;
static ITUBackground* msgAndCameraListBackground;
static ITUBackground* msgPlayerCtrlBackground;
static ITURadioBox* messageRadioBox;
static ITURadioBox* camerawatchRadioBox;
static ITUContainer* cameraBottomContainer;
static ITUSprite* msgPlayerVolSprite;
static ITUProgressBar* msgPlayerVolProgressBar;
static ITUTrackBar* msgPlayerVolTrackBar;
static ITUButton* msgRecordButton;
static ITUBackground* topMenuBackground;

static int cameraInfoCount, cameraPageIndex, cameraShowingIndex,entryCount;
static DeviceInfo cameraInfoArray[MAX_CAMERA_COUNT];
static DeviceInfo cameraDeviceInfo;
static uint32_t cameraLastTick;
static bool cameraVideoRecordEnable, cameraIsSnapshoting;

static MTAL_SPEC mtal_spec = { 0 };
static int x, y, width, height = 0;
int LastMsgPlayerVoice = 0;
static bool videoPlayerIsFileEOF = false;
uint8_t* data;
int size;

float       cameraCountDown = 0;
bool        LockRecordButton = false;
float       LockRecordButtonCountDown = 0;
uint32_t    LockRecordButtonLastTick = 0;

float       IPCamDisplayCountDown = 0;
uint32_t    IPCamDisplayLastTick = 0;
bool        bWaitingIPCamDisplay = false;

extern bool isIPCAM_FirstVideoGetted;
extern bool captureDeletePic;

static void EventHandler(PLAYER_EVENT nEventID, void *arg)
{
	switch (nEventID)
	{
	case PLAYER_EVENT_EOF:
		printf("File EOF\n");
		videoPlayerIsFileEOF = true;
		break;
	case PLAYER_EVENT_OPEN_FILE_FAIL:
		printf("Open file fail\n");
		videoPlayerIsFileEOF = true;
		break;
	case PLAYER_EVENT_UNSUPPORT_FILE:
		printf("File not support\n");
		videoPlayerIsFileEOF = true;
		break;
	default:
		break;
	}
}

bool MsgAndCameraScrollListBoxOnLoad(ITUWidget* widget, char* param)
{
    ITCTree* node;
    ITUListBox* listbox = (ITUListBox*)widget;
    ITUScrollListBox* slistbox = (ITUScrollListBox*)widget;
    int i, j, count;
    DeviceInfo* info;
	CaptureEntry* entry;
    count = ituScrollListBoxGetItemCount(slistbox);
    node = ituScrollListBoxGetLastPageItem(slistbox);
	if (ituRadioBoxIsChecked(messageRadioBox)){
		if (SceneCameraIsViewing)
			LinphoneStopCamera();
		entryCount = CaptureGetCount();
		listbox->pageCount = entryCount ? (entryCount + count - 1) / count : 1;
	}
	else{
		CaptureStopPlay();
		listbox->pageCount = cameraInfoCount ? (cameraInfoCount + count - 1) / count : 1;
	}
    if (listbox->pageIndex == 0)
    {
        // initialize
        listbox->pageIndex = 1;
		listbox->focusIndex = -1;
    }

    if (listbox->pageIndex <= 1)
    {
        for (i = 0; i < count; i++)
        {
            ITUScrollText* scrolltext = (ITUScrollText*) node;
            ituScrollTextSetString(scrolltext, "");

            node = node->sibling;
        }
    }

    i = 0;
    j = count * (listbox->pageIndex - 2);
    if (j < 0)
        j = 0;
	if (ituRadioBoxIsChecked(messageRadioBox)){
		for (; j <entryCount; j++)
		{
			ITUScrollText* scrolltext = (ITUScrollText*)node;
			
			entry = CaptureGetEntry(j);
			ituWidgetSetCustomData(scrolltext, j);			
			ituTextSetString(scrolltext, entry->name);
			printf("entry====%s",entry);

			ituListBoxSetItemRead(listbox, (ITUWidget*)scrolltext, entry->played);
			i++;

			node = node->sibling;

			if (node == NULL)
				break;
		}
		ituWidgetSetVisible(msgRecordButton,true);
	}
	else{
		for (; j < cameraInfoCount; j++)
		{
			ITUScrollText* scrolltext = (ITUScrollText*)node;

			info = &cameraInfoArray[j];
			if (info->alias[0] == '\0')
			{
				char* desc = StringGetCameraPosition(info->area, info->building, info->unit);
				ituTextSetString(scrolltext, desc);
				free(desc);
			}
			else
			{
				ituTextSetString(scrolltext, info->alias);
			}
			ituWidgetSetCustomData(scrolltext, info);

			i++;

			node = node->sibling;

			if (node == NULL)
				break;
			}
		//ituWidgetSetVisible(msgRecordButton,false);
		}
    for (; node; node = node->sibling)
    {
        ITUScrollText* scrolltext = (ITUScrollText*) node;
        ituScrollTextSetString(scrolltext, "");
    }

    if (listbox->pageIndex == listbox->pageCount)
    {
        if (i == 0)
        {
            listbox->itemCount = i;
        }
        else
        {
            listbox->itemCount = i % count;
            if (listbox->itemCount == 0)
                listbox->itemCount = count;
        }
    }
    else
        listbox->itemCount = count;

	if (listbox->pageIndex == cameraPageIndex)
		ituListBoxSelect(listbox, cameraShowingIndex);
	ituWidgetSetVisible(msgPlayerCtrlBackground,false);
	ituWidgetSetVisible(cameraBottomContainer,false);
    return true;
}

static void CameraWatch(int arg)
{
    ITUWidget* item = ituListBoxGetFocusItem(&msgAndCameraScrollListBox->listbox);
    if (item)
    {
        char buf[4];
        DeviceInfo* info = (DeviceInfo*)ituWidgetGetCustomData(item);

        LinphoneWatchCamera(info->ip);
        SceneSetRemoteIP(info->ip);
        //LinphoneSetWindowID(cameraScreenBackground);

        cameraCountDown    = CAMERA_TIMEOUT;
        cameraLastTick     = SDL_GetTicks();

        sprintf(buf, "%d", (int)cameraCountDown);
        ituTextSetString(cameraCountDownText, buf);

        SceneCameraSetViewing(true);
    }
}

static void IPCamWatch(int arg)
{
    ITUWidget* item = ituListBoxGetFocusItem(&msgAndCameraScrollListBox->listbox);
    if (item)
    {
        char rtsp_url[64] = {0};
        char buf[4];
        DeviceInfo* info = (DeviceInfo*)ituWidgetGetCustomData(item);
        
        sprintf(rtsp_url,"%s:%s@%s", info->id, info->pw, info->ip);
        IPCamStartStreaming();
#ifdef CFG_RTSP_CLIENT_ENABLE
        SetRTSPClientMode(IPCAM_MODE);
        startRTSPClient(rtsp_url, 554, NULL, NULL);
#endif
        cameraCountDown    = CAMERA_TIMEOUT;
        cameraLastTick     = SDL_GetTicks();
        
        sprintf(buf, "%d", (int)cameraCountDown);
        ituTextSetString(cameraCountDownText, buf);

        SceneCameraSetViewing(true);
    }
}

bool MsgAndCameraScrollListOnSelection(ITUWidget* widget, char* param)
{
    ITUListBox* listbox = (ITUListBox*)widget;
    ITUScrollText* item =(ITUScrollText*) ituListBoxGetFocusItem(&msgAndCameraScrollListBox->listbox);
	if (item)
	{
		if (ituRadioBoxIsChecked(messageRadioBox)){	
		int count=ituWidgetGetCustomData(item);
		CaptureEntry* entry=CaptureGetEntry(count);
			if (entry->type == MEDIA_VIDEO){
				CaptureStopPlay();
				ituWidgetSetVisible(imageIcon,false);
				SceneEnterVideoState(33);
				//LinphoneSetWindowID(cameraScreenBackground);
				CaptureStartPlay(listbox->focusIndex);
			}
			else if (entry->type == MEDIA_IMAGE){
				CaptureStopPlay();
				ituWidgetSetVisible(imageIcon,true);
				char buf[PATH_MAX];
				FILE* f;
				//char* capturePath = CaptureGetPath();
				//if (!capturePath)
				//    return true;

				strcpy(buf, SNAPSHOT_PATH);
				strcat(buf, "/");
				strcat(buf, CaptureGetEntry(count)->filename);

				// try to load jpeg file if exists
				printf("filename =%s", buf);
				f = fopen(buf, "rb");
				if (f)
				{
					uint8_t* data;
					int size;
					struct stat sb;

					if (fstat(fileno(f), &sb) != -1)
					{
						size = sb.st_size;
						data = malloc(size);
						if (data)
						{
							size = fread(data, 1, size, f);
							ituIconLoadJpegData(imageIcon, data, size);
							free(data);
						}
					}
					fclose(f);
				}
				CaptureStartPlay(count);
			}
		}
		else{
			ituWidgetSetVisible(cameraBottomContainer, true);
			ituWidgetSetVisible(imageIcon,false);
			DeviceInfo* info = (DeviceInfo*)ituWidgetGetCustomData(item);
			if (info)
			{
				if (cameraDeviceInfo.type == DEVICE_IPCAM)
				{
					if (cameraVideoRecordEnable)
					{
						IPCamStopRecord();
						ituWidgetSetVisible(cameraRecordButton, true);
						ituWidgetSetVisible(cameraStopRecordButton, false);
					}
#ifdef CFG_RTSP_CLIENT_ENABLE            
					stopRTSPClient();
#endif
					IPCamStopStreaming();
				}

				memcpy(&cameraDeviceInfo, info, sizeof(DeviceInfo));

				if (info->type == DEVICE_IPCAM)
				{
					if (SceneCameraIsViewing())
					{
						if (cameraVideoRecordEnable)
						{
							CaptureStopRecord();
							ituWidgetSetVisible(cameraRecordButton, true);
							ituWidgetSetVisible(cameraStopRecordButton, false);
						}
						LinphoneStopCamera();
						ituSceneExecuteCommand(&theScene, 6, IPCamWatch, 0);
					}
					else
					{
						IPCamWatch(0);
					}
				}
				else
				{
					if (SceneCameraIsViewing())
					{
						if (cameraVideoRecordEnable)
						{
							CaptureStopRecord();
							ituWidgetSetVisible(cameraRecordButton, true);
							ituWidgetSetVisible(cameraStopRecordButton, false);
						}
						LinphoneStopCamera();
						ituSceneExecuteCommand(&theScene, 6, CameraWatch, 0);
					}
					else
					{
						CameraWatch(0);
					}
				}
				cameraPageIndex = msgAndCameraScrollListBox->listbox.pageIndex;
				cameraShowingIndex = msgAndCameraScrollListBox->listbox.focusIndex;
			}
		}
	}
    return true;
}

bool CameraScreenButtonOnPress(ITUWidget* widget, char* param)
{
	if (ituRadioBoxIsChecked(messageRadioBox)){
		if (ituWidgetIsVisible(msgAndCameraListBackground)){
			ituWidgetSetVisible(msgAndCameraListBackground, false);
			ituWidgetSetVisible(cameraBottomContainer, false);
			ituWidgetSetVisible(msgPlayerCtrlBackground,false);
			ituWidgetSetVisible(topMenuBackground,false);
		}
		else{
			if (CaptureGetCount() == 0){
				ituWidgetSetVisible(msgPlayerCtrlBackground, true);
				ituWidgetSetVisible(topMenuBackground, true);
			}
			else{
				ituWidgetSetVisible(msgPlayerCtrlBackground, true);
				ituWidgetSetVisible(topMenuBackground, true);
				ituWidgetSetVisible(msgAndCameraListBackground, true);
			}
		}
	}
	else{
		if (ituWidgetIsVisible(msgAndCameraListBackground)){
			ituWidgetSetVisible(msgAndCameraListBackground, false);
			ituWidgetSetVisible(cameraBottomContainer, false);
			ituWidgetSetVisible(msgPlayerCtrlBackground, false);
			ituWidgetSetVisible(topMenuBackground, false);
		}
		else{
			ituWidgetSetVisible(msgAndCameraListBackground, true);
			ituWidgetSetVisible(cameraBottomContainer, true);
			ituWidgetSetVisible(topMenuBackground, true);
		}
	}
    return true;
}

bool MsgRecordButtonOnPress(ITUWidget* widget, char* param){
	if (cameraVideoRecordEnable)
	{
		if (LockRecordButton) return false;
		if (SceneCameraIsViewing() && cameraCountDown < 2.0f) return false;

		if (cameraDeviceInfo.type == DEVICE_IPCAM)
		{
			if (VideoMemoStartRecord(MEDIA_VIDEO, cameraDeviceInfo.ip))
				return false;
		}
		else if (VideoMemoStartRecord(MEDIA_VIDEO, cameraDeviceInfo.ip))
			return false;

		LockRecordButton = true;
		LockRecordButtonCountDown = 1.0f;
		LockRecordButtonLastTick = SDL_GetTicks();
	}
	return true;
}

static void CameraEnableSnapshot(int arg)
{
    cameraIsSnapshoting = false;
}

bool CameraRecordButtonOnPress(ITUWidget* widget, char* param)
{
    if (cameraVideoRecordEnable)
    {
        if(LockRecordButton) return false;
        if(SceneCameraIsViewing() && cameraCountDown < 2.0f) return false;
		
        if (cameraDeviceInfo.type == DEVICE_IPCAM)
        {
            if (IPCamStartRecord(MEDIA_VIDEO, cameraDeviceInfo.ip))
                return false;    
        }
        else if (CaptureStartRecord(MEDIA_VIDEO, cameraDeviceInfo.ip))
            return false;
        
        LockRecordButton = true;
        LockRecordButtonCountDown = 1.0f;
        LockRecordButtonLastTick = SDL_GetTicks();
    }
	return true;
}

bool CameraStopRecordButtonOnPress(ITUWidget* widget, char* param)
{
    if (cameraVideoRecordEnable)
    {
        if(LockRecordButton) return false;
        if (cameraDeviceInfo.type == DEVICE_IPCAM)
        {
            IPCamStopRecord();
        }
        else
        {
            CaptureStopRecord();
        }
    
        LockRecordButton = true;
        LockRecordButtonCountDown = 1.0f;
        LockRecordButtonLastTick = SDL_GetTicks();
    }
	return true;
}

bool CameraUnlockButtonOnPress(ITUWidget* widget, char* param)
{
    SceneOpenDoor(cameraDeviceInfo.ip);
    return true;
}

bool CameraSnapshotButtonOnPress(ITUWidget* widget, char* param)
{
    if (cameraIsSnapshoting)
        return false;
    
    if (cameraDeviceInfo.type == DEVICE_IPCAM)
        IPCamSnapshot(cameraDeviceInfo.ip, 0);
    else    
        CaptureSnapshot(cameraDeviceInfo.ip, 0);
    cameraIsSnapshoting = true;
    ituSceneExecuteCommand(&theScene, 1000/MS_PER_FRAME, CameraEnableSnapshot, 0);
	return true;
}

/*bool CameraDisconnectButtonOnPress(ITUWidget* widget, char* param)
{
    if(cameraDeviceInfo.type == DEVICE_IPCAM)
    {
        if (cameraVideoRecordEnable)
        {
            IPCamStopRecord();
            ituWidgetSetVisible(cameraRecordButton, true);
            ituWidgetSetVisible(cameraStopRecordButton, false);
        }
#ifdef CFG_RTSP_CLIENT_ENABLE            
        stopRTSPClient();
#endif
        IPCamStopStreaming();
        SceneCameraSetViewing(false);
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_CAMERA_TIMEOUT, NULL);
        cameraCountDown = 0.0f;
		cameraPageIndex = cameraShowingIndex = -1;
    }
    else if (SceneCameraIsViewing())
    {
        if (cameraVideoRecordEnable)            
        {
            CaptureStopRecord();
            ituWidgetSetVisible(cameraRecordButton, true);
            ituWidgetSetVisible(cameraStopRecordButton, false);
        }
        LinphoneStopCamera();
        SceneCameraSetViewing(false);
        // UI receive this event to invisible cameraViewLayer
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_CAMERA_TIMEOUT, NULL);
        cameraCountDown = 0.0f;
		cameraPageIndex = cameraShowingIndex = -1;
    }
    return true;
}*/

bool CameraOnTimer(ITUWidget* widget, char* param)
{
    if (IPCamDisplayCountDown > 0.0f)
	{
        uint32_t tick = SDL_GetTicks();
        uint32_t diff = tick - IPCamDisplayLastTick;
        if (diff >= 1000)
        {
            IPCamDisplayCountDown -= (float)diff / 1000.0f;
            IPCamDisplayLastTick = tick;
        }
        if (IPCamDisplayCountDown <= 0.0f)
        {
            IPCamDisplayCountDown = 0;
        }
	}

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
    if (cameraCountDown > 0.0f)
    {
        uint32_t diff, tick = SDL_GetTicks();

        if (tick >= cameraLastTick)
        {
            diff = tick - cameraLastTick;
        }
        else
        {
            diff = 0xFFFFFFFF - cameraLastTick + tick;
        }

        //if (diff >= 1000)
        {
            char buf[4];

            cameraCountDown -= (float)diff / 1000.0f;
            cameraLastTick = tick;

            if (cameraCountDown >= 0.0f)
            {
                sprintf(buf, "%d", (int)cameraCountDown);
                ituTextSetString(cameraCountDownText, buf);
            }
        }
        if (cameraCountDown <= 0.0f)
        {
            if(cameraDeviceInfo.type == DEVICE_IPCAM)
            {
                if (cameraVideoRecordEnable)
                {
                    IPCamStopRecord();
                    ituWidgetSetVisible(cameraRecordButton, true);
                    ituWidgetSetVisible(cameraStopRecordButton, false);
                }
#ifdef CFG_RTSP_CLIENT_ENABLE            
                stopRTSPClient();
#endif
                IPCamStopStreaming();
                SceneCameraSetViewing(false);
                ituSceneSendEvent(&theScene, EVENT_CUSTOM_CAMERA_TIMEOUT, NULL);
				cameraPageIndex = cameraShowingIndex = -1;
            }
            else if (SceneCameraIsViewing())
            {
                if (cameraVideoRecordEnable)            
                {
                    CaptureStopRecord();
                    ituWidgetSetVisible(cameraRecordButton, true);
                    ituWidgetSetVisible(cameraStopRecordButton, false);
                }
                LinphoneStopCamera();
                SceneCameraSetViewing(false);
                // UI receive this event to invisible cameraViewLayer
                ituSceneSendEvent(&theScene, EVENT_CUSTOM_CAMERA_TIMEOUT, NULL);
				cameraPageIndex = cameraShowingIndex = -1;
            }
        }
    }

	if(captureDeletePic)
	{
		ituWidgetSetVisible(cameraSnapshotButton, false);
	}
	else
	{
		ituWidgetSetVisible(cameraSnapshotButton, true);
	}
	
    return false;
}

#if (CFG_CHIP_FAMILY == 9850) && (CFG_VIDEO_ENABLE)
static void cameraScreenBackgroundDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    ITURectangle* rect = (ITURectangle*) &widget->rect;

    destx = rect->x + x;
    desty = rect->y + y;
    
    if(cameraDeviceInfo.type == DEVICE_IPCAM && !isIPCAM_FirstVideoGetted)
    {
        ITUColor color;
    
        color.alpha = 0;
        color.red = 0;
        color.green = 0;
        color.blue = 0;
        ituColorFill(dest, destx, desty, rect->width, rect->height, &color);
    }
    else    
        ituDrawVideoSurface(dest, destx, desty, rect->width, rect->height);
    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}

static void cameraViewScreenBackgroundDraw(ITUWidget* widget, ITUSurface* dest, int x, int y, uint8_t alpha)
{
    int destx, desty;
    ITURectangle* rect = (ITURectangle*) &widget->rect;

    destx = rect->x + x;
    desty = rect->y + y;
    
    if(cameraDeviceInfo.type == DEVICE_IPCAM && !isIPCAM_FirstVideoGetted)
    {
        ITUColor color;
    
        color.alpha = 0;
        color.red = 0;
        color.green = 0;
        color.blue = 0;
        ituColorFill(dest, destx, desty, rect->width, rect->height, &color);

        bWaitingIPCamDisplay = true;
    }
    else if(cameraDeviceInfo.type == DEVICE_IPCAM && (bWaitingIPCamDisplay || IPCamDisplayCountDown))
    {
        ITUColor color;
    
        color.alpha = 0;
        color.red = 0;
        color.green = 0;
        color.blue = 0;
        ituDrawVideoSurface(dest, destx, desty, rect->width, rect->height);
        ituColorFill(dest, destx, desty, rect->width, rect->height, &color);
        
        if(bWaitingIPCamDisplay)
        {
            bWaitingIPCamDisplay = false;
            IPCamDisplayCountDown = 0.5f;
            IPCamDisplayLastTick = SDL_GetTicks();
        }
    }
    else
    {
        ituDrawVideoSurface(dest, destx, desty, rect->width, rect->height);
    }
    ituWidgetDrawImpl(widget, dest, x, y, alpha);
}
#endif

bool MsgAndCameraOnEnter(ITUWidget* widget, char* param)
{
    if (!cameraScreenBackground)
    {
        cameraScreenBackground = ituSceneFindWidget(&theScene, "cameraScreenBackground");
        assert(cameraScreenBackground);
#if (CFG_CHIP_FAMILY == 9850) && (CFG_VIDEO_ENABLE)        
        ituWidgetSetDraw(cameraScreenBackground, cameraScreenBackgroundDraw);
#endif 

		imageIcon = ituSceneFindWidget(&theScene, "imageIcon");
		assert(imageIcon);

        msgAndCameraScrollListBox = ituSceneFindWidget(&theScene, "msgAndCameraScrollListBox");
        assert(msgAndCameraScrollListBox);

        cameraCountDownText = ituSceneFindWidget(&theScene, "cameraCountDownText");
        assert(cameraCountDownText);          

        cameraRecordButton = ituSceneFindWidget(&theScene, "cameraRecordButton");
        assert(cameraRecordButton);

        cameraStopRecordButton = ituSceneFindWidget(&theScene, "cameraStopRecordButton");
        assert(cameraStopRecordButton);

		cameraSnapshotButton = ituSceneFindWidget(&theScene, "cameraSnapshotButton"); 
		assert(cameraSnapshotButton);

		msgAndCameraListBackground = ituSceneFindWidget(&theScene, "msgAndCameraListBackground");
		assert(msgAndCameraListBackground);

		msgPlayerCtrlBackground = ituSceneFindWidget(&theScene, "msgPlayerCtrlBackground");
		assert(msgPlayerCtrlBackground);

		messageRadioBox = ituSceneFindWidget(&theScene, "messageRadioBox");
		assert(messageRadioBox);

		camerawatchRadioBox = ituSceneFindWidget(&theScene, "camerawatchRadioBox");
		assert(camerawatchRadioBox);

		cameraBottomContainer = ituSceneFindWidget(&theScene, "cameraBottomContainer");
		assert(cameraBottomContainer);

		msgPlayerVolSprite = ituSceneFindWidget(&theScene, "msgPlayerVolSprite");
		assert(msgPlayerVolSprite);

		msgPlayerVolProgressBar= ituSceneFindWidget(&theScene, "msgPlayerVolProgressBar");
		assert(msgPlayerVolProgressBar);

		msgPlayerVolTrackBar = ituSceneFindWidget(&theScene, "msgPlayerVolTrackBar");
		assert(msgPlayerVolTrackBar);

		msgRecordButton = ituSceneFindWidget(&theScene, "msgRecordButton");
		assert(msgRecordButton);

		topMenuBackground = ituSceneFindWidget(&theScene, "topMenuBackground");
		assert(topMenuBackground);
    }
	SceneEnterVideoState(33);
	printf("theDeviceInfo======%d",theDeviceInfo.type);
    cameraIsSnapshoting = false;
	cameraVideoRecordEnable = true;
	CaptureInit();

	int vol = AudioGetVolume();
	ituSpriteGoto(msgPlayerVolSprite, vol > 0 ? 1 : 0);
	ituProgressBarSetValue(msgPlayerVolProgressBar, vol);
	ituTrackBarSetValue(msgPlayerVolTrackBar, vol);
	LastMsgPlayerVoice = vol;
	ituWidgetGetGlobalPosition(cameraScreenBackground, &x, &y);
	width = ituWidgetGetWidth(cameraScreenBackground);
	height = ituWidgetGetHeight(cameraScreenBackground);
#ifdef CFG_VIDEO_ENABLE        
	itv_set_video_window(x, y, width, height);
	mtal_pb_init(EventHandler);
#endif        
        // count down
        ituTextSetString(cameraCountDownText, NULL);
        cameraCountDown = 0.0f;
        LinphoneSetWindowID(cameraScreenBackground);
		if (cameraInfoCount == 0)
		cameraInfoCount = AddressBookGetCameraInfoArray(cameraInfoArray, MAX_CAMERA_COUNT, &theDeviceInfo);
		ituRadioBoxSetChecked(messageRadioBox,true);
		ituWidgetSetVisible(msgPlayerCtrlBackground,false);
		ituWidgetSetVisible(cameraBottomContainer,false);
        return true;   
}

bool MsgAndCameraOnLeave(ITUWidget* widget, char* param)
{
    if (strcmp(param, "msgAndCameraLayer"))
    {	
		CaptureStopPlay();
        if(cameraDeviceInfo.type == DEVICE_IPCAM)
        {
            IPCamStopRecord();
#ifdef CFG_RTSP_CLIENT_ENABLE        
            stopRTSPClient();
#endif
            IPCamStopStreaming();
            SceneCameraSetViewing(false);
        }
        else
        {
            if (cameraVideoRecordEnable)        
                CaptureStopRecord();

            LinphoneStopCamera();
            SceneCameraSetViewing(false);
        }

        if(strcmp(param, "calledLayer") && strcmp(param, "videoRecordLayer"))
            SceneLeaveVideoState();
        SceneContinuePlayGuardAlarm();
    }
    return true;
}

void CameraReset(void)
{
    cameraScreenBackground = NULL;
}