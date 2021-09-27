// Copyright (c) 2021 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
* @file     led.c
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

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include "led.h"
#include "drivers/peripheral/timer/timer.h"

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
	timer_ch_t	tim_ch;			/**<Timer channel which drive LED */
	led_mode_t	mode;			/**<Current LED mode */
	uint8_t		blink_cnt;		/**<Blink LED live counter */
} led_t;

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 * 	LED data
 */
static led_t g_led[ eLED_NUM_OF ] =
{
	//	Initial duty	Max duty 			Fade time			Fade in factor 								Fade out factor								Period			Period time			On time 			Timer channel							Mode						Blink counter
	{	.duty = 0.0f,	.max_duty = 1.0f,	.fade_time = 0.0f,	.fade_out_time = 1.0f, .fade_in_k = LED_FADE_IN_COEF_T_TO_DUTY, 	.fade_out_k = LED_FADE_OUT_COEF_T_TO_DUTY,	.period = 0.0f,	.per_time = 0.0f, 	.on_time = 0.0f, 	.tim_ch = eTIMER_TIM3_CH1_LED_R,	.mode = eLED_MODE_NORMAL, 	.blink_cnt = 0U	},
	{	.duty = 0.0f,	.max_duty = 1.0f,	.fade_time = 0.0f,	.fade_out_time = 1.0f, .fade_in_k = LED_FADE_IN_COEF_T_TO_DUTY, 	.fade_out_k = LED_FADE_OUT_COEF_T_TO_DUTY,	.period = 0.0f,	.per_time = 0.0f, 	.on_time = 0.0f, 	.tim_ch = eTIMER_TIM3_CH2_LED_G,	.mode = eLED_MODE_NORMAL,	.blink_cnt = 0U	},
};

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
void led_init(void)
{
    // Turn both LEDs off
    led_set( eLED_L, eLED_OFF );
    led_set( eLED_R, eLED_OFF );
}

////////////////////////////////////////////////////////////////////////////////
/**
*   LED handler
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
void led_hndl(void)
{
	// Loop through all LEDs
	for ( uint8_t i = 0; i < eLED_NUM_OF; i++ )
	{
		switch( g_led[i].mode )
		{
			case eLED_MODE_NORMAL:
				// No action...
				break;

			case eLED_MODE_FADE_IN:
				led_fade_in_hndl( i, eLED_MODE_NORMAL );
				break;

			case eLED_MODE_FADE_OUT:
				led_fade_out_hndl( i, eLED_MODE_NORMAL );
				break;

			case eLED_MODE_BLINK:
				led_blink_hndl( i );
				break;

			case eLED_MODE_FADE_BLINK:
				led_fade_blink_hndl( i );
				break;

			default:
				PROJECT_CONFIG_ASSERT( 0 );
				break;
		}

		// Setup timer
		timer_set_pwm( g_led[i].tim_ch, g_led[i].duty );

		// Manage period time
		led_hndl_period_time( i );

		// Manage LED timings
		led_manage_time( i );
	}
}

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
*   Set LED
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
void led_set(const led_num_t num, const led_state_t state)
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
			g_led[num].duty = 0.0f;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/**
*   Set LED smooth mode (fade in/out)
*
* @return   void
*/
////////////////////////////////////////////////////////////////////////////////
void led_set_smooth(const led_num_t num, const led_state_t state)
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
void led_blink(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink)
{
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
void led_blink_smooth(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink)
{
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
}

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
void led_set_cfg(const led_num_t num, const float32_t fade_in_time, const float32_t fade_out_time, const float32_t max_duty)
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
}

////////////////////////////////////////////////////////////////////////////////
/**
*   	Get LED active (ON) time
*
* @param[in]	num		- LED number
* @return   	time	- Active ON time of LED
*/
////////////////////////////////////////////////////////////////////////////////
float32_t led_get_active_time(const led_num_t num)
{
	float32_t time = 0.0f;

	if ( num < eLED_NUM_OF )
	{
		time = g_led[num].active_time;
	}

	return time;
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
/*!
 * @} <!-- END GROUP -->
 */
////////////////////////////////////////////////////////////////////////////////
