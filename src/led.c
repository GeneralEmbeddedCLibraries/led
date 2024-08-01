// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     led.c
* @brief    LED manipulations
* @author   Ziga Miklosic
* @email    ziga.miklosic@gmail.com
* @date     08.11.2023
* @version  V1.1.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
 * @addtogroup LED
 * @{ <!-- BEGIN GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "led.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *     LED handler period in second and frequency in Hz
 */
#define LED_HNDL_PERIOD_S                   ( LED_CFG_HNDL_PERIOD_MS / 1000.0f )
#define LED_HNDL_FREQ_HZ                    ((float32_t) ( 1.0 / PWR_HNDL_PERIOD_S ))

/**
 *     Default fade IN/OUT time
 *
 * @note Resolution of LED_HNDL_PERIOD_S
 *
 *     Unit: sec
 */
#define LED_FADE_IN_TIME_S                  ( 1.0 )
#define LED_FADE_OUT_TIME_S                 ( 1.0 )

/**
 *      Default fading factor
 *
 *      @note Times two is becase of x^2 derivative is 2x
 *
 *      Unit: duty [0-1.0]
 */
#define LED_FADE_IN_COEF_T_TO_DUTY          (float32_t) ( 2.0 * 1.0 / ( LED_FADE_IN_TIME_S * LED_FADE_IN_TIME_S ))
#define LED_FADE_OUT_COEF_T_TO_DUTY         (float32_t) ( 2.0 * 1.0 / ( LED_FADE_OUT_TIME_S * LED_FADE_OUT_TIME_S ))

/**
 *     Time keeping limitations
 */
#define LED_TIME_LIMIT_S                    ( 1E6f )
#define LED_TIME_LIM(time)                  (( time > LED_TIME_LIMIT_S ) ? ( LED_TIME_LIMIT_S ) : ( time ))

/**
 *     Blink counter continuous code
 */
#define LED_BLINK_CNT_CONT_VAL              ((uint8_t) ( 0xFFU ))

/**
 *     LED mode
 */
typedef enum
{
    eLED_MODE_NORMAL = 0,       /**<Normal mode */
    eLED_MODE_FADE_IN,          /**<Fade-in mode */
    eLED_MODE_FADE_OUT,         /**<Fade-out mode */
    eLED_MODE_FADE_TOGGLE,      /**<Fade-in-out continuously */
    eLED_MODE_BLINK,            /**<Blink mode */
    eLED_MODE_FADE_BLINK,       /**<Blink mode with fading */

    eLED_MODE_NUM_OF
} led_mode_t;

/**
 *     LED data
 */
typedef struct
{
    float32_t   duty;           /**<Duty cycle of LED */
    float32_t   max_duty;       /**<Maximum duty cycle of LED in % */
    float32_t   min_duty;       /**<Minumum duty cycle of LED in % */
    float32_t   fade_time;      /**<Time for fading functionalities */
    float32_t   fade_in_k;      /**<Fade in factor */
    float32_t   fade_out_k;     /**<Fade out factor */
    float32_t   fade_out_time;  /**<Time for fading out */
    float32_t   period;         /**<Period of toggle mode */
    float32_t   per_time;       /**<Period time keeping */
    float32_t   on_time;        /**<On time for blink mode */
    float32_t   active_time;    /**<LED active time - turned ON time */
    led_mode_t  mode;           /**<Current LED mode */
    uint8_t     blink_cnt;      /**<Blink LED live counter */
} led_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *     LED data
 */
static led_t g_led[eLED_NUM_OF] = { 0 };

/**
 *     Initialization guard
 */
static bool gb_is_init = false;

/**
 *     Pointer to configuration table
 */
static const led_cfg_t * gp_cfg_table = NULL;

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
static void         led_fade_in_hndl        (const led_num_t num, const led_mode_t exit_mode);
static void         led_fade_out_hndl       (const led_num_t num, const led_mode_t exit_mode);
static void         led_blink_hndl          (const led_num_t num);
static void         led_fade_blink_hndl     (const led_num_t num);
static void         led_hndl_period_time    (const led_num_t num);
static bool         led_is_on_time          (const led_num_t num);
static bool         led_is_period_time      (const led_num_t num);
static void         led_blink_cnt_hndl      (const led_num_t num);
static void         led_manage_time         (const led_num_t num);
static led_status_t led_init_drv            (void);
static void         led_set_gpio            (const led_num_t led_num, const float32_t duty, const float32_t duty_max);
static void         led_set_timer           (const led_num_t led_num, const float32_t duty);
static void         led_set_low             (const led_num_t led_num, const float32_t duty, const float32_t duty_max);

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*       Manage LED timings
*
* @param[in]    num     - Number of LED
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_manage_time(const led_num_t num)
{
    if ( g_led[num].duty >= ( g_led[num].max_duty / 2.0f ))
    {
        g_led[num].active_time += LED_HNDL_PERIOD_S;
        g_led[num].active_time = LED_TIME_LIM( g_led[num].active_time );
    }
    else
    {
        g_led[num].active_time = 0.0f;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Fade in FMS state
*
* @param[in]    num         - LED number
* @param[in]    exit_mode   - Mode to transition on exit
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_fade_in_hndl(const led_num_t num, const led_mode_t exit_mode)
{
    // Increase duty by the square function
    g_led[num].duty += ( g_led[num].fade_in_k * g_led[num].fade_time * (float32_t) LED_HNDL_PERIOD_S );

    // Is LED fully ON?
    if ( g_led[num].duty <= g_led[num].max_duty )
    {
        // Increment time
        g_led[num].fade_time += LED_HNDL_PERIOD_S;
        g_led[num].fade_time = LED_TIME_LIM( g_led[num].fade_time );
    }

    // LED fully ON
    else
    {
        // Limit duty
        g_led[num].duty = g_led[num].max_duty;

        // Reset time
        g_led[num].fade_time = 0.0f;

        // Goto NORMAL mode
        g_led[num].mode = exit_mode;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Fade out FMS state
*
* @param[in]    num            - LED number
* @param[in]    exit_mode    - Mode to transition on exit
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_fade_out_hndl(const led_num_t num, const led_mode_t exit_mode)
{
    float32_t time = 0.0f;

    // Calculate negative time in order to get square characteristics in negative time domain
    time = ( g_led[num].fade_out_time - g_led[num].fade_time );

    // Watch out for end of negative characteristics
    if ( time > 0.0f  )
    {
        g_led[num].duty -= g_led[num].fade_out_k * ( time * (float32_t) LED_HNDL_PERIOD_S );
    }
    else
    {
        g_led[num].duty = g_led[num].min_duty;
    }

    // Is LED fully OFF?
    if ( g_led[num].duty > ( g_led[num].min_duty + 0.001f ))
    {
        // Increment time
        g_led[num].fade_time += LED_HNDL_PERIOD_S;
        g_led[num].fade_time = LED_TIME_LIM( g_led[num].fade_time );
    }

    // LED fully in OFF state (doesn't mean that it is not shining)
    else
    {
        // Limit duty
        g_led[num].duty = g_led[num].min_duty;

        // Reset time
        g_led[num].fade_time = 0.0f;

        // Goto NORMAL mode
        g_led[num].mode = exit_mode;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       LED blink FSM state
*
* @param[in]    num            - LED number
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_blink_hndl(const led_num_t num)
{
    if ( true == led_is_on_time( num ))
    {
        g_led[num].duty = g_led[num].max_duty;
    }
    else
    {
        g_led[num].duty = g_led[num].min_duty;
    }

    // Manage blink counter
    led_blink_cnt_hndl( num );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       LED fade blink FSM state
*
* @param[in]    num            - LED number
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_fade_blink_hndl(const led_num_t num)
{
    if ( true == led_is_on_time( num ))
    {
        led_fade_in_hndl( num , eLED_MODE_FADE_BLINK);
    }
    else
    {
        led_fade_out_hndl( num, eLED_MODE_FADE_BLINK );
    }

    // Manage blink counter
    led_blink_cnt_hndl( num );
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Period time handler
*
* @param[in]    num            - LED number
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_hndl_period_time(const led_num_t num)
{
    if ( g_led[num].per_time >= g_led[num].period )
    {
        g_led[num].per_time = 0.0f;
    }
    else
    {
        g_led[num].per_time += LED_HNDL_PERIOD_S;
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Check if it is time for ON LED state
*
* @param[in]    num            - LED number
* @return       is_on_time    - Is currently LED on or off
*/
////////////////////////////////////////////////////////////////////////////////
static bool led_is_on_time(const led_num_t num)
{
    bool is_on_time = false;

    if ( g_led[num].per_time < g_led[num].on_time )
    {
        is_on_time = true;
    }

    return is_on_time;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Check if it is period of blink
*
*   Return true only on period update
*
* @param[in]    num            - LED number
* @return       is_period    - Period update event
*/
////////////////////////////////////////////////////////////////////////////////
static bool led_is_period_time(const led_num_t num)
{
    bool is_period = false;

    if ( g_led[num].per_time >= g_led[num].period )
    {
        is_period = true;
    }

    return is_period;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Manage blink counter
*
* @param[in]    num        - LED number
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_blink_cnt_hndl(const led_num_t num)
{
    // On blink period
    if ( true == led_is_period_time( num ))
    {
        // Not continuous blinking
        if ( LED_BLINK_CNT_CONT_VAL != g_led[num].blink_cnt )
        {
            // Blink count expire
            if ( 0 == g_led[num].blink_cnt )
            {
                g_led[num].mode = eLED_MODE_NORMAL;
            }

            // Decrease blink counts
            else
            {
                g_led[num].blink_cnt--;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Initialize low lever drivers
*
* @return       status - Status of low level initialization
*/
////////////////////////////////////////////////////////////////////////////////
static led_status_t led_init_drv(void)
{
    led_status_t status = eLED_OK;

    #if ( 1 == LED_CFG_TIMER_USE_EN )

        if ( eTIMER_OK != timer_init() )
        {
            status |= eLED_ERROR_INIT;
        }

    #endif

    #if ( 1 == LED_CFG_GPIO_USE_EN )

        if ( eGPIO_OK != gpio_init() )
        {
            status |= eLED_ERROR_INIT;
        }

    #endif

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Set LED via GPIO driver
*
*  @note    Based on duty cycle LED GPIO state is being determine!
*
* @param[in]    led_num     - Number of LED
* @param[in]    duty        - Current duty of LED
* @param[in]    max_duty    - Maximum duty of LED
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_set_gpio(const led_num_t led_num, const float32_t duty, const float32_t max_duty)
{
    #if ( 1 == LED_CFG_GPIO_USE_EN )

        gpio_state_t state = eGPIO_LOW;

        if ( duty >= max_duty )
        {
            if ( eLED_POL_ACTIVE_LOW == gp_cfg_table[led_num].polarity )
            {
                state = eGPIO_LOW;
            }
            else
            {
                state = eGPIO_HIGH;
            }
        }
        else
        {
            if ( eLED_POL_ACTIVE_LOW == gp_cfg_table[led_num].polarity )
            {
                state = eGPIO_HIGH;
            }
            else
            {
                state = eGPIO_LOW;
            }
        }

        // Set GPIO
        gpio_set( gp_cfg_table[led_num].drv_ch.gpio_pin, state );

    #else
        (void) led_num;
        (void) duty;
        (void) max_duty;
    #endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Set led via TIMER driver
*
* @param[in]    led_num     - Number of LED
* @param[in]    duty        - Current duty of LED in %
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_set_timer(const led_num_t led_num, const float32_t duty)
{
    #if ( 1 == LED_CFG_TIMER_USE_EN )

        float32_t tim_duty = duty;

        // Apply polarity
        if ( eLED_POL_ACTIVE_LOW == gp_cfg_table[led_num].polarity )
        {
            tim_duty = ( 100.0f - duty );

            if ( tim_duty < g_led[led_num].min_duty )
            {
                tim_duty = g_led[led_num].min_duty;
            }
        }

        // Set timer PWM
        timer_pwmo_ch_set( gp_cfg_table[led_num].drv_ch.tim_ch, tim_duty );

    #else
        (void) led_num;
        (void) duty;
    #endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Set LED via low level driver
*
* @param[in]    led_num     - Number of LED
* @param[in]    duty        - Current duty of LED
* @param[in]    max_duty    - Maximum duty of LED
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_set_low(const led_num_t led_num, const float32_t duty, const float32_t max_duty)
{
    // Set timer
    if ( eLED_DRV_TIMER_PWM == gp_cfg_table[led_num].drv_type )
    {
        led_set_timer( led_num, duty );
    }

    // Set GPIO
    else if ( eLED_DRV_GPIO == gp_cfg_table[led_num].drv_type )
    {
        led_set_gpio( led_num, duty, max_duty );
    }

    // Unknown driver
    else
    {
        LED_ASSERT( 0 );
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup LED_API
* @{ <!-- BEGIN GROUP -->
*
*     Following functions are part of API calls.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*       Initialize LEDs
*
* @return   status - Status of initialisation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_init(void)
{
    led_status_t status = eLED_OK;

    // Prevent multiple inits
    if ( false == gb_is_init )
    {
        // Get configuration matrix
        gp_cfg_table = led_cfg_get_table();

        if ( NULL != gp_cfg_table )
        {
            // Initialize low level drivers
            if ( eLED_OK == led_init_drv())
            {
                // Set init success
                gb_is_init = true;

                // Set up live LED configuration
                for ( led_num_t num = 0; num < eLED_NUM_OF; num++ )
                {
                    g_led[num].duty             = 0.0f;     // %
                    g_led[num].max_duty         = 100.0f;   // %
                    g_led[num].min_duty         = 0.0f;     // %
                    g_led[num].fade_time        = 0.0f;
                    g_led[num].fade_in_k        = LED_FADE_IN_COEF_T_TO_DUTY;
                    g_led[num].fade_out_k       = LED_FADE_OUT_COEF_T_TO_DUTY;
                    g_led[num].fade_out_time    = 1.0f;
                    g_led[num].period           = 0.0f;
                    g_led[num].per_time         = 0.0f;
                    g_led[num].on_time          = 0.0f;
                    g_led[num].active_time      = 0.0f;
                    g_led[num].mode             = eLED_MODE_NORMAL;
                    g_led[num].blink_cnt        = 0;

                    // Set LED initial value
                    led_set( num, gp_cfg_table[num].initial_state );
                    led_set_low( num, g_led[num].duty, g_led[num].max_duty );
                }
            }

            // Low level drivers not initialised
            else
            {
                LED_DBG_PRINT( "LED: Low level drivers not initialised error!" );
                LED_ASSERT( 0 );
                status = eLED_ERROR_INIT;
            }
        }
        else
        {
            LED_DBG_PRINT( "LED: Config table unknown error!" );
            LED_ASSERT( 0 );
            status = eLED_ERROR_INIT;
        }
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       De-Initialize LEDs
*
* @return   status - Status of de-init
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_deinit(void)
{
    led_status_t status = eLED_OK;

    if ( true == gb_is_init )
    {
        // Set all LEDs to initial state
        for ( led_num_t num = 0; num < eLED_NUM_OF; num++ )
        {
            led_set( num, gp_cfg_table[num].initial_state );
        }

        // De-init success
        gb_is_init = false;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   Get LED initialisation flag
*
* @param[out]   p_is_init   - Initialisation flag
* @return       status      - Status of initialisation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_is_init(bool * const p_is_init)
{
    led_status_t status = eLED_OK;

    if ( NULL != p_is_init )
    {
        *p_is_init = gb_is_init;
    }
    else
    {
        status = eLED_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       LED handler
*
* @note     This function shall be called with constant period of value
*           set in "led_cfg.h" with macro "LED_CFG_HNDL_PERIOD_MS".
*
* @return   status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_hndl(void)
{
    led_status_t status = eLED_OK;

    LED_ASSERT( true == gb_is_init );

    if ( true == gb_is_init )
    {
        // Loop through all LEDs
        for ( uint8_t led_num = 0; led_num < eLED_NUM_OF; led_num++ )
        {
            switch( g_led[led_num].mode )
            {
                case eLED_MODE_NORMAL:
                case eLED_MODE_FADE_TOGGLE:
                    // No action...
                    break;

                case eLED_MODE_FADE_IN:
                    led_fade_in_hndl( led_num, eLED_MODE_NORMAL );
                    break;

                case eLED_MODE_FADE_OUT:
                    led_fade_out_hndl( led_num, eLED_MODE_NORMAL );
                    break;

                case eLED_MODE_BLINK:
                    led_blink_hndl( led_num );
                    break;

                case eLED_MODE_FADE_BLINK:
                    led_fade_blink_hndl( led_num );
                    break;

                case eLED_MODE_NUM_OF:
                default:
                    LED_ASSERT( 0 );
                    break;
            }

            // Set LED low level driver
            led_set_low( led_num, g_led[led_num].duty, g_led[led_num].max_duty );

            // Manage period time
            led_hndl_period_time( led_num );

            // Manage LED timings
            led_manage_time( led_num );
        }
    }
    else
    {
        status = eLED_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Set LED state
*
* @param[in]    num     - LED enumeration number
* @param[in]    state   - State of LED
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_set(const led_num_t num, const led_state_t state)
{
    led_status_t status = eLED_OK;

    LED_ASSERT( true == gb_is_init );
    LED_ASSERT( num < eLED_NUM_OF );

    if ( true == gb_is_init )
    {
        if ( num < eLED_NUM_OF )
        {
            g_led[num].mode = eLED_MODE_NORMAL;

            if ( eLED_ON == state )
            {
                g_led[num].duty = g_led[num].max_duty;
            }
            else
            {
                g_led[num].duty = g_led[num].min_duty;
            }
        }
        else
        {
            status = eLED_ERROR;
        }
    }
    else
    {
        status = eLED_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Toggle LED
*
* @param[in]    num     - LED enumeration number
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_toggle(const led_num_t num)
{
    led_status_t status = eLED_OK;

    LED_ASSERT( true == gb_is_init );
    LED_ASSERT( num < eLED_NUM_OF );

    if ( true == gb_is_init )
    {
        if ( num < eLED_NUM_OF )
        {
            g_led[num].mode = eLED_MODE_NORMAL;

            if ( g_led[num].duty >= g_led[num].max_duty )
            {
                g_led[num].duty = g_led[num].min_duty;
            }
            else
            {
                g_led[num].duty = g_led[num].max_duty;
            }
        }
        else
        {
            status = eLED_ERROR;
        }
    }
    else
    {
        status = eLED_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   Put LED into blink mode
*
* @param[in]    num     - Number of LED
* @param[in]    on_time - Time that LED will be turned ON
* @param[in]    period  - Period of blink
* @param[in]    blink   - Number of blinks
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_blink(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink)
{
    led_status_t status = eLED_OK;

    LED_ASSERT( true == gb_is_init );
    LED_ASSERT( num < eLED_NUM_OF );
    LED_ASSERT( on_time < period  );

    if ( true == gb_is_init )
    {
        if  (   ( num < eLED_NUM_OF )
            &&  ( on_time < period )
            &&  ( eLED_MODE_NORMAL == g_led[num].mode ))
        {
            g_led[num].mode     = eLED_MODE_BLINK;
            g_led[num].on_time  = on_time;
            g_led[num].period   = period;
            g_led[num].per_time = 0.0f;

            if ( eLED_BLINK_CONTINUOUS == blink )
            {
                g_led[num].blink_cnt = LED_BLINK_CNT_CONT_VAL;
            }
            else
            {
                g_led[num].blink_cnt = (uint8_t) blink;
            }
        }
        else
        {
            status = eLED_ERROR;
        }
    }
    else
    {
        status = eLED_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Get LED active (ON) time
*
* @param[in]    num             - LED number
* @param[out]   p_active_time   - Pointer to LED active time
* @return       status          - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_get_active_time(const led_num_t num, float32_t * const p_active_time)
{
    led_status_t status = eLED_OK;

    LED_ASSERT( true == gb_is_init );
    LED_ASSERT( num < eLED_NUM_OF );
    LED_ASSERT( NULL != p_active_time );

    if ( true == gb_is_init )
    {
        if  (   ( num < eLED_NUM_OF )
            &&  ( NULL != p_active_time ))
        {
            *p_active_time = g_led[num].active_time;
        }
        else
        {
            status = eLED_ERROR;
        }
    }
    else
    {
        status = eLED_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Is LED in idle state
*
* @brief    Get if LED is in idle state, meaning that it no longer blinks!
*
* @param[in]    num         - LED number
* @param[out]   p_is_idle   - Is LED in idle state
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_is_idle(const led_num_t num, bool * const p_is_idle)
{
    led_status_t status = eLED_OK;

    LED_ASSERT( true == gb_is_init );
    LED_ASSERT( num < eLED_NUM_OF );
    LED_ASSERT( NULL != p_is_idle );

    if ( true == gb_is_init )
    {
        if  (   ( num < eLED_NUM_OF )
            &&  ( NULL != p_is_idle ))
        {
            if ( eLED_MODE_NORMAL == g_led[num].mode )
            {
                *p_is_idle = true;
            }
            else
            {
                *p_is_idle = false;
            }
        }
        else
        {
            status = eLED_ERROR;
        }
    }
    else
    {
        status = eLED_ERROR_INIT;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*       Is LED turned on (any amount)
*
*   LED is considered turned on if its in any transient mode, is blinking, or
*   its duty cycle is any amount greated then 0.
*
* @param[in]    num         - LED number
* @param[out]   p_is_on     - Is LED on
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_is_on(const led_num_t num, bool *const p_is_on)
{
    bool is_idle;
    led_status_t status = led_is_idle(num, &is_idle);
    if (eLED_OK == status)
    {
        *p_is_on = (true != is_idle) || (0 != g_led[num].duty);
    }

    return status;
}

#if ( 1 == LED_CFG_TIMER_USE_EN )

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *   Set LED smooth mode (fade in/out)
    *
    * @param[in]    num     - LED enumeration
    * @param[in]    state   - LED state
    * @return       status  - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    led_status_t led_set_smooth(const led_num_t num, const led_state_t state)
    {
        led_status_t status = eLED_OK;

        LED_ASSERT( true == gb_is_init );
        LED_ASSERT( num < eLED_NUM_OF );

        if ( true == gb_is_init )
        {
            if ( num < eLED_NUM_OF )
            {
                if ( eLED_ON == state )
                {
                    g_led[num].mode = eLED_MODE_FADE_IN;
                }
                else
                {
                    g_led[num].mode = eLED_MODE_FADE_OUT;
                }
            }
            else
            {
                status = eLED_ERROR;
            }
        }
        else
        {
            status = eLED_ERROR_INIT;
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *   Put LED into smooth blink mode (fade in/out)
    *
    * @param[in]    num     - Number of LED
    * @param[in]    on_time - Time that LED will be turned ON
    * @param[in]    period  - Period of blink
    * @param[in]    blink   - Number of blinks
    * @return       status  - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    led_status_t led_blink_smooth(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink)
    {
        led_status_t status = eLED_OK;

        LED_ASSERT( true == gb_is_init );
        LED_ASSERT( num < eLED_NUM_OF );
        LED_ASSERT( on_time < period);

        if ( true == gb_is_init )
        {
            if  (   ( num < eLED_NUM_OF )
                &&  ( on_time < period )
                &&  ( eLED_MODE_NORMAL == g_led[num].mode ))
            {
                g_led[num].mode     = eLED_MODE_FADE_BLINK;
                g_led[num].on_time  = on_time;
                g_led[num].period   = period;
                g_led[num].per_time = 0.0f;

                if ( eLED_BLINK_CONTINUOUS == blink )
                {
                    g_led[num].blink_cnt = LED_BLINK_CNT_CONT_VAL;
                }
                else
                {
                    g_led[num].blink_cnt = (uint8_t) blink;
                }
            }
            else
            {
                status = eLED_ERROR;
            }
        }
        else
        {
            status = eLED_ERROR_INIT;
        }

        return status;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**
    *       Configure LED fade timings and brightness
    *
    * @param[in]    num         - LED number
    * @param[in]    p_fade_cfg  - Fading configuration
    * @return       status      - Status of operation
    */
    ////////////////////////////////////////////////////////////////////////////////
    led_status_t led_set_fade_cfg(const led_num_t num, const led_fade_cfg_t * const p_fade_cfg)
    {
        led_status_t status = eLED_OK;

        LED_ASSERT( true == gb_is_init );
        LED_ASSERT( num < eLED_NUM_OF );
        LED_ASSERT( NULL != p_fade_cfg );

        if ( true == gb_is_init )
        {
            if  (   ( num < eLED_NUM_OF )
                &&  ( NULL != p_fade_cfg )
                &&  ( eLED_MODE_NORMAL == g_led[num].mode ))
            {
                g_led[num].max_duty         = p_fade_cfg->max_duty;
                g_led[num].min_duty         = p_fade_cfg->min_duty;
                g_led[num].fade_in_k        = (float32_t) ( 2.0f * ( g_led[num].max_duty - g_led[num].min_duty ) / ( p_fade_cfg->fade_in_time * p_fade_cfg->fade_in_time ));
                g_led[num].fade_out_k       = (float32_t) ( 2.0f * ( g_led[num].max_duty - g_led[num].min_duty ) / ( p_fade_cfg->fade_out_time * p_fade_cfg->fade_out_time ));
                g_led[num].fade_out_time    = p_fade_cfg->fade_out_time;
            }
            else
            {
                status = eLED_ERROR;
            }
        }
        else
        {
            status = eLED_ERROR_INIT;
        }

        return status;
    }

#endif

////////////////////////////////////////////////////////////////////////////////
/*!
 * @} <!-- END GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////
