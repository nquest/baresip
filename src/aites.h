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
 * File Name    :  	aites.h
 *
 * Description  :	Header file for SIP Application contains required structures
 *
 * History:
 *  OCT/26/2021, UBV,	Created the file
 *
 ******************************************************************************/
#ifndef _INCLUDE_AITES_H
#define _INCLUDE_AITES_H

/*********************
 * Include Files
 *********************/
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "mytimer.h"

/*********************
 * Macros and contants
 *********************/
#define IP_STRING_LEN               20              // IP address string Lenght
#define PORT_STRING_LEN             6               // Port address string length
#define MAX_STRING                  256             // String buffer length
#define MAX_DATA_LEN                4096            // Max API response length

#define RETURN_ERROR    	        (-1)            // Error return value
#define RETURN_OK                   (0)             // OK return value


/* TTL port related constants                                                 */
#define UART_PORT_NAME		      "/dev/ttymxc4"      // Serial port name
//#define UART_PORT_NAME		       "/dev/ttyUSB0"      // Serial port name
#define UART_BAUDRATE			   B9600            // Serial port baud rate
#define UART_MSG_CMD               "*"              // Command to get TTL data
#define UART_MSG_START		       "#"              // start char of TTL data
#define UART_MSG_END			   "@"              // end char of TTL data


#define BARESIP_ACCNTS_FILE		"/etc/baresip/accounts"
//#define BARESIP_ACCNTS_FILE		    "testConfig.ini"   // Baresip app config file
#define CONNECTION_ERR_FILE		    "/usr/bin/networkcableerr.wav"
#define SERVER_UNAVAILABLE_FILE	    "/usr/bin/servererr.wav"
//#define CONNECTION_ERR_FILE		    "networkcableerr.wav"
//#define SERVER_UNAVAILABLE_FILE	    "servererr.wav"

#define BROADCAST_CMD_PORT 			7700            // Server cmd listening port

/* SIP server side command handler related constants                          */
#define SERVER_CMD_PORT 			5500            // Server cmd listening port
#define MAX_LISTEN_CLIENT           5               // MAX supported clients
#define SERVER_DATA_SIZE 			10              // Server command size
#define LOCAL_DATALOG_FILE		   "/usr/bin/locallog.log"   // Local log file name

/* GPIO port names                                                            */
//#define DEVBOARD    1
#define MEDIABOARD  1

#ifdef DEVBOARD
#define GPIO_1       			    "110"            // GPIO port 1 name
#define GPIO_2       			    "109"            // GPIO port 2 name
#define GPIO_3        	            "111"            // GPIO - LED 1
#define GPIO_4        	            "112"            // GPIO - LED 2
#endif

#ifdef MEDIABOARD
#define GPIO_1       			    "96"             // Input GPIO PTT2 - GPIO4.IO0     (din 7)
#define GPIO_2       			    "137"            // Input GPIO Tamper9 - GPIO5.IO9  (din 6) 
#define GPIO_3        	            "100"             // Output GPIO LED1- GPIO4.IO4    (dout 1) (network)
#define GPIO_4        	            "99"             // Output GPIO LED2- GPIO4.IO3     (dout 2) (middleone - call status)
#endif

#define GPIO_5        	            "97"             // Output GPIO LED3- GPIO4.IO1    (dout 3)  (push button - registration) 
#define GPIO_6       		     	"98"             // Input GPIO PTT1 - GPIO4.IO2    (din 8) 
#define GPIO_7       			    "133"            // Input GPIO Tamper5 - GPIO5.IO5 (din 9) 
#define GPIO_8        			    "132"            // Input GPIO Tamper4 - GPIO5.IO4 (din 10)
#define GPIO_9        	            "129"            // Input GPIO Tamper1 - GPIO5.IO1 (din 11)
#define GPIO_10      		     	"136"            // Input GPIO Tamper8 - GPIO5.IO8 (din 2)
#define GPIO_11       			    "128"            // Input GPIO Tamper0 - GPIO5.IO0 (din 1) 
#define GPIO_12        			    "131"            // Input GPIO Tamper3 - GPIO5.IO3 (din 4)
#define GPIO_13        			    "135"            // Input GPIO Tamper7 - GPIO5.IO7 (din 3)
#define GPIO_14        			    "130"            // Input GPIO Tamper2 - GPIO5.IO2 (din 5)



/* Config file name string constants                                          */
#define CONFIG_PARAM_MODEL                      "Model"
#define CONFIG_PARAM_HWVERSION                  "hwVersion"
#define CONFIG_PARAM_SWVERSION                  "swVersion"
#define CONFIG_PARAM_DESCRIPTION                "Description"
#define CONFIG_PARAM_SPKVOL                     "spkVol"
#define CONFIG_PARAM_MICVOL                     "micVol"
#define CONFIG_PARAM_AUDPLYCNT                  "playCnt"
#define CONFIG_PARAM_MAC                        "Mac"
#define CONFIG_PARAM_IPADDR                     "IPAddr"
#define CONFIG_PARAM_GATEWAY                    "Gateway"
#define CONFIG_PARAM_SUBNET                     "Subnet"
#define CONFIG_PARAM_DNS                        "Dns"
#define CONFIG_PARAM_INTERVAL                   "Interval"
#define CONFIG_PARAM_QUERYCHAR                  "QueryChar"
#define CONFIG_PARAM_STARTCHAR                  "StartChar"
#define CONFIG_PARAM_ENDCHAR                    "EndChar"
#define CONFIG_PARAM_DIALNUM                    "DialNum"
#define CONFIG_PARAM_PRIMARY                    "Primary"
#define CONFIG_PARAM_PRMSERVERIP                "PrmServerIP"
#define CONFIG_PARAM_PRMSIPPORT                 "PrmSipPort"
#define CONFIG_PARAM_PRMSIPSERVERUSERID         "PrmSipServerUserID"
#define CONFIG_PARAM_PRMSIPSERVERPASS           "PrmSipServerPass"
#define CONFIG_PARAM_PRMSIPAUTOANSFLAG          "PrmSipAutoAnsFlag"
#define CONFIG_PARAM_PRMSIPTCPTRANSPORT         "PrmSipTCPTransport"
#define CONFIG_PARAM_PRMGPIOLOGSERVERIP         "PrmGpioLogServerIP"
#define CONFIG_PARAM_PRMGPIOLOGSERVERTIMESYNCPATH   "PrmGpioLogServerTimeSyncPath"
#define CONFIG_PARAM_PRMGPIOLOGSERVEGPIOLOGPATH     "PrmGpioLogServeGpioLogPath"
#define CONFIG_PARAM_PRMGPIOLOGSERVESECKEY          "PrmGpioLogServeSecKey"
#define CONFIG_PARAM_PRMGPIOLOGSERVERRESTAPIPORT    "PrmGpioLogServerRestAPIPort"
#define CONFIG_PARAM_SCNDSERVERIP                "ScndServerIP"
#define CONFIG_PARAM_SCNDSIPPORT                 "ScndSipPort"
#define CONFIG_PARAM_SCNDSIPSERVERUSERID         "ScndSipServerUserID"
#define CONFIG_PARAM_SCNDSIPSERVERPASS           "ScndSipServerPass"
#define CONFIG_PARAM_SCNDSIPAUTOANSFLAG          "ScndSipAutoAnsFlag"
#define CONFIG_PARAM_SCNDSIPTCPTRANSPORT         "ScndSipTCPTransport"
#define CONFIG_PARAM_SCNDGPIOLOGSERVERIP         "ScndGpioLogServerIP"
#define CONFIG_PARAM_SCNDGPIOLOGSERVERTIMESYNCPATH   "ScndGpioLogServerTimeSyncPath"
#define CONFIG_PARAM_SCNDGPIOLOGSERVEGPIOLOGPATH     "ScndGpioLogServeGpioLogPath"
#define CONFIG_PARAM_SCNDGPIOLOGSERVESECKEY          "ScndGpioLogServeSecKey"
#define CONFIG_PARAM_SCNDGPIOLOGSERVERRESTAPIPORT    "ScndGpioLogServerRestAPIPort"
#define CONFIG_PARAM_SCNDSERVER                  "ScndServer"


/*********************
 * Typedef structures
 *********************/
/* Sructure to store the data values to be send to server at regular interval */
typedef struct deviceData_{

    // Input GPIO Values
    char gpio1Val;                          // Input GPIO PTT2
    char gpio2Val;                          // Input GPIO Tamper9
    char gpio3Val;                          // Output GPIO LED1
    char gpio4Val;                          // Output GPIO LED2
    char gpio5Val;                          // Output GPIO LED3

    // GPIO for future
    char gpio6Val;                          // Input GPIO PTT2
    char gpio7Val;                          // Input GPIO Tamper5
    char gpio8Val;                          // Input GPIO Tamper4
    char gpio9Val;                          // Input GPIO Tamper1
    char gpio10Val;                         // Input GPIO Tamper8
    char gpio11Val;                         // Input GPIO Tamper0
    char gpio12Val;                         // Input GPIO Tamper3
    char gpio13Val;                         // Input GPIO Tamper7
    char gpio14Val;                         // Input GPIO Tamper2

    char ttlData[MAX_STRING];               // Store TTL data string
    int  ttlDataSize;                       // Stores the TTL data string size

} deviceData;

/* Sructure for the SIP server configuration                                  */
typedef struct sipServerConfig_{
	char sipServerIP[IP_STRING_LEN];           // SIP server IP Address
	char sipPort[PORT_STRING_LEN];             // SIP server SIP port
	char sipServerUserID[MAX_STRING];          // SIP user ID
	char sipServerPass[MAX_STRING];            // SIP Password
	bool sipAutoAnsFlag;                       // SIP Call Autoanswer flag
    bool sipTCPTransport;                      // SIP transport flag


    char gpioLogServerIP[IP_STRING_LEN];       // SIP server IP Address
	char gpioLogServerTimeSyncPath[MAX_STRING];    // SIP server get time API path
	char gpioLogServeGpioLogPath[MAX_STRING];      // SIP server GPIO data API path
	char gpioLogServeSecKey[MAX_STRING];           // SIP server secrete key RESTAPI
    int  gpioLogServerRestAPIPort;                 // SIP server RESTAPI port

} sipServerConfig;

/* Over all applicatoin/device structure                                      */
typedef struct deviceInfo_{

    // Device information
    char deviceModel[MAX_STRING];               // Stores device name
    char hardwareVersion[MAX_STRING];           // Stores hardware version
    char softwareVersion[MAX_STRING];           // Stores software version
    char deviceDescription[MAX_STRING];         // Stores device Description
    int spkVolume;                              // Speaker volume
    int micVolume;                              // mic volume

    // Network config
    char deviceMac[MAX_STRING];                 // Device MAC address
    char deviceIPAddr[MAX_STRING];              // Device IP address
    char deviceGateway[MAX_STRING];             // Device network gateway
    char deviceSubnet[MAX_STRING];              // Device network subnet mask
    char deviceDns[MAX_STRING];                 // Device networn dns

    // SIP server config
	sipServerConfig primaryServerConfig;   // Primary SIP server configuration
    sipServerConfig secondaryServerConfig; // Secondary SIP server configuration

    // UART log
    int timeInterval;                   // Time interval to send data
    char queryChar;                     // TTL query command char
    char startChar;                     // TTL data start char
    char endChar;                       // TTL data end char

    // Press to talk
    int serverDialNum;                  // Default server SIP dial number
    int audPlayCnt;                     // Count to play error audio file
    bool    isServerAvailable;          // Flag to indicate primary or secondary server available or not
    bool    isPrimary;                  // Flag to know primary server status
    bool    localDataFlag;              // Flag indicate local data log file has data
    bool    localDataSendOnFlag;        // Flag indicate local data sending is on
    bool    ethConnectionFlag;          // Flag indicate network cable is connected
    bool    sipCallStatus;              // Flag to check SIP call going on or not
    bool    dialTrue;                   // Flag to indicate dialing to default number
    bool    secondaryServer;            // Flag to indiecate secondary server configured or not
}deviceInfo;

/************************
 *	 Function
 *	Declarations
 ************************/

/* Function to write SIP configutation string to baresip app configuration    */
int updateBareSIPConfig(sipServerConfig* sipPrimaryServerConfig,
                        sipServerConfig* sipSecondaryServerConfig);

/* Function to creat SIP configutation string for servers                     */
int makeSIPConfigStr(char* resultStr, sipServerConfig* sipServerConfig);

/* Function to write SIP configutation string to baresip app configuration    */
void restartSIPApp(void);

/* Function to make SIP call to primary server number                         */
void makeSIPCall(int dialNo);

/* Function to write SIP configutation string to baresip app configuration    */
void killSIPApp(void);

/* Function to read the UART data                                             */
int readTTLData(char* dataBuffer, int* iSize);

/* Get SIP application PID to kill sip application                            */
int getSIPAppPID(void);

/* Get Server side time and set as local time                                 */
int getServerTime(char* serverIP, int serverPort, char* serverPath);

/* Set */
extern char readGPIO(char *gpioNumber);
extern int GpioInitialization(void);
extern int onGPIO(char *gpioNumber);
extern int offGPIO(char *gpioNumber);

typedef struct aites_call  {
	int reg1State;
	int reg2State;
	struct ua *uac;
};


#endif	/* _INCLUDE_AITES_H */

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


