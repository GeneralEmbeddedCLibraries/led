// Copyright (c) 2021 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     led.h
* @brief    LED manipulations
* @author   Ziga Miklosic
* @date     08.04.2021
*/
////////////////////////////////////////////////////////////////////////////////
/**
* @addtogroup LED_API
* @{ <!-- BEGIN GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __LED_H_
#define __LED_H_

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include "project_config.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *  LED state
 */
typedef enum
{
    eLED_OFF = 0,   /**<LED off state */
    eLED_ON         /**<LED on state */
} led_state_t;

/**
 *  LED
 */
typedef enum
{
    eLED_L = 0,	/**<Left LED */
    eLED_R,		/**<Right LED */

    eLED_NUM_OF
} led_num_t;

/**
 *  Blink mode
 */
typedef enum
{
	eLED_BLINK_1X = 0,
	eLED_BLINK_2X,
	eLED_BLINK_3X,
	eLED_BLINK_4X,
	eLED_BLINK_5X,
	eLED_BLINK_CONTINUOUS,

	eLED_BLINK_NUM_OF
} led_blink_t;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
void 		led_init   			(void);
void 		led_hndl			(void);
void 		led_set    			(const led_num_t num, const led_state_t state);
void 		led_set_smooth		(const led_num_t num, const led_state_t state);
void 		led_blink			(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink);
void 		led_blink_smooth	(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink);
void 		led_set_cfg			(const led_num_t num, const float32_t fade_in_time, const float32_t fade_out_time, const float32_t max_duty);
float32_t	led_get_active_time	(const led_num_t num);

#endif // __LED_H_
////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////