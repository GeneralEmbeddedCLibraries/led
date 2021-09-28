// Copyright (c) 2021 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     led.c
* @brief    LED manipulations
* @author   Ziga Miklosic
* @date     27.09.2021
* @version	V1.0.0
*/
////////////////////////////////////////////////////////////////////////////////
/**
 * @addtogroup LED_API
 * @{ <!-- BEGIN GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

#include "led.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	LED handler period in second and frequency in Hz
 */
#define LED_HNDL_PERIOD_S				( 0.01 )
#define LED_HNDL_FREQ_HZ				((float32_t) ( 1.0 / PWR_HNDL_PERIOD_S ))

/**
 * 	Default fade IN/OUT time
 *
 * @note Resolution of LED_HNDL_PERIOD_S
 *
 * 	Unit: sec
 */
#define LED_FADE_IN_TIME_S				( 1.0 )
#define LED_FADE_OUT_TIME_S				( 1.0 )

/**
 * 	 Default fading factor
 *
 * 	 @Note Times two is becase of x^2 derivative is 2x
 *
 * 	 Unit: duty [0-1.0]
 */
#define LED_FADE_IN_COEF_T_TO_DUTY		(float32_t) ( 2.0 * 1.0 / ( LED_FADE_IN_TIME_S * LED_FADE_IN_TIME_S ))
#define LED_FADE_OUT_COEF_T_TO_DUTY		(float32_t) ( 2.0 * 1.0 / ( LED_FADE_OUT_TIME_S * LED_FADE_OUT_TIME_S ))

/**
 * 	Time keeping limitations
 */
#define LED_TIME_LIMIT_S				( 1E6f )
#define LED_TIME_LIM(time)				(( time > LED_TIME_LIMIT_S ) ? ( LED_TIME_LIMIT_S ) : ( time ))

/**
 * 	Blink counter continuous code
 */
#define LED_BLINK_CNT_CONT_VAL			((uint8_t) ( 0xFFU ))

/**
 * 	LED mode
 */
typedef enum
{
	eLED_MODE_NORMAL = 0,		/**<Normal mode */
	eLED_MODE_FADE_IN,			/**<Fade-in mode */
	eLED_MODE_FADE_OUT,			/**<Fade-out mode */
	eLED_MODE_FADE_TOGGLE,		/**<Fade-in-out continuously */
	eLED_MODE_BLINK,			/**<Blink mode */
	eLED_MODE_FADE_BLINK,		/**<Blink mode with fading */

	eLED_MODE_NUM_OF
} led_mode_t;

/**
 * 	LED data
 */
typedef struct
{
	float32_t	duty;			/**<Duty cycle of LED */
	float32_t 	max_duty;		/**<Maximum duty cycle of LED */
	float32_t	fade_time;		/**<Time for fading functionalities */
	float32_t	fade_in_k;		/**<Fade in factor */
	float32_t	fade_out_k;		/**<Fade out factor */
	float32_t 	fade_out_time;
	float32_t	period;			/**<Period of toggle mode */
	float32_t	per_time;		/**<Period time keeping */
	float32_t	on_time;		/**<On time for blink mode */
	float32_t 	active_time;	/**<LED active time - turned ON time */
	//timer_ch_t	tim_ch;			/**<Timer channel which drive LED */
	led_mode_t	mode;			/**<Current LED mode */
	uint8_t		blink_cnt;		/**<Blink LED live counter */
} led_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 * 	LED data
 */
static led_t g_led[ eLED_NUM_OF ] = { 0 };
/*{
	//	Initial duty	Max duty 			Fade time			Fade in factor 								Fade out factor								Period			Period time			On time 			Timer channel							Mode						Blink counter
	{	.duty = 0.0f,	.max_duty = 1.0f,	.fade_time = 0.0f,	.fade_out_time = 1.0f, .fade_in_k = LED_FADE_IN_COEF_T_TO_DUTY, 	.fade_out_k = LED_FADE_OUT_COEF_T_TO_DUTY,	.period = 0.0f,	.per_time = 0.0f, 	.on_time = 0.0f, 	.tim_ch = eTIMER_TIM3_CH1_LED_R,	.mode = eLED_MODE_NORMAL, 	.blink_cnt = 0U	},
	{	.duty = 0.0f,	.max_duty = 1.0f,	.fade_time = 0.0f,	.fade_out_time = 1.0f, .fade_in_k = LED_FADE_IN_COEF_T_TO_DUTY, 	.fade_out_k = LED_FADE_OUT_COEF_T_TO_DUTY,	.period = 0.0f,	.per_time = 0.0f, 	.on_time = 0.0f, 	.tim_ch = eTIMER_TIM3_CH2_LED_G,	.mode = eLED_MODE_NORMAL,	.blink_cnt = 0U	},
};*/

/**
 * 	Initialization guard
 */
static bool gb_is_init = false;

/**
 * 	Pointer to configuration table
 */
static const led_cfg_t * gp_cfg_table = NULL;

////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////////////////////////
static void led_fade_in_hndl		(const led_num_t num, const led_mode_t exit_mode);
static void led_fade_out_hndl		(const led_num_t num, const led_mode_t exit_mode);
static void led_blink_hndl			(const led_num_t num);
static void led_fade_blink_hndl		(const led_num_t num);
static void led_hndl_period_time	(const led_num_t num);
static bool led_is_on_time			(const led_num_t num);
static bool led_is_period_time		(const led_num_t num);
static void led_blink_cnt_hndl		(const led_num_t num);
static void led_manage_time			(const led_num_t num);

static led_status_t led_check_drv_init	(void);
static void led_set_gpio				(const led_num_t led_num, const float32_t duty, const float32_t duty_max);
static void led_set_timer				(const led_num_t led_num, const float32_t duty);




////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*   Initialise LEDs
*
*@precondition Timers shall be initialised before
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_init(void)
{
	led_status_t 	status 	= eLED_OK;
	led_num_t		num		= 0;

	LED_ASSERT( false == gb_is_init );

	if ( false == gb_is_init )
	{
		// Get configuration matrix
		gp_cfg_table = led_cfg_get_table();

		if ( NULL != gp_cfg_table )
		{
			// Check low level drivers
			if ( eLED_OK == led_check_drv_init())
			{
				// Set up live LED configuration
				for ( num = 0; num < eLED_NUM_OF; num++ )
				{
					g_led[num].duty 			= 0.0f;
					g_led[num].max_duty 		= 1.0f;
					g_led[num].fade_time 		= 0.0f;
					g_led[num].fade_in_k 		= LED_FADE_IN_COEF_T_TO_DUTY;
					g_led[num].fade_out_k 		= LED_FADE_OUT_COEF_T_TO_DUTY;
					g_led[num].fade_out_time 	= 1.0f;
					g_led[num].period		 	= 0.0f;
					g_led[num].per_time		 	= 0.0f;
					g_led[num].on_time		 	= 0.0f;
					g_led[num].active_time	 	= 0.0f;
					g_led[num].mode			 	= eLED_MODE_NORMAL;
					g_led[num].blink_cnt		= 0;
				}
			}

			// Low level drivers not initialized
			else
			{
				LED_ASSERT( 0 );
				status = eLED_ERROR_INIT;
			}
		}
		else
		{
			status = eLED_ERROR_INIT;
		}
	}
	else
	{
		status = eLED_ERROR_INIT;
	}

	return status;

    // Turn both LEDs off
    //led_set( eLED_L, eLED_OFF );
    //led_set( eLED_R, eLED_OFF );
}

////////////////////////////////////////////////////////////////////////////////
/**
*   LED handler
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_hndl(void)
{
	led_status_t status = eLED_OK;

	// Loop through all LEDs
	for ( uint8_t led_num = 0; led_num < eLED_NUM_OF; led_num++ )
	{
		switch( g_led[led_num].mode )
		{
			case eLED_MODE_NORMAL:
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

			default:
				LED_ASSERT( 0 );
				break;
		}

		// Set timer
		if ( eLED_DRV_TIMER_PWM == gp_cfg_table[led_num].drv_type )
		{
			led_set_timer( led_num, g_led[led_num].duty );
		}

		// Set GPIO
		else if ( eLED_DRV_GPIO == gp_cfg_table[led_num].drv_type  )
		{
			led_set_gpio( led_num, g_led[led_num].duty, g_led[led_num].max_duty );
		}

		// Unknown driver
		else
		{
			LED_ASSERT( 0 );
		}

		// Manage period time
		led_hndl_period_time( led_num );

		// Manage LED timings
		led_manage_time( led_num );
	}

	return status;
}



////////////////////////////////////////////////////////////////////////////////
/**
*   Set LED
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_set(const led_num_t num, const led_state_t state)
{
	led_status_t status = eLED_OK;

	if ( num < eLED_NUM_OF )
	{
		g_led[num].mode = eLED_MODE_NORMAL;

		if ( eLED_ON == state )
		{
			g_led[num].duty = g_led[num].max_duty;
		}
		else
		{
			g_led[num].duty = 0.0f;
		}
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   Put LED into blink mode
*
* @param[in]	num 	- Number of LED
* @param[in]	on_time - Time that LED will be turned ON
* @param[in]	period	- Period of blink
* @return   	void
*/
////////////////////////////////////////////////////////////////////////////////
led_status_t led_blink(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink)
{
	led_status_t status = eLED_OK;

	if 	(	( num < eLED_NUM_OF )
		&&	( on_time < period )
		&&	( eLED_MODE_NORMAL == g_led[num].mode ))
	{
		g_led[num].mode 		= eLED_MODE_BLINK;
		g_led[num].on_time 		= on_time;
		g_led[num].period 		= period;
		g_led[num].per_time 	= 0.0f;

		if ( eLED_BLINK_CONTINUOUS == blink )
		{
			g_led[num].blink_cnt = LED_BLINK_CNT_CONT_VAL;
		}
		else
		{
			g_led[num].blink_cnt = (uint8_t) blink;
		}
	}

	return status;
}

#if ( 1 == LED_CFG_TIMER_USE_EN )

	////////////////////////////////////////////////////////////////////////////////
	/**
	*   Set LED smooth mode (fade in/out)
	*
	* @return   void
	*/
	////////////////////////////////////////////////////////////////////////////////
	led_status_t led_set_smooth(const led_num_t num, const led_state_t state)
	{
		led_status_t status = eLED_OK;

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

		return status;
	}

	////////////////////////////////////////////////////////////////////////////////
	/**
	*   Put LED into smooth blink mode (fade in/out)
	*
	* @param[in]	num 	- Number of LED
	* @param[in]	on_time - Time that LED will be turned ON
	* @param[in]	period	- Period of blink
	* @return   	void
	*/
	////////////////////////////////////////////////////////////////////////////////
	led_status_t led_blink_smooth(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink)
	{
		led_status_t status = eLED_OK;

		if 	(	( num < eLED_NUM_OF )
			&&	( on_time < period )
			&&	( eLED_MODE_NORMAL == g_led[num].mode ))
		{
			g_led[num].mode 	= eLED_MODE_FADE_BLINK;
			g_led[num].on_time 	= on_time;
			g_led[num].period 	= period;
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

		return status;
	}

#endif

////////////////////////////////////////////////////////////////////////////////
/**
*   	Configure LED driving
*
* @param[in]	num				- LED number
* @param[in]	fade_in_time	- LED fade in time
* @param[in]	fade_out_time	- LED fade out time
* @param[in]	max_duty 		- LED maximum brightness
* @return   	void
*/
////////////////////////////////////////////////////////////////////////////////
/*void led_set_cfg(const led_num_t num, const float32_t fade_in_time, const float32_t fade_out_time, const float32_t max_duty)
{
	if 	(	( num < eLED_NUM_OF )
		&&	(( fade_in_time < 10.0f ) && ( fade_in_time > 0.1f ))
		&&	(( fade_out_time < 10.0f ) && ( fade_out_time > 0.1f ))
		&&	( max_duty <= 1.0f )
		&&	( eLED_MODE_NORMAL == g_led[num].mode ))
	{
		g_led[num].max_duty 	= max_duty;
		g_led[num].fade_in_k 	= (float32_t) ( 2.0f * g_led[num].max_duty / ( fade_in_time * fade_in_time ));
		g_led[num].fade_out_k 	= (float32_t) ( 2.0f * g_led[num].max_duty / ( fade_out_time * fade_out_time ));

		g_led[num].fade_out_time = fade_out_time;
	}
}*/

////////////////////////////////////////////////////////////////////////////////
/**
*   	Get LED active (ON) time
*
* @param[in]	num		- LED number
* @return   	time	- Active ON time of LED
*/
////////////////////////////////////////////////////////////////////////////////
/*float32_t led_get_active_time(const led_num_t num)
{
	float32_t time = 0.0f;

	if ( num < eLED_NUM_OF )
	{
		time = g_led[num].active_time;
	}

	return time;
}*/

////////////////////////////////////////////////////////////////////////////////
/**
*   Manage LED timings
*
* @param[in]	num 	- Number of LED
* @return   	void
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
*   	Fade in FMS state
*
* @param[in]	num			- LED number
* @param[in]	exit_mode	- Mode to transition on exit
* @return   	void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_fade_in_hndl(const led_num_t num, const led_mode_t exit_mode)
{
	// Increase duty by the square function
	g_led[num].duty += g_led[num].fade_in_k * g_led[num].fade_time * (float32_t) LED_HNDL_PERIOD_S;

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
*   	Fade out FMS state
*
* @param[in]	num			- LED number
* @param[in]	exit_mode	- Mode to transition on exit
* @return   	void
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
		g_led[num].duty = 0.0f;
	}

	// Is LED fully OFF?
	if ( g_led[num].duty > 0.001f )
	{
		// Increment time
		g_led[num].fade_time += LED_HNDL_PERIOD_S;
		g_led[num].fade_time = LED_TIME_LIM( g_led[num].fade_time );
	}

	// LED fully OFF
	else
	{
		// Limit duty
		g_led[num].duty = 0.0f;

		// Reset time
		g_led[num].fade_time = 0.0f;

		// Goto NORMAL mode
		g_led[num].mode = exit_mode;
	}
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	LED blink FSM state
*
* @param[in]	num			- LED number
* @return   	void
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
		g_led[num].duty = 0.0f;
	}

	// Manage blink counter
	led_blink_cnt_hndl( num );
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	LED fade blink FSM state
*
* @param[in]	num			- LED number
* @return   	void
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
*   	Period time handler
*
* @param[in]	num			- LED number
* @return   	void
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
*   	Check if it is time for ON LED state
*
* @param[in]	num			- LED number
* @return   	void
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
*   	Check if it is period of blink
*
*   Return true only on period update
*
* @param[in]	num			- LED number
* @return   	is_period	- Period update event
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
*   	Manage blink counter
*
* @param[in]	num			- LED number
* @return   	void
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
*   	Check that low level drivers are initialized
*
* @return   	status - Status of low level initialization
*/
////////////////////////////////////////////////////////////////////////////////
static led_status_t led_check_drv_init(void)
{
	led_status_t 	status = eLED_OK;

	#if ( 1 == LED_CFG_TIMER_USE_EN )

		bool tim_drv_init = false;

		// Get init flag
		timer_is_init( &tim_drv_init );

		if ( false == tim_drv_init )
		{
			status |= eLED_ERROR_INIT;
		}

	#endif

	#if ( 1 == LED_CFG_GPIO_USE_EN )

		bool gpio_drv_init = false;

		// Get init flag
		gpio_is_init( &gpio_drv_init );

		if ( false == gpio_drv_init )
		{
			status |= eLED_ERROR_INIT;
		}

	#endif

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Set LED via GPIO driver
*
*  @note	Based on duty cycle LED GPIO state is being determine!
*
* @param[in]	led_num		- Number of LED
* @param[in]	duty		- Current duty of LED
* @param[in]	max_duty	- Maximum duty of LED
* @return   	void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_set_gpio(const led_num_t led_num, const float32_t duty, const float32_t duty_max)
{
	#if ( 1 == LED_CFG_GPIO_USE_EN )
		gpio_state_t state = eGPIO_LOW;

		if ( duty >= duty_max )
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
	#endif
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Set led via TIMER driver
*
* @param[in]	led_num		- Number of LED
* @param[in]	duty		- Current duty of LED
* @return   	void
*/
////////////////////////////////////////////////////////////////////////////////
static void led_set_timer(const led_num_t led_num, const float32_t duty)
{
	#if ( 1 == LED_CFG_TIMER_USE_EN )
		float32_t tim_duty = duty;

		// Apply polarity
		if ( eLED_POL_ACTIVE_LOW == gp_cfg_table[led_num].polarity )
		{
			tim_duty = ( 1.0 - duty );
		}

		// Set timer PWM
		timer_set_pwm( gp_cfg_table[led_num].drv_ch.tim_ch, tim_duty );

	#endif
}

////////////////////////////////////////////////////////////////////////////////
/*!
 * @} <!-- END GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////
