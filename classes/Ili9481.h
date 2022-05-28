#pragma once

#include <stdio.h>
#include "pico/stdlib.h"

#include "spi.h"

#include "../include/errors.h"
#include "../include/properties.h"
#include "../include/structures.h"

/*****************************************************************************\
|* Context for initialising a display driver
\*****************************************************************************/
typedef struct 
    {
    Spi::SpiContext spi;    // Spi interface
    int pinRST;             // LCD reset pin, active low
    int pinCD;              // LCD command/data pin
    } DpyContext;



class Ili9481
    {
    NON_COPYABLE_NOR_MOVEABLE(Ili9481)

 	/*************************************************************************\
    |* Properties
    \*************************************************************************/
    GET(Rect, bounds);                      // Overall bounds of the display
    GETSET(Rect, clip, Clip);               // Current clipping rectangle
    
    private:
        DpyContext _ctx;                    // The display context
        Spi        _spi;                    // The SPI connection

    public:
        /*********************************************************************\
        |* Constructors and Destructor
        \*********************************************************************/
        explicit Ili9481(void);

        /*********************************************************************\
        |* Initialise the set of GPIOs to a known state
        \*********************************************************************/
        int init(DpyContext *ctx);

        /*********************************************************************\
        |* Fill a rectangle on the screen with a given colour
        \*********************************************************************/
        void rectFill(Rect r, RGB rgb);

        /*********************************************************************\
        |* Fill a rectangle on the screen with a given colour
        \*********************************************************************/
        int fetchAddressMode(void);

    
    private:
        /*********************************************************************\
        |* Write a command to the display
        \*********************************************************************/
        void _writeCommand(uint8_t c, bool handleCS=true);

        /*********************************************************************\
        |* Write data to the display
        \*********************************************************************/
        void _writeData(uint8_t d, bool handleCS=true);

        /*********************************************************************\
        |* Set the display active-window to be the supplied rect
        \*********************************************************************/
        void _setWindow(Rect r, bool handleCS=false);

        /*********************************************************************\
        |* Push a block of colour data to the LCD, which will be expecting it
        \*********************************************************************/
        void _pushBlock(Rect r, RGB rgb, bool handleCS=false);

        /*********************************************************************\
        |* 
        \*********************************************************************/
       
   };