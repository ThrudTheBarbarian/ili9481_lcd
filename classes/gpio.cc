
#include "gpio.h"

/*****************************************************************************\
|* Contructor
\*****************************************************************************/
Gpio::Gpio(void)
    {}

/*****************************************************************************\
|* Initialise the GPIOs to a known state
\*****************************************************************************/
void Gpio::init(void)
    {
    // Make all GPIO inputs by default
    for (int i=0; i<PIN_MAX; i++)
        {
        gpio_init(PIN0 + i);                        // Initialise GPIO to SIO
        gpio_set_dir(PIN0 + i, false);              // false = input
        gpio_disable_pulls(PIN0+i);                 // No pullup/pulldown
        gpio_set_function(PIN0+i, GPIO_FUNC_SIO);   // SIO mode
        gpio_set_irq_enabled(PIN0+i, 0, false);     // No interrupts by default
        }

    
    // Make 25 be LED output, but disabled
    gpio_set_dir(PIN25, OUTPUT);
    gpio_put(PIN25, LO);
    }

/*****************************************************************************\
|* Set a GPIO's output state
\*****************************************************************************/
void Gpio::set(int pin, Gpio::State state)
    {
    gpio_put(pin, state);
    }