#ifndef __HARDWARE_H__
#define __HARDWARE_H__  

/*
    INCLUDE 
*/
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include <stdbool.h>

/*
    DEFINE 
*/

/*===========================================================power==========================================*/
#define GSM_PWKEY_Pin   GPIO_Pin_0
#define GSM_PWKEY_GPIO  GPIOB

#define GSM_PWKEY_LOW() GPIO_SetBits(GSM_PWKEY_GPIO, GSM_PWKEY_Pin)
#define GSM_PWKEY_HIGH() GPIO_ResetBits(GSM_PWKEY_GPIO, GSM_PWKEY_Pin)
/*===========================================================uart==========================================*/
#define DEBUG_TX    GPIO_Pin_2
#define DEBUG_RX    GPIO_Pin_3
#define DEBUG   GPIOA
#define DEBUG_UART USART2

#define SIM_TX    GPIO_Pin_9
#define SIM_RX    GPIO_Pin_10 
#define SIM     GPIOA 
#define SIM_UART    USART1 
/*
    FUNCION
*/
void rcc_config(void);
void systick_config(void);
void gsm_gpio_init(void);
bool gsm_power_on(void);
bool gsm_power_off(void);

uint32_t get_tick_ms(void);
void tick_ms_increment(void);
void delay_ms(uint32_t ms);

#endif // __HARDWARE_H__