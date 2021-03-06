#pragma once

#include <stdint.h>
#include <stdlib.h>
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


/*****************************************************************************\
|* Helper construct : swap any type
\*****************************************************************************/
template <typename T> static inline void
Swapper(T& a, T& b) 
    {
    T t = a; 
    a = b; 
    b = t; 
    }


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
        void line(Point p0, Point p1, RGB colour);

        /*********************************************************************\
        |* Draw a rectangle, optionally filled
        \*********************************************************************/
        void box(Rect r, RGB rgb, bool filled=false, int rounded=0);
        
        /*********************************************************************\
        |* Draw a pixel
        \*********************************************************************/
        void plot(Point p, RGB colour);
        
        /*********************************************************************\
        |* Draw a circle, optionally filled
        \*********************************************************************/
        void circle(Point p, int r, RGB colour, bool fill=false);
        
        /*********************************************************************\
        |* Draw an ellipse, optionally filled
        \*********************************************************************/
        void ellipse(Point p, int rx, int ry, RGB rgb, bool fill=false);
          
        /*********************************************************************\
        |* Draw a triangle, optionally filled
        \*********************************************************************/
        void triangle(Point p0, Point p1, Point p2, RGB rgb, bool fill=false);
        
        /*********************************************************************\
        |* Clear the screen to a colour
        \*********************************************************************/
        void clear( RGB rgb = RGB(0,0,0));
    
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
        |* Draw/fill a circle
        \*********************************************************************/
        void _circle(int x, int y, int r, RGB rgb);
        void _circleHelper(int x, int y, int r, uint8_t corner, RGB rgb);
        void _filledCircleHelper(int x0, int y0, int r, 
                                 uint8_t corner, int delta, RGB rgb);


        /*********************************************************************\
        |* Draw/fill an ellipse
        \*********************************************************************/
        void _ellipse(int x, int y, int rx, int ry, RGB rgb);
        void _ellipseFill(int x, int y, int rx, int ry, RGB rgb);

        /*********************************************************************\
        |* Fill a triangle
        \*********************************************************************/
        void _triangleFill(Point p0, Point p1, Point p2, RGB rgb);

   };