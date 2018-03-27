#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scene.h"
#include "doorbell.h"
#include "linphone/linphone_castor3.h"
#include "libxml/xpath.h"

// need to install VS90SP1-KB980263-x86.exe for vs2008
//#pragma execution_character_set("utf-8")
static ITUTextBox* callTextBox;
static ITUScrollListBox*  phoneScrollListBox;
static ITUButton* deleteButton;
static ITURadioBox* contactRadioBox;
static ITURadioBox* historyRadioBox;

static xmlDocPtr abDoc;
static xmlXPathContext* abXpathCtx;
static int callNumIndex;
static xmlNodePtr cur;
static int entryCount = 7;


bool DeleteButtonOnPressed(ITUWidget* widget, char* param){
	char* str = ituTextGetString((ITUText*)callTextBox);
	int count = str ? strlen(str):0;
	printf("count= %d",count);
	if (count > 0){
		callNumIndex = count;
		char buf[512];
		strcpy(buf,str);
		buf[callNumIndex-1] = '\0';
		printf("buf=%s",buf);
		ituTextBoxSetString(callTextBox,buf);
	}
	return true;	
}

bool PhoneCall1ButtonOnPress(ITUWidget* widget,char* param){
	printf("phoneCallButton is Pressed...");
	/*//解析xml文件
	abDoc = xmlParseFile("A:/rings/test.xml");
	//xmlpath上下文指针
	abXpathCtx = xmlXPathNewContext(abDoc);
	xmlXPathObject* xpathObj;
	char* ret = NULL;
	//上下文指针确定所需指向的位置
	xpathObj = xmlXPathEvalExpression(BAD_CAST"/name1",abXpathCtx);
	if (!xmlXPathNodeSetIsEmpty(xpathObj->nodesetval)){
		xmlNode* node = xpathObj->nodesetval->nodeTab[0];
		ret = xmlNodeListGetString(abDoc,node->children,1);
		printf("ret===%s\n",ret);
	}
	//遍历xml中node节点.
	xmlNodePtr cur = xmlDocGetRootElement(abDoc);// get root node
	cur = cur->xmlChildrenNode;
	cur = cur->next;*/
	/*while (cur != NULL){
		printf("Current Node: %s\n",cur->name);
		cur = cur->next;
		cur = cur->next;
	}*/
	//xmlNodeSetContent(cur, (const xmlChar*)"test");//修改节点后是内容
	/*while (cur != NULL){
		ret = xmlNodeListGetString(abDoc, cur->xmlChildrenNode, 1);
		xmlChar* value = xmlNodeGetContent(cur);
		printf("ret====%s\n", ret);
		printf("value====%s\n",value);
		
		xmlFree(value);
		cur = cur->next;
		cur = cur->next;
	}
	//xmlSaveFile("A:/rings/test.xml", abDoc);
	xmlSaveFormatFileEnc("A:/rings/test.xml", abDoc, "UTF-8", 1);
	xmlXPathFreeObject(xpathObj);
	xmlFree(ret);*/
	return true;
}

bool PhoneScrollListBoxOnLoad(ITUWidget* widget, char* param)
{	
	ITCTree* node;
	ITUListBox* listbox = (ITUListBox*)widget;
	ITUScrollListBox* slistbox = (ITUScrollListBox*)widget;
	int i, j, count;

	count = ituScrollListBoxGetItemCount(slistbox);
	node = ituScrollListBoxGetLastPageItem(slistbox);
	listbox->pageCount = entryCount ? (entryCount + count - 1) / count : 1;

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
			ITUScrollText* scrolltext = (ITUScrollText*)node;
			ituScrollTextSetString(scrolltext, "");

			node = node->sibling;
		}
	}

	i = 0;
	j = count * (listbox->pageIndex - 2);
	if (j < 0)
		j = 0;

	xmlChar* value = NULL;
	//xmlKeepBlanksDefault(0);
	abDoc = xmlParseFile("A:/rings/test.xml");
	cur = xmlDocGetRootElement(abDoc);// get root node
	cur = cur->xmlChildrenNode;
	cur = cur->next;
	for (; j < entryCount; j++)
	{	
		if (cur != NULL){
			ITUScrollText* scrolltext = (ITUScrollText*)node;
			value = xmlNodeGetContent(cur);
			ituTextSetString(scrolltext, value);
			//ituWidgetSetCustomData(scrolltext, value);
			//ituListBoxSetItemRead(listbox, (ITUWidget*)scrolltext,true);
			cur = cur->next;
			cur = cur->next;
			i++;
			node = node->sibling;
		}
		else{
			break;
		}
		if (node == NULL)
			break;
	}

	for (; node; node = node->sibling)
	{
		ITUScrollText* scrolltext = (ITUScrollText*)node;
		//while (cur != NULL){
		//ituScrollTextSetString(scrolltext, xmlNodeGetContent(cur));
		//cur = cur->next;
		//cur = cur->next;
		//}
		ituScrollTextSetString(scrolltext,"");
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

	return true;
}
bool Called1OnEnter(ITUWidget* widget, char* param)
{
    if (!callTextBox)
    {
        callTextBox = ituSceneFindWidget(&theScene, "callTextBox");
        assert(callTextBox);    

        phoneScrollListBox = ituSceneFindWidget(&theScene, "phoneScrollListBox");
        assert(phoneScrollListBox);    
   
		deleteButton = ituSceneFindWidget(&theScene, "deleteButton");
		assert(deleteButton);

		contactRadioBox = ituSceneFindWidget(&theScene, "contactRadioBox");
		assert(contactRadioBox);

		historyRadioBox = ituSceneFindWidget(&theScene, "historyRadioBox");
		assert(historyRadioBox);
    }
	return true;
}

bool Called1OnLeave(ITUWidget* widget, char* param)
{
	return true;
}

void Called1Reset(void)
{
    callTextBox = NULL;
	phoneScrollListBox = NULL;
}
