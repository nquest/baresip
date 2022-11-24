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
 * File Name    :  	sendGPIOData.c
 *
 * Description  :	File containing function to send log data to server
 *
 * History:
 *  OCT/25/2021, UBV,	Created the file.
 *
 ******************************************************************************/

/****************
 * Include Files
****************/
#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit, atoi, malloc, free */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include "aites.h"

#define LASTLOGFILE_NAME   "/www/lastlog"  // Config file name


int storeDataToLocalLogFile(char* dataStr);
int sendDataToServer(char* serverIP, int serverPort, char* serverPath,
                     char* dataStr, char*keyHeader);
int makeDataStrAndSend(deviceData* devData, deviceInfo* devConfig);

/*******************************************************************************
 * Function:    storeDataToLocalLogFile
 *
 * Description: This function store the log data to local log file when
 *              connectivity is down
 *
 * Parameters :
 *		char* dataStr      - Logging data
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful log data sent to server
 *              RETURN_ERROR- Error while sending log data to server
 ******************************************************************************/
int storeDataToLocalLogFile(char* dataStr)
{
    // creating file pointer to work with files
    FILE *fptr;
    int   retVal;

    // opening file in writing mode
    fptr = fopen(LOCAL_DATALOG_FILE, "a");
    if (fptr == NULL)
    {
        printf ("error %d opening %s: %s", errno, LOCAL_DATALOG_FILE,
                 strerror (errno));
        return RETURN_ERROR;
    }

    // Write data to local log file
    retVal = fprintf(fptr, "%s\n", dataStr);
    if(retVal < 0)
    {
        printf ("error %d writing log data string to local log file %s: %s", errno,
                LOCAL_DATALOG_FILE, strerror (errno));
        fclose(fptr);
        return RETURN_ERROR;
    }

    fclose(fptr);
    return RETURN_OK;
}

/*******************************************************************************
 * Function:    sendDataToServer
 *
 * Description: This function prepare POST request and send it to server
 *
 * Parameters :
 *		char* serverIP     - Datalog server IP address
 *		int	  serverPort   - Datalog server port
 *		char* serverPath   - Datalog server side API path
 *		char* dataStr      - Logging data
 *		char* keyHeader    - Datalog server keyheader
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful log data sent to server
 *              RETURN_ERROR- Error while sending log data to server
 ******************************************************************************/
int sendDataToServer(char* serverIP, int serverPort, char* serverPath,
                     char* dataStr, char*keyHeader)
{
    int errCode;

    if(( serverIP == NULL) || (serverPort < 0) || (serverPath == NULL))
    {
        printf("Invalid GET request parameters \n");
		errCode = RETURN_ERROR;
		return errCode;
    }

    // first where are we going to send it?
    int portno = serverPort;
    char *host = serverIP;

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total, message_size;
    char *message, response[4096];

    // How big is the message?

    message_size=0;
    message_size+=strlen("%s %s HTTP/1.0\r\n");
    message_size+=strlen("POST");                         // method
    message_size+=strlen(serverPath);                     // path
    message_size+=strlen("seckey:");
    message_size+=strlen(keyHeader);                      // keyHeader
    message_size+=strlen("content-Type:application/json");// Content type
    message_size+=strlen("Content-Length: %d\r\n")+10;    // content length
    message_size+=strlen("\r\n");                         // blank line
    message_size+=strlen(dataStr);                        // body

//    printf("message_size = %d\n", message_size);
    // allocate space for the message
    message=malloc(message_size);

    memset(message, 0x00, message_size);
//    printf("serverPathe = %s\n", serverPath);

    // fill in the parameters
    sprintf(message,"%s %s HTTP/1.0\r\n",
        "POST",                  // method
        serverPath);             // path


    // headers
     strcat(message,"seckey:");
     strcat(message,keyHeader);
     strcat(message,"\r\n");
     strcat(message,"content-Type:application/json");
     strcat(message,"\r\n");

    sprintf(message+strlen(message),"Content-Length: %d\r\n", (int)strlen(dataStr));
    strcat(message,"\r\n");
    strcat(message,dataStr);                           //  body

    // What are we going to send?
//    printf("Request:\n%s\n",message);

    // create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        printf("ERROR opening socket\n");
         return RETURN_ERROR;
    }

    // lookup the ip address
    server = gethostbyname(host);
    if (server == NULL)
    {
        printf("ERROR, no such host\n");
         return RETURN_ERROR;
    }

    // fill in the structure
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
//    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
    inet_pton(AF_INET, host, &serv_addr.sin_addr);

    // connect the socket
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting");
         return RETURN_ERROR;
    }

    // send the request
    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sockfd,message+sent,total-sent);
        if (bytes < 0)
            printf("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    // receive the response
    memset(response,0x00,4096);
    total = sizeof(response)-1;
    received = 0;
    do {
        bytes = read(sockfd,response+received,total-received);
        if (bytes < 0)
            printf("ERROR reading response from socket");
        if (bytes == 0)
            break;
        received+=bytes;
    } while (received < total);

    if (received == total)
    {
         printf("ERROR storing complete response from socket");
    }

    // close the socket
    close(sockfd);

    // process response
    if(received > 0)
    {
//        printf("Response:\n%s\n",response);
        free(message);

        if(strstr(response, "HTTP/1.1 200 OK") != NULL)
        {
            printf("OK response received\n");
            return RETURN_OK;
        }
        else
        {
            printf("Server returned error\n");
            return RETURN_ERROR;
        }
    }
    return RETURN_ERROR;
}

/*******************************************************************************
 * Function:    makeDataStrAndSend
 *
 * Description: This function prepare log data string and send it to server
 *
 * Parameters :
 *		deviceData* devData     - global Device data structure ptr to get data
 *                                value
 *		deviceInfo* devConfig   - global device configuration structure ptr
 *
 * Return Value:
 *		int   - RETURN_OK   - Successfully send data log to server
 *              RETURN_ERROR- Error on sending log data to server
 ******************************************************************************/
int makeDataStrAndSend(deviceData* devData, deviceInfo* devConfig)
{
    char dataStr[1000];
    char timeValStr[32];
    time_t t;
    struct tm * timeinfo;
    sipServerConfig* sipSrvrConfig;
    FILE * fp;
    char cmdOutput[MAX_STRING];
    int x; /*,y;*/

    // get current time
    time(&t);
    timeinfo = localtime(&t);

    memset(dataStr, 0x00, 1000);
    memset(timeValStr, 0x00, 32);

    // Prepare date time string as per server required format
    sprintf(timeValStr, "%d-%02d-%02dT%02d:%02d:%02d",timeinfo->tm_year + 1900,
            timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour,
            timeinfo->tm_min, timeinfo->tm_sec);

    // Prepare log data string
//    sprintf(dataStr, "{\"macid\":\"%s\",\"data\":{\"din1\":%c,\"din2\":%c,\"din3\":%c,\"din4\":%c,\"din5\":%c,\"dout1\":%c,\"dout2\":%c,\"dout3\":%c,\"din6\":%c,\"din7\":%c,\"din8\":%c,\"din9\":%c,\"din10\":%c,\"din11\":%c,\"psu\": \"%s\",\"rssi\":0,\"logdate\":\"%s\"}}",
//    devConfig->deviceMac, devData-> gpio9Val, devData-> gpio10Val, devData-> gpio7Val, devData-> gpio8Val, devData-> gpio2Val, devData-> gpio3Val,
//    devData-> gpio4Val, devData-> gpio5Val, devData-> gpio1Val, devData-> gpio6Val, devData-> gpio11Val, devData-> gpio12Val, devData-> gpio13Val,
//    devData-> gpio14Val, devData->ttlData, timeValStr);

    // Prepare log data string
    sprintf(dataStr, "{\"macid\":\"%s\",\"data\":{\"din1\":%c,\"din2\":%c,\"din3\":%c,\"din4\":%c,\"psu\": \"%s\",\"rssi\":0,\"logdate\":\"%s\"}}",
    devConfig->deviceMac, devData-> gpio9Val, devData-> gpio10Val, devData-> gpio7Val, devData-> gpio8Val, devData->ttlData, timeValStr);

//    printf("Send data:\n %s\n", dataStr);

//URMIL    if(devConfig->isServerAvailable)
    {
        // Check server ping for primary server
        memset(cmdOutput, 0x00, MAX_STRING);
        sprintf(cmdOutput, "ping -c 1 -w 1 %s > /dev/null 2>&1",
                            devConfig->primaryServerConfig.gpioLogServerIP);
//        printf("%s\n",cmdOutput );
        x = system(cmdOutput);
//        printf("x =  %d\n", x);
        if(x == 0)
        {
            sipSrvrConfig = &devConfig->primaryServerConfig;
            printf("From Primary\n");
        }
        else
        {
            sipSrvrConfig = &devConfig->secondaryServerConfig;
            printf("From Secondary\n");
        }

        if(sendDataToServer(sipSrvrConfig->gpioLogServerIP,
                            sipSrvrConfig->gpioLogServerRestAPIPort,
                            sipSrvrConfig->gpioLogServeGpioLogPath,
                            dataStr,
                            sipSrvrConfig->gpioLogServeSecKey ) == RETURN_ERROR)
        {
            printf("Error returned from server while sending data to server\n");
            // Dont loose the data store it in the local log data file
            if(storeDataToLocalLogFile(dataStr) == RETURN_ERROR)
            {
                printf("Error in writing log data to local log file we will loose this data sample\n");
            }
            else
            {
                printf("Data is stored in local log file as there is error while sending data to server\n");
                devConfig->localDataFlag = true;
            }
            return RETURN_ERROR;
        }
        else
        {
            printf("Data Sent successfully so update last log file\n");
            if((fp = fopen(LASTLOGFILE_NAME, "w")) == NULL) 
            {
                printf("Error in opening lastlog file\n");
            }
            else
            {
                // Write configuration to the file.
                fprintf(fp, "%s",dataStr);
                fclose(fp);
            }
        }
    }
//    else
//    {
//        // Server is not available so store data in to local file.
//        if(storeDataToLocalLogFile(dataStr) == RETURN_ERROR)
//        {
//            printf("Error in writing log data to local log file we will loose this data sample\n");
//        }
//        else
//        {
//            printf("Data is stored in local log file as there is error while sending data to server\n");
//            devConfig->localDataFlag = true;
//        }
//    }

    return RETURN_OK;
}


//int sendDataToServer(char* serverIP, int serverPort, char* serverPath, char* dataStr, char*keyHeader)
/*
int main(int argc, char *argv[])
{
    sendDataToServer("122.169.108.88", 8081, "/sececbwebapi/api/GpioLogs",  "{\"macid\":\"abcd\",\"data\":{\"din1\":0,\"din2\":0,\"din3\":0,\"din4\":0,\"psu\":\"\",\"rssi\":0,\"logdate\":\"2021-09-29T15:18:00\"}}", "seckey:sec322" );
}
 */
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
