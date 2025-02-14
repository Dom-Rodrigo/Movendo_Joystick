#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"

#define VRX 27 // input 1 s:24 h:1885 b:4090
#define VRY 26 // input 0 s:24 h:2087 b:4090
#define BUTTON_BOOTSEL 6

uint32_t last_time;
void gpio_irq_handler(uint gpio, uint32_t event_mask) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if (current_time - last_time > 200000){
        if (!gpio_get(BUTTON_BOOTSEL)) {
            last_time = current_time;
            rom_reset_usb_boot(0, 0);
        }
    }   
}

int main()
{
    stdio_init_all();
    adc_init();

    adc_gpio_init(27);
    adc_gpio_init(26);

    gpio_init(BUTTON_BOOTSEL);
    gpio_set_dir(BUTTON_BOOTSEL, GPIO_IN);
    gpio_pull_up(BUTTON_BOOTSEL);

    gpio_set_irq_enabled_with_callback(BUTTON_BOOTSEL, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    while (true) {
        adc_select_input(0);
        uint16_t vry_value = adc_read();

        adc_select_input(1);
        uint16_t vrx_value = adc_read();

        printf("X: %d\n", vrx_value);
        printf("Y: %d\n", vry_value);
        sleep_ms(500);

    }
}
