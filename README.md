# **LED library** 
General C library for LED manipulation for human-machine interface. It support simple turning LEDs ON/OFF, turning ON/OFF with fading, blinking with and without fading. 

Library must have either GPIO or timer PWM low level code support in order to manipulate with LEDs. Library by itself only takes care of timing, fading, blinking functionalities. Complete setup of library is condensed into single configuration table.

## **Dependencies**

Depents on what kind of low level driver is being used for driving LED. Two low level driver are supported GPIO and TIMER.

### **1. GPIO Module**
When GPIO low level driver is enabled in **led_cfg.h**:
```C
/**
 * 	Using GPIO for driving LED
 */
#define LED_CFG_GPIO_USE_EN						( 1 )
```

then GPIO module shall be in following project path:
```
"root/drivers/peripheral/gpio/gpio/src/gpio.h"
```

GPIO module shall have following API function:
```C
gpio_status_t   gpio_is_init	(bool * const p_is_init);
gpio_status_t   gpio_set   		(const gpio_pin_t pin, const gpio_state_t state);
```

### **2. Timer Module**
When Timer low level driver is enabled in **led_cfg.h**:
```C
/**
 * 	Using timer for driving LED
 */
#define LED_CFG_TIMER_USE_EN					( 1 )
```

then Timer module shall be in following project path:
```
"root/drivers/peripheral/timer/timer/src/timer.h"
```

Timer module shall have following API function:
```C
timer_status_t timer_is_init	(const timer_ch_t tim_ch, bool * const p_is_init);
timer_status_t timer_pwm_set  	(const timer_ch_t tim_ch, const float32_t duty);
```

## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: 

```
root/drivers/hmi/led/led/"module_space"
```

 ## **API**
---
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **led_init** 				    | Initialization of LED 		            | led_status_t led_init(void) |
| **led_deinit** 			    | De-initialization of LED 		            | led_status_t led_init(void) |
| **led_is_init** 			    | Get initialization flag 		            | led_status_t 	led_is_init(bool * const p_is_init) |
| **led_hndl** 				    | Main LED handler 				            | led_status_t led_hndl(void) |
| **led_set** 				    | Set LED state 				            | led_status_t led_set(const led_num_t num, const led_state_t state) |
| **led_set_full** 		        | Set LED state to either 0% or 100% 		| led_status_t led_set_full(const led_num_t num, const led_state_t state) |
| **led_toggle** 			    | Toggle LED state 				            | led_status_t led_toggle(const led_num_t num) |
| **led_blink** 			    | Blink LED 					            | led_status_t led_blink(const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink |
| **led_get_active_time** 	    | Get LED ON time 				            | led_status_t led_get_active_time(const led_num_t num, float32_t * const p_active_time) |
| **led_is_idle** 			    | Is LED in idle state			            | led_status_t led_is_idle(const led_num_t num, bool * const p_is_idle) |
| **led_is_on** 			    | Is LED in turned ON		                | led_status_t led_is_on(const led_num_t num, bool *const p_is_on) |
| **led_set_on_brightness**     | Set LED ON brightness	(max. duty)         | led_status_t led_set_on_brightness(const led_num_t num, const float32_t duty_cycle) |
| **led_set_off_brightness**    | Set LED OFF brightness (min. duty)        | led_status_t led_set_off_brightness(const led_num_t num, const float32_t duty_cycle) |
| **led_get_duty**              | Get LED current duty cycle                | led_status_t led_get_duty(const led_num_t num, float32_t * const p_duty_cycle) |

Enabled only if using timer PWM as low level driver:
| Fading API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **led_set_smooth** 	            | Set LED state with fading 		| led_status_t led_set_smooth(const led_num_t num, const led_state_t state) |
| **led_blink_smooth** 	            | Blink LED with fading 			| led_status_t led_blink_smooth (const led_num_t num, const float32_t on_time, const float32_t period, const led_blink_t blink) |
| **led_set_fade_cfg** 	            | Set LED fading configurations 	| led_status_t led_set_fade_cfg	(const led_num_t num, const led_fade_cfg_t * const p_fade_cfg) |
| **led_is_in_smooth_blink_mode** 	| Is LED in smooth blink mode	    | bool led_is_in_smooth_blink_mode(const led_num_t num) |

## **How to use**
---

**1. List all LEDs inside **led_cfg.h** file:**
```C
/**
 *     List of LEDs
 *
 * @note    User shall provide LED name here as it would be using
 *          later inside code.
 *
 * @note    User shall change code only inside section of "USER_CODE_BEGIN"
 *          and "USER_CODE_END".
 */
typedef enum
{
    // USER CODE START...

    eLED_STATUS = 0,    /**<General status indicator */
    eLED_ERR_COM,       /**<Error communication indicator */

    // USER CODE END...

    eLED_NUM_OF
} led_num_t;
```

**2. Set up used low level drivers and main handler period inside **led_cfg.h** file:**
```C
/**
 * 	Main LED handler period
 * 	Unit: sec
 */
#define LED_CFG_HNDL_PERIOD_MS					( 0.01f )

/**
 * 	Using timer for driving LED
 */
#define LED_CFG_TIMER_USE_EN					( 1 )

/**
 * 	Using GPIO for driving LED
 */
#define LED_CFG_GPIO_USE_EN						( 1 )
```

**3. Set up configuration table inside **led_cfg.c** file:**
```C
/**
 *     LED configuration table
 *
 *
 *    @brief     This table is being used for setting up LED low level drivers.
 *
 *            Two options are supported:
 *                1. GPIO
 *                2. Timer PWM
 *
 *
 *     @note     Low level gpio and timer code must be compatible!
 */
static const led_cfg_t g_led_cfg[ eLED_NUM_OF ] =
{

    // USER CODE BEGIN...

    // -------------------------------------------------------------------------------------------------------------------------------------------------------
    //                      Driver type                 LED driver channel                      Initial State               Polarity
    // -------------------------------------------------------------------------------------------------------------------------------------------------------
    [eLED_STATUS]   =   { .drv_type = eLED_DRV_GPIO,    .drv_ch.gpio_pin = eGPIO_LED_STATUS,        .initial_state = eLED_ON,   .polarity = eLED_POL_ACTIVE_HIGH    },
    [eLED_ERR_COM]  =   { .drv_type = eLED_DRV_GPIO,    .drv_ch.gpio_pin = eGPIO_LED_ERR_COM,       .initial_state = eLED_ON,   .polarity = eLED_POL_ACTIVE_HIGH    },


    // USER CODE END...

};
```
**4. Include, initialize & handle:**

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

**5. Blink with LEDs at will...**
```C
// Set LED ON
led_set( eLED_DEBUG, eLED_ON );

// Set LED OFF
led_set( eLED_DEBUG, eLED_OFF );

// Blink LED continously wiht period of 1 sec, 0.5 sec ON
// NOTE: Single call of this function will cause blinking continously by led_hndl()
led_blink( eLED_DEBUG, 0.5f, 1.0f, eLED_BLINK_CONTINUOUS );
```
