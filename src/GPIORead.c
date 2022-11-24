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
 * File Name    :  	GPIORead.c
 *
 * Description  :	File containing function to initialize and read GPIO ports
 *
 * History:
 *  OCT/25/2021, UBV,	Created the file.
 *
******************************************************************************/

/****************
 * Include Files
****************/
#include "aites.h"
#include <errno.h>

int setGPIO(char *gpioNumber, int direction);
int GpioInitialization(void);
char readGPIO(char *gpioNumber);
int onGPIO(char *gpioNumber);
int offGPIO(char *gpioNumber);
int readAllGPIOValue(deviceData* deviceData);


/*******************************************************************************
 * Function:    setGPIO
 *
 * Description: This function set the GPIO in read mode
 *
 * Parameters :
 *		char* gpioNumber   - GPIO number to be set in read mode
 *		int   direction    - 0 - In , 1 - Out,  gpio direction
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful setting of GPIO done
 *              RETURN_ERROR- Error while setting GPIO as input
 ******************************************************************************/
int setGPIO(char *gpioNumber, int direction)
{
    int fd, retVal;
    char configStr[MAX_STRING];

    memset(configStr, 0x00, MAX_STRING);

    // export GPIO
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(fd < 0)
    {
        printf ("Failed to GPIO export file error %s\n", strerror (errno));
        return RETURN_ERROR;
    }
    else
    {
        retVal = write(fd, gpioNumber, strlen(gpioNumber));
        if(retVal < 0)
        {
            printf ("Failed to set GPIO %s as export error %s\n",
                     gpioNumber, strerror (errno));
            return RETURN_ERROR;
        }
        close(fd);
    }

    // Configure as input or output as per direction value
    sprintf(configStr,"/sys/class/gpio/gpio%s/direction", gpioNumber);
    fd = open(configStr, O_WRONLY);
    if(fd < 0)
    {
        printf ("Failed to open GPIO %s direction file error %s\n",
                gpioNumber, strerror (errno));
        return RETURN_ERROR;
    }
    else
    {
        if(direction == 0)
            retVal = write(fd, "in", 2);
        else
            retVal = write(fd, "out", 3);
        if(retVal < 0)
        {
            printf ("Failed to write GPIO %s direction error %s\n",
                     gpioNumber, strerror (errno));
            return RETURN_ERROR;
        }
        close(fd);
    }
    return RETURN_OK;
}

/*******************************************************************************
 * Function:    GpioInitialization
 *
 * Description: This function all the GPIO as input type GPIO
 *
 * Parameters : None
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful initialization done
 *              RETURN_ERROR- Error while initialization of GPIOs
 ******************************************************************************/
int GpioInitialization(void)
{
    int retVal = RETURN_OK;

    if (setGPIO(GPIO_1, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_1);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_2, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_2);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_3, 1) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_3);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_4, 1) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_4);
        retVal = RETURN_ERROR;
    }

    if (setGPIO(GPIO_5, 1) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_5);
        retVal = RETURN_ERROR;
    }

    if (setGPIO(GPIO_6, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_7);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_7, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_7);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_8, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_8);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_9, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_9);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_10, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_10);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_11, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_11);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_12, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_12);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_13, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_13);
        retVal = RETURN_ERROR;
    }
    if (setGPIO(GPIO_14, 0) == RETURN_ERROR)
    {
        printf("Not able to config GPIO %s\n", GPIO_14);
        retVal = RETURN_ERROR;
    }

    return retVal;

}

/*******************************************************************************
 * Function:    readGPIO
 *
 * Description: This function read the current value of GPIO
 *
 * Parameters :
 *		char* gpioNumber   - GPIO number to be set in read mode
 *
 * Return Value:
 *		char   - value of GPIO (0 or 1) on successful execution
 *              255 in case of error
 ******************************************************************************/
char readGPIO(char *gpioNumber)
{
    int fd, retVal;
    char configStr[MAX_STRING];
    char value;

    memset(configStr, 0x00, MAX_STRING);

    // Read the GPIO value file
    sprintf(configStr,"/sys/class/gpio/gpio%s/value", gpioNumber);
    fd = open(configStr, O_RDONLY | O_SYNC);
    if(fd < 0)
    {
        printf ("Failed to open GPIO %s value file error %s\n",
                gpioNumber, strerror (errno));
        return RETURN_ERROR;
    }
    else
    {
        retVal = read(fd, &value, 1);
        if(retVal < 0)
        {
            printf ("Failed to read GPIO %s  error %s\n",
                     gpioNumber, strerror (errno));
            value = (char)0xFF;
            return RETURN_ERROR;
        }
        close(fd);
    }
    return value;
}

/*******************************************************************************
 * Function:    onGPIO
 *
 * Description: This function set value 1 to GPIO
 *
 * Parameters :
 *		char* gpioNumber   - GPIO number to be set in read mode
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful setting of GPIO on
 *              RETURN_ERROR- Error while setting GPIO as on
 ******************************************************************************/
int onGPIO(char *gpioNumber)
{
    int fd, retVal;
    char configStr[MAX_STRING];

    memset(configStr, 0x00, MAX_STRING);

//    printf("ON---------------------GPIO %s\n", gpioNumber);
    // Read the GPIO value file
    sprintf(configStr,"/sys/class/gpio/gpio%s/value", gpioNumber);
    fd = open(configStr, O_WRONLY | O_SYNC);
    if(fd < 0)
    {
        printf ("Failed to open GPIO %s value file error %s\n",
                gpioNumber, strerror (errno));
        return RETURN_ERROR;
    }
    else
    {
        retVal = write(fd, "1", 1);
        if(retVal < 0)
        {
            printf ("Failed to turn on GPIO %s  error %s\n",
                     gpioNumber, strerror (errno));
            close(fd);
            return RETURN_ERROR;
        }
        close(fd);
    }
    return RETURN_OK;
}

/*******************************************************************************
 * Function:    offGPIO
 *
 * Description: This function set value 0 to GPIO
 *
 * Parameters :
 *		char* gpioNumber   - GPIO number to be set in read mode
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful setting of GPIO off
 *              RETURN_ERROR- Error while setting GPIO as off
 ******************************************************************************/
int offGPIO(char *gpioNumber)
{
    int fd, retVal;
    char configStr[MAX_STRING];

    memset(configStr, 0x00, MAX_STRING);

//    printf("OFF---------------------GPIO %s\n", gpioNumber);
    // Read the GPIO value file
    sprintf(configStr,"/sys/class/gpio/gpio%s/value", gpioNumber);
    fd = open(configStr, O_WRONLY | O_SYNC);
    if(fd < 0)
    {
        printf ("Failed to open GPIO %s value file error %s\n",
                gpioNumber, strerror (errno));
        return RETURN_ERROR;
    }
    else
    {
        retVal = write(fd, "0", 1);
        if(retVal < 0)
        {
            printf ("Failed to turn off GPIO %s  error %s\n",
                     gpioNumber, strerror (errno));
            close(fd);
            return RETURN_ERROR;
        }
        close(fd);
    }
    return RETURN_OK;
}


/*******************************************************************************
 * Function:    readAllGPIOValue
 *
 * Description: This function read all input GPIOs values and store in the
 *              device data structure.
 *
 * Parameters :
 *      deviceData* deviceData - Device data structure to store gpio values
 *
 * Return Value:
 *		int   - RETURN_OK   - Successful initialization done
 *              RETURN_ERROR- Error while initialization of GPIOs
 ******************************************************************************/
int readAllGPIOValue(deviceData* devData)
{
    int retVal = RETURN_OK;
    char gpioVal;

    gpioVal = readGPIO(GPIO_1);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio1Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_1);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_2);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio2Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_2);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_6);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio6Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_6);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_7);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio7Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_7);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_8);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio8Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_8);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_9);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio9Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_9);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_10);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio10Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_10);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_11);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio11Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_11);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_12);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio12Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_12);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_13);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio13Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_13);
        retVal = RETURN_ERROR;
    }

    gpioVal = readGPIO(GPIO_14);
    if (gpioVal != (char)0xFF)
    {
        devData->gpio14Val = gpioVal;
    }
    else
    {
        printf("Not able to read GPIO %s value\n", GPIO_14);
        retVal = RETURN_ERROR;
    }

    return retVal;
}

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
