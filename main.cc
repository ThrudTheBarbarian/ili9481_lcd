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

    dpy.clear(RGB(50,200,200));

	dpy.box({100, 100, 120, 280}, RGB(0xff, 0x00, 0x00));
    dpy.line(100, 100, 220, 380, RGB(0xff, 0x00, 0x00));
    dpy.line(220, 100, 100, 380, RGB(0xff, 0x00, 0xff));

    dpy.plot(50, 50, RGB(0x0, 0x00, 0x0));

    dpy.circle(180, 300, 90, RGB(128,0,255), true);
    dpy.circle(150,350, 100, {200, 100, 0});

    dpy.ellipse(200,430,100,40, RGB(90, 110, 255));
    dpy.ellipse(200,430,98,38, RGB(255, 110, 90), true);
    
    dpy.box({80, 10, 200, 30}, {255, 50, 200}, false, 5);
    dpy.box({80, 45, 200, 30}, {255, 50, 200}, true, 5);
    
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
	