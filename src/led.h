// Copyright (c) 2025 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     led.h
* @brief    LED manipulations
* @author   Ziga Miklosic
* @email    ziga.miklosic@gmail.com
* @date     01.02.2025
* @version  V1.2.0
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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../led_cfg.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     Module version
 */
#define LED_VER_MAJOR       ( 1 )
#define LED_VER_MINOR       ( 2 )
#define LED_VER_DEVELOP     ( 0 )

// Float definition
typedef float float32_t;

/**
 *     LED status
 */
typedef enum
{
    eLED_OK         = 0x00U,        /**<Normal operation */
    eLED_ERROR      = 0x01U,        /**<General error */
    eLED_ERROR_INIT = 0x02U,        /**<Initialization error */
} led_status_t;

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

#if ( 1 == LED_CFG_TIMER_USE_EN )

    /**
     *     LED fade configuration
     */
    typedef struct
    {
        float32_t fade_in_time;     /**<Fade in time */
        float32_t fade_out_time;    /**<Fade out time */
        float32_t max_duty;         /**<Maximum duty cycle in % */
        float32_t min_duty;         /**<Minimum duty cycle in % */
    } led_fade_cfg_t;

#endif

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
led_status_t led_init               (void);
led_status_t led_deinit             (void);
led_status_t led_is_init            (bool * const p_is_init);
led_status_t led_hndl               (void);
led_status_t led_set                (const led_num_t num, const led_state_t state);
led_status_t led_set_full           (const led_num_t num, const led_state_t state);
led_status_t led_toggle             (const led_num_t num);
led_status_t led_blink              (const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink);
led_status_t led_get_active_time    (const led_num_t num, float32_t * const p_active_time);
led_status_t led_is_idle            (const led_num_t num, bool * const p_is_idle);
led_status_t led_is_on              (const led_num_t num, bool *const p_is_on);
led_status_t led_set_on_brightness  (const led_num_t num, const float32_t duty_cycle);
led_status_t led_set_off_brightness (const led_num_t num, const float32_t duty_cycle);
led_status_t led_get_duty           (const led_num_t num, float32_t * const p_duty_cycle);

#if ( 1 == LED_CFG_TIMER_USE_EN )
    led_status_t led_set_smooth             (const led_num_t num, const led_state_t state);
    led_status_t led_blink_smooth           (const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink);
    led_status_t led_set_fade_cfg           (const led_num_t num, const led_fade_cfg_t * const p_fade_cfg);
    bool         led_is_in_smooth_blink_mode(const led_num_t num);
#endif

#endif // __LED_H_

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
