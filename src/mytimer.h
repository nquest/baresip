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
#ifndef TIME_H
#define TIME_H
#include <stdlib.h>
#include <unistd.h>

typedef enum
{
TIMER_SINGLE_SHOT = 0,
TIMER_PERIODIC
} t_timer;

typedef void (*time_handler)(size_t timer_id, void * user_data);

int     initialize(void);
size_t  start_timer(unsigned int interval, time_handler handler, t_timer type, void * user_data);
void    stop_timer(size_t timer_id);
void    finalize(void);

#endif

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
