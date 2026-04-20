#include "freertos/FreeRTOS.h"      //Põhiliste käskude kasutamiseks    
#include "freertos/task.h"          //vTaskDelay kasutamine
#include "driver/gpio.h"            //GPIO pinnide kasutamine
#include "driver/adc.h"             //ADC kasutamiseks, et lugeda potentsiomeetri väärtust
#include "driver/ledc.h"            //PWM kasutamiseks, et muuta LED-i heledust

#define BUTTON_PIN 5                //SW4 nupp (GPIO5)
#define LED_PIN 23                  //LED pin IO23
#define POT_CHANNEL ADC_CHANNEL_2   //GPIO2 (potentsiomeeter)

void app_main(void)                
{
    gpio_config(&(gpio_config_t){             //GPIO seadistus
        .pin_bit_mask = 1ULL << BUTTON_PIN,  //GPIO pin 5 kasutamine
        .mode = GPIO_MODE_INPUT             //Pin määramine sisendiks
    });

    adc1_config_width(ADC_WIDTH_BIT_12);                        //ADC täpsus 12 bitti, vahemik 0-4095
    adc1_config_channel_atten(POT_CHANNEL, ADC_ATTEN_DB_11);    //Mõõtepiirkond, et lugeda 3.3V sisendpinget

    ledc_timer_config(&(ledc_timer_config_t){     //PWM taimeri seadistus (Määrab sageduse ja täpsuse)
        .speed_mode = LEDC_LOW_SPEED_MODE,        //Tööreziim, ESP kasutab LOW_SPEED_MODE
        .duty_resolution = LEDC_TIMER_8_BIT,      //PWM täpsus 8 bit-i
        .timer_num = LEDC_TIMER_0,                //Kasutatav taimeri kanal 0
        .freq_hz = 1000,                          //Sagedus 1000Hz, signaal lülitub väga kiiresti
        .clk_cfg = LEDC_AUTO_CLK                  //Süsteem valib automaatse kellasignaali
    });

    ledc_channel_config(&(ledc_channel_config_t){   //PWM kanali seadisuts, PWM seotakse konkreetse pinniga
        .gpio_num = LED_PIN,                        //See pin on 23, kuhu on ühendatud ka LED
        .speed_mode = LEDC_LOW_SPEED_MODE,          //Tööreziim peab olema sama, mis taimeril
        .channel = LEDC_CHANNEL_0,                  //Kasutatav PWM kanal 0
        .timer_sel = LEDC_TIMER_0                   //Seob selle kanali taimeri kanaliga
    });

    int led = 0, old = 1;                        //Muutuja led hoiab LED-i olekut, 0= väljas
    //Muutuja old hoiab nupu eelmist olekut 1

    while (1) {
        int button = gpio_get_level(BUTTON_PIN);   //Loeb nupu hetkeoleku

        if (old == 1 && button == 0) {             //kontrollib kas nupp läks olekusse "vajutatud"
            led = !led;                            //Vahetab LED-i oleku vastupidiseks 0-1; 1-0
            vTaskDelay(pdMS_TO_TICKS(200));        //Debounce ehk värinaeemaldus, et üks vajutus ei loeks mitmena
        }

        old = button;                              //Jätab nupu oleku meelde järgmiseks tsükli korraks

        if (led)                                   //Kui LED on sisse lülitatud olekus
        {
            int raw = adc1_get_raw(POT_CHANNEL);   //Loeb potentsiomeetrilt väärtuse (0-4095)
            int pwm = raw / 16;                    //Teisendab 12-bitise ADC väärtuse 8-bitiseks PWM-ile (4096 / 16 = 256)

            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, pwm);  //Seab uue ereduse
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);    //Rakendab muudatuse
        }
        else
        {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);    //Seab ereduse nulli
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);    //Rakendab muudatuse
        }

        vTaskDelay(pdMS_TO_TICKS(50));   //paus enne järgmist tsüklit, et vähendada koormust ja stabiliseerida programmi tööd
    }
}
