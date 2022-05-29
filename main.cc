#include <stdio.h>
#include "pico/stdlib.h"

#include "classes/ili9481.h"

int main (int argc, char **argv)
    {
    stdio_init_all();
    sleep_ms(2000);

    /*************************************************************************\
    |* Initialise GPIO. 
    \*************************************************************************/
    Gpio gpio;
    gpio.init();
 
    /*************************************************************************\
    |* Initialise the display 
    \*************************************************************************/
    int fspi        = 20;
    DpyContext ctx  = 
        {
            {
            Spi::SPI0,      // Spi device 0
            Gpio::PIN18,    // SPI SCK
            Gpio::PIN17,    // SPI CS
            Gpio::PIN19,    // SPI TX
            Gpio::PIN16,    // SPI RX
            fspi            // SPI clock in MHz
            },
        Gpio::PIN27,        // LCD /RST
        Gpio::PIN26,        // LCD D/C
        };    

    Ili9481 dpy;
    int ok      = dpy.init(&ctx);
    dpy.setRotation(Ili9481::INVERTED_PORTRAIT);

	dpy.rectFill({0, 0, 320, 480}, RGB(0xff, 0xff, 0xff));
	dpy.rectFill({100, 100, 120, 280}, RGB(0xff, 0x00, 0x00));

    dpy.drawLine (20, 20,  200, 20, {0x00, 0x00, 0x00});
    dpy.drawLine (20, 200, 200, 200, {0x00, 0x00, 0x00});
 
    dpy.drawLine (20,  20, 20,  200, {0x00, 0x00, 0x00});
    dpy.drawLine (200, 20, 200, 200, {0x00, 0x00, 0x00});

    Gpio::State toggle = Gpio::HI;
    while (1)
		{
        sleep_ms(200);
        toggle = (toggle == Gpio::LO) ? Gpio::HI : Gpio::LO;
        gpio.set(Gpio::PIN25, toggle);
		}
    
    /* not reached */
    return 0;
	}
	