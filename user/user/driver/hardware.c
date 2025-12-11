#include "hardware.h"
#include "stm32f10x_rcc.h"
#include "misc.h"


static uint32_t tick_ms = 0;

/*
    IT WILL NUMBER OF S 
*/
uint32_t get_tick_ms(void) {
    return tick_ms;
}

void tick_ms_increment(void) {
    tick_ms++;
}

void delay_ms(uint32_t ms) {
    uint32_t start_time = get_tick_ms();
    while((get_tick_ms() - start_time) < ms) {}
}

/*
    RCC CONFIG 
*/
void rcc_config(void){
    
    RCC_HSEConfig(RCC_HSE_ON);
    
    ErrorStatus HSEStatus = RCC_WaitForHSEStartUp();
    
    if(HSEStatus == SUCCESS) {
        
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);  
        FLASH_SetLatency(FLASH_Latency_2); 
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        RCC_PLLCmd(ENABLE);
        
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) {}
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        
        while(RCC_GetSYSCLKSource() != 0x08) {}
        
    }
    
    SystemCoreClockUpdate();
}

/*
    SYSTICK_CONFIG
*/

void systick_config(void){
    
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK);
    
    if(SysTick_Config(SystemCoreClock / 1000) != 0) {
        
        while(1) {
            
        }
    }
    
    NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) - 1);
}

/*
    INIT GSM_GPIO
*/

void gsm_gpio_init(void){
    // POWER
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef gsm_power;

    gsm_power.GPIO_Pin = GSM_PWKEY_Pin;
    gsm_power.GPIO_Speed = GPIO_Speed_50MHz;
    gsm_power.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GSM_PWKEY_GPIO, &gsm_power);

    GSM_PWKEY_HIGH();

    //STATUS

}

/*
    HANDLE_GSM_POWER_ON
*/

bool gsm_power_on(void){
    static uint16_t step_on = 0;
    static uint32_t last_time_ms = 0;
    static uint32_t time_wait_ms = 0;

  
    if(get_tick_ms() - last_time_ms < time_wait_ms) {
        return false;  
    }

    switch (step_on)
    {
    case 0:
        GSM_PWKEY_LOW();  
        last_time_ms = get_tick_ms();
        time_wait_ms = 1000;  
        step_on++;    
        return false;  
        break;

    case 1:
        GSM_PWKEY_HIGH();  
        last_time_ms = get_tick_ms();
        time_wait_ms = 5000; 
        step_on++;    
        return false;  
        break;
    
    default:
        
        return true;  
        break;
    }
}

/*
    HANDLE_GSM_POWER_OFF
*/

bool gsm_power_off(void){
    static uint16_t step_off = 0;
    static uint32_t last_time_ms = 0;
    static uint32_t time_wait_ms = 0;

  
    if(get_tick_ms() - last_time_ms < time_wait_ms) {
        return false;  
    }

    switch (step_off)
    {
    case 0:
        GSM_PWKEY_LOW();  
        last_time_ms = get_tick_ms();
        time_wait_ms = 2500;  
        step_off++;    
        return false;  
        break;

    case 1:
        GSM_PWKEY_HIGH();  
        last_time_ms = get_tick_ms();
        time_wait_ms = 3000; 
        step_off++; 
        return false;  
        break;
    
    default:
        
        return true;  
        break;
    }
}