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
* File Name    :        getServerTime.c
*
* Description  :	 	File for Functions to rest api to get server time
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

/*******************************************************************************
 * Function:	getServerTime
 *
 * Description:	This function call the server API and get the server time value
 *              it also set the system time as per server time
 *
 * Parameters :
 *		char*	 serverIP     - Pointer to the server IP string
 *		int		 serverPort   - Server port address for API callling
 *		char*	 serverPath   - Server side get time API path
 *
 * Return Value:
 * Return Value:
 *		int   - RETURN_OK   - Successful time set done
 *              RETURN_ERROR- Error from server or any other error
 ******************************************************************************/
int getServerTime(char* serverIP, int serverPort, char* serverPath)
{
    int errCode;                // store error code to return
    int portno = serverPort;    // Server port
    char *host = serverIP;      // Server IP

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total, message_size;
    char *message, response[MAX_DATA_LEN];

    // Check the input arguments
    if(( serverIP == NULL) || (serverPort < 0) || (serverPath == NULL))
    {
        printf("Invalid GET request parameters \n");
		errCode = RETURN_ERROR;
		return errCode;
    }

    // Calculate the message length
    message_size=0;
    message_size+=strlen("%s %s%s%s HTTP/1.0\r\n");      // method
    message_size+=strlen("GET");                         // path
    message_size+=strlen(serverPath);                    // headers
    message_size+=strlen("\r\n");                        // blank line

    // allocate memory for the message
    message=malloc(message_size);

    // fill in the parameters
    sprintf(message,"%s %s HTTP/1.0\r\n",
            "GET",                                        // method
            serverPath);                                  // path
            strcat(message,"\r\n");                       // blank line

    // What are we going to send
    printf("Request:\n%s\n",message);

    // create the socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)     if (sockfd < 0)
    {
        printf("ERROR opening socket\n");
        free(message);
        return RETURN_ERROR;
    }

    // lookup the ip address
    server = gethostbyname(host);
    if (server == NULL)
    {
        printf("ERROR, no such host\n");
        free(message);
        return RETURN_ERROR;
    }


    // fill in the structure
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
//    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);
    inet_pton(AF_INET, host, &serv_addr.sin_addr);
    // connect the socket to server
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        printf("ERROR connecting");
        free(message);
        return RETURN_ERROR;
    }

    // send the get time request
    total = strlen(message);
    sent = 0;
    do {
        bytes = write(sockfd,message+sent,total-sent);
        if (bytes < 0)
        {
            printf("ERROR writing message to socket");
            free(message);
            return RETURN_ERROR;
        }
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    // receive the response
    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    do {
        bytes = read(sockfd,response+received,total-received);
        if (bytes < 0)
        {
            printf("ERROR reading response from socket");
            free(message);
            return RETURN_ERROR;
        }
        if (bytes == 0)
            break;
        received+=bytes;
    } while (received < total);

    if (received == total)
        printf("ERROR storing complete response from socket");

    // close the socket
    close(sockfd);
    free(message);

    // process response
    printf("Response:\n%s\n",response);

    // process response
    if(received > 0)
    {
        printf("Response:\n%s\n",response);
        if(strstr(response, "HTTP/1.1 200 OK") != NULL)
        {
            printf("OK response received\n");
            char* tempPtr = response;
            char* token;
            char parseStr[2] = "{";
            char dateCmd[MAX_STRING];
            memset(dateCmd, 0x00, MAX_STRING);

           /* reach to time */
           token = strtok(tempPtr, parseStr);
           printf("Token:\n%s\n",token);
           token = strtok(NULL, parseStr);
           printf("Token:\n%s\n",token);
           token = strtok(NULL, ":");
           printf("Token:\n%s\n",token);
           token = strtok(NULL, "T");
           printf("Token:\n%s\n",token);
           strcpy(dateCmd, "date -s '");
           strcat(dateCmd, token + 1);
           strcat(dateCmd, " ");
           printf("dateCmd = %s\n",dateCmd);
           token = strtok(NULL, "\"");
           printf("Token:\n%s\n",token);
           strcat(dateCmd, token);
           strcat(dateCmd, "'");
           printf("dateCmd = %s\n",dateCmd);

           system(dateCmd);
           setenv("TZ", "IST", 1);
           tzset();

           return RETURN_OK;
        }
        else
        {
            printf("Server returned error\n");
            return RETURN_ERROR;
        }
    }

    return RETURN_OK;
}

/*
int main(int argc, char *argv[])
{
    getServerTime("122.169.108.88", 8081, "/sececbwebapi/api/ServerTime");
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
