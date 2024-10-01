# Problema: Sistema de Control de Velocidad para Motor DC

### Objetivo:
Diseñar un sistema de control de velocidad para un motor DC utilizando la LPC1769. 
El sistema utilizará un potenciómetro para ajustar la velocidad deseada, y empleará PWM para controlar la velocidad real del motor. 
Además, implementará un sistema tal que si detecta una sobrecarga, reducirá la velocidad del motor a un 80% de la velocidad deseada.

### Requisitos:
- Utilizar el ADC para leer la posición del potenciómetro, que representará la velocidad deseada.
- Implementar un timer para generar una señal PWM de 10kHz que controle la velocidad del motor.
- Utilizar otro timer para accionar el ADC. 

### Estados de Operación:
1. **Reposo**: Motor apagado (0% duty cycle)
2. **Normal**: Motor funcionando a la velocidad deseada
3. **Sobrecarga**: Motor funcionando a velocidad reducida (reducir duty cycle en un 20%)

### Sistema de Seguridad:
- Detectar condiciones de sobrecarga mediante un sensor conectado a un pin GPIO.

### Indicadores:
- Utilizar LEDs para indicar el estado actual del sistema.




# Explicación de la Implementación

### Configuración del ADC:
- Se configura el pin `P0.23` como entrada analógica (`AD0.0`).
- Se inicializa el ADC con una frecuencia de 200kHz.
- Se habilita la interrupción para el canal 0 del ADC.

### Configuración de Timers:
- `TIMER0` se configura para generar una señal PWM de 10kHz.
- `TIMER1` se configura para accionar el ADC y medir la velocidad deseada cada 20ms

### Configuración de GPIO:
- `P1.28` se configura como salida para la señal PWM (`MAT0.0`).
- `P2.0`, `P2.1` y `P2.2` se configuran como salidas para los LEDs indicadores.
- `P2.10` se configura como entrada para el sensor de sobrecarga.

### Manejo de Interrupciones:
- En la interrupción del ADC, se lee el valor del potenciómetro, sobrecarga y se actualiza la velocidad deseada.
- En la interrupción del Timer0, se actualiza el valor de coincidencia para el PWM. 

# Diagrama ilustrativo
```mermaid
%%% Esquemático
graph LR
    subgraph LPC1769
    ADC_PIN[P0.23 - AD0.0]
    PWM_PIN[P1.28 - MAT0.0]
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


# Diagrama de flujos

```mermaid
graph TD

subgraph Inicio
    A[Inicio] --> B[Configurar ADC]
    B --> C[Configurar Timers]
    C --> D[Configurar GPIO]
    D --> E[Bucle Principal]
    E --> E
end
 
subgraph Timer0
    F[Interrupción TIMER0] --> G{¿Estado PWM = TON?}
    G -->|Sí| H[Cambiar a TOFF]
    G -->|No| I[Cambiar a TON]
    H --> J[Calcular tiempo en alto]
    I --> K[Calcular tiempo en bajo]
    J --> L[Actualizar valor de comparación del temporizador]
    K --> L
    L --> M[Limpiar bandera de interrupción]
    M --> N[Fin interrupción]
  
  end


subgraph Timer2
    O[Interrupción Timer2] --> P[Obtener velocidad real]
    P --> Q[Calcular error]
    Q --> R[Calcular integral del error]
    R --> S[Calcular derivada del error]
    S --> T[Calcular salida PID]
    T --> U[Limitar salida PID a 0-100%]
    U --> V{¿Estado = OVERLOAD?}
    V -->|Sí| W[Reducir salida al 80%]
    V -->|No| X[Usar salida completa]
    W --> Y[Actualizar PWM]
    X --> Y
    Y --> Z[Fin interrupción] 

end

```  



# Diagrama de flujos de la interrupción del ADC

```mermaid
graph TD
    Z[Interrupcion del ADC] --> Y[Leer valor del potenciómetro]
    Y --> X[Leer estado del sensor de sobrecarga]
    X --> B{¿Velocidad deseada = 0?}
    B -->|Sí| C[Estado = IDLE]
    B -->|No| D{¿Sobrecarga detectada?}
    D -->|Sí| E[Estado = OVERLOAD]
    D -->|No| F[Estado = NORMAL]
    C --> G[Encender LED IDLE]
    E --> H[Encender LED OVERLOAD]
    F --> I[Encender LED NORMAL]
    G --> J[Apagar otros LEDs]
    H --> J
    I --> J
    J --> K[Fin interrupción] 
``` 

# Diagrama de secuencia


```mermaid
sequenceDiagram
    participant T1 as TIMER1
    participant ADC
    participant MC as Microcontrolador
    participant PID as Controlador PID
    participant T0 as TIMER0
    participant Motor

    T1->>ADC: Iniciar conversión
    ADC->>MC: Interrupción con nuevo valor
    MC->>MC: Actualizar velocidad deseada
    MC->>MC: Actualizar estado del sistema
    T0->>MC: Interrupción PWM
    MC->>Motor: Actualizar ciclo de trabajo PWM
    T0->>MC: Interrupción PWM
    MC->>Motor: Actualizar ciclo de trabajo PWM
    Note over MC,Motor: El ciclo PWM se repite
    T0->>PID: Interrupción de actualización PID
    PID->>PID: Calcular nueva salida
    PID->>MC: Actualizar salida PID
    MC->>T0: Actualizar parámetros PWM

```