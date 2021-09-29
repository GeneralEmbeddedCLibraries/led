# **LED library** 
General C library for LED manipulation for human-machine interface. It support simple turning LEDs ON/OFF, turning ON/OFF with fading, blinking with and without fading. 

Library must have either GPIO or timer PWM low level code support in order to manipulate with LEDs. Library by itself only takes care of timing, fading, blinking functionalities. Complete setup of library is condensed into single configuration table.

## **Dependencies**
---
Depents on what kind of low level driver is being used for driving LED. Two low level driver are supported. Based on configurations following function definitions must be vissible to LED module:
 - **GPIO**
    - gpio_status_t gpio_is_init(bool * const p_is_init)
    - void	gpio_set(const gpio_pins_t pin, const gpio_state_t state)
 - **Timer PWM**
    - timer_status_t 	timer_is_init		(bool * const p_is_init)
    - timer_status_t 	timer_set_pwm		(const timer_ch_t ch, const float32_t duty)

Make sure to properly set configurations inside **led_cfg.h**:

```C
/**
 * 	Using timer for driving LED
 */
#define LED_CFG_TIMER_USE_EN					( 1 )

/**
 * 	Using GPIO for driving LED
 */
#define LED_CFG_GPIO_USE_EN						( 1 )
```

For library version V1.0.0 GPIO or/and timer PWM translation unit must be under following project path:
```C
#if ( 1 == LED_CFG_TIMER_USE_EN )
	#include "drivers/peripheral/timer/timer.h"
#endif

#if ( 1 == LED_CFG_GPIO_USE_EN )
	#include "drivers/peripheral/gpio/gpio.h"
#endif
```

Additional definition of float32_t must be provided by user. In current implementation it is defined in "project_config.h". Just add following statement to your code where it suits the best.

```C
// Define float
typedef float float32_t;
```


 ## **API**
---
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **led_init** | Initialization of LED | led_status_t led_init(void) |****
| **led_is_init** | Get initialization flag | led_status_t 	led_is_init(bool * const p_is_init) |
| **led_hndl** | Main LED handler | led_status_t led_hndl(void) |
| **led_set** | Set LED state | led_status_t led_set(const led_num_t num, const led_state_t state) |
| **led_toggle** | Toggle LED state | led_status_t led_toggle(const led_num_t num) |
| **led_blink** | Blink LED | led_status_t led_blink(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink |
| **led_get_active_time** | Get LED ON time | led_status_t led_get_active_time(const led_num_t num, float32_t * const p_active_time)|


Enabled only if using timer PWM as low level driver:
| Fading API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **led_set_smooth** | Set LED state with fading | led_status_t led_set_smooth(const led_num_t num, const led_state_t state) |
| **led_blink_smooth** | Blink LED with fading | led_status_t led_blink_smooth (const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink) |
| **led_set_fade_cfg** | Set LED fading configurations | led_status_t led_set_fade_cfg	(const led_num_t num, const led_fade_cfg_t * const p_fade_cfg) |

## **How to use**
---

1. List all LEDs inside **led_cfg.h** file:
```C
/**
 * 	List of LEDs
 *
 * @note 	User shall provide LED name here as it would be using
 * 			later inside code.
 *
 * @note 	User shall change code only inside section of "USER_CODE_BEGIN"
 * 			ans "USER_CODE_END".
 */
typedef enum
{
	// USER CODE START...

	eLED_DEBUG = 0,
    eLED_R,
    eLED_G,

	// USER CODE END...

	eLED_NUM_OF
} led_num_t;
```

2. Set up used low level drivers and main handler period inside **led_cfg.h** file:
```C
/**
 * 	Main LED handler period
 * 	Unit: sec
 */
#define LED_CFG_HNDL_PERIOD_S					( 0.01f )

/**
 * 	Using timer for driving LED
 */
#define LED_CFG_TIMER_USE_EN					( 1 )

/**
 * 	Using GPIO for driving LED
 */
#define LED_CFG_GPIO_USE_EN						( 1 )
```

3. Set up configuration table inside **led_cfg.c** file:
```C
/**
 * 	LED configuration table
 *
 *
 *	@brief 	This table is being used for setting up LED low level drivers.
 *
 *			Two options are supported:
 *				1. GPIO
 *				2. Timer PWM
 *
 *
 * 	@note 	Low level gpio and timer code must be compatible!
 */
static const led_cfg_t g_led_cfg[ eLED_NUM_OF ] =
{

	// USER CODE BEGIN...

	// -------------------------------------------------------------------------------------------------------------------------------------------------------
	//	Driver type							LED driver channel							Initial State					Polarity
	// -------------------------------------------------------------------------------------------------------------------------------------------------------
	{ 	.drv_type = eLED_DRV_GPIO,			.drv_ch.gpio_pin = eGPIO_DEBUG_LED,			.initial_state = eLED_ON, 		.polarity = eLED_POL_ACTIVE_HIGH	},
	{ 	.drv_type = eLED_DRV_TIMER_PWM,		.drv_ch.tim_ch = eTIMER_TIM3_CH1_LED_R,		.initial_state = eLED_ON, 		.polarity = eLED_POL_ACTIVE_HIGH	},
	{ 	.drv_type = eLED_DRV_TIMER_PWM,		.drv_ch.tim_ch = eTIMER_TIM3_CH2_LED_G,		.initial_state = eLED_OFF,		.polarity = eLED_POL_ACTIVE_HIGH	},


	// USER CODE END...

};
```
4. Include, initialize & handle:

Main LED handler **led_hndl()** must be called with a fixed period of **LED_CFG_HNDL_PERIOD_S** (defined inside led_cfg.h) in order to produce nice fading/effects. 

```C
// Include single file to your application!
#include "led.h"

// Init LED
if ( eLED_OK != led_init())
{
    // Init failed...
    // Further actions here...
}


@LED_CFG_HNDL_PERIOD_S period
{
    // Handle LED
    led_hndl();
}
```

5. Blink with LEDs at will...
```C
// Set LED ON
led_set( eLED_DEBUG, eLED_ON );

// Set LED OFF
led_set( eLED_DEBUG, eLED_OFF );

// Blink LED continously wiht period of 1 sec, 0.5 sec ON
// NOTE: Single call of this function will cause blinking continously by led_hndl()
led_blink( eLED_DEBUG, 0.5f, 1.0f, eLED_BLINK_CONTINUOUS );

```
