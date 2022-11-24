/*******************************************************************************
 * Copyright (c) 2021 XXYYZZ - All rights reserved.
 *
 * This software is authored by XXYYZZ and is XXYYZZ' intellectual
 * property, including the copyrights in all countries in the world.
 * This software is provided under a license to use only with all other rights,
 * including ownership rights, being retained by XXYYZZ.
 *
 * This file may not be distributed, copied, or reproduced in any manner,
 * electronic or otherwise, without the written consent of XXYYZZ.
 ******************************************************************************/

/*******************************************************************************
 *
 * File Name    :  	SIPCall.c
 *
 * Description  :	 File containing function related to SIP functionality by
 *                   controlling and BARESIP app configuration and execution
 *
 * History:
 *  OCT/25/2021, UBV,	Created the file.
 *
******************************************************************************/

/****************
* Include Files
****************/
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <re.h>
#include <baresip.h>
#include "aites.h"


extern deviceInfo      gblDeviceConfig;				// Device Structure

int getSIPAppPID(void);
void killSIPApp(void);
void makeSIPCall(int dialNo);
void restartSIPApp(void);
int makeSIPConfigStr(char* resultStr, sipServerConfig* sipServerConfig);
int updateBareSIPConfig(sipServerConfig* sipPrimaryServerConfig,
                        sipServerConfig* sipSecondaryServerConfig);

/*******************************************************************************
 * Function:    getSIPAppPID
 *
 * Description: This function return the baresip application PID
 *
 * Parameters : None
 *
 * Return Value:
 *		int   -  0      - if baresip application is not running
 *               >0     - baresip application PID
 ******************************************************************************/
int getSIPAppPID(void)
{
        char pidline[MAX_STRING];
        char *pid;
        int pidno;
        FILE *fp = popen("pidof baresip","r");
        fgets(pidline,MAX_STRING,fp);
        //printf("%s",pidline);
        pid = strtok (pidline," ");
        pidno= atoi(pid);
        printf("%d\n",pidno);
        pclose(fp);
        return pidno;
}

/*******************************************************************************
 * Function:    killSIPApp
 *
 * Description: This function Kill baresip application instance
 *
 * Parameters : None
 *
 * Return Value: None
 *
 ******************************************************************************/
void killSIPApp(void)
{
    pid_t pid;
	char killCmd[MAX_STRING];
	memset(killCmd, 0x00, MAX_STRING);

	// Find baresip app pid
    pid = getSIPAppPID();
    if(pid != 0)
    {
        sprintf(killCmd, "kill -9 %d \r\n", pid);
        printf("killcmd = %s\n", killCmd);
        system(killCmd);
    }
    else
    {
        printf("Baresip App pid not found\n");
    }
}

/*******************************************************************************
 * Function:    makeSIPCall
 *
 * Description: This function make SIP call to primary server number
 *
 * Parameters :
 *		int		 dialNo      - SIP server dial number
 *
 * Return Value: None
 *
 ******************************************************************************/
void makeSIPCall(int dialNo)
{
	char sipCallCmd[MAX_STRING];
	memset(sipCallCmd, 0x00, MAX_STRING);

//	if(reg1State > 0)
	if(gblDeviceConfig.isPrimary == true)
	{
		sprintf(sipCallCmd, "sip:%d@%s:%s",
							dialNo,
							gblDeviceConfig.primaryServerConfig.sipServerIP,
							gblDeviceConfig.primaryServerConfig.sipPort);
	}
//	else if(reg2State > 0)
	else
	{
		sprintf(sipCallCmd, "sip:%d@%s:%s",
							dialNo,
							gblDeviceConfig.secondaryServerConfig.sipServerIP,
							gblDeviceConfig.secondaryServerConfig.sipPort);
	}

	if(aites_call_s.reg1State == 0 && aites_call_s.reg2State == 0)
	{
		printf("Both server are down so return\n");
		return;
	}
	
/*	else
	{
		printf("Both server are down so return\n");
		return;
	}
*/	


/*	else
	{
		printf("Both server are down so return\n");
		return;
	}
*/	
	printf("Calling to = %s\n", sipCallCmd);
	aites_call_s.uac = uag_find_requri(sipCallCmd);
	printf("Current aor = %s\n",account_aor(aites_call_s.uac));
    ua_connect(aites_call_s.uac, NULL, NULL, sipCallCmd, VIDMODE_OFF);	

/*
	// kill the baresip app instance if it exists
    killSIPApp();

	// start SIP app with dial command
    sprintf(sipCallCmd, "baresip -f /etc/baresip -e \"d %d\" &\r\n", dialNo);
    printf("sipCallCmd = %s\n", sipCallCmd);
    system(sipCallCmd);
*/	
}

/*******************************************************************************
 * Function:    restartSIPApp
 *
 * Description: This function make restart baresip application
 *
 * Parameters : None
 *
 * Return Value: None
 *
 ******************************************************************************/
void restartSIPApp(void)
{
	char sipCmd[MAX_STRING];
	memset(sipCmd, 0x00, MAX_STRING);

    // kill the baresip app instance if it exists
    killSIPApp();

    printf("Now start baresip app again\n");
	// start SIP app with dial command
    sprintf(sipCmd, "baresip -f /etc/baresip &\r\n");
    printf("sipCmd = %s\n", sipCmd);
    system(sipCmd);
    printf(" baresip app started\n");
}

/*******************************************************************************
 * Function:    makeSIPConfigStr
 *
 * Description: This function creat SIP configutation string for servers
 *
 * Parameters :
 *		char* 	 resultStr  - String pointer to return configuration string
 *		sipServerConfig* sipServerConfig - SIP server configuration structure ptr
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful config string created
 *              RETURN_ERROR- Error while config string creation
 ******************************************************************************/
int makeSIPConfigStr(char* resultStr, sipServerConfig* sipServerConf)
{
    char autoFlag[10];

	// check input variables
	if ((resultStr == NULL) || (sipServerConf->sipServerIP == NULL) ||
        (sipServerConf->sipServerUserID == NULL) ||
        (sipServerConf->sipServerPass == NULL))
	{
		printf("Invalid SIP configuration parameters \n");
		return RETURN_ERROR;
	}

    memset(autoFlag, 0x00, 10);
    if(sipServerConf->sipAutoAnsFlag)
    {
        memcpy(autoFlag, "auto", sizeof("auto"));
    }
    else
    {
        memcpy(autoFlag, "manual", sizeof("manual"));
    }

	if(strcmp(sipServerConf->sipPort,"-") != 0)
    {
		sprintf(resultStr, "<sip:%s@%s:%s>;auth_pass=%s;answermode=%s;regint=5",
                            sipServerConf->sipServerUserID,
                            sipServerConf->sipServerIP,
                            sipServerConf->sipPort,
                            sipServerConf->sipServerPass,
                            autoFlag);
    }
	else
    {
		sprintf(resultStr, "<sip:%s@%s>;auth_pass=%s;answermode=%s;regint=5",
                            sipServerConf->sipServerUserID,
                            sipServerConf->sipServerIP,
                            sipServerConf->sipServerPass,
                            autoFlag);
    }
	// make string from the input variable
	printf("Config String = %s\n", resultStr);

    return RETURN_OK;
}


/*******************************************************************************
 * Function:    updateBareSIPConfig
 *
 * Description: This function write SIP configutation string to baresip app
 *              configuration file
 *
 * Parameters :
 *		sipServerConfig* sipPrimaryServerConfig - Primary SIP server
 *                                                configuration structure ptr
 *		sipServerConfig* sipSecondaryServerConfig- Secondary SIP server
 *                                                configuration structure ptr
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful baresip config file updated
 *              RETURN_ERROR- Error while updating baresip config file
 ******************************************************************************/
int updateBareSIPConfig(sipServerConfig* sipPrimaryServerConfig,
                        sipServerConfig* sipSecondaryServerConfig)
{
	int errCode = RETURN_OK;
	FILE *fpConfig;
	char resultPrimaryConfig[MAX_STRING];
	char resultSecondaryConfig[MAX_STRING];

	memset(&resultPrimaryConfig, 0x00, MAX_STRING);
	memset(&resultSecondaryConfig, 0x00, MAX_STRING);

	if ((sipPrimaryServerConfig == NULL) || (sipSecondaryServerConfig == NULL))
	{
		printf("Invalid SIP configuration parameters \n");
		errCode = RETURN_ERROR;
		return errCode;
	}

	// Call makeSIPConfigStr() to get the primary server confuguration sring.
	if(makeSIPConfigStr(resultPrimaryConfig, sipPrimaryServerConfig) == RETURN_OK)
	{
		printf("Primary Server Config string = %s\n", resultPrimaryConfig);
	}
	else
	{
		errCode = RETURN_ERROR;
		return errCode;
	}

	// Call makeSIPConfigStr() to get the secondary confuguration sring.
	if(makeSIPConfigStr(resultSecondaryConfig, sipSecondaryServerConfig) == RETURN_OK)
	{
		printf("Secondary Server Config string = %s\n", resultSecondaryConfig);
	}
	else
	{
		errCode = RETURN_ERROR;
		return errCode;
	}


	// Delete old config file and create new
	fpConfig = fopen(BARESIP_ACCNTS_FILE, "wb");
	if(fpConfig == NULL)
	{
		printf("Unable to open Baresip accounts file %s\n", BARESIP_ACCNTS_FILE);
		errCode = RETURN_ERROR;
		return errCode;
	}

	//write configuration
   	fprintf(fpConfig, "%s\n", resultPrimaryConfig);
    fprintf(fpConfig, "%s\n", resultSecondaryConfig);

	fclose(fpConfig);
	return errCode;
}

/*******************************************************************************
 * Copyright (c) 2021 XXYYZZ - All rights reserved.
 *
 * This software is authored by XXYYZZ and is XXYYZZ' intellectual
 * property, including the copyrights in all countries in the world.
 * This software is provided under a license to use only with all other rights,
 * including ownership rights, being retained by XXYYZZ.
 *
 * This file may not be distributed, copied, or reproduced in any manner,
 * electronic or otherwise, without the written consent of XXYYZZ.
 ******************************************************************************/
