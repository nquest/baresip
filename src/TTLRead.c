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
 * File Name    :  	TTLRead.c
 *
 * Description  :	File containing function to read Solarpanel TTL data
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
#include <sys/time.h>
#include "aites.h"

int setInterfaceAttribs (int fd, int speed, int parity);
int setBlocking (int fd, int shouldBlock);
int readTTLData(char* dataBuffer, int* iSize);


/*******************************************************************************
 * Function:    setInterfaceAttribs
 *
 * Description: This function set the serial port attributes its baudrate,
 *              parity
 *
 * Parameters :
 *		int 	 fd         - Serial port file handle
 *		int		 speed      - Server port baudrate
 *		int		 parity     - Server port parity setting
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful serial port attribute set
 *              RETURN_ERROR- Error on seria port attribute set failure
 ******************************************************************************/
int setInterfaceAttribs (int fd, int speed, int parity)
{
    struct termios tty;
    if (tcgetattr (fd, &tty) != 0)
    {
            printf ("error %d from tcgetattr", errno);
            return RETURN_ERROR;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
//    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
            printf ("error %d from tcsetattr", errno);
            return RETURN_ERROR;
    }
    return RETURN_OK;
}

/*******************************************************************************
 * Function:    setBlocking
 *
 * Description: This function set the serial port blocking mode attributes
 *
 * Parameters :
 *		int 	 fd         - Serial port file handle
 *		int		 shouldBlock- Server port blocking flag 1 for block and
 *                            0 non blocking
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful blocking mode set
 *              RETURN_ERROR- Error while settiong blocking mode config
 ******************************************************************************/
int setBlocking (int fd, int shouldBlock)
{
        struct termios tty;
        memset (&tty, 0, sizeof tty);
        if (tcgetattr (fd, &tty) != 0)
        {
                printf ("error %d from tggetattr", errno);
                return RETURN_ERROR;
        }

        tty.c_cc[VMIN]  = shouldBlock ? 1 : 0;
        tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

        if (tcsetattr (fd, TCSANOW, &tty) != 0)
        {
            printf ("error %d setting term attributes", errno);
            return RETURN_ERROR;
        }
    return RETURN_OK;
}

/*******************************************************************************
 * Function:    readTTLData
 *
 * Description: This function read the TTL data from serial port
 *
 * Parameters :
 *		char * 	 dataBuffer - Return bytes read from serial port
 *		int *	 iSize      - Return number of bytes read from serial port
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful data read
 *              RETURN_ERROR- Error while reading TTL data
 ******************************************************************************/
int readTTLData(char* dataBuffer, int* iSize)
{
    int retVal, retCnt, lpCnt;
    int startFlag = 0;
    retCnt = 0;

    if( dataBuffer == NULL )
    {
        printf("Invalid data buffer passed \n");
		return RETURN_ERROR;
    }


    int fd = open (UART_PORT_NAME, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        printf ("error %d opening %s: %s", errno, UART_PORT_NAME,
                 strerror (errno));
        return RETURN_ERROR;
    }

    // set speed to 115,200 bps, 8n1 (no parity)
    retVal = setInterfaceAttribs (fd, UART_BAUDRATE , 0);
    if (retVal == RETURN_ERROR)
    {
        printf ("error setting serial port buadrate and parity\n");
        close(fd);
        return RETURN_ERROR;
    }

    // set no blocking
    retVal = setBlocking (fd, 0);
    if (retVal == RETURN_ERROR)
    {
        printf ("error setting serial port buadrate and parity\n");
        close(fd);
        return RETURN_ERROR;
    }

    // send read command
    retVal = write (fd, UART_MSG_CMD, 1);
    if(retVal < 0)
    {
        printf ("Failed to write on serial port %s error %s\n",
                 UART_PORT_NAME, strerror (errno));
        close(fd);
        return RETURN_ERROR;
    }

    printf("* command sent to panel \n");
    lpCnt = 0;
//    usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus
//                                         // receive 25:  approx 100 uS per char transmit
    // Loop to read data from serial port
    while(startFlag == 0)
    {
        char ch;
        retVal = read (fd, &ch, 1);        // read char by char to check start
        if(retVal < 0)
        {
            printf ("Failed to read serial port data from %s  error %s\n",
                     UART_PORT_NAME, strerror (errno));
            close(fd);
            return RETURN_ERROR;
        }
//        printf("---%c\n", ch);
        // Check start cmd
        if(ch == '#')
        {
            startFlag = 1;
            printf("Start code received\n");
            break;
        }
        else
//            sys_usleep(100);
            usleep(100);

        lpCnt++;
        if(lpCnt >= 2)
        {
            printf("Start code not received so trying again\n");
            close(fd);
            return RETURN_ERROR;
        }

    }

    lpCnt = 0;
    while (startFlag == 1)
    {
        char ch;
        retVal = read (fd, &ch, 1);  // read char by char to check end
        if(retVal < 0)
        {
            printf ("Failed to read serial port data from %s  error %s\n",
                     UART_PORT_NAME, strerror (errno));
            close(fd);
            return RETURN_ERROR;
        }

//        printf("---%c\n", ch);
        // Check end cmd
        if(ch == '@')
        {
            startFlag = 2;
            break;
        }
        else
        {
            dataBuffer[retCnt++] = ch;
//            printf("data = %s ---- size = %d\n", dataBuffer, *iSize);
            *iSize = retCnt;
        }
    }

    close(fd);
    return RETURN_OK;
}
/*

int main(int argc, char *argv[])
{
    char data[100];
    int size = 0;
    memset(data, 0x00, 100);

    if(readTTLData(data, &size) == RETURN_ERROR)
    {
        printf ("Error while reading TTL data values \n");
    }
    else
    {
        printf ("Read data = %s \n", data);
        printf ("Read data size = %d \n", size);
    }
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
