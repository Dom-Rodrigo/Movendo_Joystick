#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/bootrom.h"
#include "hardware/pwm.h"
#include "inc/ssd1306.h"

#define VRX 27 // input 1 s:24 h:1885 b:4090
#define VRY 26 // input 0 s:24 h:2087 b:4090
#define LED_GREEN 11
#define LED_BLUE 12
#define LED_RED 13
#define BUTTON_JOYSTICK 22
#define BUTTON_A 5
#define BUTTON_BOOTSEL 6
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C


bool red_on = false;
bool blue_on = false;
bool leds_pwm = true;
bool cor = true;

ssd1306_t ssd; // Inicializa a estrutura do display


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
            ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
            cor = !cor;
            ssd1306_send_data(&ssd);

            last_time = current_time;
        }
        if (!gpio_get(BUTTON_A)) {
            leds_pwm = !leds_pwm;
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

void handle_leds_joystick_pwm(uint16_t vrx_value, uint16_t vry_value){
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
}

int main()
{
    stdio_init_all();
    adc_init();
    i2c_init(I2C_PORT, 400 * 1000); // Inicia o i2c com 400kHz

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);


    uint slice_red = pwm_setup(LED_RED);
    uint slice_blue = pwm_setup(LED_BLUE);

    adc_gpio_init(27);
    adc_gpio_init(26);
    
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);

    gpio_init(BUTTON_BOOTSEL);
    gpio_set_dir(BUTTON_BOOTSEL, GPIO_IN);
    gpio_pull_up(BUTTON_BOOTSEL);

    gpio_set_irq_enabled_with_callback(BUTTON_BOOTSEL, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    while (true) {
        adc_select_input(0);
        uint16_t vry_value = adc_read();

        adc_select_input(1);
        uint16_t vrx_value = adc_read();
        
        if (leds_pwm){
            handle_leds_joystick_pwm(vrx_value, vry_value);
        }

        printf("X: %d\n", vrx_value);
        printf("Y: %d\n", vry_value);
        sleep_ms(200);

    }
}
