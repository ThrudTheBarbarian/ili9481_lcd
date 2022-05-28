
#include <string.h>

#include "../include/errors.h"
#include "spi.h"

#define SPI0_SCK    0x40044
#define SPI0_CS     (SPI0_SCK >> 1)
#define SPI0_TX     (SPI0_SCK << 1)
#define SPI0_RX     (SPI0_CS  >> 1)

#define SPI1_SCK    0x4400
#define SPI1_CS     (SPI1_SCK >> 1)
#define SPI1_TX     (SPI1_SCK << 1)
#define SPI1_RX     (SPI1_CS  >> 1)

#define SPI_MAX_PIN Gpio::PIN19

typedef struct 
    {
    uint32_t sck;
    uint32_t cs;
    uint32_t tx;
    uint32_t rx;
    } SpiPinMap;

static SpiPinMap _map[2] = 
    {
        {SPI0_SCK, SPI0_CS, SPI0_TX, SPI0_RX}, 
        {SPI1_SCK, SPI1_CS, SPI1_TX, SPI1_RX}
    };

#define SCK_NO_MAP(pin) ((_map[_ctx.device].sck & (1<<pin)) == 0)
#define CS_NO_MAP(pin)  ((_map[_ctx.device].sck & (1<<pin)) == 0)
#define TX_NO_MAP(pin)  ((_map[_ctx.device].sck & (1<<pin)) == 0)
#define RX_NO_MAP(pin)  ((_map[_ctx.device].sck & (1<<pin)) == 0)


/*****************************************************************************\
|* Constructor - set up a SPI bus
\*****************************************************************************/
Spi::Spi(void)
    :_ctx((SpiContext){Spi::SPI0,0,0,0,0,0})
    ,_device(nullptr)
    {}


/*****************************************************************************\
|* Method : Initialise a SPI device with a pin-set and speed
\*****************************************************************************/
int Spi::init(SpiContext c)
    {
    _ctx    = c;
    
    _device = _spiDevice(c.device);
    if (_device == nullptr)
        {
        printf(T_ERR "Cannot initialise SPI%d\n", c.device);
        return E_NO_DEVICE;
        }

    if ((c.pinSCK >= 0) && (c.pinSCK <= SPI_MAX_PIN) && SCK_NO_MAP(c.pinSCK))
        {
        printf("Invalid pin (%d) for SPI%d SCK\n", c.pinSCK, c.device);
        return E_NO_RESOURCE;
        }
    
    if ((c.pinCS >= 0) && (c.pinCS <= SPI_MAX) && CS_NO_MAP(c.pinCS))
        {
        printf("Invalid pin for SPI%d /CS\n", c.device);
        return E_NO_RESOURCE;
        }

    if ((c.pinTX >= 0) && (c.pinTX <= SPI_MAX) && TX_NO_MAP(c.pinTX))
        {
        printf("Invalid pin for SPI%d TX\n", c.device);
        return E_NO_RESOURCE;
        }

    if ((c.pinRX >= 0) && (c.pinRX <= SPI_MAX) && RX_NO_MAP(c.pinRX))
        {
        printf("Invalid pin for SPI%d RX\n", c.device);
        return E_NO_RESOURCE;
        }


     /*************************************************************************\
    |* Configure the spi interface
    \*************************************************************************/
   _ctx.speedInMhz  = spi_init(_device, c.speedInMhz * 1000000) / 1000000;

    if ((c.pinSCK >= 0) && (c.pinSCK <= SPI_MAX_PIN))
        {
        gpio_set_dir(c.pinSCK, Gpio::OUTPUT);
        gpio_put(c.pinSCK, 1);
        gpio_set_function(c.pinSCK, GPIO_FUNC_SPI);
        gpio_pull_up(c.pinSCK);
       }
    
    if ((c.pinCS >= 0) && (c.pinCS <= SPI_MAX_PIN))
        {
        gpio_set_dir(c.pinCS, Gpio::OUTPUT);
        gpio_pull_up(c.pinCS);
       }
    
    if ((c.pinTX >= 0) && (c.pinTX <= SPI_MAX_PIN))
        {
        gpio_set_dir(c.pinTX, Gpio::OUTPUT);
        gpio_set_function(c.pinTX, GPIO_FUNC_SPI);
        gpio_pull_up(c.pinTX);
       }
    
    if ((c.pinRX >= 0) && (c.pinRX <= SPI_MAX_PIN))
        {
        gpio_set_function(c.pinRX, GPIO_FUNC_SPI);
        gpio_pull_up(c.pinRX);
       }

    return E_OK;
    }

/*****************************************************************************\
|* Private method : Get the corresponding device for an enum
\*****************************************************************************/
spi_inst_t * Spi::_spiDevice(Spi::DeviceId device)
    {
    return (device == SPI0)  ? spi0 
         : (device == SPI1)  ? spi1
         : nullptr;
    }