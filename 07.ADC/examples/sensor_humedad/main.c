#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"

// PINES LEDs
#define LED_VERDE (1<<0)  // P2.0
#define LED_ROJO  (1<<1)  // P2.1

// Umbral de temperatura para encender alarma
#define UMBRAL_TEMPERATURA 25 // °C
 
volatile uint32_t adc_value = 0;
 


void init_adc(void) {
    // Configurar P0.23 como AD0.0
    PINSEL_CFG_Type pinsel_cfg;
    pinsel_cfg.Portnum = 0;
    pinsel_cfg.Pinnum = 23;
    pinsel_cfg.Funcnum = 1;
    pinsel_cfg.Pinmode = 0; 
    PINSEL_ConfigPin(&pinsel_cfg);

    // Inicializar ADC
    ADC_Init(LPC_ADC, 200000); // 200kHz ADC clock
    ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
    ADC_BurstCmd(LPC_ADC, DISABLE);

    // Habilitar interrupción del ADC
    NVIC_EnableIRQ(ADC_IRQn);
}

void config_timer(void) {
    TIM_TIMERCFG_Type timer_cfg;
    TIM_MATCHCFG_Type match_cfg;

    timer_cfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timer_cfg.PrescaleValue = 1000; // 1ms

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_cfg);

    match_cfg.MatchChannel = 0;
    match_cfg.IntOnMatch = ENABLE;
    match_cfg.ResetOnMatch = ENABLE;
    match_cfg.StopOnMatch = DISABLE;
    match_cfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match_cfg.MatchValue = 1000; // 1 segundo, matchea e interrumpe

    TIM_ConfigMatch(LPC_TIM0, &match_cfg);
    TIM_Cmd(LPC_TIM0, ENABLE);

    NVIC_EnableIRQ(TIMER0_IRQn);
}



void config_leds(void) {
    // GPIO por defecto
	// P2.0 - led verde
	// P2.1 - led rojo
    GPIO_SetDir(2, LED_VERDE | LED_ROJO, 1);
    GPIO_ClearValue(2, LED_VERDE | LED_ROJO);
}

// Manejador de interrupción del ADC
void ADC_IRQHandler(void) {
    adc_value = ADC_ChannelGetData(LPC_ADC, 0); // 0 - 3.3V -> 0 - 4095
    
    // Convertir el valor ADC a temperatura (ajustar según el sensor)
    float temperatura = (adc_value * 3.3 / 4095.0) * 100.0; //  Pasa 0 - 4095 -> 0°C a 100°C
    
    if (temperatura > UMBRAL_TEMPERATURA) {
        GPIO_SetValue(2, LED_ROJO);
        GPIO_ClearValue(2, LED_VERDE);
    } else {
        GPIO_SetValue(2, LED_VERDE);
        GPIO_ClearValue(2, LED_ROJO);
    }
}

void TIMER0_IRQHandler(void) {

    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        ADC_StartCmd(LPC_ADC, ADC_START_NOW);
    }

    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

int main(void) {
    config_leds();
    init_adc();
    config_timer();

    while(1) {
        __WFI();  // wait for interrupt
    }

    return 0;
}