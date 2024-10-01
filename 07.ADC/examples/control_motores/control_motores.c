#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "stdlib.h"

/* Definiciones de pines */
#define PIN_ADC_POTENCIOMETRO   23  // AD0.0 en P0.23 
#define PIN_PWM_OUTPUT          28  // MAT0.0 en P1.28
#define PIN_LED_IDLE            0   // LED en P2.0
#define PIN_LED_NORMAL          1   // LED en P2.1
#define PIN_LED_OVERLOAD        2   // LED en P2.2
#define PIN_OVERLOAD_SENSOR     10  // Sensor en P2.10

/* Definiciones de estados del temporizador */
#define TON                1
#define TOFF               0
#define PWM_PERIOD_US      100 // 100us -> 10kHz

/* Estados del sistema */
typedef enum {
    IDLE,
    NORMAL,
    OVERLOAD
} StateSystem;

/* Variables globales */
volatile uint8_t velocidad_deseada = 0;     // Velocidad deseada en porcentaje (0-100%)
volatile uint8_t velocidad_real    = 0;     // Velocidad real medida en porcentaje (0-100%)
volatile uint8_t pid_output        = 0;     // Salida del controlador PID
volatile StateSystem estado_actual = IDLE;  // Estado actual del sistema 
volatile uint8_t pwm_status        = TON;   // Estado del temporizador para PWM

/* Variables para el control PID */
volatile float pid_error          = 0.0f;
volatile float pid_integral       = 0.0f;
volatile float pid_derivative     = 0.0f;
volatile float pid_previous_error = 0.0f;

/* Constantes del controlador PID, se deben ajustar manualmente */
#define PID_KP    (float)1.0f
#define PID_KI    (float)0.1f
#define PID_KD    (float)0.01f

/**
 * @brief Configura el ADC para leer el potenciómetro.
 *
 * Configura el pin del potenciómetro como entrada ADC y ajusta el ADC para que
 * inicie la conversión mediante un evento externo (TIMER1 MAT1.1).
 * Habilita el canal 0 y la interrupción correspondiente.
 * 
 */
void config_adc(void) {
    PINSEL_CFG_Type pinsel_cfg;

    /* Configurar pin como función ADC */
    pinsel_cfg.Funcnum = 1;
    pinsel_cfg.OpenDrain = 0;
    pinsel_cfg.Pinmode = 0;
    pinsel_cfg.Pinnum = PIN_ADC_POTENCIOMETRO;
    pinsel_cfg.Portnum = 0;
    PINSEL_ConfigPin(&pinsel_cfg);

    /* Inicializar ADC */
    ADC_Init(LPC_ADC, 200000);

    /* Configurar ADC para disparo externo (TIMER1 MAT1.1) */
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT11);
    ADC_ChannelCmd(LPC_ADC, 0, ENABLE);           // Habilitar canal 0
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE); // Habilitar interrupción del canal 0 
    NVIC_EnableIRQ(ADC_IRQn);                     // Habilitar interrupción del ADC 
}

/**
 * @brief Configura los temporizadores para PWM y el ADC.
 *
 * Configura TIMER0 para generar el PWM mediante interrupciones y TIMER1 para iniciar
 * la conversión ADC cada 20 ms. También configura TIMER2 para ejecutar el controlador
 * PID periódicamente.
 *
 */
void config_timers(void) {
    /* Configuración del temporizador para PWM (TIMER0) */
    TIM_TIMERCFG_Type timer_cfg;
    timer_cfg.PrescaleOption = TIM_PRESCALE_USVAL; // Los valores de prescaler son en microsegundos
    timer_cfg.PrescaleValue = 1;

    TIM_MATCHCFG_Type match_cfg;
    match_cfg.MatchChannel = 0;  // MAT0.0 
    match_cfg.IntOnMatch = ENABLE;
    match_cfg.ResetOnMatch = ENABLE;
    match_cfg.StopOnMatch = DISABLE;
    match_cfg.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    match_cfg.MatchValue = 50;   // Inicia con 50% de ciclo de trabajo, Ton = 50us

    /* Inicializar y configurar TIMER0 */
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_cfg);
    TIM_ConfigMatch(LPC_TIM0, &match_cfg);
    TIM_Cmd(LPC_TIM0, ENABLE);


    /* Configuración del temporizador para ADC (TIMER1) */
    timer_cfg.PrescaleValue = 1000;  // 1 ms 
    match_cfg.MatchChannel = 1;      // MAT1.1 
    match_cfg.IntOnMatch = DISABLE;
    match_cfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match_cfg.MatchValue = 20;       // Contar 20 ms 

    /* Inicializar y configurar TIMER1 */
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timer_cfg);
    TIM_ConfigMatch(LPC_TIM1, &match_cfg);
    TIM_Cmd(LPC_TIM1, ENABLE);


    /* Configuración del temporizador para controlador PID (TIMER2) */
    timer_cfg.PrescaleValue = 1000;  // 1 ms 
    match_cfg.MatchChannel = 0;      // MAT2.0 
    match_cfg.IntOnMatch = ENABLE;
    match_cfg.ResetOnMatch = ENABLE;
    match_cfg.StopOnMatch = DISABLE;
    match_cfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    match_cfg.MatchValue = 2;       // Ejecutar cada 2 ms, el PID se actualiza 10 veces antes de detectar el prox cambio de velocidad


    /* Inicializar y configurar TIMER2 */
    TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timer_cfg);
    TIM_ConfigMatch(LPC_TIM2, &match_cfg);
    TIM_Cmd(LPC_TIM2, ENABLE);      


    NVIC_EnableIRQ(TIMER0_IRQn); // Habilitar interrupción de TIMER0     
    NVIC_EnableIRQ(TIMER2_IRQn);     // Habilitar interrupción de TIMER2 
}

/**
 * @brief Configura los pines GPIO para PWM, LEDs y sensor de sobrecarga.
 *
 * Configura el pin de salida PWM, los pines de los LEDs como salida y el pin
 * del sensor de sobrecarga como entrada.
 *
 * @param void
 * @return void
 */
void config_gpio(void) {
    PINSEL_CFG_Type pinsel_cfg;

    /* Configurar pin PWM */
    pinsel_cfg.Funcnum = 2;
    pinsel_cfg.OpenDrain = 0;
    pinsel_cfg.Pinmode = 0;
    pinsel_cfg.Pinnum = PIN_PWM_OUTPUT;
    pinsel_cfg.Portnum = 1;
    PINSEL_ConfigPin(&pinsel_cfg);

    /* Configurar pines LED como salida */
    GPIO_SetDir(2, (1 << PIN_LED_IDLE) | (1 << PIN_LED_NORMAL) | (1 << PIN_LED_OVERLOAD), 1);

    /* Configurar pin sensor de sobrecarga como entrada */
    GPIO_SetDir(2, (1 << PIN_OVERLOAD_SENSOR), 0);
}

/**
 * @brief Actualiza el estado del sistema y los LEDs en base a la velocidad deseada y sobrecarga.
 *
 * Cambia el estado del sistema según la velocidad deseada y la detección de sobrecarga.
 * Actualiza los LEDs para reflejar el estado actual.
 * 
 * @param velocidad_des Velocidad deseada en porcentaje (0-100%)
 * @param sobrecarga_detectada Indica si se detectó sobrecarga
 */
void actualizar_estado_sistema(uint8_t velocidad_des, uint8_t sobrecarga_detectada) { 
    
    if (velocidad_des == 0) {
        estado_actual = IDLE;

        GPIO_SetValue(2, (1 << PIN_LED_IDLE));
        GPIO_ClearValue(2, (1 << PIN_LED_NORMAL) | (1 << PIN_LED_OVERLOAD));

    } else if (sobrecarga_detectada && velocidad_des > 0) {
        estado_actual = OVERLOAD;
        
        GPIO_SetValue(2, (1 << PIN_LED_OVERLOAD));
        GPIO_ClearValue(2, (1 << PIN_LED_NORMAL) | (1 << PIN_LED_IDLE));
    
    } else {
        estado_actual = NORMAL;
        
        GPIO_SetValue(2, (1 << PIN_LED_NORMAL));
        GPIO_ClearValue(2, (1 << PIN_LED_IDLE) | (1 << PIN_LED_OVERLOAD));
    
    } 
}

/**
 * @brief Actualiza el ciclo de trabajo del PWM en base a la salida PID.
 *
 * Ajusta el ciclo de trabajo del PWM según la salida del controlador PID,
 * teniendo en cuenta el estado actual del sistema.
 *
 * @param duty_cycle Ciclo de trabajo en porcentaje (0-100%) 
 */
void actualizar_pwm(float duty_cycle) {
    // Limitar el ciclo de trabajo al rango 0-100
    duty_cycle = (duty_cycle > 100) ? 100 : (duty_cycle < 0) ? 0 : duty_cycle;
    
    uint8_t ton = (duty_cycle * PWM_PERIOD_US) / 100; // Ton en microsegundos
    uint8_t toff = PWM_PERIOD_US - ton; // Toff en microsegundos

     if (pwm_status == TON) {
            TIM_UpdateMatchValue(LPC_TIM0, 0, ton); 
        } else if (pwm_status == TOFF) {
            TIM_UpdateMatchValue(LPC_TIM0, 0, toff); 
        } 
}

/**
 * @brief Obtiene la velocidad real del motor.
 *
 * En una implementación real, esta función debería leer la velocidad del motor mediante un sensor,
 * como (encoder por ejemplo). Esto es una simulacion con la velocidad deseada y un ruido aleatorio.
 *
 * @param void
 * @return float Velocidad real del motor en porcentaje (0-100%)
 */
float get_actual_speed(void) {
      
    float disturbance = (rand() % 11) - 5.0; // Perturbación entre -5% y 5%
    return pid_output + disturbance;
}

/**
 * @brief Implementa el controlador PID para ajustar la velocidad del motor.
 *
 * Calcula la salida del controlador PID en base al error entre la velocidad deseada y la real,
 * y actualiza el ciclo de trabajo del PWM en consecuencia.
 *
 * @param void
 * @return void
 */
void pid_controller_update(void) {
    float error = 0.0f;
    float dt = 0.002f; /* Intervalo de muestreo en segundos (2 ms) */

    /* Obtener la velocidad real del motor */
    velocidad_real = get_actual_speed();

    /* Calcular el error */
    error = velocidad_deseada - velocidad_real;

    /* Calcular la integral del error */
    pid_integral += error * dt;

    /* Calcular la derivada del error */
    pid_derivative = (error - pid_previous_error) / dt;

    /* Calcular la salida PID */
    pid_output = (PID_KP * error) + (PID_KI * pid_integral) + (PID_KD * pid_derivative);

    /* Guardar el error para la próxima iteración */
    pid_previous_error = error;

    /* Limitar la salida PID al rango 0-100% */
    if (pid_output > 100) {
        pid_output = 100;
    } else if (pid_output < 0) {
        pid_output = 0;
    }

    /* Ajustar el PWM según el estado del sistema */
    if (estado_actual == OVERLOAD) {
        /* Reducir la velocidad en caso de sobrecarga */
        actualizar_pwm(pid_output * 0.8f); /* Reducir al 80% de la salida PID */
    } else {
        actualizar_pwm(pid_output);
    }
}

/**
 * @brief Interrupción del temporizador TIMER0 para generar el PWM.
 *
 * Alterna entre los estados de encendido y apagado del PWM para generar el ciclo de trabajo
 * deseado utilizando interrupciones del temporizador. Lo hace de por medio de la salida del 
 * PID, para que sea más suave.
 *
 * @param void
 * @return void
 */
void TIMER0_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
        pwm_status = pwm_status == TON ? TOFF : TON; // Pasa de TON a TOFF y viceversa. 
        actualizar_pwm(pid_output);  // Con un duty cycle igual a la salida del PID
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
    }
}

/**
 * @brief Interrupción del ADC.
 *
 * Lee el valor del potenciómetro y actualiza la velocidad deseada.
 * También actualiza el estado del sistema.
 *
 * @param void
 * @return void
 */
void ADC_IRQHandler(void) {
    /* Leer el valor del potenciómetro (velocidad deseada) */
    velocidad_deseada = (ADC_ChannelGetData(LPC_ADC, 0) * 100) / 4095;
    
    uint8_t sobrecarga_detectada = (GPIO_ReadValue(2) >> PIN_OVERLOAD_SENSOR) & 1; // Leer el pin del sensor de sobrecarga 

    /* Actualizar el estado del sistema */
    actualizar_estado_sistema(velocidad_deseada, sobrecarga_detectada);
}

/**
 * @brief Interrupción del temporizador TIMER2 para el controlador PID.
 *
 * Llama a la función del controlador PID para actualizar el control de velocidad.
 *
 * @param void
 * @return void
 */
void TIMER2_IRQHandler(void) {
    if (TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)) {
        pid_controller_update();
        TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
    }
}

/**
 * @brief Función principal del programa.
 *
 * Inicializa el ADC, los temporizadores y los GPIO. Luego entra en un bucle infinito
 * esperando interrupciones.
 *
 * @param void
 * @return int Código de retorno (0 si es correcto)
 */
int main(void) {
    /* Configurar periféricos */
    config_adc();
    config_timers();
    config_gpio();

    /* Bucle principal */
    while (1) {
        __WFI();  /* Esperar a que ocurra una interrupción */
    }
    return 0;
}
