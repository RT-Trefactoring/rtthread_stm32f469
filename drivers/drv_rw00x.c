#include <rtthread.h>
#include <rtdevice.h>
#include <drv_spi.h>
#include <board.h>
#define SPI_BUS_NAME    "spi2"
#define CS_PIN          D10 //PH6
#define INT_BUSY_PIN    D9  //PA7
#define RESET_PIN       D8  //PG10
#define MODE_BIT0_PIN   D13 //PD3
#define MODE_BIT1_PIN   CS_PIN
#define MODE_READY_PIN  INT_BUSY_PIN
extern void spi_wifi_isr(int vector);

static void set_rw00x_mode(int mode)
{
    rt_pin_mode(RESET_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MODE_READY_PIN, PIN_MODE_INPUT_PULLDOWN);
    rt_pin_mode(MODE_BIT0_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MODE_BIT1_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(RESET_PIN, PIN_LOW);
    rt_pin_write(MODE_BIT0_PIN, mode & 0x01 ? PIN_HIGH : PIN_LOW);
    rt_pin_write(MODE_BIT1_PIN, mode & 0x02 ? PIN_HIGH : PIN_LOW);
    rt_thread_delay(rt_tick_from_millisecond(100));
    rt_pin_write(RESET_PIN, PIN_HIGH);
    while(!rt_pin_read(MODE_READY_PIN))
    {
    }
    rt_thread_delay(rt_tick_from_millisecond(100));
}
void wifi_spi_device_init(const char * device_name)
{
    set_rw00x_mode(1);
    stm32_spi_bus_attach_device(CS_PIN, SPI_BUS_NAME, device_name);
}
static void int_wifi_irq(void * p)
{
    ((void)p);
    if(rt_pin_read(INT_BUSY_PIN))
    {
        spi_wifi_isr(0);
    }
}

void spi_wifi_hw_init(void)
{
    rt_pin_attach_irq(INT_BUSY_PIN, PIN_IRQ_MODE_FALLING, int_wifi_irq, 0);
    rt_pin_irq_enable(INT_BUSY_PIN, RT_TRUE);
}

rt_bool_t spi_wifi_is_busy(void)
{
    return !rt_pin_read(INT_BUSY_PIN);
}

void spi_wifi_int_cmd(rt_bool_t cmd)
{
    rt_pin_write(D6, cmd);
    rt_pin_write(D5, rt_pin_read(INT_BUSY_PIN));
    rt_pin_irq_enable(INT_BUSY_PIN, cmd);
    rt_pin_write(D5, rt_pin_read(INT_BUSY_PIN));
}


