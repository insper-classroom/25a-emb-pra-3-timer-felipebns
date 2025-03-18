#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include <string.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

const int X_PIN =  16;
const int Y_PIN =  17;

volatile uint64_t start_us = 0;
volatile uint64_t end_us = 0;

volatile bool timer_fired = false;
volatile int echo = 0;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    timer_fired = true;
    return 0;
}

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { //fall
        end_us = to_us_since_boot(get_absolute_time());
    } else if (events == 0x8) { //rise
        start_us = to_us_since_boot(get_absolute_time());
    }
    echo = 1;
}


int main() {    stdio_init_all();

    gpio_init(Y_PIN);
    gpio_set_dir(Y_PIN, GPIO_OUT);

    gpio_init(X_PIN);
    gpio_set_dir(X_PIN, GPIO_IN);

    gpio_set_irq_enabled_with_callback(X_PIN, (GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL), true, &btn_callback);

    double vel_som = 0.03403;

    datetime_t t = {
        .year  = 2025,
        .month = 03,
        .day   = 18,
        .dotw  = 2, // 0 is Sunday, so 3 is Wednesday
        .hour  = 13,
        .min   = 50,
        .sec   = 00
    };

    // Start the RTC
    rtc_init();
    rtc_set_datetime(&t);

    while (true) {
        sleep_ms(1000);

        int ch = 0;
        int i = 0;
        char str[150] = {};
        printf("Digite: \n");
        
        while (ch != '!') {
            ch = getchar();
            str[i] = ch;
            i++;
        }
        str[i] = '\0';
        
        if (strcmp(str, "start!") == 0) {

            while (1) {
                sleep_ms(750);
                
                int ch2 = 0;
                ch2 = getchar_timeout_us(20);

                if (ch2 == 'x') {
                    break;
                }

                gpio_put(Y_PIN, 1);
                sleep_us(10);
                gpio_put(Y_PIN, 0);
                alarm_id_t alarm = add_alarm_in_ms(5000, alarm_callback, NULL, false);

                if (echo){
                    cancel_alarm(alarm);
                }
                
                if (timer_fired) {
                    printf("Fio disconectado \n");
                    timer_fired = 0;
                } else {
                    echo = 0;
                    datetime_t t2 = {0};
                    rtc_get_datetime(&t2);
                    char datetime_buf[256];
                    char *datetime_str = &datetime_buf[0];
                    datetime_to_str(datetime_str, sizeof(datetime_buf), &t2);
    
                    uint64_t dt = end_us - start_us; 
                    if ((dt*vel_som)/2 > 300) {
                        printf("Muito longe \n");
                    } else {
                        printf("%s - %f cm\n", datetime_str,(dt*vel_som)/2); //divide por 2 (ida e volta)
                    }
                }

            }
        }
    }
}
