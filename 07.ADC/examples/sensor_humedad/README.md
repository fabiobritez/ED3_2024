# Implementación de un Sistema de Monitoreo de Temperatura con LEDs

 
Implementar un sistema que lea el valor de un sensor de temperatura analógico conectado al canal 0 del ADC, configurado para generar una conversión periódica mediante un temporizador. El sistema debe levantar una interrupción cuando la conversión ADC esté lista, y en base al valor de temperatura, tomar decisiones. Si la temperatura supera un umbral predefinido, se debe encender un LED rojo (simulando una alarma), sino un LED verde. Tener en cuenta los valores de referencia del ADC.


## Solución

### Configuración:
- Se configura el ADC para leer del canal 0 (AD0.0, pin P0.23).
- Se configura un timer (TIMER0) para generar interrupciones cada 1 segundo.
- Se configuran dos pines GPIO para los LEDs (verde en P2.0, rojo en P2.1).

### Operación:
1. Cada segundo, el timer genera una interrupción que inicia una conversión ADC.
2. Cuando la conversión ADC está lista, se genera otra interrupción.
3. En la interrupción del ADC, se lee el valor, se convierte a temperatura y se decide qué LED encender.

### Control de LEDs:
- Si la temperatura supera el umbral predefinido (`UMBRAL_TEMPERATURA`), se enciende el LED rojo.
- Si la temperatura está por debajo del umbral, se enciende el LED verde.

