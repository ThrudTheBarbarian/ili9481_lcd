#pragma once

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"

#include "gpio.h"

/*****************************************************************************\
|* Context for initialising a SPI driver
\*****************************************************************************/

class Spi
    {
    NON_COPYABLE_NOR_MOVEABLE(Spi)

	/*************************************************************************\
    |* Public Enums in this namespace
    \*************************************************************************/
    public:
        typedef enum
            {
            SPI0 = 0,
            SPI1 = 1,
            SPI_MAX
            } DeviceId;

        struct SpiContext
            {
            DeviceId device;    // SPI device index to use Spi::SPI{0|1}
            int pinSCK;         // SPI SCK pin
            int pinCS;          // SPI CS pin
            int pinTX;          // SPI TX pin
            int pinRX;          // SPI RX pin
            int speedInMhz;     // SPI clock rate
            };
 
  
	/*************************************************************************\
    |* Properties
    \*************************************************************************/
    GET(spi_inst_t*, device);
    GET(SpiContext, ctx);

       
    public:
        /*********************************************************************\
        |* Constructors and Destructor
        \*********************************************************************/
        Spi(void);


        /*********************************************************************\
        |* Initialise the port
        \*********************************************************************/
        int init(SpiContext ctx);

	
    private:
        /*********************************************************************\
        |* Get the corresponding device for an enum
        \*********************************************************************/
        spi_inst_t * _spiDevice(DeviceId device);
    };

