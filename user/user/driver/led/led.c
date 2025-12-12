#include "led.h"

void led_init(void){
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
    GPIO_InitTypeDef gpio_config_led;

    gpio_config_led.GPIO_Pin = GPIO_Pin_13;
    gpio_config_led.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_config_led.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_config_led);
}

void turn_on_led(){
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void turn_off_led(){
     GPIO_ResetBits(GPIOC, GPIO_Pin_13); 
}