#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_clkpwr.h"

// Definición de estados
#define REPOSO 0
#define NORMAL 1
#define SOBRECARGA 2

// Variables globales
volatile uint32_t velocidad_deseada = 0;
volatile uint32_t velocidad_actual = 0;
volatile int estado_sistema = REPOSO;

// Configuración del ADC
void config_adc(void) {
    PINSEL_CFG_Type PinCfg;
    
    // Configurar P0.23 como AD0.0
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 23;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    // Inicializar ADC
    ADC_Init(LPC_ADC, 200000); // 200kHz ADC clock
    
    // Configurar canal 0 del ADC
    ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
    
    // Configurar interrupción del ADC
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
    NVIC_EnableIRQ(ADC_IRQn);
}

// Configuración de Timers
void config_timers(void) {
    TIM_TIMERCFG_Type TIM_ConfigStruct;
    TIM_MATCHCFG_Type TIM_MatchConfigStruct;
    
    // Configurar TIMER0 para PWM
    TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
    TIM_ConfigStruct.PrescaleValue = 1; // actualiza el TC cada microsegundo
    
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &TIM_ConfigStruct);
    
    TIM_MatchConfigStruct.MatchChannel = 0;
    TIM_MatchConfigStruct.IntOnMatch = DISABLE;
    TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
    TIM_MatchConfigStruct.StopOnMatch = DISABLE;
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    TIM_MatchConfigStruct.MatchValue = 1000; // 1ms período (1kHz)
    
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
    
    TIM_MatchConfigStruct.MatchChannel = 1;
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_LOW; // Inicialmente apagado
    TIM_MatchConfigStruct.MatchValue = 0; // Inicialmente 0% duty cycle
    
    TIM_ConfigMatch(LPC_TIM0, &TIM_MatchConfigStruct);
    


    // Configurar TIMER1 para muestreo de velocidad
    TIM_ConfigStruct.PrescaleValue = 100; // 100us
    
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &TIM_ConfigStruct);
    
    TIM_MatchConfigStruct.MatchChannel = 0;
    TIM_MatchConfigStruct.IntOnMatch = ENABLE;
    TIM_MatchConfigStruct.ResetOnMatch = ENABLE;
    TIM_MatchConfigStruct.StopOnMatch = DISABLE;
    TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    TIM_MatchConfigStruct.MatchValue = 1000; // 100ms
    
    TIM_ConfigMatch(LPC_TIM1, &TIM_MatchConfigStruct);
    
    NVIC_EnableIRQ(TIMER1_IRQn);
}

// Configuración de GPIO
void config_gpio(void) {
    // Configurar P1.28 como salida para PWM (MAT0.1)
    PINSEL_CFG_Type PinCfg;
    PinCfg.Funcnum = 3;
    PinCfg.OpenDrain = 0;
    PinCfg.Pinmode = 0;
    PinCfg.Pinnum = 28;
    PinCfg.Portnum = 1;
    PINSEL_ConfigPin(&PinCfg);
    
    // Configurar LEDs (P2.0, P2.1, P2.2) como salidas
    GPIO_SetDir(2, 0x07, 1);
}

// Manejador de interrupción del ADC
void ADC_IRQHandler(void) {
    velocidad_deseada = ((ADC_ChannelGetData(LPC_ADC, 0) * 1000) / 0xFFF); // calcular velocidad deseada, mapea de 0 a 1000
    if (estado_sistema == NORMAL) {
        TIM_UpdateMatchValue(LPC_TIM0, 1, velocidad_deseada);
    }
}

// Manejador de interrupción del TIMER1
void TIMER1_IRQHandler(void) {
    TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
    
    // Simular lectura de velocidad actual (en la práctica, esto vendría de un sensor)
    velocidad_actual = (velocidad_deseada * 9) / 10; // Simula una pequeña diferencia
    
    // Detección de sobrecarga (simulada)
    static uint32_t contador_sobrecarga = 0;
    if (velocidad_actual < (velocidad_deseada * 8) / 10) {
        contador_sobrecarga++;
        if (contador_sobrecarga > 10 && estado_sistema == NORMAL) {
            estado_sistema = SOBRECARGA;
        }
    } else {
        contador_sobrecarga = 0;
        if (estado_sistema == SOBRECARGA) {
            estado_sistema = NORMAL;
        }
    }
    
    actualizar_estado_sistema();
}

// Función para actualizar el estado del sistema
void actualizar_estado_sistema(void) {
    switch(estado_sistema) {
        case REPOSO:
            TIM_UpdateMatchValue(LPC_TIM0, 1, 0); // 0% duty cycle
            GPIO_ClearValue(2, 0x07); // Apagar todos los LEDs
            GPIO_SetValue(2, 0x01);   // Encender LED de REPOSO
            break;
        case NORMAL:
            TIM_UpdateMatchValue(LPC_TIM0, 1, velocidad_deseada);
            GPIO_ClearValue(2, 0x07);
            GPIO_SetValue(2, 0x02); // Encender LED de NORMAL
            break;
        case SOBRECARGA:
            TIM_UpdateMatchValue(LPC_TIM0, 1, velocidad_deseada / 2); // Reducir velocidad
            GPIO_ClearValue(2, 0x07);
            GPIO_SetValue(2, 0x04); // Encender LED de SOBRECARGA
            break;
    }
}

int main(void) {
    config_adc();
    config_timers();
    config_gpio();
    
    // Iniciar timers
    TIM_Cmd(LPC_TIM0, ENABLE);
    TIM_Cmd(LPC_TIM1, ENABLE);
    
    while(1) {
        // El trabajo principal se realiza en las interrupciones
               __WFI(); // Wait for interrupt (modo de bajo consumo)
    }
}