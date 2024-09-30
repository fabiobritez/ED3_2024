# Configuración de los timers con la librería lpc17xx_timer.h


## Modos de operación
Primero debemos seleccionar el modo de operación del timer, si queremos que funcione como un timer o como contador:
Para esto existen dos estructuras de datos: `TIM_TIMERCFG_Type` y `TIM_COUNTERCFG_Type`.

### Modo Timer

Este modo se configura cuando se desea que el timer cuente en base a un clock externo o interno. Se configura con la estructura `TIM_TIMERCFG_Type`.

```c
typedef struct
{
	uint8_t PrescaleOption;		/**< Timer Prescale option, debería ser:
									- TIM_PRESCALE_TICKVAL: El prescale es en valor de ticks
									- TIM_PRESCALE_USVAL: El prescale es en valor de microsegundos
									*/
	uint8_t Reserved[3];		/**< Reservado */
	uint32_t PrescaleValue;		/**< Valor del prescale */
} TIM_TIMERCFG_Type;

```

El campo PrescaleOption define cómo se interpretará el valor que pasemos en el campo PrescaleValue. 
- Si **PrescaleOption** es `TIM_PRESCALE_TICKVAL`, el valor de PrescaleValue será el número de ticks que se deben contar antes de incrementar el contador. 

Ejemplo: 

    Si el clock del timer (PCLKSEL_TIMER = CCLK/Div) tiene una frecuencia de 25 MHz y se configura PrescaleValue en 99, entonces el temporizador contará un "tick" por cada 100 ciclos de reloj del timer (porque se inicia desde 0). En este caso, la frecuencia efectiva del temporizador sería de 250kHz (25 MHz / 100).

- Si **PrescaleOption** es `TIM_PRESCALE_USVAL`, el valor de PrescaleValue será el número de microsegundos que se deben contar antes de incrementar el contador. En este caso la funcion TIM_Init() calculará el valor del prescale en función de la frecuencia del clock.

Ejemplo:

    Si se configura PrescaleValue en 1 (1 microsegundo) y el clock es de 25 MHz, entonces el temporizador calculará automáticamente que debe contar 25 ciclos de reloj (porque 25 MHz = 1 ciclo cada 40 nanosegundos, y 1 microsegundo = 25 veces 40 nanosegundos).

Para configurar el timer, se debe llamar a la función TIM_Init() y pasarle la estructura TIM_TIMERCFG_Type.

```c
TIM_TIMERCFG_Type configTimer;

...

TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &configTimer);
```


### Modo Contador (Capture)

Este modo se configura cuando se desea que el timer cuente eventos externos. Para esto, cada timer tiene dos canales de captura (CAPn.0 y CAPn.1).  


> **Importante**: Para este caso previamente se deben haber configurado la funcion de los pines correspondientes en PINSELx. 


Despues de eso, se debe seleccionar el canal de captura con la siguiente estructura:

```c
typedef struct {

	uint8_t CounterOption;		/**< Counter Option, debería ser:
								- TIM_COUNTER_INCAP0: CAPn.0 input pin for TIMERn
								- TIM_COUNTER_INCAP1: CAPn.1 input pin for TIMERn
								*/
	uint8_t CountInputSelect;  /**< Counter Input Select, debería ser:
                                - 0: CAPn.0 for TIMERn
                                - 1: CAPn.1 for TIMERn
                                */
	uint8_t Reserved[2];
} TIM_COUNTERCFG_Type;
```

Luego, se debe configurar la estructura TIM_CAPTURECFG_Type para definir las condiciones de captura.

```c
typedef struct {
	uint8_t CaptureChannel;	/**< Canal de captura, debe estar en el rango
							de 0 a 1 */
	uint8_t RisingEdge;		/**< Captura en flanco ascendente, debe ser:
							- ENABLE: Habilitar flanco ascendente.
							- DISABLE: Deshabilitar esta función.
							*/
	uint8_t FallingEdge;	/**< Captura en flanco descendente, debe ser:
							- ENABLE: Habilitar flanco descendente.
							- DISABLE: Deshabilitar esta función.
							*/
	uint8_t IntOnCaption;	/**< Interrupción en captura, debe ser:
							- ENABLE: Habilitar función de interrupción.
							- DISABLE: Deshabilitar esta función.
							*/

} TIM_CAPTURECFG_Type;

```


En este caso se configura el timer con la función TIM_Init() y TIM_ConfigCapture().

```c
TIM_COUNTERCFG_Type configCounter;
TIM_CAPTURECFG_Type configCapture;

...

 // TIM_COUNTER_RISING_MODE: Counter rising 
 // TIM_COUNTER_FALLING_MODE: Counter falling 
 // TIM_COUNTER_ANY_MODE:Counter on both edges
TIM_Init(LPC_TIM0, TIM_COUNTER_ANY_MODE, &configCounter);
TIM_ConfigCapture(LPC_TIM0, &configCapture);   // Configura la captura
```


## Condiciones de Match

Si queremos tomar alguna acción cuando el timer alcance un valor específico, debemos configurar los registros de Match. Para esto se utiliza la estructura TIM_MATCHCFG_Type.

```c
typedef struct {
	uint8_t MatchChannel;	/**< Canal de Match, debe estar en el rango
							de 0 a 3 */
	uint8_t IntOnMatch;		/**< Interrupción en coincidencia, debe ser:
							- ENABLE: Habilitar esta función.
							- DISABLE: Deshabilitar esta función.
							*/
	uint8_t StopOnMatch;	/**< Detener en coincidencia, debe ser:
							- ENABLE: Habilitar esta función.
							- DISABLE: Deshabilitar esta función.
							*/
	uint8_t ResetOnMatch;	/**< Reiniciar en coincidencia, debe ser:
							- ENABLE: Habilitar esta función.
							- DISABLE: Deshabilitar esta función.
							*/

	uint8_t ExtMatchOutputType;	/**< Tipo de salida ante un match, debe ser:
							 -	 TIM_EXTMATCH_NOTHING:	No hacer nada con el pin de salida externo.
							 -   TIM_EXTMATCH_LOW:	Forzar el pin de salida externo a 0.
							 - 	 TIM_EXTMATCH_HIGH: Forzar el pin de salida externo a 1.
							 -   TIM_EXTMATCH_TOGGLE: Alternar el estado del pin de salida externo.
							*/
	uint8_t Reserved[3];	/** Reservado */
	uint32_t MatchValue;	/** Valor de coincidencia */
} TIM_MATCHCFG_Type;

```


## Explicación de TIM_Init()
Esta función  se encarga de inicializar el Timer. Se debe llamar a esta función antes de usar el timer. Tiene 3 parámetros: el registro del timer (LPC_TIMx), el modo de operación y la estructura de configuración. Y tiene la siguiente secuencia de pasos:

1. Verifica los parámetros de entrada.
2. Habilita el suministro de energía del temporizador. Y configura el divisor del clock en 1/4 (PCLK=CCLK/4) por defecto.
3. Configurar el modo del temporizador/contador.
	- Si el modo es temporizador, configura el prescaler.
	- Si el modo es contador, configura el canal de captura.
4. Resetea el TC y PC. Y habilita el temporizador/contador.
5. Limpia cualquier interrupción pendiente.
 


## Gestion de interrupciones de Timer
Para habilitar las interrupciones de los timers en el NVIC, se utiliza la función NVIC_EnableIRQ().
Por ejemplo, para habilitar las interrupciones del Timer 0:
```c
NVIC_EnableIRQ(TIMER0_IRQn);
```


Cuando ocurre una interrupción, el procesador llama al handler correspondiente.  Es importante verificar qué tipo de interrupción ocurrió (match o capture) y luego realizar las acciones necesarias. Y al final, limpiar la bandera de interrupción.


### Funcion para obtener el estado de la interrupción
Para verificar si ocurrió una interrupción de match o de captura, se utiliza la función TIM_GetIntStatus(). Esta función devuelve el estado de la interrupción que se está verificando (SET o RESET).

```c
 * @param[in]    TIMx Selección del temporizador, debe ser:
 *               - LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3:

 * @param[in]    IntFlag: tipo de interrupción, debe ser:
 *               - TIM_MR0_INT (Interrupción MATCH 0)
 *               - TIM_MR1_INT, TIM_MR2_INT, TIM_MR3_INT 
 *               - TIM_CR0_INT: Interrupción para el canal 0 de captura, 
 				 - TIM_CR1_INT 

 * @return         FlagStatus
 *               - SET : interrupción activa
 *               - RESET : no interrumpió
 
FlagStatus TIM_GetIntStatus(LPC_TIM_TypeDef *TIMx, TIM_INT_TYPE IntFlag)

```

### Funcion para limpiar la interrupción

Para limpiar la bandera de interrupción, se utiliza la función TIM_ClearIntPending(). Esta función limpia la bandera de interrupción de match o de captura.

Ejemplo:
```c
 TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
```	

Por ejemplo:
```c

void TIMER0_IRQHandler(void)
{
    // Verificar si la interrupción fue generada por el canal de match 0
    if (TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT) == SET) {
        // Limpiar la interrupción del match 0
        TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
        
        // Acciones a realizar cuando ocurre el match
    }

    // Verificar si la interrupción fue generada por un evento de captura en el canal 0
    if (TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT) == SET) {
        // Limpiar la interrupción de captura
        TIM_ClearIntCapturePending(LPC_TIM0, TIM_CR0_INT);
        
        // Acciones a realizar cuando ocurre la captura 
        uint32_t capturedValue = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0);
    }
}

```


## Otras funciones


### TIM_GetCaptureValue() – Obtener el Valor de Captura

Esta función se utiliza para leer el valor del contador en el momento en que se captura un evento. Devuelve el valor del contador (TC) capturado cuando ocurrió el evento.

Parámetros:
- **TIMx**: Selecciona el temporizador (LPC_TIM0, LPC_TIM1, etc.).
- **CaptureChannel**: El canal de captura del cual deseas leer el valor (puede ser TIM_COUNTER_INCAP0 o TIM_COUNTER_INCAP1). 

```c
// asumiendo que se configuró en modo timer, con frecuencia de 25 MHz

uint32_t captureValue = TIM_GetCaptureValue(LPC_TIM0, TIM_COUNTER_INCAP0);

float time_in_ms = (float)captureValue / 25000; // 25 MHz = 25 ciclos de PCLK por microsegundo
```


### TIM_UpdateMatch() – Actualizar el Valor de Match

Esta función se utiliza para actualizar el valor de match. Se debe llamar a esta función desoues de configurar el valor de match en la estructura TIM_MATCHCFG_Type.

```c

/** Tiene 3 parametros:
 * @param[in]    TIMx Selección del temporizador, debe ser:
 *               - LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3:
 * @param[in]    MatchChannel: Canal de Match, debe estar en el rango
 *               de 0 a 3
 * @param[in]    MatchValue: Valor de coincidencia 
 */

 // Ejemplo (se configuró previamente el valor de match en la estructura configMatch)

TIM_UpdateMatch(LPC_TIM0, 0, 1000); // Actualiza el valor de match 0 a 1000

```

### TIM_SetMatchExt() – Configurar el valor de Match Externo

void TIM_SetMatchExt(LPC_TIM_TypeDef *TIMx, TIM_EXTMATCH_OPT ext_match)

Esta función se utiliza para configurar la salida externa del Match. Define lo que sucede con el pin de salida cuando se produce una coincidencia en el Timer.

```c
	/** Tiene 2 parametros:
	 * @param[in]    TIMx Selección del temporizador, debe ser:
	 *               - LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3:
	 * @param[in]    ext_match: Opciones de coincidencia externa, debe ser:
	 *               - TIM_EXTMATCH_NOTHING 
	 *               - TIM_EXTMATCH_LOW 
	 *               - TIM_EXTMATCH_HIGH 
	 *               - TIM_EXTMATCH_TOGGLE   
	 */
	 
// Configura el pin de salida externo para alternar cuando ocurra una coincidencia
TIM_SetMatchExt(LPC_TIM0, TIM_EXTMATCH_TOGGLE);  
```


### TIM_Cmd() – Habilitar/Desabilitar el Timer 
Esta función habilita o deshabilita el Timer. Cuando se habilita, el Timer comienza a contar. Cuando se deshabilita, el Timer se detiene.


```c
    /** Tiene 2 parametros:
	 * @param[in]    TIMx Selección del temporizador, debe ser:
	 *               - LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3:
	 * @param[in]    NewState: Estado del Timer:
	 *               - ENABLE: Habilita el Timer.
	 *               - DISABLE: Deshabilita el Timer.
	 */  
```


### TIM_ResetCounter() – Reiniciar el Contador
Esta función reinicia el contador del Timer. El contador se reinicia a 0.

```c
	/** Tiene 1 parametro:
	 * @param[in]    TIMx Selección del temporizador, debe ser:
	 *               - LPC_TIM0, LPC_TIM1, LPC_TIM2, LPC_TIM3:
	 */  

TIM_ResetCounter(LPC_TIM0); // Reinicia el contador del Timer 0	
```

 
