#include "freertos/FreeRTOS.h"      //FreeRTOS funktsioonid
#include "freertos/task.h"          //vTaskDelay kasutamine
#include "driver/gpio.h"            //GPIO pinnide kasutamine
#include "driver/adc.h"             //ADC kasutamiseks (potentsiomeeter)
#include "driver/ledc.h"            //PWM kasutamiseks (LED heledus)

#define BUTTON_PIN 5                //SW4 nupp (GPIO5)
#define POT_CHANNEL ADC_CHANNEL_2   //GPIO2 (potentsiomeeter)
#define LED1_PIN 23                //LED1 (vilkumine)
#define LED2_PIN 22                //LED2 (PWM heledus)

void app_main(void)                 //Programmi algus
{
    // -------------------------------
    // NUPP seadistus
    // -------------------------------
    gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << BUTTON_PIN),  //Kasuta GPIO5
        .mode = GPIO_MODE_INPUT                //Sisend (nupp)
    });

    // -------------------------------
    // LED1 seadistus (tavaline ON/OFF)
    // -------------------------------
    gpio_config(&(gpio_config_t){
        .pin_bit_mask = (1ULL << LED1_PIN),    //GPIO23
        .mode = GPIO_MODE_OUTPUT               //Väljund (LED)
    });

    // -------------------------------
    // ADC seadistus (potentsiomeeter)
    // -------------------------------
    adc1_config_width(ADC_WIDTH_BIT_12);               //12-bit ADC (0–4095)
    adc1_config_channel_atten(POT_CHANNEL, ADC_ATTEN_DB_11);  
    //Lubab lugeda kuni ~3.3V

    // -------------------------------
    // PWM taimer LED2 jaoks
    // -------------------------------
    ledc_timer_config(&(ledc_timer_config_t){
        .speed_mode = LEDC_LOW_SPEED_MODE,     //PWM režiim
        .duty_resolution = LEDC_TIMER_8_BIT,   //8-bit (0–255)
        .timer_num = LEDC_TIMER_0,             //Taimer 0
        .freq_hz = 1000,                      //Sagedus 1000 Hz
        .clk_cfg = LEDC_AUTO_CLK              //Automaatne kell
    });

    // -------------------------------
    // PWM kanal LED2 jaoks
    // -------------------------------
    ledc_channel_config(&(ledc_channel_config_t){
        .gpio_num = LED2_PIN,                 //GPIO22
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,            //PWM kanal 0
        .timer_sel = LEDC_TIMER_0             //Seotud taimeriga
    });

    // -------------------------------
    // Muutujad
    // -------------------------------
    int blink_on = 0;        //Kas vilkumine on sees (1) või väljas (0)
    int led1_state = 0;      //LED1 olek (0/1)
    int old_button = 1;      //Eelmine nupu olek

    TickType_t last_blink = xTaskGetTickCount();  //Viimase vilkumise aeg
    TickType_t last_press = 0;                    //Viimase nupuvajutuse aeg

    while (1) {

        // ---------------------------
        // Loeb sisendid
        // ---------------------------
        int button = gpio_get_level(BUTTON_PIN);   //Nupu olek (0 või 1)
        int pot = adc1_get_raw(POT_CHANNEL);       //Potentsiomeeter (0–4095)

        // ---------------------------
        // LED2 PWM heledus
        // ---------------------------
        int pwm = pot / 16;   //12-bit → 8-bit (0–255)
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm);  //Seab heleduse
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);    //Rakendab

        // ---------------------------
        // Vilkumise kiirus (1–10 Hz)
        // ---------------------------
        // pool perioodi: 500 ms → 50 ms
        int half_period_ms = 500 - ((pot * 450) / 4095);

        // ---------------------------
        // Nupu vajutuse tuvastus
        // ---------------------------
        if (old_button == 1 && button == 0) {   //Lahti → vajutatud
            if (xTaskGetTickCount() - last_press > pdMS_TO_TICKS(200)) {
                blink_on = !blink_on;           //Lülitab vilkumise sisse/välja
                last_press = xTaskGetTickCount();  //Salvestab vajutuse aja
            }
        }

        old_button = button;   //Uuendab nupu oleku

        // ---------------------------
        // LED1 vilkumine
        // ---------------------------
        if (blink_on) {   //Kui vilkumine on sees
            if (xTaskGetTickCount() - last_blink >= pdMS_TO_TICKS(half_period_ms)) {
                led1_state = !led1_state;               //Vahetab LED olekut
                gpio_set_level(LED1_PIN, led1_state);   //Rakendab LED1-le
                last_blink = xTaskGetTickCount();       //Uuendab aega
            }
        } else {
            gpio_set_level(LED1_PIN, 0);   //Kui vilkumine väljas → LED kustus
            led1_state = 0;
        }

        // ---------------------------
        // Väike paus (lubatud)
        // ---------------------------
        vTaskDelay(pdMS_TO_TICKS(10));   //10 ms
    }
}