#pragma once

#include <stdint.h>
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
    |* Enums
    \*************************************************************************/
    public:
        enum Rotation
            {
            PORTRAIT                         = 0,
            LANDSCAPE,
            INVERTED_PORTRAIT,
            INVERTED_LANDSCAPE
            };

 	/*************************************************************************\
    |* Properties
    \*************************************************************************/
    GET(Rect, bounds);                      // Overall bounds of the display
    GETSET(Rect, clip, Clip);               // Current clipping rectangle
    GET(Rotation, rotation);                // Orientation of the display

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
        int fetchAddressMode(void);

        /*********************************************************************\
        |* Reset the clip rectangle
        \*********************************************************************/
        void resetClipRectangle(void);

        /*********************************************************************\
        |* Set the display orienatation
        \*********************************************************************/
        void setRotation(Rotation rotation);

        /*********************************************************************\
        |* Draw a line
        \*********************************************************************/
        void line(int x1, int y1, int x2, int y2, RGB colour);

        /*********************************************************************\
        |* Draw a rectangle, optionally filled
        \*********************************************************************/
        void box(Rect r, RGB rgb, bool filled=false);
        
        /*********************************************************************\
        |* Draw a pixel
        \*********************************************************************/
        void plot(int x, int y, RGB colour);
        
        /*********************************************************************\
        |* Draw a circle
        \*********************************************************************/
        void circle(int x, int y, int r, RGB colour, bool filled=false);
    
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
        void _pushBlock(Rect r, uint16_t *rgb, bool handleCS=false);

        /*********************************************************************\
        |* Draw a horizontal or vertical line
        \*********************************************************************/
        void _hline(int x, int y, int w, RGB colour);
        void _vline(int x, int y, int h, RGB colour);

        /*********************************************************************\
        |* Fill a rectangle on the screen with a given colour
        \*********************************************************************/
        void _rectFill(Rect r, RGB rgb);

        /*********************************************************************\
        |* Paint a circle
        \*********************************************************************/
        void _circle(int x, int y, int r, RGB rgb);


        /*********************************************************************\
        |* 
        \*********************************************************************/
       

        /*********************************************************************\
        |* 
        \*********************************************************************/
       
   };