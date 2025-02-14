#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"

#define VRX 27 // input 1 s:24 h:1885 b:4090
#define VRY 26 // input 0 s:24 h:2087 b:4090
#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13
#define BUTTON_JOYSTICK 22
#define BUTTON_BOOTSEL 6

bool red_on = false;
bool blue_on = false;

uint32_t last_time;
void gpio_irq_handler(uint gpio, uint32_t event_mask) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if (current_time - last_time > 200000){
        if (!gpio_get(BUTTON_BOOTSEL)) {
            last_time = current_time;
            rom_reset_usb_boot(0, 0);
        }
        if (!gpio_get(BUTTON_JOYSTICK)) {
            gpio_put(LED_GREEN, !gpio_get(LED_GREEN));
            last_time = current_time;
        }
    }   
}

uint pwm_setup(uint LED){
    gpio_set_function(LED, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(LED);
    pwm_set_wrap(slice, 4090);
    pwm_set_clkdiv(slice, 69); // 442Hz
    pwm_set_enabled(slice, true);

    return slice;
}


int main()
{
    stdio_init_all();
    adc_init();
    uint slice_red = pwm_setup(LED_RED);
    uint slice_blue = pwm_setup(LED_BLUE);

    adc_gpio_init(27);
    adc_gpio_init(26);
    
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);

    gpio_init(BUTTON_BOOTSEL);
    gpio_set_dir(BUTTON_BOOTSEL, GPIO_IN);
    gpio_pull_up(BUTTON_BOOTSEL);

    gpio_set_irq_enabled_with_callback(BUTTON_BOOTSEL, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        adc_select_input(0);
        uint16_t vry_value = adc_read();

        adc_select_input(1);
        uint16_t vrx_value = adc_read();
        
        if (vry_value < 2087+600 && vry_value > 2087-600) // VALOR DE Y CONTROLA LED AZUL, APAGA SE TIVER NO MEIO
            blue_on = false;
        else 
            blue_on = true;
    
        if (vrx_value < 1880+600 && vrx_value > 1880-600) // VALOR DE X CONTROLA LED VERMELHO, APAGA SE TIVER NO MEIO
            red_on = false;
        else
            red_on = true;        
        
        if (blue_on)
            pwm_set_gpio_level(LED_BLUE, vry_value);
        else
            pwm_set_gpio_level(LED_BLUE, 0);

        if (red_on)
            pwm_set_gpio_level(LED_RED, vrx_value);
        else
            pwm_set_gpio_level(LED_RED, 0);


        printf("X: %d\n", vrx_value);
        printf("Y: %d\n", vry_value);
        sleep_ms(200);

    }
}
