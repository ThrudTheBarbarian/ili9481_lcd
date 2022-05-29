#include "Ili9481.h"

/*****************************************************************************\
|* Defines
\*****************************************************************************/

#define LCD_DELAY       0x80

#define LCD_CS(state)   gpio_put(_ctx.spi.pinCS, state)
#define LCD_DATA        gpio_put(_ctx.pinCD, Gpio::HI)
#define LCD_CMD         gpio_put(_ctx.pinCD, Gpio::LO)

#define BUSY_DELAY      10
#define SPI_WRITE(ptr, num)                                                 \
    while (!spi_is_writable(_spi.device()))                                 \
        sleep_us(BUSY_DELAY);                                               \
    spi_write_blocking(_spi.device(), ptr, num)

#define CLIP_TALL       {0, 0, 319, 479}
#define CLIP_WIDE       {0, 0, 479, 319}

#define CMD_START       LCD_CS(Gpio::LO); LCD_CMD
#define CMD_STOP        LCD_CS(Gpio::HI)

/*****************************************************************************\
|* Enums
\*****************************************************************************/

enum
    {
    SPI_CMD_SET_COLUMN_ADDRESS          = 0x2A,
    SPI_CMD_SET_ROW_ADDRESS             = 0x2B,
    SPI_CMD_WRITE_MEMORY_START          = 0x2C,
    SPI_CMD_SET_ADDRESS_MODE            = 0x36,
    };

enum
    {
    AM_VERTICAL_FLIP                    = 0x01,
    AM_HORIZONTAL_FLIP                  = 0x02,
    AM_BGR                              = 0x08,
    AM_SWAP_PAGE_COLUMN                 = 0x20,
    };

/*****************************************************************************\
|* Statics
\*****************************************************************************/

static Rect _limits = CLIP_TALL;        // Start in portrait mode

static const uint8_t _initData[] =
    {
    /*************************************************************************\
    |* 0x00     : NOP
    |* 
    |* This is sent as a dummy byte to make sure the SCK line has the correct
    |* polarity for the *real* 0x01 next...
    \*************************************************************************/
	1, 0x00,

    /*************************************************************************\
    |* 0x01     : Software reset
    |* 
    |* Try to avoid any problems with the SPI engine being confused on the 
    |* display
    \*************************************************************************/
	1, 0x01,
	LCD_DELAY, 240,

    /*************************************************************************\
    |* 0x11     : Exit sleep mode
    |* 
    |* No parameters, must wait 5ms before sending cmd, or 120ms before sleep
    \*************************************************************************/
	1, 0x11,
	LCD_DELAY, 120,

    /*************************************************************************\
    |* 0xD0     : Power setting: (pg 124, 8.2.46)
    |*
    |*  0x7     : Reference voltage - 1.0x Voltage-in
    |*  0x42    : Generate Vgl, config power rails
    |*  0x18    : Internal ref=2.5v
    \*************************************************************************/
	4, 0xD0, 0x07, 0x42, 0x18,

    /*************************************************************************\
    |* 0xD1     : VCOM voltages (pg 116, 8.2.47)
    |* 
    |* 0x00     : Register D1 selected for VCM setting 
    |* 0x07     : VCOMH voltage     = 0.72 * VREG1_OUT
    |* 0x02     : VCOM amplitude    = 0.74 * VREG1_OUT
    \*************************************************************************/
	4, 0xD1, 0x00, 0x07, 0x10,

    /*************************************************************************\
    |* 0xD2     : Power setting for normal mode (pg 118, 8.2.48)
    |* 
    |* 0x01     : Gamma driver = 1.0x, Source driver = 1.0x
    |* 0x02     : fDCDC1 = Fosc, fDCDC2 = Fosc/64
    \*************************************************************************/
	3, 0xD2, 0x01, 0x02,

    /*************************************************************************\
    |* 0xC0     : Panel driving setting (pg 101, 8.2.39)
    |* 
    |* 0x10     : Enable greyscale inversion, scan top left to bottom right
    |* 0x3B     : Number of groups of 8 lines to drive on LCD (0x3b=480 lines)
    |* 0x00     : Starting point in graphics RAM for scanning LCD data in
    |* 0x02     : Inactive areas, output +ve, polarity GND, amp:both, freqs:Y
    |* 0x11     : Inactive areas, interval scan, 3 frames, 50ms
    \*************************************************************************/
	6, 0xC0, 0x10, 0x3B, 0x00, 0x02, 0x11,

    /*************************************************************************\
    |* 0xC5     : Frame Rate and Inversion Control (pg 111, 8.2.43)
    |* 
    |* 0x00     : 125 fps
    \*************************************************************************/
	2, 0xC5, 0x00,

    /*************************************************************************\
    |* 0xC8     : Gamma (pg 113, 8.2.45)
    |* 
    |* params   : See datasheet :)
    \*************************************************************************/
	13, 0xC8, 0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 
              0x37, 0x75, 0x77, 0x54, 0x0C, 0x00,


    /*************************************************************************\
    |* 0x36     : Set_address_mode (pg 79, 8.2.25)
    |* 
    |* 0x0A     : bit 7 = top to bottom order
    |*            bit 6 = left to right order
    |*            bit 5 = normal page/column order
    |*            bit 4 = LCD refresh from top to bottom
    |*            bit 3 = pixels in BGR order 
    |*            bit 2 = n/a 
    |*            bit 1 = flip horozontally 
    |*            bit 0 = no flip vertically
    \*************************************************************************/
	2, 0x36, 0x0A,

    /*************************************************************************\
    |* 0x3A     : Set_pixel_format (pg 87, 8.2.29)
    |* 
    |* 0x55     : bits 6,5,4    = DPI pixel format is 16 bits/pix
    |*          : bits 2,1,0    = DBI pixel format is 16 bits/pix
    \*************************************************************************/
	2, 0x3A, 0x66,

    /*************************************************************************\
    |* 0x29     : Enter invert mode
    |* 
    |* No parameters
    \*************************************************************************/
    1, 0x21,

    /*************************************************************************\
    |* 0xB3     : Frame memory access
    |* 
    |* No parameters
    \*************************************************************************/
    5, 0xB3, 0x00, 0x00, 0x00, 0x01,

    /*************************************************************************\
    |* 0x2A     : Set_column_address (pg 60, 8.2.17)
    |* 
    |* 0x00     : left-most column,  hi-byte = 0
    |* 0x00     : left-most column,  lo-byte = 0        = 0
    |* 0x01     : right-most column, hi-byte = 1
    |* 0x3F     : right-most column, lo-byte = 63       = 319
    \*************************************************************************/
	5, 0x2A, 0x00, 0x00, 0x01, 0x3F,
	LCD_DELAY, 0xff,

    /*************************************************************************\
    |* 0x2B     : Set_page_address (pg 62, 8.2.18) [page = row]
    |* 
    |* 0x00     : upper-most row,  hi-byte = 0
    |* 0x00     : upper-most row,  lo-byte = 0        = 0
    |* 0x01     : lower-most row,  hi-byte = 1
    |* 0xDF     : lower-most row,  lo-byte = 223      = 479
    \*************************************************************************/
	5, 0x2B, 0x00, 0x00, 0x01, 0xDF,

	LCD_DELAY, 120,

    /*************************************************************************\
    |* 0x29     : Turn the display on (pg 59, 8.2.16)
    |* 
    |* No parameters
    \*************************************************************************/
	1, 0x29,

	LCD_DELAY, 25,

    /*************************************************************************\
    |* End of parameters
    \*************************************************************************/
	0
    };

/*****************************************************************************\
|* Constructor
\*****************************************************************************/
Ili9481::Ili9481(void)
        :_clip(_limits)
        ,_rotation(Ili9481::PORTRAIT)
    {}

/*****************************************************************************\
|* Initialise an ILI9481 display driver using the SPI interface
\*****************************************************************************/
int Ili9481::init(DpyContext *ctx)
    {
    /*************************************************************************\
    |* Make sure the context is valid
    \*************************************************************************/
   if (ctx == nullptr)
        {
        printf(T_ERR "Passed null pointer to Display init()\n");
        return E_INVALID;
        }

    /*************************************************************************\
    |* Copy context and start to use it to configure with
    \*************************************************************************/
    _ctx        = *ctx;
    int errs    = 0;

    /*************************************************************************\
    |* Enable SPI CS and pull it high
    \*************************************************************************/
    if (_ctx.spi.pinCS >= 0)
        {
        gpio_set_dir(_ctx.spi.pinCS, Gpio::OUTPUT);
        gpio_put(_ctx.spi.pinCS, Gpio::HI);
        }
    else
        errs++;


    /*************************************************************************\
    |* Enable the C/D pin as an output if >=0, pull it high
    \*************************************************************************/
    if ((errs==0 ) && (_ctx.pinCD >= 0))
        {
        gpio_set_dir(_ctx.pinCD, Gpio::OUTPUT);
        gpio_pull_up(_ctx.pinCD);
        }

    /*************************************************************************\
    |* Enable the /RST pin as an output if >=0, and trigger a reset
    \*************************************************************************/
    if ((errs==0 ) && (_ctx.pinRST >= 0))
        {
        gpio_set_dir(_ctx.pinRST, Gpio::OUTPUT);
        gpio_pull_up(_ctx.pinRST);

        gpio_put(_ctx.pinRST, Gpio::HI);
        
        sleep_ms(5);
        gpio_put(_ctx.pinRST, Gpio::LO);
        sleep_ms(15);
        gpio_put(_ctx.pinRST, Gpio::HI);
        sleep_ms(150);
        }
    else
        errs ++;

    /*************************************************************************\
    |* Then try to bring up SPI
    \*************************************************************************/
    int ok = 0;
    if (errs == 0)
        {
        int ok = _spi.init(_ctx.spi);
        if (ok != E_OK)
            {
            errs ++;
            printf("Cannot initialise SPI%d device for ILI9481\n", 
                    _ctx.spi.device);
            }
        }

    /*************************************************************************\
    |* Configure the spi interface
    \*************************************************************************/
    spi_set_format(_spi.device(), 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);

    /*************************************************************************\
    |* Run the default init sequence
    \*************************************************************************/
	uint8_t  numBytes;
    uint8_t * addr  = (uint8_t *) _initData;

	while ((numBytes=(*addr++))>0)              // end marker == 0
        { 
		if ( numBytes & LCD_DELAY) 
            {
			uint8_t tmp = *addr++;
			sleep_ms(tmp);                      // up to 255 millis
		    } 
        else 
            {
            _writeCommand(*addr ++); 
            while (--numBytes) 
				_writeData(*addr ++); 
		    }
	    }


    return (errs == 0) ? E_OK : -errs;
    }



/*****************************************************************************\
|* Method : Fetch the address mode
\*****************************************************************************/
int Ili9481::fetchAddressMode(void)
    {
    /*************************************************************************\
    |* Start the command (handle /CS and C/D)
    \*************************************************************************/
    CMD_START;

    uint8_t cmd = 0x0B;
    SPI_WRITE(&cmd, 1); 
    
    /*************************************************************************\
    |* Enter data mode and read back the value
    \*************************************************************************/
    LCD_DATA;
    uint8_t result[3] = {0xff,0xff,0x0};
    spi_read_blocking(_spi.device(), 0xfe, result, 2);

    /*************************************************************************\
    |* Stop the command (handle /CS)
    \*************************************************************************/
    CMD_STOP;

    return result[1] & 0xF8;
    }


/*****************************************************************************\
|* Method : Reset the clip rectangle
\*****************************************************************************/
void Ili9481::resetClipRectangle(void)
    {
    _clip = _limits;
    }

/*****************************************************************************\
|* Method : Set the display orienatation
\*****************************************************************************/
void Ili9481::setRotation(Rotation rotation)
    {
    /*************************************************************************\
    |* Set the address mode 
    \*************************************************************************/
    LCD_CS(Gpio::LO);
    _writeCommand(SPI_CMD_SET_ADDRESS_MODE);

    switch (rotation)
        {
        case PORTRAIT:
            _writeData(AM_BGR | AM_HORIZONTAL_FLIP);
            _limits = CLIP_TALL;
            break;
        case LANDSCAPE:
            _writeData(AM_BGR | AM_SWAP_PAGE_COLUMN);
            _limits = CLIP_WIDE;
            break;
        case INVERTED_PORTRAIT:
            _writeData(AM_BGR | AM_VERTICAL_FLIP);
            _limits = CLIP_TALL;
            break;
        case INVERTED_LANDSCAPE:
            _writeData(AM_BGR 
                     | AM_SWAP_PAGE_COLUMN 
                     | AM_HORIZONTAL_FLIP
                     | AM_HORIZONTAL_FLIP);
            _limits = CLIP_WIDE;
            break;
        }
 
    setClip(_limits);
 
    /*************************************************************************\
    |* Stop the command (handle /CS)
    \*************************************************************************/
    CMD_STOP;
    }

/*****************************************************************************\
|* Method : draw a circle, optionally filled
\*****************************************************************************/
void Ili9481::circle(int x, int y, int r, RGB rgb, bool filled)
    {
    if (filled == false)
        _circle(x, y, r, rgb);
    else
        {
        int  xx  = 0;
        int  dx = 1;
        int  dy = r+r;
        int  p  = -(r>>1);

        _hline(x-r, y, dy+1, rgb);
        while (xx < r)
            {
            if (p>=0)
                {
                _hline(x - xx, y + r, dx, rgb);
                _hline(x - xx, y - r, dx, rgb);
                dy-=2;
                p-=dy;
                r--;
                }

            dx+=2;
            p+=dx;
            xx++;

            _hline(x - r, y + xx, dy+1, rgb);
            _hline(x - r, y - xx, dy+1, rgb);
            }
        }
    }

/*****************************************************************************\
|* Method : draw an ellipse, optionally filled
\*****************************************************************************/
void Ili9481::ellipse(int x, int y, int rx, int ry, RGB rgb, bool filled)
    {
    if (filled)
        _ellipseFill(x, y, rx, ry, rgb);
    else
        _ellipse(x, y, rx, ry, rgb);
    }


/*****************************************************************************\
|* Method : draw a line
\*****************************************************************************/
void Ili9481::line(int x0, int y0, int x1, int y1, RGB rgb)
    {
    if (y1 == y0)
        _hline(x0, y0, x1-x0, rgb);
    else if (x0 == x1)
        _vline(x0, y0, y1-y0, rgb);
    else
        {
        bool steep = abs(y1 - y0) > abs(x1 - x0);
        if (steep) 
            {
            Swapper(x0, y0);
            Swapper(x1, y1);
            }

        if (x0 > x1) 
            {
            Swapper(x0, x1);
            Swapper(y0, y1);
            }

        int dx = x1 - x0;
        int dy = abs(y1 - y0);

        int err = dx >> 1;
        int ystep = -1;
        int xs = x0;
        int dlen = 0;

        if (y0 < y1) 
            ystep = 1;

        // Split into steep and not steep for FastH/V separation
        if (steep) 
            {
            for (; x0 <= x1; x0++) 
                {
                dlen++;
                err -= dy;
                if (err < 0) 
                    {
                    if (dlen == 1) 
                        plot(y0, xs, rgb);
                    else 
                        _vline(y0, xs, dlen, rgb);
                    dlen = 0;
                    y0 += ystep; xs = x0 + 1;
                    err += dx;
                    }
                }
            
            if (dlen) 
                _vline(y0, xs, dlen, rgb);
            }
        else
            {
            for (; x0 <= x1; x0++) 
                {
                dlen++;
                err -= dy;
                if (err < 0) 
                    {
                    if (dlen == 1) 
                        plot(xs, y0, rgb);
                    else 
                        _hline(xs, y0, dlen, rgb);
                    dlen = 0;
                    y0 += ystep; xs = x0 + 1;
                    err += dx;
                    }
                }
            if (dlen) 
                _hline(xs, y0, dlen, rgb);
            }
        }
    }


/*****************************************************************************\
|* Method : plot a pixel
\*****************************************************************************/
void Ili9481::plot(int x, int y, RGB rgb)
    {
    _hline(x, y, 1, rgb);
    }

/*****************************************************************************\
|* Method : draw a rectangle, filled or not
\*****************************************************************************/
void Ili9481::box(Rect r, RGB rgb, bool filled, int pix)
    {
    if (pix > 0)
        {
        int px2 = pix * 2;
        int rx2 = r.x + r.w - pix - 1;
        int ry2 = r.y + r.h - pix - 1;
        
        if (filled)
            {
            _rectFill({r.x, r.y + pix, r.w, r.h - px2}, rgb);

            // draw four corners
            int delta = r.w - px2 - 1;
            _filledCircleHelper(r.x + pix, ry2, pix, 1, r.w - px2 - 1, rgb);
            _filledCircleHelper(r.x + pix, r.y + pix, pix, 2, delta, rgb);
            }
        else
            {
            _hline(r.x + pix    , r.y          , r.w - px2, rgb); // Top
            _hline(r.x + pix    , r.y + r.h - 1, r.w - px2, rgb); // Bottom
            _vline(r.x          , r.y + pix    , r.h - px2, rgb); // Left
            _vline(r.x + r.w - 1, r.y + pix    , r.h - px2, rgb); // Right
            
            // draw four corners
            _circleHelper(r.x + pix , r.y + pix , pix, 1, rgb);
            _circleHelper(rx2       , r.y + pix , pix, 2, rgb);
            _circleHelper(rx2       , ry2       , pix, 4, rgb);
            _circleHelper(r.x + pix , ry2       , pix, 8, rgb);
            }
        }
    else if (filled)
        _rectFill(r, rgb);
    else
        {
        _hline(r.x, r.y, r.w, rgb);
        _hline(r.x, r.y+r.h, r.w, rgb);
        _vline(r.x, r.y, r.h, rgb);
        _vline(r.x+r.w, r.y, r.h, rgb);
        }
    }


/*****************************************************************************\
|* Method : Fill the screen with a colour
\*****************************************************************************/
void Ili9481::clear(RGB rgb)
    {
    _rectFill(_limits, rgb);
    }

#pragma mark - Private Methods

/*****************************************************************************\
|* Private Method : write a command over the SPI bus
\*****************************************************************************/
void Ili9481::_writeCommand(uint8_t c, bool handleCS)
    {
    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::LO);
    
    /*************************************************************************\
    |* Enter command mode
    \*************************************************************************/
    LCD_CMD;

    /*************************************************************************\
    |* Write the SPI command
    \*************************************************************************/
    SPI_WRITE(&c, 1); 
     
    /*************************************************************************\
    |* Return to data mode
    \*************************************************************************/
    LCD_DATA;

    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::HI);
   }

/*****************************************************************************\
|* Private Method : write a data byte over the SPI bus
\*****************************************************************************/
void Ili9481::_writeData(uint8_t d, bool handleCS)
    {
    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::LO);
    
    /*************************************************************************\
    |* Should already be in data mode, but...
    \*************************************************************************/
    LCD_DATA;

    /*************************************************************************\
    |* Write the SPI data
    \*************************************************************************/
    SPI_WRITE(&d, 1); 
     
    /*************************************************************************\
    |* This is just a delay, really
    \*************************************************************************/
    LCD_CS(Gpio::LO);

    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::HI);
    }

/*****************************************************************************\
|* Private Method : set the active window in which to write data.
\*****************************************************************************/
void Ili9481::_setWindow(Rect r, bool handleCS)
    {
    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::LO);

    /*************************************************************************\
    |* Set the top-left corner
    \*************************************************************************/
    _writeCommand(SPI_CMD_SET_COLUMN_ADDRESS, false);
    _writeData((r.x >> 8) & 0xFF, false);
    _writeData((r.x & 0xFF), false);
    _writeData(((r.x + r.w) >> 8) & 0xFF, false);
    _writeData(((r.x + r.w) & 0xFF), false);
 
    /*************************************************************************\
    |* Set the bottom right corner
    \*************************************************************************/
    _writeCommand(SPI_CMD_SET_ROW_ADDRESS, false);
    _writeData((r.y >> 8) & 0xFF, false);
    _writeData((r.y & 0xFF), false);
    _writeData(((r.y + r.h) >> 8) & 0xFF, false);
    _writeData(((r.y + r.h) & 0xFF), false);
 
    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::HI);
    }


/*****************************************************************************\
|* Private Method : Push a block of N 16-bit words to the LCD.
|*
|* On entry to this method, CS ought to be already low
\*****************************************************************************/
void Ili9481::_pushBlock(Rect r, RGB rgb, bool handleCS)
    {
    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::LO);

    
    /*************************************************************************\
    |* Signal that we're about to write pixel data
    \*************************************************************************/
    _writeCommand(SPI_CMD_WRITE_MEMORY_START, false);

    /*************************************************************************\
    |* And do so...
    \*************************************************************************/
    uint8_t buf[3]  = {rgb.r, rgb.g, rgb.b};
    int num         = (r.w + 1) * (r.h + 1);
    for (int i=0; i<num; i++)
        spi_write_blocking(_spi.device(), buf,3);

    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::HI);
    }

/*****************************************************************************\
|* Private Method : Push a block of N 16-bit words to the LCD.
|*
|* On entry to this method, CS ought to be already low
\*****************************************************************************/
void Ili9481::_pushBlock(Rect r, uint16_t *rgb, bool handleCS)
    {
    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::LO);

    /*************************************************************************\
    |* Signal that we're about to write pixel data
    \*************************************************************************/
    _writeCommand(SPI_CMD_WRITE_MEMORY_START, false);

    /*************************************************************************\
    |* And do so...
    \*************************************************************************/
    uint8_t  buf[3];
    int num         = (r.w + 1) * (r.h + 1);
    for (int i=0; i<num; i++)
        {
        uint16_t pix = *rgb ++;
        buf[0] = (pix & 0xF800) >> 9;
        buf[1] = (pix & 0x07E0) >> 3;
        buf[2] = (pix & 0x003F) << 2;
        spi_write_blocking(_spi.device(), buf,3);
        }

    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    if (handleCS)
        LCD_CS(Gpio::HI);
    }


/*****************************************************************************\
|* Private Method : Optimised method to draw a horizontal line
\*****************************************************************************/
void Ili9481::_hline(int x, int y, int w, RGB colour)
    {
    /*************************************************************************\
    |* Clipping : early exit if we're entirely out of bounds
    \*************************************************************************/
    if ((y < _clip.y) || (x >= _clip.x + _clip.w) || (y >= _clip.y + _clip.h))
        return;
    
    /*************************************************************************\
    |* Clipping : adjust x
    \*************************************************************************/
    if (x < _clip.x)
        {
        w += x - _clip.x;
        x  = _clip.x;
        }
    
    if (x+w > _clip.x  + _clip.w)
        w = _clip.w - x;
    
    /*************************************************************************\
    |* Clipping : exit if we're invisible
    \*************************************************************************/
    if (w < 1)
        return;
    

    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    LCD_CS(Gpio::LO);

    /*************************************************************************\
    |* Set the window on-screen that we want to fill to
    \*************************************************************************/
    Rect r = {x, y, w, 1};
    _setWindow(r);
    _pushBlock(r, colour);
 
    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    LCD_CS(Gpio::HI);
    }

/*****************************************************************************\
|* Private Method : Optimised method to draw a vertical line
\*****************************************************************************/
void Ili9481::_vline(int x, int y, int h, RGB colour)
    {
    /*************************************************************************\
    |* Clipping : early exit if we're entirely out of bounds
    \*************************************************************************/
    if ((x < _clip.x) || (x >= _clip.x + _clip.w) || (y >= _clip.y + _clip.h))
        return;
    
    /*************************************************************************\
    |* Clipping : adjust x
    \*************************************************************************/
    if (y < _clip.y)
        {
        h += y - _clip.y;
        y  = _clip.y;
        }
    
    if (y+h > _clip.y  + _clip.h)
        h = _clip.h - y;
    
    /*************************************************************************\
    |* Clipping : exit if we're invisible
    \*************************************************************************/
    if (h < 1)
        return;
    
    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    LCD_CS(Gpio::LO);

    /*************************************************************************\
    |* Set the window on-screen that we want to fill to
    \*************************************************************************/
    Rect r = {x, y, 1, h};
    _setWindow(r);
    _pushBlock(r, colour);
 
    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    LCD_CS(Gpio::HI);
    }

/*****************************************************************************\
|* Private Method : Fill a rectangle on the display with a colour
\*****************************************************************************/
void Ili9481::_rectFill(Rect r, RGB colour)
    {
    /*************************************************************************\
    |* Clipping : If we're entirely OUB just return
    \*************************************************************************/
    if ((r.x >= _clip.x + _clip.w) || (r.y >= _clip.y + _clip.h))
        return;
    
    /*************************************************************************\
    |* Clipping : If we're off to the left, clip x & adjust w
    \*************************************************************************/
    if (r.x < _clip.x)
        {
        r.w += r.x - _clip.x;
        r.x  = _clip.x;
        }
  
    /*************************************************************************\
    |* Clipping : If we're up above, clip y & adjust h
    \*************************************************************************/
    if (r.y < _clip.y)
        {
        r.h += r.y - _clip.y;
        r.y  = _clip.y;
        }
   
    /*************************************************************************\
    |* Clipping : Make sure we're not too wide or too tall
    \*************************************************************************/
    if ((r.x + r.w) > _clip.w)
        r.w = _clip.w - r.x;
    
    if ((r.y + r.h) > _clip.h)
        r.h = _clip.h - r.y;
   
    /*************************************************************************\
    |* Clipping : If clipping means nothing to draw, return
    \*************************************************************************/
    if ((r.w < 1) || (r.h < 1))
        return;

    /*************************************************************************\
    |* Take CS low
    \*************************************************************************/
    LCD_CS(Gpio::LO);

    /*************************************************************************\
    |* Set the window on-screen that we want to fill to
    \*************************************************************************/
    _setWindow(r);
    _pushBlock(r, colour);
 
    /*************************************************************************\
    |* Take CS high
    \*************************************************************************/
    LCD_CS(Gpio::HI);
   }

/*****************************************************************************\
|* Private Method : plot a circle
\*****************************************************************************/
void Ili9481::_circle(int x, int y, int r, RGB rgb)
    {
    int f       =  1 - r;
    int ddfY    = -2 * r;
    int ddfX    =  1;
    int xs      = -1;
    int xe      =  0;
    int len     =  0;

    bool first  = true;
    do
        {
        while (f < 0)
            {
            xe ++;
            f  += (ddfX += 2);
            }
        f += (ddfY += 2);

        if (xe > xs - 1)
            {
            if (first)
                {
                len = 2 * (xe - xs) - 1;
                _hline(x - xe, y + r, len, rgb);
                _hline(x - xe, y - r, len, rgb);
                _vline(x + r, y - xe, len, rgb);
                _vline(x - r, y - xe, len, rgb);
                first = false;
                }
            else
                {
                len = xe - xs++;
                _hline(x - xe, y + r, len, rgb);
                _hline(x - xe, y - r, len, rgb);
                _hline(x + xs, y - r, len, rgb);
                _hline(x + xs, y + r, len, rgb);

                _vline(x + r, y + xs, len, rgb);
                _vline(x + r, y - xe, len, rgb);
                _vline(x - r, y - xe, len, rgb);
                _vline(x - r, y + xs, len, rgb);
                }
            }
        else
            {
            xs ++;
            plot(x - xe, y + r, rgb);
            plot(x - xe, y - r, rgb);
            plot(x + xs, y - r, rgb);
            plot(x + xs, y + r, rgb);

            plot(x + r, y + xs, rgb);
            plot(x + r, y - xe, rgb);
            plot(x - r, y - xe, rgb);
            plot(x - r, y + xs, rgb);
            }
        xs = xe;
        }
    while (xe < --r);
    }

/*****************************************************************************\
|* Private Method : circle plotting helper
\*****************************************************************************/
void Ili9481::_circleHelper(int x0, int y0, int rr, uint8_t corner, RGB rgb)
    {
    if (rr <= 0) 
        return;
  
    int f     = 1 - rr;
    int ddF_x = 1;
    int ddF_y = -2 * rr;
    int xe    = 0;
    int xs    = 0;
    int len   = 0;

    while (xe < rr--)
        {
        while (f < 0) 
            {
            xe ++;
            f += (ddF_x += 2);
            }
        
        f += (ddF_y += 2);

        if (xe-xs==1) 
            {
            if (corner & 0x1) 
                { // left top
                plot(x0 - xe, y0 - rr, rgb);
                plot(x0 - rr, y0 - xe, rgb);
                }

            if (corner & 0x2) 
                { // right top
                plot(x0 + rr    , y0 - xe, rgb);
                plot(x0 + xs + 1, y0 - rr, rgb);
                }
        
            if (corner & 0x4) 
                { // right bottom
                plot(x0 + xs + 1, y0 + rr, rgb);
                plot(x0 + rr, y0 + xs + 1, rgb);
                }
        
            if (corner & 0x8) 
                { // left bottom
                plot(x0 - rr, y0 + xs + 1, rgb);
                plot(x0 - xe, y0 + rr    , rgb);
                }
            }
        else 
            {
            len = xe - xs++;
        
            if (corner & 0x1) 
                { // left top
                _hline(x0 - xe, y0 - rr, len, rgb);
                _vline(x0 - rr, y0 - xe, len, rgb);
                }
        
            if (corner & 0x2) 
                { // right top
                _vline(x0 + rr, y0 - xe, len, rgb);
                _hline(x0 + xs, y0 - rr, len, rgb);
                }

            if (corner & 0x4) 
                { // right bottom
                _hline(x0 + xs, y0 + rr, len, rgb);
                _vline(x0 + rr, y0 + xs, len, rgb);
                }
        
            if (corner & 0x8) 
                { // left bottom
                _vline(x0 - rr, y0 + xs, len, rgb);
                _hline(x0 - xe, y0 + rr, len, rgb);
                }
            }
        xs = xe;
        }
    }

/*****************************************************************************\
|* Private Method : filled circle plotting helper
\*****************************************************************************/
void Ili9481::_filledCircleHelper(int x0, int y0, int r, 
                                  uint8_t corner, int delta, RGB rgb)
    {       
    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -r - r;
    int y     = 0;

    delta++;

    while (y < r) 
        {
        if (f >= 0) 
            {
            if (corner & 0x1) 
                _hline(x0 - y, y0 + r, y + y + delta, rgb);
            if (corner & 0x2) 
                _hline(x0 - y, y0 - r, y + y + delta, rgb);
            r--;
            ddF_y += 2;
            f += ddF_y;
            }

        y++;
        ddF_x += 2;
        f += ddF_x;

        if (corner & 0x1) 
            _hline(x0 - r, y0 + y, r + r + delta, rgb);
        if (corner & 0x2) 
            _hline(x0 - r, y0 - y, r + r + delta, rgb);
        }
    }

/*****************************************************************************\
|* Private Method : draw an ellipse
\*****************************************************************************/
void Ili9481::_ellipse(int x, int y, int rx, int ry, RGB rgb)
    {
    if ((rx<2) || (ry < 2))
        return;

    int xx, yy;
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int fx2 = 4 * rx2;
    int fy2 = 4 * ry2;
    int s;

    for (xx = 0, yy = ry, s = 2*ry2+rx2*(1-2*ry); ry2*xx <= rx2*yy; xx++) 
        {
        // These are ordered to minimise coordinate changes in x or y
        // drawPixel can then send fewer bounding box commands
        plot(x + xx, y + yy, rgb);
        plot(x - xx, y + yy, rgb);
        plot(x - xx, y - yy, rgb);
        plot(x + xx, y - yy, rgb);
        if (s >= 0) 
            {
            s += fx2 * (1 - yy);
            yy--;
            }
        s += ry2 * ((4 * xx) + 6);
        }

    for (xx = rx, yy = 0, s = 2*rx2+ry2*(1-2*rx); rx2*yy <= ry2*xx; yy++) 
        {
        // These are ordered to minimise coordinate changes in x or y
        // drawPixel can then send fewer bounding box commands
        plot(x + xx, y + yy, rgb);
        plot(x - xx, y + yy, rgb);
        plot(x - xx, y - yy, rgb);
        plot(x + xx, y - yy, rgb);
        if (s >= 0)
            {
            s += fy2 * (1 - xx);
            xx--;
            }
        s += rx2 * ((4 * yy) + 6);
        }
    }

/*****************************************************************************\
|* Private Method : draw an ellipse
\*****************************************************************************/
void Ili9481::_ellipseFill(int x, int y, int rx, int ry, RGB rgb)
    {
    if ((rx<2) || (ry < 2))
        return;

    int xx, yy;
    int rx2 = rx * rx;
    int ry2 = ry * ry;
    int fx2 = 4 * rx2;
    int fy2 = 4 * ry2;
    int s;

    for (xx = 0, yy = ry, s = 2*ry2+rx2*(1-2*ry); ry2*xx <= rx2*yy; xx++) 
        {
        _hline(x - xx, y - yy, xx + xx + 1, rgb);
        _hline(x - xx, y + yy, xx + xx + 1, rgb);

        if (s >= 0) 
            {
            s += fx2 * (1 - yy);
            yy--;
            }
        s += ry2 * ((4 * xx) + 6);
        }

    for (xx = rx, yy = 0, s = 2*rx2+ry2*(1-2*rx); rx2*yy <= ry2*xx; yy++) 
        {
        _hline(x - xx, y - yy, xx + xx + 1, rgb);
        _hline(x - xx, y + yy, xx + xx + 1, rgb);

        if (s >= 0) 
            {
            s += fy2 * (1 - xx);
            xx--;
            }
        s += rx2 * ((4 * yy) + 6);
        }
    }