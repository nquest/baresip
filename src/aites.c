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
 * File Name    :  	aites.c
 *
 * Description  :	 	Main file for AITES SIP Application
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
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <re.h>
#include <baresip.h>
#include "aites.h"
#define MAGIC 0xca11ca11
#include "magic.h"
/** SIP Call Control object */
struct call {
	MAGIC_DECL                /**< Magic number for debugging           */
	struct le le;             /**< Linked list element                  */
	const struct config *cfg; /**< Global configuration                 */
	struct ua *ua;            /**< SIP User-agent                       */
	struct account *acc;      /**< Account (ref.)                       */
	struct sipsess *sess;     /**< SIP Session                          */
	struct sdp_session *sdp;  /**< SDP Session                          */
	struct sipsub *sub;       /**< Call transfer REFER subscription     */
	struct sipnot *not;       /**< REFER/NOTIFY client                  */
	struct call *xcall;       /**< Cross ref Transfer call              */
	struct list streaml;      /**< List of mediastreams (struct stream) */
	struct audio *audio;      /**< Audio stream                         */
	struct video *video;      /**< Video stream                         */
	enum call_state state;    /**< Call state                           */
	int32_t adelay;           /**< Auto answer delay in ms              */
	char *aluri;              /**< Alert-Info URI                       */
	char *local_uri;          /**< Local SIP uri                        */
	char *local_name;         /**< Local display name                   */
	char *peer_uri;           /**< Peer SIP Address                     */
	char *peer_name;          /**< Peer display name                    */
	char *diverter_uri;       /**< Diverter SIP Address                 */
	char *id;                 /**< Cached session call-id               */
	char *replaces;           /**< Replaces parameter                   */
	uint16_t supported;       /**< Supported header tags                */
	struct tmr tmr_inv;       /**< Timer for incoming calls             */
	struct tmr tmr_dtmf;      /**< Timer for incoming DTMF events       */
	struct tmr tmr_answ;      /**< Timer for delayed answer             */
	struct tmr tmr_reinv;     /**< Timer for outgoing re-INVITES        */
	time_t time_start;        /**< Time when call started               */
	time_t time_conn;         /**< Time when call initiated             */
	time_t time_stop;         /**< Time when call stopped               */
	bool outgoing;            /**< True if outgoing, false if incoming  */
	bool answered;            /**< True if call has been answered       */
	bool got_offer;           /**< Got SDP Offer from Peer              */
	bool on_hold;             /**< True if call is on hold (local)      */
	bool early_confirmed;     /**< Early media confirmed by PRACK       */
	struct mnat_sess *mnats;  /**< Media NAT session                    */
	bool mnat_wait;           /**< Waiting for MNAT to establish        */
	struct menc_sess *mencs;  /**< Media encryption session state       */
	int af;                   /**< Preferred Address Family             */
	uint16_t scode;           /**< Termination status code              */
	call_event_h *eh;         /**< Event handler                        */
	call_dtmf_h *dtmfh;       /**< DTMF handler                         */
	void *arg;                /**< Handler argument                     */

	struct config_avt config_avt;    /**< AVT config                    */
	struct config_call config_call;  /**< Call config                   */

	uint32_t rtp_timeout_ms;  /**< RTP Timeout in [ms]                  */
	uint32_t linenum;         /**< Line number from 1 to N              */
	struct list custom_hdrs;  /**< List of custom headers if any        */

	enum sdp_dir estadir;      /**< Established audio direction         */
	enum sdp_dir estvdir;      /**< Established video direction         */
	bool use_video;
	bool use_rtp;
	char *user_data;           /**< User data related to the call       */
	bool evstop;               /**< UA events stopped flag              */
};



/*********************
 * Macros and contants
 *********************/
#define CONFIGFILE_NAME	   "/usr/bin/AppConfig.ini"  // Config file name
#define ECBSTATUSFILE_NAME "/www/ecbstatus"
//#define CONFIGFILE_NAME	   "AppConfig.ini"  // Config file name
#define CONFIGFILE_LINE	   41               // Config file lines
#define GPIOCHECK_TIME	   500                // GPIO check timer interval
#define SIPCALL_CHECKTIME  1000                // Timer to check SIP call status
#define NETWORK_CHECKTIME  5000                // Timer to check Network status

/****************
 * Globals
 ****************/
deviceInfo      gblDeviceConfig;				// Device Structure
deviceData      gblDeviceData;					// Device Data Structure
pthread_t       serverThreadHandle;				// Server MSG thread handle
pthread_t       brdCastThreadHandle;		    // Server MSG thread handle
pthread_t       audPlayThreadHandle;	       	// Error audio play thread handle
pthread_t       localDataSendThreadHandle;		// local data send thread handle

pthread_t       timerThreadHandle;				// Timer thread handle thread handle
pthread_t       gpioCheckTimerThreadHandle;		// Timer handle for GPIO read thread handle
pthread_t       sipCallCheckTimerThreadHandle;	// Timer handle for check SIP Call status thread handle
pthread_t       networkCheckTimerThreadHandle;	// Timer handle for network status thread handle

int             dataSendTimer;		// Timer thread handle
int             gpioCheckTimer;		// Timer handle for GPIO read
int             sipCallCheckTimer;	// Timer handle for check SIP Call status
int             networkCheckTimer;	// Timer handle for network status

sem_t           timerThreadMutex;           // Timer thread Mutex
sem_t           gpioCheckTimerMutex;        // gpio read Timer thread Mutex
sem_t           sipCallCheckTimerMutex;     // sip call status timer mutex
sem_t           networkCheckTimerMutex;     // network status timer mutex

int             endFlag;            // App End flag to close all thread

struct aites_call *aites_call_s;
/****************
 * Extern functions
 ****************/
extern int getSIPAppPID(void);
extern int setGPIO(char *gpioNumber);
extern char readGPIO(char *gpioNumber);
extern int GpioInitialization(void);
extern int makeDataStrAndSend(deviceData* devData, deviceInfo* devConfig);
extern int readAllGPIOValue(deviceData* deviceData);
extern int readTTLData(char* dataBuffer, int* iSize);
extern int sendDataToServer(char* serverIP, int serverPort, char* serverPath,
                     char* dataStr, char*keyHeader);

int validateConfigFile(void);
int readAppConfig(deviceInfo * deviceConfig);
int writeAppConfig(deviceInfo * deviceConfig);
void printAppConfig(deviceInfo * deviceConfig);
void setDefaultConfig(deviceInfo * deviceConfig);
void timerHandlerFunction(size_t timer_id, void * user_data);
void timerThread(deviceInfo * deviceConfig);
int audioPlayThread(deviceInfo* deviceConfig);
void gpioCheckTimerHandler(size_t timer_id, void * user_data);
void gpioCheckThread(deviceInfo *deviceConfig);
void sipCallCheckTimerHandler(size_t timer_id, void * user_data);
void sipCallCheckThread(deviceInfo * deviceConfig);
int localDataSendThread(deviceInfo* deviceConfig);
void networkCheckTimerHandler(size_t timer_id, void * user_data);
void networkCheckThread(deviceInfo* deviceConfig);
int ServerCmdHandler(deviceInfo* deviceConfig);
int BroadcastCmdHandler(deviceInfo* deviceConfig);
void getEth0Config(void);
void audConf(int spkVol, int micVol);
void signal_callback_handler(int signum);

void updateWebStatus(void)
{
    FILE *fp;

    if((fp = fopen(ECBSTATUSFILE_NAME, "w")) == NULL) 
    {
        printf("Error in opening ECBStatus file\n");
    }
    else
    {
        // Write configuration to the file.
        fprintf(fp, "prmySipregStatus-%d\n", (aites_call_s->reg1State>0)?1:0); //Reg1
        fprintf(fp, "networkStatus-%d\n", gblDeviceConfig.ethConnectionFlag);    // Cable connection
        fprintf(fp, "prmyLogserverStatus-%d\n",gblDeviceConfig.isPrimary); // Ping
        fprintf(fp, "scndSipregStatus-%d\n", (aites_call_s->reg2State>0)?1:0);   // Reg2
        fprintf(fp, "scndLogserverStatus-%d\n", 0); // not implemented
        fclose(fp);
    }

}


/*******************************************************************************
 * Function:    validateConfigFile
 *
 * Description: This function validate the config file size
 *
 * Parameters : None
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful
 *              RETURN_ERROR- Error
 ******************************************************************************/
int validateConfigFile(void)
{
    FILE *file = fopen ( CONFIGFILE_NAME, "r" );
    int lineCnt = 0;

    if (file != NULL)
    {
        char line [1000];
        while(fgets(line,sizeof line,file)!= NULL) /* read a line from a file */
        {
//            fprintf(stdout,"%s",line); //print the file contents on stdout.
            lineCnt++;
        }
        fclose(file);
    }
    else
    {
        perror(CONFIGFILE_NAME); //print the error message on stderr.
        return RETURN_ERROR;
    }
//    printf("Line count = %d\n", lineCnt);
    if(lineCnt == CONFIGFILE_LINE)
        return RETURN_OK;
    else
        return RETURN_ERROR;
}

/*******************************************************************************
 * Function:    readAppConfig
 *
 * Description: This function read the config file and stored configuration to
 *              global data structure
 *
 * Parameters :
 *      deviceInfo * deviceConfig  - Pointer to global app config structure
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful
 *              RETURN_ERROR- Error
 ******************************************************************************/
int readAppConfig(deviceInfo * deviceConfig)
{
    int retVal, lineCnt;
    FILE * fp;
    char* configParamValue;
    char* configParamName;
    char lineString[1000];
    char parseString[1000];
    retVal = validateConfigFile();
    if(retVal == RETURN_ERROR)
    {
        printf("Configuration file is currepted \n");
        return retVal;
    }

    if((fp = fopen(CONFIGFILE_NAME, "r")) == NULL) {
        printf("No such file\n");
        return RETURN_ERROR;
    }

    lineCnt = 0;
    memset(lineString, 0x00, 1000);
    memset(parseString, 0x00, 1000);
    memset(deviceConfig, 0x00, sizeof(deviceInfo));
    while(fgets(lineString, 1000, fp))
    {
        lineCnt++;
//        printf("line %d ==== %s\n", lineCnt, lineString);
        memcpy(parseString, lineString, strlen(lineString));
        // Tokenize the input string
//        configParamValue = lineString;
        configParamName = strtok(lineString, "-");
//        printf("Token:%s\n",configParamName);
        configParamValue = parseString + strlen(configParamName) + 1;
//        printf("%s:%s\n",configParamName, configParamValue);
        if(!strcmp(configParamName, CONFIG_PARAM_MODEL))
        {
            memcpy(deviceConfig->deviceModel, configParamValue, strlen(configParamValue) -1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_HWVERSION))
        {
            memcpy(deviceConfig->hardwareVersion, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SWVERSION))
        {
            memcpy(deviceConfig->softwareVersion, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_DESCRIPTION))
        {
            memcpy(deviceConfig->deviceDescription, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SPKVOL))
        {
            deviceConfig->spkVolume = atoi(configParamValue);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_MICVOL))
        {
            deviceConfig->micVolume = atoi(configParamValue);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_AUDPLYCNT))
        {
            deviceConfig->audPlayCnt = atoi(configParamValue);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_MAC))
        {
            memcpy(deviceConfig->deviceMac, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_IPADDR))
        {
            memcpy(deviceConfig->deviceIPAddr, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_GATEWAY))
        {
            memcpy(deviceConfig->deviceGateway, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SUBNET))
        {
            memcpy(deviceConfig->deviceSubnet, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_DNS))
        {
            memcpy(deviceConfig->deviceDns, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_INTERVAL))
        {
            deviceConfig->timeInterval = atoi(configParamValue);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_QUERYCHAR))
        {
            deviceConfig->queryChar = configParamValue[0];
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_STARTCHAR))
        {
            deviceConfig->startChar = configParamValue[0];
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_ENDCHAR))
        {
            deviceConfig->endChar = configParamValue[0];
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_DIALNUM))
        {
            deviceConfig->serverDialNum  = atoi(configParamValue);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRIMARY))
        {
            if(configParamValue[0] == '1')
                deviceConfig->isPrimary = true;
            else
                deviceConfig->isPrimary = false;
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMSERVERIP))
        {
            memcpy(deviceConfig->primaryServerConfig.sipServerIP, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMSIPPORT))
        {
            memcpy(deviceConfig->primaryServerConfig.sipPort, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMSIPSERVERUSERID))
        {
            memcpy(deviceConfig->primaryServerConfig.sipServerUserID, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMSIPSERVERPASS))
        {
            memcpy(deviceConfig->primaryServerConfig.sipServerPass, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMSIPAUTOANSFLAG))
        {
            if(configParamValue[0] == '1')
                deviceConfig->primaryServerConfig.sipAutoAnsFlag = true;
            else
                deviceConfig->primaryServerConfig.sipAutoAnsFlag = false;
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMSIPTCPTRANSPORT))
        {
            if(configParamValue[0] == '1')
                deviceConfig->primaryServerConfig.sipTCPTransport = true;
            else
                deviceConfig->primaryServerConfig.sipTCPTransport = false;
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMGPIOLOGSERVERIP))
        {
            memcpy(deviceConfig->primaryServerConfig.gpioLogServerIP, configParamValue, strlen(configParamValue) -1 );
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMGPIOLOGSERVERTIMESYNCPATH))
        {
            memcpy(deviceConfig->primaryServerConfig.gpioLogServerTimeSyncPath, configParamValue, strlen(configParamValue) -1 );
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMGPIOLOGSERVEGPIOLOGPATH))
        {
            memcpy(deviceConfig->primaryServerConfig.gpioLogServeGpioLogPath, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMGPIOLOGSERVESECKEY))
        {
            memcpy(deviceConfig->primaryServerConfig.gpioLogServeSecKey, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_PRMGPIOLOGSERVERRESTAPIPORT))
        {
            deviceConfig->primaryServerConfig.gpioLogServerRestAPIPort   = atoi(configParamValue);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDSERVERIP))
        {
            memcpy(deviceConfig->secondaryServerConfig.sipServerIP, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDSIPPORT))
        {
            memcpy(deviceConfig->secondaryServerConfig.sipPort, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDSIPSERVERUSERID))
        {
            memcpy(deviceConfig->secondaryServerConfig.sipServerUserID, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDSIPSERVERPASS))
        {
            memcpy(deviceConfig->secondaryServerConfig.sipServerPass, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDSIPAUTOANSFLAG))
        {
            if(configParamValue[0] == '1')
                deviceConfig->secondaryServerConfig.sipAutoAnsFlag = true;
            else
                deviceConfig->secondaryServerConfig.sipAutoAnsFlag = false;
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDSIPTCPTRANSPORT))
        {
            if(configParamValue[0] == '1')
                deviceConfig->secondaryServerConfig.sipTCPTransport = true;
            else
                deviceConfig->secondaryServerConfig.sipTCPTransport = false;
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDGPIOLOGSERVERIP))
        {
            memcpy(deviceConfig->secondaryServerConfig.gpioLogServerIP, configParamValue, strlen(configParamValue) -1 );
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDGPIOLOGSERVERTIMESYNCPATH))
        {
            memcpy(deviceConfig->secondaryServerConfig.gpioLogServerTimeSyncPath, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDGPIOLOGSERVEGPIOLOGPATH))
        {
            memcpy(deviceConfig->secondaryServerConfig.gpioLogServeGpioLogPath, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDGPIOLOGSERVESECKEY))
        {
            memcpy(deviceConfig->secondaryServerConfig.gpioLogServeSecKey, configParamValue, strlen(configParamValue) - 1);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDGPIOLOGSERVERRESTAPIPORT))
        {
            deviceConfig->secondaryServerConfig.gpioLogServerRestAPIPort   = atoi(configParamValue);
        }
        else if(!strcmp(configParamName, CONFIG_PARAM_SCNDSERVER))
        {
            if(configParamValue[0] == '1')
                deviceConfig->secondaryServer = true;
            else
                deviceConfig->secondaryServer = false;
        }

        memset(lineString, 0x00, 1000);
        memset(parseString, 0x00, 1000);
    }
    return RETURN_OK;
}

// Function to write the configuration
int writeAppConfig(deviceInfo * deviceConfig)
{
    FILE * fp;

    if((fp = fopen(CONFIGFILE_NAME, "w")) == NULL) {
        printf("No such file\n");
        return RETURN_ERROR;
    }

    // Write configuration to the file.
    fprintf(fp, "%s-%s\n","Model", deviceConfig->deviceModel);
    fprintf(fp, "%s-%s\n","hwVersion", deviceConfig->hardwareVersion);
    fprintf(fp, "%s-%s\n","swVersion", deviceConfig->softwareVersion);
    fprintf(fp, "%s-%s\n","Description", deviceConfig->deviceDescription);
    fprintf(fp, "%s-%d\n","spkVol", deviceConfig->spkVolume);
    fprintf(fp, "%s-%d\n","micVol", deviceConfig->micVolume);
    fprintf(fp, "%s-%d\n","playCnt", deviceConfig->audPlayCnt);
    fprintf(fp, "%s-%s\n","IPAddr", deviceConfig->deviceIPAddr);
    fprintf(fp, "%s-%s\n","Mac", deviceConfig->deviceMac);
    fprintf(fp, "%s-%s\n","Gateway", deviceConfig->deviceGateway);
    fprintf(fp, "%s-%s\n","Subnet", deviceConfig->deviceSubnet);
    fprintf(fp, "%s-%s\n","Dns", deviceConfig->deviceDns);
    fprintf(fp, "%s-%d\n","Interval", deviceConfig->timeInterval);
    fprintf(fp, "%s-%c\n","QueryChar", deviceConfig->queryChar);
    fprintf(fp, "%s-%c\n","StartChar", deviceConfig->startChar);
    fprintf(fp, "%s-%c\n","EndChar", deviceConfig->endChar);
    fprintf(fp, "%s-%d\n","DialNum", deviceConfig->serverDialNum);
    fprintf(fp, "%s-%d\n","Primary", deviceConfig->isPrimary);
    fprintf(fp, "%s-%s\n","PrmServerIP", deviceConfig->primaryServerConfig.sipServerIP);
    fprintf(fp, "%s-%s\n","PrmSipPort", deviceConfig->primaryServerConfig.sipPort);
    fprintf(fp, "%s-%s\n","PrmSipServerUserID", deviceConfig->primaryServerConfig.sipServerUserID);
    fprintf(fp, "%s-%s\n","PrmSipServerPass", deviceConfig->primaryServerConfig.sipServerPass);
    fprintf(fp, "%s-%d\n","PrmSipAutoAnsFlag", deviceConfig->primaryServerConfig.sipAutoAnsFlag);
    fprintf(fp, "%s-%d\n","PrmSipTCPTransport", deviceConfig->primaryServerConfig.sipTCPTransport);

    fprintf(fp, "%s-%s\n","PrmGpioLogServerIP", deviceConfig->primaryServerConfig.gpioLogServerIP);
    fprintf(fp, "%s-%d\n","PrmGpioLogServerRestAPIPort", deviceConfig->primaryServerConfig.gpioLogServerRestAPIPort);
    fprintf(fp, "%s-%s\n","PrmGpioLogServerTimeSyncPath", deviceConfig->primaryServerConfig.gpioLogServerTimeSyncPath);
    fprintf(fp, "%s-%s\n","PrmGpioLogServeGpioLogPath", deviceConfig->primaryServerConfig.gpioLogServeGpioLogPath);
    fprintf(fp, "%s-%s\n","PrmGpioLogServeSecKey", deviceConfig->primaryServerConfig.gpioLogServeSecKey);

    fprintf(fp, "%s-%s\n","ScndServerIP", deviceConfig->secondaryServerConfig.sipServerIP);
    fprintf(fp, "%s-%s\n","ScndSipPort", deviceConfig->secondaryServerConfig.sipPort);
    fprintf(fp, "%s-%s\n","ScndSipServerUserID", deviceConfig->secondaryServerConfig.sipServerUserID);
    fprintf(fp, "%s-%s\n","ScndSipServerPass", deviceConfig->secondaryServerConfig.sipServerPass);
    fprintf(fp, "%s-%d\n","ScndSipAutoAnsFlag", deviceConfig->secondaryServerConfig.sipAutoAnsFlag);
    fprintf(fp, "%s-%d\n","ScndSipTCPTransport", deviceConfig->secondaryServerConfig.sipTCPTransport);

    fprintf(fp, "%s-%s\n","ScndGpioLogServerIP", deviceConfig->secondaryServerConfig.gpioLogServerIP);
    fprintf(fp, "%s-%d\n","ScndGpioLogServerRestAPIPort", deviceConfig->secondaryServerConfig.gpioLogServerRestAPIPort);
    fprintf(fp, "%s-%s\n","ScndGpioLogServerTimeSyncPath", deviceConfig->secondaryServerConfig.gpioLogServerTimeSyncPath);
    fprintf(fp, "%s-%s\n","ScndGpioLogServeGpioLogPath", deviceConfig->secondaryServerConfig.gpioLogServeGpioLogPath);
    fprintf(fp, "%s-%s\n","ScndGpioLogServeSecKey", deviceConfig->secondaryServerConfig.gpioLogServeSecKey);
    fprintf(fp, "%s-%d\n","ScndServer", deviceConfig->secondaryServer);

    fclose(fp);
	return RETURN_OK;
}

void printAppConfig(deviceInfo * deviceConfig)
{
    // Write configuration to the file.
    printf("%s-%s\n","Model", deviceConfig->deviceModel);
    printf("%s-%s\n","hwVersion", deviceConfig->hardwareVersion);
    printf("%s-%s\n","swVersion", deviceConfig->softwareVersion);
    printf("%s-%s\n","Description", deviceConfig->deviceDescription);
    printf("%s-%d\n","spkVol", deviceConfig->spkVolume);
    printf("%s-%d\n","micVol", deviceConfig->micVolume);
    printf("%s-%d\n","playCnt", deviceConfig->audPlayCnt);
    printf("%s-%s\n","Mac", deviceConfig->deviceMac);
    printf("%s-%s\n","IPAddr", deviceConfig->deviceIPAddr);
    printf("%s-%s\n","Gateway", deviceConfig->deviceGateway);
    printf("%s-%s\n","Subnet", deviceConfig->deviceSubnet);
    printf("%s-%s\n","Dns", deviceConfig->deviceDns);
    printf("%s-%d\n","Interval", deviceConfig->timeInterval);
    printf("%s-%c\n","QueryChar", deviceConfig->queryChar);
    printf("%s-%c\n","StartChar", deviceConfig->startChar);
    printf("%s-%c\n","EndChar", deviceConfig->endChar);
    printf("%s-%d\n","DialNum", deviceConfig->serverDialNum);
    printf("%s-%d\n","Primary", deviceConfig->isPrimary);
    printf("%s-%s\n","PrmServerIP", deviceConfig->primaryServerConfig.sipServerIP);
    printf("%s-%s\n","PrmSipPort", deviceConfig->primaryServerConfig.sipPort);
    printf("%s-%s\n","PrmSipServerUserID", deviceConfig->primaryServerConfig.sipServerUserID);
    printf("%s-%s\n","PrmSipServerPass", deviceConfig->primaryServerConfig.sipServerPass);
    printf("%s-%d\n","PrmSipAutoAnsFlag", deviceConfig->primaryServerConfig.sipAutoAnsFlag);
    printf("%s-%d\n","PrmSipTCPTransport", deviceConfig->primaryServerConfig.sipTCPTransport);

    printf("%s-%s\n","PrmGpioLogServerIP", deviceConfig->primaryServerConfig.gpioLogServerIP);
    printf("%s-%s\n","PrmGpioLogServerTimeSyncPath", deviceConfig->primaryServerConfig.gpioLogServerTimeSyncPath);
    printf("%s-%s\n","PrmGpioLogServeGpioLogPath", deviceConfig->primaryServerConfig.gpioLogServeGpioLogPath);
    printf("%s-%s\n","PrmGpioLogServeSecKey", deviceConfig->primaryServerConfig.gpioLogServeSecKey);
    printf("%s-%d\n","PrmGpioLogServerRestAPIPort", deviceConfig->primaryServerConfig.gpioLogServerRestAPIPort);

    printf("%s-%s","ScndServerIP", deviceConfig->secondaryServerConfig.sipServerIP);
    printf("%s-%s\n","ScndSipPort", deviceConfig->secondaryServerConfig.sipPort);
    printf("%s-%s\n","ScndSipServerUserID", deviceConfig->secondaryServerConfig.sipServerUserID);
    printf("%s-%s\n","ScndSipServerPass", deviceConfig->secondaryServerConfig.sipServerPass);
    printf("%s-%d\n","ScndSipAutoAnsFlag", deviceConfig->secondaryServerConfig.sipAutoAnsFlag);
    printf("%s-%d\n","ScndSipTCPTransport", deviceConfig->secondaryServerConfig.sipTCPTransport);

    printf("%s-%s","ScndGpioLogServerIP", deviceConfig->secondaryServerConfig.gpioLogServerIP);
    printf("%s-%s\n","ScndGpioLogServerTimeSyncPath", deviceConfig->secondaryServerConfig.gpioLogServerTimeSyncPath);
    printf("%s-%s\n","ScndGpioLogServeGpioLogPath", deviceConfig->secondaryServerConfig.gpioLogServeGpioLogPath);
    printf("%s-%s\n","ScndGpioLogServeSecKey", deviceConfig->secondaryServerConfig.gpioLogServeSecKey);
    printf("%s-%d\n","ScndGpioLogServerRestAPIPort", deviceConfig->secondaryServerConfig.gpioLogServerRestAPIPort);
    printf("%s-%d\n","ScndServer", deviceConfig->secondaryServer);
}


void setDefaultConfig(deviceInfo * deviceConfig)
{
    memset(deviceConfig, 0x00, sizeof(deviceInfo));
    // Write configuration to the file.
    memcpy(deviceConfig->deviceModel, "XYZ", strlen("XYZ"));
    memcpy(deviceConfig->hardwareVersion, "1.00", strlen("1.00"));
    memcpy(deviceConfig->softwareVersion, "1.00", strlen("1.00"));
    memcpy(deviceConfig->deviceDescription, "Default TollBooth", strlen("Default TollBooth"));
    deviceConfig->spkVolume = 10;
    deviceConfig->micVolume = 10;
    deviceConfig->audPlayCnt = 3;
    memcpy(deviceConfig->deviceMac, "06:72:40:07:AC:CD", strlen("06:72:40:07:AC:CD"));
    memcpy(deviceConfig->deviceIPAddr, "192.168.220.63", strlen("192.168.220.63"));
    memcpy(deviceConfig->deviceGateway, "192.168.255.1", strlen("192.168.255.1"));
    memcpy(deviceConfig->deviceSubnet, "255.255.0.0", strlen("255.255.0.0"));
    memcpy(deviceConfig->deviceDns, "8.8.8.8", strlen("8.8.8.8"));
    deviceConfig->timeInterval = 2;
    deviceConfig->queryChar = '*';
    deviceConfig->startChar = '#';
    deviceConfig->endChar = '@';
    deviceConfig->serverDialNum = 174;
    deviceConfig->isPrimary = 1;
    memcpy(deviceConfig->primaryServerConfig.sipServerIP, "122.169.108.88", strlen("122.169.108.88"));
    memcpy(deviceConfig->primaryServerConfig.sipPort, "8080", strlen("8080"));
    memcpy(deviceConfig->primaryServerConfig.sipServerUserID, "110", strlen("110"));
    memcpy(deviceConfig->primaryServerConfig.sipServerPass, "Aites123", strlen("Aites123"));
    deviceConfig->primaryServerConfig.sipAutoAnsFlag = 1;
    deviceConfig->primaryServerConfig.sipTCPTransport = 1;
    memcpy(deviceConfig->primaryServerConfig.gpioLogServerIP, "122.169.108.88", strlen("122.169.108.88"));
    memcpy(deviceConfig->primaryServerConfig.gpioLogServerTimeSyncPath,"/sececbwebapi/api/ServerTime", strlen("/sececbwebapi/api/ServerTime"));
    memcpy(deviceConfig->primaryServerConfig.gpioLogServeGpioLogPath,"/sececbwebapi/api/GpioLogs", strlen("/sececbwebapi/api/GpioLogs"));
    memcpy(deviceConfig->primaryServerConfig.gpioLogServeSecKey, "sec322", strlen("sec322"));
    deviceConfig->primaryServerConfig.gpioLogServerRestAPIPort = 8081;
    memcpy(deviceConfig->secondaryServerConfig.sipServerIP, "122.169.108.88", strlen("122.169.108.88"));
    memcpy(deviceConfig->secondaryServerConfig.sipPort, "8080", strlen("8080"));
    memcpy(deviceConfig->secondaryServerConfig.sipServerUserID, "110", strlen("110"));
    memcpy(deviceConfig->secondaryServerConfig.sipServerPass, "Aites123", strlen("Aites123"));
    deviceConfig->secondaryServerConfig.sipAutoAnsFlag = 1;
    deviceConfig->secondaryServerConfig.sipTCPTransport = 1;
    memcpy(deviceConfig->secondaryServerConfig.gpioLogServerIP, "122.169.108.88", strlen("122.169.108.88"));
    memcpy(deviceConfig->secondaryServerConfig.gpioLogServerTimeSyncPath,"/sececbwebapi/api/ServerTime", strlen("/sececbwebapi/api/ServerTime"));
    memcpy(deviceConfig->secondaryServerConfig.gpioLogServeGpioLogPath,"/sececbwebapi/api/GpioLogs", strlen("/sececbwebapi/api/GpioLogs"));
    memcpy(deviceConfig->secondaryServerConfig.gpioLogServeSecKey, "sec322", strlen("sec322"));
    deviceConfig->secondaryServerConfig.gpioLogServerRestAPIPort = 8081;
}

/*******************************************************************************
 * Function:    timerHandlerFunction
 *
 * Description: This timer handler function signal timer thread function
 *              to read send local data to server
 *
 * Parameters :
 *
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
void timerHandlerFunction(size_t timer_id, void * user_data)
{
    // Single gpio check thread function
    sem_post(&timerThreadMutex);
}

/*******************************************************************************
 * Function:    timerHandlerFunction
 *
 * Description: This is timer function called on every interval to send
 *              the sensor data to base station.
 *
 * Parameters :
 *      endNode* gblEndNode - Pointer to global end node structure
 *
 * Return Value:
 *      None
 ******************************************************************************/
void timerThread(deviceInfo * deviceConfig)
{
    printf("Timer function called\n");

    while(1)
    {
        sem_wait(&timerThreadMutex);
        if(endFlag == 1)
            break;

        printf("Get All GPIO data\n");
        sleep(30);
        // if(readAllGPIOValue(&gblDeviceData) == RETURN_ERROR)
        // {
        //     printf ("Error while reading GPIO values \n");
        // }
        // else
        // {
        //     printf("Get TTL  data\n");
        //     memset(gblDeviceData.ttlData, 0x00, MAX_STRING);
        //     gblDeviceData.ttlDataSize = 0;
        //     if(readTTLData(gblDeviceData.ttlData, &gblDeviceData.ttlDataSize) == RETURN_ERROR)
        //     {
        //         printf ("Error while reading TTL data values \n");
        //         memcpy(gblDeviceData.ttlData, "psu:-1", strlen("psu:-1"));
        //         gblDeviceData.ttlDataSize = strlen("psu:-1");
        //     }
        //     else
        //     {
        //         printf ("Read data = %s \n", gblDeviceData.ttlData);
        //         printf ("Read data size = %d \n", gblDeviceData.ttlDataSize);
        //     }

        //     if(makeDataStrAndSend(&gblDeviceData, &gblDeviceConfig) == RETURN_ERROR)
        //     {
        //         printf("Error in sending log data to server \n");
        //     }
        // }
    }
}

/*******************************************************************************
 * Function:    audioPlayThread
 *
 * Description: This thread function plays error audio file.
 *
 * Parameters :
 *      deviceInfo* gblDeviceConfig - Pointer to global end node structure
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
int audioPlayThread(deviceInfo* deviceConfig)
{
    int retVal, cnt;
    char aplayCmd[MAX_STRING];
    memset(aplayCmd, 0x00, MAX_STRING);
    printf("eth = %d, isserver = %d, reg1 = %d, reg2 = %d, sipcall = %d\n", 
            gblDeviceConfig.ethConnectionFlag, 
            gblDeviceConfig.isServerAvailable,
            aites_call_s->reg1State, aites_call_s->reg2State, deviceConfig->sipCallStatus);

    if(deviceConfig->sipCallStatus == 0)
    {
        if(deviceConfig->ethConnectionFlag == 0)
        {
            for(cnt = 0; cnt < deviceConfig->audPlayCnt; cnt++)
            {
                sprintf(aplayCmd, "aplay %s", CONNECTION_ERR_FILE);
                printf("aplaycmd = %s", aplayCmd);
                retVal = system(aplayCmd);
                if(retVal == 0)
                    printf("eth Error play Success\n");
                else
                    printf("eth Error play Failed\n");
            }
        }
        else if(deviceConfig->isServerAvailable == false)
        {
            for(cnt = 0; cnt < deviceConfig->audPlayCnt; cnt++)
            {
                sprintf(aplayCmd, "aplay %s", SERVER_UNAVAILABLE_FILE);
                printf("aplaycmd = %s", aplayCmd);
                retVal = system(aplayCmd);
                if(retVal == 0)
                    printf("Server Error play Success\n");
                else
                    printf("Server Error play Failed\n");
            }
        }
    }
    return 0;
}

/*******************************************************************************
 * Function:    gpioCheckTimerHandler
 *
 * Description: This timer handler function signal gpioCheck thread function
 *              to read gpio data
 *
 * Parameters :
 *
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
void gpioCheckTimerHandler(size_t timer_id, void * user_data)
{
    // Single gpio check thread function
    sem_post(&gpioCheckTimerMutex);
}

/*******************************************************************************
 * Function:    gpioCheckThread
 *
 * Description: This thread function is to read serverside command  and
 *              on command receive execute appropriate functions
 *
 * Parameters :
 *      deviceInfo* gblDeviceConfig - Pointer to global end node structure
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
void gpioCheckThread(deviceInfo *deviceConfig)
{
    char ch;
    int  retVal;
    int  regLED = 0;
    int  callLED = 0;
    int  changeFlag = 0;
    int oldcall = 0;
    int oldreg1 = 0;
    int oldreg2 = 0;
    int sound = 0;
    char cmdStr[MAX_STRING];
    struct call *curr_call = malloc(sizeof(struct call));
    
    
    // while(1) {
    //     printf("gpioCheckThread enter");
    //     sleep(30);
    // };
    //     return;
    
    
    while(1)
    {
        // Wait for signal
        sem_wait(&gpioCheckTimerMutex);
        if(endFlag == 1)
            break;

        //printf("DEBUG: BEFORE UA_CALL\n");
        if (aites_call_s->uac == NULL) {
            printf("DEBUG: UAC is null\n");
            curr_call = aites_call_s->callState;
            if (!curr_call)
            {
                printf("Curr_call is NULL\n");
                curr_call->state = 0;
            }
        } else {
            curr_call->state = aites_call_s->callState;
        }
        //printf("DEBUG: After UA_CALL\n");
        
        
        if( (oldcall != curr_call->state) || (oldreg1 != aites_call_s->reg1State) || (oldreg2 != aites_call_s->reg2State))
        {
            printf("call state = %d - %d\n", curr_call->state, oldcall);
            printf("Reg1 state = %d - %d\n", aites_call_s->reg1State, oldreg1);
            printf("Reg2 state = %d - %d\n", aites_call_s->reg2State, oldreg2);
            oldcall = curr_call->state;
            oldreg1 = aites_call_s->reg1State;
            oldreg2 = aites_call_s->reg2State;
            updateWebStatus();
        }
        changeFlag = 0;
        if((aites_call_s->reg1State == 0) && (aites_call_s->reg2State == 0))
        {
//            printf("Reg toggle\n");
            if(regLED == 0)
            {
                regLED = 1;
                offGPIO(GPIO_5);
            }
            else
            {
                regLED = 0;
                onGPIO(GPIO_5);
            }
        }
        else
        {
//            printf("Reg on\n");
            onGPIO(GPIO_5);
        }

        if(curr_call->state == 3)
        {
//            printf("call toggle\n");
            if(callLED == 0)
            {
                callLED = 1;
                offGPIO(GPIO_4);
            }
            else
            {
                callLED = 0;
                onGPIO(GPIO_4);
            }
        }
        else if(curr_call->state == 5)
        {
//            printf("call on\n");
            onGPIO(GPIO_4);
            if (sound == 0)
            {
                sound = 1;
                int volume = 205 + (deviceConfig->spkVolume *5);
                memset(cmdStr, 0x00, MAX_STRING);
                sprintf(cmdStr, "amixer set \'Playback\' %d", volume);
                system(cmdStr);
            }
        }
        else
        {
//            printf("call off\n");
            offGPIO(GPIO_4);
            sound = 0;
        }

        ch = 'a';
        ch = readGPIO(GPIO_9);
        if(gblDeviceData.gpio9Val != ch)
        {
            gblDeviceData.gpio9Val = ch;
            changeFlag = 1;
            printf("Value of GPIO %s (TMPR1 - din1) = %c\n", GPIO_9, ch);
        }

        ch = 'a';
        ch = readGPIO(GPIO_6);
        if(gblDeviceData.gpio6Val != ch)
        {
            gblDeviceData.gpio6Val = ch;
            changeFlag = 1;
            printf("Value of GPIO %s (PTT1 - din 7) = %c\n", GPIO_6, ch);
        }
        if(ch == '0')
        {
            if(gblDeviceConfig.sipCallStatus == 0)
            {
                printf("CALL Button pressed & sip call status off so make SIP call to server\n");
                // URMIL
                printf("eth = %d, isserver = %d, reg1 = %d, reg2 = %d\n", 
                        gblDeviceConfig.ethConnectionFlag, 
                        gblDeviceConfig.isServerAvailable,
                        aites_call_s->reg1State, aites_call_s->reg2State);
                if((gblDeviceConfig.ethConnectionFlag == 0) || 
                   (gblDeviceConfig.isServerAvailable == false) ||
                   ((aites_call_s->reg1State <= 0) && (aites_call_s->reg2State <= 0)))
                {
                    printf("Connection or server error so play error message \n");
                    // Call thread to play error audio
                    retVal = pthread_create(
                                       &audPlayThreadHandle,
                                       NULL,
                                       (void*)audioPlayThread,
                                       (int*)&gblDeviceConfig
                                       );

                    if( retVal != 0 )
                    {
                       printf("Error in creation of error audio play Thread\n");
                    }
                    else
                    {
                       printf("Error audio play thread created\n");
                    }
                }
                else
                {
                    makeSIPCall(gblDeviceConfig.serverDialNum);
                    gblDeviceConfig.dialTrue = true;
                }
            }
        }

        ch = 'a';
        ch = readGPIO(GPIO_1);
        if(gblDeviceData.gpio1Val != ch)
        {
            gblDeviceData.gpio1Val = ch;
            changeFlag = 1;
            printf("Value of GPIO %s (PTT2 - din 6) = %c\n", GPIO_1, ch);
        }

        ch = 'a';
        ch = readGPIO(GPIO_2);
        if(gblDeviceData.gpio2Val != ch)
        {
            gblDeviceData.gpio2Val = ch;
            changeFlag = 1;
            printf("Value of GPIO %s (TMPR9 - din 5) = %c\n", GPIO_2, ch);
        }

        ch = 'a';
        ch = readGPIO(GPIO_7);
        if(gblDeviceData.gpio7Val != ch)
        {
            gblDeviceData.gpio7Val = ch;
            changeFlag = 1;
            printf("Value of GPIO %s (TMPR5 - din 3) = %c\n", GPIO_7, ch);
        }

        ch = 'a';
        ch = readGPIO(GPIO_8);
        if(gblDeviceData.gpio8Val != ch)
        {
            gblDeviceData.gpio8Val = ch;
            changeFlag = 1;
            printf("Value of GPIO %s (TMPR4 - din 4) = %c\n", GPIO_8, ch);
        }

        ch = 'a';
        ch = readGPIO(GPIO_10);
        if(gblDeviceData.gpio10Val != ch)
        {
            gblDeviceData.gpio10Val = ch;
            changeFlag = 1;
            printf("Value of GPIO %s (TMPR8 - din 2) = %c\n", GPIO_10, ch);
        }

        ch = 'a';
        ch = readGPIO(GPIO_11);
//        printf("Value of GPIO %s (TMPR0) = %c\n", GPIO_11, ch);
        gblDeviceData.gpio11Val = ch;

        ch = 'a';
        ch = readGPIO(GPIO_12);
//        printf("Value of GPIO %s (TMPR3) = %c\n", GPIO_12, ch);
        gblDeviceData.gpio12Val = ch;

        ch = 'a';
        ch = readGPIO(GPIO_13);
//        printf("Value of GPIO %s (TMPR7) = %c\n", GPIO_13, ch);
        gblDeviceData.gpio13Val = ch;

        ch = 'a';
        ch = readGPIO(GPIO_14);
//        printf("Value of GPIO %s (TMPR2) = %c\n", GPIO_14, ch);
        gblDeviceData.gpio14Val = ch;

        if(changeFlag == 1)
        {
            sem_post(&timerThreadMutex);
        }
        
    }
}

/*******************************************************************************
 * Function:    sipCallCheckTimerHandle
 *
 * Description: This timer handler function signal sip call check thread function
 *              to read check sip call status
 *
 * Parameters :
 *
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
void sipCallCheckTimerHandler(size_t timer_id, void * user_data)
{
    // Single gpio check thread function
    sem_post(&sipCallCheckTimerMutex);
}

/*******************************************************************************
 * Function:    sipCallCheckThread
 *
 * Description: This timer function is check that baresip call is on or not
 *
 * Parameters :
 *      deviceInfo* gblDeviceConfig - Pointer to global end node structure
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
void sipCallCheckThread(deviceInfo * deviceConfig)
{
    // check the value of soundcard use and set flag
    char sndCardUseStatus[MAX_STRING];
    int dialCnt = 0;

    while(1)
    {
        sem_wait(&sipCallCheckTimerMutex);
        memset(sndCardUseStatus, 0x00, MAX_STRING);
        if(deviceConfig->dialTrue == true)
            dialCnt++;
        else
            dialCnt = 0;

        printf("Dial Cnt = %d\n", dialCnt);
        FILE *fp = popen("cat /proc/asound/card0/pcm0c/sub0/status","r");
        fgets(sndCardUseStatus,MAX_STRING,fp);
        if(strstr(sndCardUseStatus, "closed") != NULL)
        {
            gblDeviceConfig.sipCallStatus = 0;
            // turn off SIP call LED 1
            if(gblDeviceData.gpio4Val == '1')
            {
                if(offGPIO(GPIO_4) == RETURN_ERROR)
                {
                    printf ("Error while turning off LED1 for SIP call\n");
                }
                gblDeviceData.gpio4Val = '0';
            }
            if(dialCnt >= 5)
            {
                // Call is not connected to server so restart baresip app
                deviceConfig->dialTrue = false;
                dialCnt = 0;
                // Start the baresip application
                restartSIPApp();
            }
        }
        else
        {
            dialCnt = 0;
            gblDeviceConfig.sipCallStatus = 1;
            // turn on SIP Call LED 1
            if(gblDeviceData.gpio4Val == '0')
            {
                // Remove:
                // Enable mic
//                printf("i2cset calling\n");
//                system("i2cset -f -y 0 0x1a 0x32 0xFE");
//                printf("i2cset done\n");
                if(onGPIO(GPIO_4) == RETURN_ERROR)
                {
                    printf ("Error while turning on LED1 for SIP call\n");
                }
                gblDeviceData.gpio4Val = '1';
            }
            deviceConfig->dialTrue = false;
        }
        pclose(fp);
    }
}

/*******************************************************************************
 * Function:    localDataSendThread
 *
 * Description: This thread function sends the local log to server once connection
 *              is established.
 *
 * Parameters :
 *      deviceInfo* gblDeviceConfig - Pointer to global end node structure
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
int localDataSendThread(deviceInfo* deviceConfig)
{
    FILE *fptr;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    bool errFlag = false;
    sipServerConfig* sipSrvrConfig;

    // set local data sending flag on
    deviceConfig->localDataSendOnFlag = true;
    // Open log file
    fptr = fopen(LOCAL_DATALOG_FILE, "r");
    if (fptr == NULL)
    {
        printf ("error %d opening %s: %s", errno, LOCAL_DATALOG_FILE,
                 strerror (errno));
        return RETURN_ERROR;
    }

    while ((read = getline(&line, &len, fptr)) != -1)
    {
        printf("Retrieved line of length %zu:\n", read);
        printf("%s", line);
        // Send data to server
        if(deviceConfig->isPrimary)
        {
            sipSrvrConfig = &deviceConfig->primaryServerConfig;
        }
        else
        {
            sipSrvrConfig = &deviceConfig->secondaryServerConfig;
        }

        if(sendDataToServer(sipSrvrConfig->gpioLogServerIP,
                            sipSrvrConfig->gpioLogServerRestAPIPort,
                            sipSrvrConfig->gpioLogServeGpioLogPath,
                            line,
                            sipSrvrConfig->gpioLogServeSecKey ) == RETURN_ERROR)
        {
            errFlag = true;
            printf("Error returned from server while sending data to server %d\n", errFlag);
            // TODO: add code to write in to temp backup file.
            // after file send done if error flag on then delete log file and rename temp backup file
            // to avoid any loss of data in worst worst conditions
            // Very rare scenario to occur

        }
    }    // Write data to local log file
    fclose(fptr);

    // loop to send data to available server
    if (remove(LOCAL_DATALOG_FILE) == 0)
        printf("Deleted successfully");
    else
        printf("Unable to delete the file");

    // set local data sending flag off
    deviceConfig->localDataSendOnFlag = false;

    return 0;
}

/*******************************************************************************
 * Function:    networkCheckTimerHandler
 *
 * Description: This timer handler function signal network check thread function
 *              to check network status
 *
 * Parameters :
 *
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
void networkCheckTimerHandler(size_t timer_id, void * user_data)
{
    // Single gpio check thread function
    sem_post(&networkCheckTimerMutex);
}

/*******************************************************************************
 * Function:    networkCheckTimerHandler
 *
 * Description: This timer function is check network status
 *
 * Parameters :
 *      deviceInfo* gblDeviceConfig - Pointer to global end node structure
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
void networkCheckThread(deviceInfo* deviceConfig)
{
    FILE *fp;
    char cmdOutput[MAX_STRING];
    int x, y, retVal; /*,y;*/
    int lastEthVal = 0;
    gblDeviceConfig.ethConnectionFlag = 0;
    while(1)
    {
        sem_wait(&networkCheckTimerMutex);
        if(endFlag == 1)
            break;

        // check Ethernet cable connected status
        memset(cmdOutput, 0x00, MAX_STRING);
    //    fp = popen("cat /sys/class/net/eno1/carrier","r");
        fp = popen("cat /sys/class/net/eth0/carrier","r");
        fgets(cmdOutput,MAX_STRING,fp);
        x = atoi(cmdOutput);
        pclose(fp);
        fp = popen("cat /sys/class/net/eth1/carrier","r");
        fgets(cmdOutput,MAX_STRING,fp);
        y = atoi(cmdOutput);
        pclose(fp);
        if((x == 1) || (y == 1))
            gblDeviceConfig.ethConnectionFlag =  1;
        else
            gblDeviceConfig.ethConnectionFlag =  0;

        if(lastEthVal != gblDeviceConfig.ethConnectionFlag)
        {
            updateWebStatus();
            lastEthVal = gblDeviceConfig.ethConnectionFlag;
            if(gblDeviceConfig.ethConnectionFlag == 0)
            {
                aites_call_s->reg1State = 0;
                aites_call_s->reg2State = 0;
            }
        }

        // Check server ping for primary server
        memset(cmdOutput, 0x00, MAX_STRING);
        sprintf(cmdOutput, "ping -c 1 -w 1 %s > /dev/null 2>&1",
                            gblDeviceConfig.primaryServerConfig.sipServerIP);
        printf("%s\n",cmdOutput );
        x = system(cmdOutput);
        printf("x =  %d\n", x);
        if(x == 0)
        {
    //        printf("primary server or both servers are available\n");
            if(gblDeviceConfig.isPrimary == false)
            {
                // set primary sip server 
                printf("PRIMARY SERVER Set\n");
                memset(cmdOutput, 0x00, MAX_STRING);
                sprintf(cmdOutput, "sip:%d@%s:%s",
                                    atoi(gblDeviceConfig.primaryServerConfig.sipServerUserID),
                                    gblDeviceConfig.primaryServerConfig.sipServerIP,
                                    gblDeviceConfig.primaryServerConfig.sipPort);
                printf("cmdOutput = %s\n", cmdOutput);
                printf("Disconnect existing call if any\n");
                ua_hangup(uag_find_requri(cmdOutput), NULL, 0, NULL);
                printf("Hangup called now change the current ua\n");               
            	struct ua *ua = NULL;
            	ua = uag_find_aor(cmdOutput);
                if(ua)
                {
                    printf("%s = ua found\n", cmdOutput);
            	    //uag_current_set(ua);
                    gblDeviceConfig.isPrimary = true;
                    struct ua *uac = uag_find_requri(cmdOutput);
                    printf("Current aor = %s\n",account_aor(uac));
                }    
                else
                {
                    printf("%s = ua not found\n", cmdOutput);
                    struct ua *uac = uag_find_requri(cmdOutput);
                    printf("Current aor = %s\n",account_aor(uac));
                }
                updateWebStatus();
            }
            gblDeviceConfig.isServerAvailable = true;
        }
        else
        {
    //        printf("Only Secondary server is available\n");
            if(gblDeviceConfig.isPrimary == true)
            {
                printf("SECONDARY SERVER Set\n");
                // Set secondary sip server
                sprintf(cmdOutput, "sip:%d@%s:%s",
                                    atoi(gblDeviceConfig.secondaryServerConfig.sipServerUserID),
                                    gblDeviceConfig.secondaryServerConfig.sipServerIP,
                                    gblDeviceConfig.secondaryServerConfig.sipPort);
                printf("cmdOutput = %s\n", cmdOutput);
                printf("Disconnect existing call if any\n");
                ua_hangup(uag_find_requri(cmdOutput), NULL, 0, NULL);
                printf("Hangup called now change the current ua\n");               
            	struct ua *ua = NULL;
            	ua = uag_find_aor(cmdOutput);
                if(ua)
                {
                    printf("%s = ua found\n", cmdOutput);
            	    //uag_current_set(ua);
                    gblDeviceConfig.isPrimary = false;                    
                    struct ua *uac = uag_find_requri(cmdOutput);
                    printf("Current aor = %s\n",account_aor(uac));
                }
                else
                {
                    printf("%s = ua not found\n", cmdOutput);
                    struct ua *uac = uag_find_requri(cmdOutput);
                    printf("Current aor = %s\n",account_aor(uac));
                }

                updateWebStatus();
            }
            gblDeviceConfig.isServerAvailable = true;
        }

        if(gblDeviceConfig.ethConnectionFlag == true)
        {
            if(onGPIO(GPIO_3) == RETURN_ERROR)
            {
                printf("Error while turning on LED3 \n");
            }
        }
        else
        {
            if(offGPIO(GPIO_3) == RETURN_ERROR)
            {
                printf("Error while turning off LED3 \n");
            }
        }

        // start thread to send data to server
        // Start thread I to continuously read the server sode commands
        if(gblDeviceConfig.isServerAvailable == true
            && gblDeviceConfig.localDataFlag == true
            && gblDeviceConfig.localDataSendOnFlag == false)
        {
            retVal = pthread_create(
                               &localDataSendThreadHandle,
                               NULL,
                               (void*)localDataSendThread,
                               (int*)&gblDeviceConfig
                               );

            if( retVal != 0 )
            {
               printf("Error in creation of local data send Thread\n");
            }
            else
            {
               printf("Local data send thread created\n");
            }
        }
    }
}


/*******************************************************************************
 * Function:    ServerCmdHandler
 *
 * Description: This thread function is to read serverside command  and
 *              on command receive execute appropriate functions
 *
 * Parameters :
 *      deviceInfo* gblDeviceConfig - Pointer to global end node structure
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
int ServerCmdHandler(deviceInfo* deviceConfig)
{
    int             serverSock;             //  Server Socket
    int             recvLen;                // Receive Length
    int             recvCnt;                // Receive byte counter
    int             recvSock;               // Connected socket
    int             retVal;                 // stores return values
    unsigned int    recvSockAddrLen;        // Store socket addr struct length
    struct sockaddr_in      recvSockAddr;   // Socket add struct
    struct sockaddr_in      serverSockAddr; //  Server socket addr structure
    char            recvCmdBuff[SERVER_DATA_SIZE]; // Configuration buffer


    while(1)
    {
    again:    
        // Create Socket
        serverSock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSock == 0)
        {
            printf ("Cannot Create server socket to receive server command data\n");
            return RETURN_ERROR;
        }

        int on = 1;
        if (-1 == setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR,(const char *) &on, sizeof(on)))
        {
            printf ("Error in setting socket option %d\n", SERVER_CMD_PORT);
            shutdown(serverSock,SHUT_RDWR);
            close(serverSock);
            goto again;
        }

        if (-1 == setsockopt(serverSock, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)))
        {
            printf ("Error in setting socket option %d\n", SERVER_CMD_PORT);
            shutdown(serverSock,SHUT_RDWR);
            close(serverSock);
            goto again;
        }
        


        // Bind Socket with Server Port
        serverSockAddr.sin_addr.s_addr    =  htonl(INADDR_ANY);
        serverSockAddr.sin_family         = AF_INET;
        serverSockAddr.sin_port           = htons (SERVER_CMD_PORT);
        if (bind (serverSock, (struct sockaddr *)&serverSockAddr,sizeof (serverSockAddr)) < 0)
        {
            printf ("Cannot bind Server Common Socket to port %d\n", SERVER_CMD_PORT);
            perror("bind failed. Error");
            shutdown(serverSock,SHUT_RDWR);
            close(serverSock);
            goto again;
        }

        // Listen at the Server Socket
        if (listen (serverSock, MAX_LISTEN_CLIENT) != 0 )
        {
            printf ("Error in Listen function of server command socket\n");
            shutdown(serverSock,SHUT_RDWR);
            close(serverSock);
            goto again;
        }
        printf("Server Command socket listen called\n");
        recvSockAddrLen = sizeof(recvSockAddr);
        while (1)
        {
            // Accept Connection from Client
            recvSock = accept ( serverSock, (struct sockaddr *) &(recvSockAddr),
                                &recvSockAddrLen);

            if (recvSock < 0)
            {
                printf("Cannot accept Connection from server command side application, %d\n", errno);
                continue;
            }

            memset(recvCmdBuff, 0x00, SERVER_DATA_SIZE);
            printf("Server command socket  request accepted\n");

            recvLen = 0;

            while(1)
            {
            //  printf("Call recv function\n");
                // Receive data from the socket and store it in Common buffer structure
                recvCnt = recv (recvSock, recvCmdBuff + recvLen, SERVER_DATA_SIZE - 1, 0);
            //  printf("recvCnt = %d\n", recvCnt);
                if (recvCnt >= 4 )
                {
                    printf("recvCmdBuff = %s\n", recvCmdBuff);
                    if(strstr(recvCmdBuff, "CALL") != NULL)
                    {
                        makeSIPCall(deviceConfig->serverDialNum);
                    }
                    else if (strstr(recvCmdBuff, "DATA") != NULL)
                    {
                        // Call function to prepare data
                        retVal = makeDataStrAndSend(&gblDeviceData, deviceConfig);
                        if(retVal == RETURN_ERROR)
                        {
                            printf("Error in sending log data to server \n");
                            close (recvSock);
                            recvSock = -1;
                        }
                    }
                    else if (strstr(recvCmdBuff, "KILL") != NULL)
                    {
                        printf("KILL received from webapp \n");
                        shutdown(serverSock,SHUT_RDWR);
                        close(serverSock);
                        close (recvSock);
                        closeApp();
                        return 1;
                        // Call function to prepare data
                    }

                }
                else
                {
                    printf("Error in recv function = %d, %d\n", recvCnt, errno);
                    close (recvSock);
                    recvSock = -1;
                    break;
                }
            }
        }
    }
    return 1;

}

/*******************************************************************************
 * Function:    BroadcastCmdHandler
 *
 * Description: This thread function is to read broadcast message and send
 *              device name and IP to server
 *
 * Parameters :
 *      deviceInfo* gblDeviceConfig - Pointer to global end node structure
 *
 * Return Value:
 *      int - Handle of serail port  must be greater then 0
 *              OR
 *              RETURN_ERROR    - Error occurred.
 ******************************************************************************/
int BroadcastCmdHandler(deviceInfo* deviceConfig)
{
    int sock;                         /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast Address */
    unsigned short broadcastPort;     /* Port */
    char recvString[MAX_STRING]; /* Buffer for received string */
    int recvStringLen;                /* Length of received string */
    char * token;
    char serverIP[IP_STRING_LEN];
    char idString[MAX_STRING * 2];
    int socket_desc;
    struct sockaddr_in server_addr;

    broadcastPort = BROADCAST_CMD_PORT;   /* First arg: broadcast port */

    /* Create a best-effort datagram socket using UDP */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf ("Cannot Create server socket to receive server command data\n");
        return RETURN_ERROR;
    }

    /* Construct bind structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
    broadcastAddr.sin_port = htons(broadcastPort);      /* Broadcast port */

    /* Bind to the broadcast port */
    if (bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0)
    {
        printf ("Cannot bind Server Common Socket to port %d\n", BROADCAST_CMD_PORT);
        perror("bind failed broadcast. Error");
        shutdown(sock,SHUT_RDWR);
        close(sock);
        return RETURN_ERROR;
    }

    while(1)
    {
        /* Receive a single datagram from the server */
        if ((recvStringLen = recvfrom(sock, recvString, MAX_STRING, 0, NULL, 0)) < 0)
        {
            printf ("Error in receiving message from broadcast server\n");
            shutdown(sock,SHUT_RDWR);
            close(sock);
            return RETURN_ERROR;
        }

        recvString[recvStringLen] = '\0';
        printf("Received: %s\n", recvString);    /* Print the received string */

        token = strtok(recvString, ":");
        if(!strcmp(token, "SENDID"))
        {
            memset(serverIP, 0x00, IP_STRING_LEN);
            memset(idString, 0x00, MAX_STRING * 2);
            sprintf(idString, "%s-%s", deviceConfig->deviceDescription, deviceConfig->deviceIPAddr);
            printf("idString = %s", idString);
            // command received
            token = strtok(NULL, ":");
            memcpy(serverIP, token, strlen(token));
            // Create socket:
            socket_desc = socket(AF_INET, SOCK_STREAM, 0);

            if(socket_desc < 0)
            {
                printf("Unable to create socket\n");
                return -1;
            }

            printf("Socket created successfully\n");

            // Set port and IP the same as server-side:
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(6000);
            server_addr.sin_addr.s_addr = inet_addr(serverIP);

            // Send connection request to server:
            if(connect(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
            {
                printf("Unable to connect\n");
                return -1;
            }
            printf("Connected with server successfully\n");

            // Send the message to server:
            if(send(socket_desc, idString, strlen(idString), 0) < 0)
            {
                printf("Unable to send message\n");
                return -1;
            }

            // Close the socket:
            close(socket_desc);

            return 0;
        }
    }
    return 1;
}

void audConf(int spkVol, int micVol)
{
    // char cmdStr[MAX_STRING];
    // int value = 205 + (spkVol * 5);
    // char *p;
    // memset(cmdStr, 0x00, MAX_STRING);

    // spk vol command
    //sprintf(cmdStr, "amixer set \'Right Input Boost Mixer RINPUT1\' 0");
    //system(cmdStr);
    //memset(cmdStr, 0x00, MAX_STRING);
    //sprintf(cmdStr, "amixer set \'Right Input Boost Mixer RINPUT2\' 0");
    //system(cmdStr);
    //memset(cmdStr, 0x00, MAX_STRING);
    //sprintf(cmdStr, "amixer set \'Right Input Boost Mixer RINPUT3\' 0");
    //system(cmdStr);
    //memset(cmdStr, 0x00, MAX_STRING);
    //sprintf(cmdStr, "amixer set \'Left Input Boost Mixer LINPUT1\' 0");
    //system(cmdStr);
    //memset(cmdStr, 0x00, MAX_STRING);
    //sprintf(cmdStr, "amixer set \'Left Input Boost Mixer LINPUT2\' 0");
    //system(cmdStr);
    //memset(cmdStr, 0x00, MAX_STRING);
    //sprintf(cmdStr, "amixer set \'Left Input Boost Mixer LINPUT3\' 0");
    //system(cmdStr);
    //memset(cmdStr, 0x00, MAX_STRING);
//    sprintf(cmdStr, "amixer set \'ADC PCM\' 207");
//    system(cmdStr);
//    memset(cmdStr, 0x00, MAX_STRING);
//    sprintf(cmdStr, "amixer set \'Capture\' 22");
//    system(cmdStr);
//    memset(cmdStr, 0x00, MAX_STRING);
//    sprintf(cmdStr, "amixer set \'Playback\' %d", value);
//    sprintf(cmdStr, "amixer set \'Playback\' 230", value);
//    p = system(cmdStr);
//    if(p != NULL)
//        printf("Playback cmdoutput = %s\n", p);
//    memset(cmdStr, 0x00, MAX_STRING);
//    sprintf(cmdStr, "amixer set \'Speaker\' %d% \r\n", (90 + spkVol));
//    system(cmdStr);
    
}

/*******************************************************************************
 * Function:    getEth0Config
 *
 * Description: This function return network configuration of eth0
 *
 * Parameters : None
 *
 * Return Value: None
 *
 ******************************************************************************/
void getEth0Config(void)
{
    FILE *fp;
    char cmdOutput[MAX_STRING];

    // Get mac address
    memset(cmdOutput, 0x00, MAX_STRING);
    printf("----------1---------\n");
    fp = popen("cat /sys/class/net/eth0/address","r");
//    fp = popen("cat /sys/class/net/eno1/address","r");
    printf("----------2---------\n");
    fgets(cmdOutput,MAX_STRING,fp);
    printf("----------3---------\n");
    memset(gblDeviceConfig.deviceMac, 0x00, MAX_STRING);
    printf("----------4---------\n");
    if(strlen(cmdOutput) > 1)
    {
        memcpy(gblDeviceConfig.deviceMac, cmdOutput, strlen(cmdOutput) - 1 );
        printf("Mac =  %s\n", gblDeviceConfig.deviceMac);
    }
    pclose(fp);

    // Get IP address
    memset(cmdOutput, 0x00, MAX_STRING);
    printf("----------5---------\n");
    fp = popen("ifconfig eth0 | grep 'inet addr:' | cut -d: -f2 | awk '{ print $1}'","r");
//    fp = popen("/sbin/ifconfig eno1 | grep 'inet ' | cut -d: -f1 | awk '{ print $2}'","r");
    printf("----------6---------\n");
    fgets(cmdOutput,MAX_STRING,fp);
    printf("----------7---------\n");
    memset(gblDeviceConfig.deviceIPAddr, 0x00, MAX_STRING);
    printf("----------8---------\n");
    if(strlen(cmdOutput) > 1)
    {
        memcpy(gblDeviceConfig.deviceIPAddr, cmdOutput, strlen(cmdOutput) -1);
        printf("IP addre =  %s\n", gblDeviceConfig.deviceIPAddr);
    }
    pclose(fp);

    // Get gateway of eth0
    printf("----------9---------\n");
    memset(cmdOutput, 0x00, MAX_STRING);
    printf("----------10---------\n");
    fp = popen("route -n | grep eth0  | grep 'UG[ \t]' | awk '{print $2}'","r");
//    fp = popen("route -n | grep eno1  | grep 'UG[ \t]' | awk '{print $2}'","r");
    printf("----------11---------\n");
    fgets(cmdOutput,MAX_STRING,fp);
    printf("----------12---------\n");
    memset(gblDeviceConfig.deviceGateway, 0x00, MAX_STRING);
    if(strlen(cmdOutput) > 1)
    {
        memcpy(gblDeviceConfig.deviceGateway, cmdOutput, strlen(cmdOutput) -1);
        printf("Gateway =  %s\n", gblDeviceConfig.deviceGateway);
    }
    pclose(fp);

    // Get subnet mask of eth0
    memset(cmdOutput, 0x00, MAX_STRING);
    printf("----------13---------\n");
    fp = popen("ifconfig eth0 | grep 'Mask:' | cut -d: -f4 | awk '{ print $1}'","r");
//    fp = popen("/sbin/ifconfig eno1 | grep 'inet ' | cut -d: -f1 | awk '{ print $4}'","r");
    printf("----------14---------\n");
    fgets(cmdOutput,MAX_STRING,fp);
    memset(gblDeviceConfig.deviceSubnet, 0x00, MAX_STRING);
    printf("----------15---------\n");
    if(strlen(cmdOutput) > 1)
    {
        memcpy(gblDeviceConfig.deviceSubnet, cmdOutput, strlen(cmdOutput) -1);
        printf("Mask =  %s\n", gblDeviceConfig.deviceSubnet);
    }
    pclose(fp);
}

void signal_callback_handler(int signum)
{   
    void * res;
    if (signum == SIGINT)
    {
        printf("Caught signal %d\n",signum);
        //Do the action need to be taken when SIGINT is received
        ua_stop_all(false);
        endFlag = 1;
        sem_post(&timerThreadMutex);
        sem_post(&gpioCheckTimerMutex);
        sem_post(&networkCheckTimerMutex);
        pthread_join(timerThreadHandle, &res);
        pthread_join(gpioCheckTimerThreadHandle, &res);
        pthread_join(networkCheckTimerThreadHandle, &res);
        stop_timer(dataSendTimer); 
        stop_timer(gpioCheckTimer);
        stop_timer(networkCheckTimer);
     }
     else 
     {
        printf("Caught signal %d\n",signum);
        //Do the action need to be taken when SIGHUP is received

	}
	exit(0);
}

void closeApp(void)
{   
    void *res;
    printf("Close App now\n");
    //Do the action need to be taken when SIGINT is received
    ua_stop_all(true);
    endFlag = 1;
    sem_post(&timerThreadMutex);
    sem_post(&gpioCheckTimerMutex);
    sem_post(&networkCheckTimerMutex);
    pthread_join(timerThreadHandle, &res);
    pthread_join(gpioCheckTimerThreadHandle, &res);
    pthread_join(networkCheckTimerThreadHandle, &res);
    stop_timer(dataSendTimer); 
    stop_timer(gpioCheckTimer);
    stop_timer(networkCheckTimer);
	exit(0);
}


int aitesmain(void)
{
    int retVal;
    void * res;

    memset(&gblDeviceData, 0x00, sizeof(gblDeviceData));
    gblDeviceData.gpio1Val = '0';
    gblDeviceData.gpio2Val = '0';
    gblDeviceData.gpio3Val = '0';
    gblDeviceData.gpio4Val = '0';
    gblDeviceData.gpio5Val = '0';
    gblDeviceData.gpio6Val = '0';
    gblDeviceData.gpio7Val = '0';
    gblDeviceData.gpio8Val = '0';
    gblDeviceData.gpio9Val = '0';
    gblDeviceData.gpio10Val = '0';
    gblDeviceData.gpio11Val = '0';
    gblDeviceData.gpio12Val = '0';
    gblDeviceData.gpio13Val = '0';
    gblDeviceData.gpio14Val = '0';
    aites_call_s = malloc(sizeof(struct aites_call));
    aites_call_s->reg1State = 0;
    aites_call_s->reg2State = 0;
    endFlag = 0;
    // sleep to initialize network ifconfig
    sleep(10);

    signal(SIGINT, signal_callback_handler);

    // Validate config file
    if(validateConfigFile() == RETURN_OK)
	{
		printf("Config file Ok\n");
        // Read configuration file and get the configuration
        if(readAppConfig(&gblDeviceConfig) == RETURN_OK)
    	{
    		printf("Config file data stored in global structure\n");
    	}
    	else
        {
    		printf("Error while storing config to global structure so used default configuration\n");
            setDefaultConfig(&gblDeviceConfig);
        }

	}
	else
		printf("Config file currepted\n");

    printAppConfig(&gblDeviceConfig);

    // Get the server time and set it as local time
    retVal = getServerTime(gblDeviceConfig.primaryServerConfig.gpioLogServerIP,
                           gblDeviceConfig.primaryServerConfig.gpioLogServerRestAPIPort,
                           gblDeviceConfig.primaryServerConfig.gpioLogServerTimeSyncPath);

    if( retVal == RETURN_ERROR )
    {
       printf("Error in setting up the server time as local time\n");
    }
    else
    {
       printf("Server time sync done\n");
    }

    // Initialize Audio codec
    audConf(gblDeviceConfig.spkVolume, gblDeviceConfig.micVolume);

    // Make baresip app configuration string as per the configuration &
    // Update baresip configuration file on system
	if(updateBareSIPConfig(&gblDeviceConfig.primaryServerConfig, &gblDeviceConfig.secondaryServerConfig) == RETURN_OK)
	{
		system("cat /etc/baresip/accounts");
	}
	else
		printf("Error in SIP Config\n");

    // Start the baresip application
//    restartSIPApp();


    // Start thread I to continuously read the server sode commands
    retVal = pthread_create(
                       &serverThreadHandle,
                       NULL,
                       (void*)ServerCmdHandler,
                       (int*)&gblDeviceConfig
                       );

    if( retVal != 0 )
    {
       printf("Error in creation of Server command handler Thread\n");
       return RETURN_ERROR;
    }
    else
    {
       printf("Server Command handler  thread created\n");
    }
    
//
//
//    // Start thread II to continuously read the broadcast message for ID
//    retVal = pthread_create(
//                       &brdCastThreadHandle,
//                       NULL,
//                       (void*)BroadcastCmdHandler,
//                       (int*)&gblDeviceConfig
//                       );
//
//    if( retVal != 0 )
//    {
//       printf("Error in creation of Broadcast message handler Thread\n");
//       return RETURN_ERROR;
//    }
//    else
//    {
//       printf("Broadcast message handler thread created\n");
//    }

    // Initialize timer
    initialize();

    // GPIO initialization
    GpioInitialization();

    // GPIO value reader timer to handle push button event
    // Init semaphore
    sem_init(&gpioCheckTimerMutex, 0, 1);
    // create GPIO check Thread
 
    retVal = pthread_create(
                       &gpioCheckTimerThreadHandle,
                       NULL,
                       (void*)gpioCheckThread,
                       (int*)&gblDeviceConfig
                       );
    if( retVal != 0 )
    {
       printf("Error in creation of GPIO check Thread\n");
       return RETURN_ERROR;
    }
    else
    {
       printf("GPIO check thread created\n");
    }

    // create timer handler
    gpioCheckTimer= start_timer(GPIOCHECK_TIME, gpioCheckTimerHandler, TIMER_PERIODIC, &gblDeviceData);

//    // Init semaphore
//    sem_init(&sipCallCheckTimerMutex, 0, 1);
//    // create GPIO check Thread
//    retVal = pthread_create(
//                       &sipCallCheckTimerThreadHandle,
//                       NULL,
//                       (void*)sipCallCheckThread,
//                       (int*)&gblDeviceConfig
//                       );
//    if( retVal != 0 )
//    {
//       printf("Error in creation of Sip call check Thread\n");
//       return RETURN_ERROR;
//    }
//    else
//    {
//       printf("Sip call check thread created\n");
//    }
//    // timer to check baresip call status
//    sipCallCheckTimer = start_timer(SIPCALL_CHECKTIME, sipCallCheckTimerHandler, TIMER_PERIODIC, &gblDeviceData);

    // Init semaphore
    sem_init(&networkCheckTimerMutex, 0, 1);
    // create GPIO check Thread
    retVal = pthread_create(
                       &networkCheckTimerThreadHandle,
                       NULL,
                       (void*)networkCheckThread,
                       (int*)&gblDeviceConfig
                       );
    if( retVal != 0 )
    {
       printf("Error in creation of network Check Thread\n");
       return RETURN_ERROR;
    }
    else
    {
       printf("Network check thread created\n");
    }

    // timer to check network status
    networkCheckTimer= start_timer(NETWORK_CHECKTIME, networkCheckTimerHandler, TIMER_PERIODIC, &gblDeviceData);

    // Init semaphore
    sem_init(&timerThreadMutex, 0, 1);
    // create GPIO check Thread
    retVal = pthread_create(
                       &timerThreadHandle,
                       NULL,
                       (void *)timerThread,
                       (int*)&gblDeviceConfig
                       );
    if( retVal != 0 )
    {
       printf("Error in creation of network Check Thread\n");
       return RETURN_ERROR;
    }
    else
    {
       printf("Network check thread created\n");
    }
    // Timer function to read data and send data to server
    dataSendTimer = start_timer((gblDeviceConfig.timeInterval * 1000), timerHandlerFunction, TIMER_PERIODIC, &gblDeviceConfig);

    // Application started successfully so turn on LED
//    if(onGPIO(GPIO_4) == RETURN_ERROR)
//    {
//        printf("Error while turning on LED1 \n");
//    }

    // Get mac address of eth0 interface and store it in to global structure
    getEth0Config();

    if(writeAppConfig(&gblDeviceConfig) == RETURN_OK)
	{
		printf("Config file data written to configuration file\n");
	}
	else
		printf("Error while writing config to configuration file\n");

//    pthread_join(serverThreadHandle, &res);
//    pthread_join(brdCastThreadHandle, &res);
//    pthread_join(sipCallCheckTimerThreadHandle, &res);
//    stop_timer(sipCallCheckTimer);

/*  Commented this to avoid blocking of baresip main code in loop
    pthread_join(timerThreadHandle, &res);
    pthread_join(gpioCheckTimerThreadHandle, &res);
    pthread_join(networkCheckTimerThreadHandle, &res);
    stop_timer(dataSendTimer); 
    stop_timer(gpioCheckTimer);
    stop_timer(networkCheckTimer);
*/
    printf("Exit\n");
    return 0;
}

/*******************************************************************************
 * Copyright (c) 2021 XXYYZZ - All rights reserved.
 *
 * This software is authored by XXYYZZ and is XXYYZZ' intellectual
 * property, including the copyrights in all countries in the world.
 * This software is provided under a license to use only with all other rights,
 * including ownership rights, being retained by XXYYZZ.
 *
 * electronic or otherwise, without the written consent of XXYYZZ.
 * This file may not be distributed, copied, or reproduced in any manner,
 ******************************************************************************/
