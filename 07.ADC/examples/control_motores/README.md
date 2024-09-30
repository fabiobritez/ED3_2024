# Problema: Sistema de Control de Velocidad para Motor DC

### Objetivo:
Diseñar un sistema de control de velocidad para un motor DC utilizando la LPC1769. 
El sistema utilizará un potenciómetro para ajustar la velocidad deseada, y empleará PWM para controlar la velocidad real del motor. 
Además, implementará un sistema de monitoreo y seguridad. Si detecta una sobrecarga, reducirá la velocidad del motor.

### Requisitos:
- Utilizar el ADC para leer la posición del potenciómetro, que representará la velocidad deseada.
- Implementar un timer para generar una señal PWM que controle la velocidad del motor.
- Utilizar otro timer para muestrear periódicamente la velocidad actual del motor (simulada).
- Emplear interrupciones para manejar las lecturas del ADC y las acciones de los timers.

### Estados de Operación:
1. **Reposo**: Motor apagado (0% duty cycle)
2. **Normal**: Motor funcionando a la velocidad deseada
3. **Sobrecarga**: Motor funcionando a velocidad reducida

### Sistema de Seguridad:
- Implementar un sistema de seguridad que detecte sobrecargas (por un pin que tenga un sensor) y ajuste el estado del sistema.

### Indicadores:
- Utilizar LEDs para indicar el estado actual del sistema.




# Explicación de la Implementación

### Configuración del ADC:
- Se configura el pin `P0.23` como entrada analógica (`AD0.0`).
- Se inicializa el ADC con una frecuencia de 200kHz.
- Se habilita la interrupción para el canal 0 del ADC.

### Configuración de Timers:
- `TIMER0` se configura para generar una señal PWM de 10kHz.
- `TIMER1` se configura para generar una interrupción cada 100ms.

### Configuración de GPIO:
- `P1.28` se configura como salida para la señal PWM (`MAT0.1`).
- `P2.0`, `P2.1` y `P2.2` se configuran como salidas para los LEDs indicadores.

### Manejo de Interrupciones:
- En la interrupción del ADC, se lee el valor del potenciómetro y se actualiza la velocidad deseada.
- En la interrupción de `TIMER1`, se simula la lectura de la velocidad actual, verifica si se detecta sobrecarga y se actualiza el estado del sistema.

### Lógica Principal:
- La función `actualizar_estado_sistema()` maneja los cambios entre estados y ajusta el PWM y los LEDs en consecuencia.


# Diagrama ilustrativo
```mermaid
%%% Esquemático
graph LR
    subgraph LPC1769
    ADC_PIN[P0.23 - AD0.0]
    PWM_PIN[P1.28 - MAT0.1]
    LED1_PIN[P2.0]
    LED2_PIN[P2.1]
    LED3_PIN[P2.2]
    SENS_PIN[P2.10]
    end

    POT2[Potenciómetro] -->|Vout| ADC_PIN
    PWM_PIN -->|PWM| DRIVER[Driver Motor]
    DRIVER -->|Control| MOTOR2[Motor DC]
    LED1_PIN -->|Reposo| LED1_2[LED]
    LED2_PIN -->|Normal| LED2_2[LED]
    LED3_PIN -->|Sobrecarga| LED3_2[LED]
    SENS2[Sensor Sobrecarga] -->|Estado| SENS_PIN

style ADC_PIN fill:#ff,stroke:#333,stroke-width:2px
style PWM_PIN fill:#555,stroke:#555,stroke-width:2px
style LED1_PIN fill:#fff,stroke:#fff,stroke-width:2px
style LED2_PIN fill:#480,stroke:#480,stroke-width:2px
style LED3_PIN fill:#a22,stroke:#a22,stroke-width:2px
style SENS_PIN fill:#ff,stroke:#333,stroke-width:2px

```