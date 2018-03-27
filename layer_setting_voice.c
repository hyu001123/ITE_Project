#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "doorbell.h"
#include "linphone/linphone_castor3.h"

#define KEY1 "key1.wav"
#define KEY2 "key2.wav"
#define KEY3 "key3.wav"
#define RING1 "ring1.wav"
#define RING2 "ring2.wav"
#define RING3 "ring3.wav"

static ITUBackground* settingVoiceBackground;
static ITUTrackBar* settingRingSoundTrackBar;
static ITUProgressBar* settingRingSoundProgressBar;
static ITUTrackBar* settingKeySoundTrackBar;
static ITUProgressBar* settingKeySoundProgressBar;
static ITUText* ringSoundText;
static ITUText* ringVolumeText;
static ITUText* keySoundText;
static ITUText* keyVolumeText;
static ITUButton* ringSoundButton;
static ITUButton* keySoundButton;
static ITUWheel* ringSoundWheel;
static ITUWheel* keySoundWheel;

// status
static int settingSoundKeyVolumeOld;
static int settingSoundRingVolumeOld;
static char settingSoundKeyOld[PATH_MAX];
static char settingSoundRingOld[PATH_MAX];

#pragma execution_character_set("utf-8")

bool SettingRingVolumeTrackBarOnChanged(ITUWidget* widget, char* param){
	int percent = atoi(param);
	if (percent >= 100)
	{
		percent = 100;
	}
	printf("percent==%d",percent);
	LinphoneSettingMode();//smooth the setting bar
	/*char prompt[256];
	strcpy(prompt, "castor3-set-ring-level ");
	strcat(prompt, "active");
	mq_send(linphoneCastor3.mq, prompt, PROMPT_MAX_LEN, 0);*/
	LinphoneSetRingLevel(percent, false);
	return true;
}

bool SettingKeyVolumeTrackBarOnChanged(ITUWidget* widget, char* param){
	int percent = atoi(param);
	if (percent >= 100)
	{
		percent = 100;
	}

	theConfig.keylevel = percent;
	LinphoneSetKeyLevel(theConfig.keylevel);
	return true;
}

bool SettingRingSoundWheelOnChanged(ITUWidget* widget,char* param){
	ITUText* text = (ITUText*)ituWheelGetFocusItem((ITUWheel*)widget);
	printf("param=%d", (int)param);
	if (strcmp(param, "0") == 0)
	{
		strcpy(theConfig.ringsound,RING1);
		LinphoneSetRingSound(RING1);
	}else if (strcmp(param, "1") == 0)
	{
		strcpy(theConfig.ringsound, RING2);
		LinphoneSetRingSound(RING2);
	}else if (strcmp(param, "2") == 0){
		strcpy(theConfig.ringsound, RING3);
		LinphoneSetRingSound(RING3);
	}
	ituButtonSetString(ringSoundButton, ituTextGetString(text));
	if (!ituWidgetIsVisible(ringSoundWheel)){
		printf(ituTextGetString(text));
		ituWidgetSetVisible(ringSoundButton, true);
		ituWidgetEnable(ringSoundButton);
	}
	return true;
}

bool SettingKeySoundWheelOnChanged(ITUWidget* widget, char* param){
	ITUText* text = (ITUText*)ituWheelGetFocusItem((ITUWheel*)widget);
	printf("param=%d", (int)param);
	if (strcmp(param, "0") == 0)
	{
		strcpy(theConfig.keysound, KEY1);
	}
	else if (strcmp(param, "1") == 0)
	{
		strcpy(theConfig.keysound, KEY2);
	}
	else if (strcmp(param, "2") == 0){
		strcpy(theConfig.keysound, KEY3);
	}
	ituButtonSetString(keySoundButton, ituTextGetString(text));
	if (!ituWidgetIsVisible(keySoundWheel)){
		printf(ituTextGetString(text));
		ituWidgetSetVisible(keySoundButton, true);
		ituWidgetEnable(keySoundButton);
	}
	return true;
}

void SettingRingAndKeySound(int language, char* RingSound, char* KeySound){
	switch (language)
	{	//Eng
	case 0:	
		if (strcmp(RingSound, RING1) == 0){
			ituButtonSetString(ringSoundButton, "Ring1");
			ituWheelGoto(ringSoundWheel, 0);
		}else if (strcmp(RingSound, RING2) == 0){
			ituButtonSetString(ringSoundButton, "Ring2");
			ituWheelGoto(ringSoundWheel, 1);
		}else if (strcmp(RingSound, RING3) == 0){
			ituButtonSetString(ringSoundButton, "Ring3");
			ituWheelGoto(ringSoundWheel, 2);
		}

		if (strcmp(KeySound, KEY1) == 0){
			ituButtonSetString(keySoundButton, "Sound1");
			ituWheelGoto(keySoundWheel, 0);
		}
		else if (strcmp(KeySound, KEY2) == 0){
			ituButtonSetString(keySoundButton, "Sound2");
			ituWheelGoto(keySoundWheel, 1);
		}
		else if (strcmp(KeySound, KEY3) == 0){
			ituButtonSetString(keySoundButton, "Sound3");
			ituWheelGoto(keySoundWheel, 2);
		}
		break;
		//Langt
	case 1:
		if (strcmp(RingSound, RING1) == 0){
			ituButtonSetString(ringSoundButton, "鈴聲1");
			ituWheelGoto(ringSoundWheel, 0);
		}
		else if (strcmp(RingSound, RING2) == 0){
			ituButtonSetString(ringSoundButton, "鈴聲2");
			ituWheelGoto(ringSoundWheel, 1);
		}
		else if (strcmp(RingSound, RING3) == 0){
			ituButtonSetString(ringSoundButton, "鈴聲3");
			ituWheelGoto(ringSoundWheel, 2);
		}

		if (strcmp(KeySound, KEY1) == 0){
			ituButtonSetString(keySoundButton, "声音1");
			ituWheelGoto(keySoundWheel, 0);
		}
		else if (strcmp(KeySound, KEY2) == 0){
			ituButtonSetString(keySoundButton, "声音2");
			ituWheelGoto(keySoundWheel, 1);
		}
		else if (strcmp(KeySound, KEY3) == 0){
			ituButtonSetString(keySoundButton, "声音3");
			ituWheelGoto(keySoundWheel, 2);
		}
		break;
		//Langs
	case 2:
		if (strcmp(RingSound, RING1) == 0){
			ituButtonSetString(ringSoundButton, "铃声1");
			ituWheelGoto(ringSoundWheel, 0);
		}
		else if (strcmp(RingSound, RING2) == 0){
			ituButtonSetString(ringSoundButton, "铃声2");
			ituWheelGoto(ringSoundWheel, 1);
		}
		else if (strcmp(RingSound, RING3) == 0){
			ituButtonSetString(ringSoundButton, "铃声3");
			ituWheelGoto(ringSoundWheel, 2);
		}

		if (strcmp(KeySound, KEY1) == 0){
			ituButtonSetString(keySoundButton, "声音1");
			ituWheelGoto(keySoundWheel, 0);
		}
		else if (strcmp(KeySound, KEY2) == 0){
			ituButtonSetString(keySoundButton, "声音2");
			ituWheelGoto(keySoundWheel, 1);
		}
		else if (strcmp(KeySound, KEY3) == 0){
			ituButtonSetString(keySoundButton, "声音3");
			ituWheelGoto(keySoundWheel, 2);
		}
		break;
	default:
		break;
	}
}

bool SettingVoiceOnEnter(ITUWidget* widget, char* param)
{
    if (!settingVoiceBackground)
    {
		settingVoiceBackground = ituSceneFindWidget(&theScene, "SettingVoiceBackground");
		assert(settingVoiceBackground);

        settingRingSoundTrackBar = ituSceneFindWidget(&theScene, "settingRingSoundTrackBar");
        assert(settingRingSoundTrackBar);

        settingRingSoundProgressBar = ituSceneFindWidget(&theScene, "settingRingSoundProgressBar");
		assert(settingRingSoundProgressBar);

        settingKeySoundTrackBar = ituSceneFindWidget(&theScene, "settingKeySoundTrackBar");
		assert(settingKeySoundTrackBar);

        settingKeySoundProgressBar = ituSceneFindWidget(&theScene, "settingKeySoundProgressBar");
		assert(settingKeySoundProgressBar);

		ringSoundText = ituSceneFindWidget(&theScene, "RingSound_Text");
		assert(ringSoundText);

		ringVolumeText = ituSceneFindWidget(&theScene, "RingVolume_Text");
		assert(ringVolumeText);

		keySoundText = ituSceneFindWidget(&theScene, "KeySound_Text");
		assert(keySoundText);

		keyVolumeText = ituSceneFindWidget(&theScene, "KeyVolume_Text");
		assert(keyVolumeText);

		ringSoundButton = ituSceneFindWidget(&theScene, "RingSoundButton");
        assert(ringSoundButton);

        keySoundButton = ituSceneFindWidget(&theScene, "KeySoundButton");
        assert(keySoundButton);

		ringSoundWheel= ituSceneFindWidget(&theScene,"RingSoundWheel");
		assert(ringSoundWheel);

		keySoundWheel= ituSceneFindWidget(&theScene,"KeySoundWheel");
		assert(keySoundWheel);

    }

    // current settings
	settingSoundRingVolumeOld = LinphoneGetRingLevel;
	printf("LinphoneRingLevel=%d",LinphoneGetRingLevel);
	if (LinphoneGetRingLevel > 100){
		settingSoundRingVolumeOld = 100;
	}
	ituTrackBarSetValue(settingRingSoundTrackBar, settingSoundRingVolumeOld);
	ituProgressBarSetValue(settingRingSoundProgressBar, settingSoundRingVolumeOld);

	settingSoundKeyVolumeOld = theConfig.keylevel;
	printf("KeyLevel=%d", settingSoundKeyVolumeOld);
	if (theConfig.keylevel > 100){
		theConfig.keylevel = 100;
	}
	ituTrackBarSetValue(settingKeySoundTrackBar, theConfig.keylevel);
	ituProgressBarSetValue(settingKeySoundProgressBar, theConfig.keylevel);

	SettingRingAndKeySound(theConfig.lang,theConfig.ringsound,theConfig.keysound);


    return true;
}

bool SettingVoiceOnLeave(ITUWidget* widget, char* param)
{
    if (settingSoundRingVolumeOld != LinphoneGetRingLevel ||
        settingSoundKeyVolumeOld != theConfig.keylevel)
    {
		//LinphoneSetRingLevel(LinphoneGetRingLevel(), false);
		ConfigSave();
    }
    return true;
}

void SettingVoiceReset(void)
{
    settingVoiceBackground = NULL;
}
