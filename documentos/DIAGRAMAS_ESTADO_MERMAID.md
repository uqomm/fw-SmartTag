# Diagramas de Estado: Sniffer y Tag (Persona)

## 1. Máquina de Estados del Sniffer (Simplificada)

```mermaid
flowchart TD
    Start([Inicio]) --> Discovery[DISCOVERY<br/>Buscar Tag]
    
    Discovery -->|TX: Discovery Poll<br/>Antena A o B| WaitDiscovery{Espera<br/>Respuesta}
    
    WaitDiscovery -->|Timeout/Error| Discovery
    WaitDiscovery -->|RX OK<br/>Mode: ONE| OneDetection[ONE_DETECTION<br/>Un solo tag]
    WaitDiscovery -->|RX OK<br/>Mode: MULTIPLE| StartQueries[SEND_TIMESTAMP_QUERY<br/>Inicia Ranging]
    
    OneDetection -->|Envía Sleep<br/>Sin queries extras| SaveOne[Guarda Tag<br/>⚠️ Sin triangulación]
    SaveOne --> Discovery
    
    StartQueries -->|Inicia timer<br/>query_timeout=1000ms| QueryLoop{Loop Queries}
    
    QueryLoop -->|Alterna Antena<br/>A ↔ B| SendQuery[TX: Timestamp Query<br/>con distancias actuales]
    SendQuery --> WaitQuery{Espera<br/>Respuesta}
    
    WaitQuery -->|RX OK| SaveReading[Guarda lectura<br/>en antena activa]
    WaitQuery -->|Error CRC| IncError[Incrementa<br/>error_crc]
    
    SaveReading -->|readings < total| QueryLoop
    SaveReading -->|readings >= total| EndReadings[END_READINGS<br/>Completo]
    
    IncError --> CheckTimeout{Timeout?}
    CheckTimeout -->|No| QueryLoop
    CheckTimeout -->|Sí 1000ms| ForceSave[Guarda datos<br/>⚠️ Incompletos]
    
    EndReadings -->|TX: Set Sleep Mode| SaveFinal[Guarda Tag Final<br/>Limpia buffers]
    ForceSave --> SaveFinal
    SaveFinal --> Discovery
    
    style Discovery fill:#87CEEB
    style OneDetection fill:#FFE4B5
    style StartQueries fill:#90EE90
    style EndReadings fill:#98FB98
    style SaveFinal fill:#98FB98
    style ForceSave fill:#FFB6C1
    style SaveOne fill:#FFB6C1
```

### Explicación del Flujo:

1. **DISCOVERY**: Busca tags cercanos, alterna antena cada vez
2. **ONE_DETECTION**: Modo especial para 1 tag (sin triangulación)
3. **SEND_TIMESTAMP_QUERY**: Hace ranging alternando antenas A/B
4. **END_READINGS**: Finaliza cuando completa lecturas o expira timeout
5. ⚠️ **Problema**: Guarda incluso con datos incompletos

---

## 2. Máquina de Estados del Tag/Persona (Simplificada)

```mermaid
flowchart TD
    Start([Power On]) --> WaitFirst[WAIT_FIRST_DETECTION<br/>Escucha Discovery]
    
    WaitFirst -->|Sin señal<br/>Estado default| SleepDefault[DEFAULT SLEEP<br/>Sleep 1000 ms]
    WaitFirst -->|RX: Discovery Poll| RespondDiscovery[TX: Discovery Response<br/>ID + Timestamps + Batería]
    
    RespondDiscovery --> StartTimer[Inicia query_timeout<br/>1000 ms]
    StartTimer --> WaitQuery[WAIT_TIMESTAMP_QUERY<br/>Escucha Queries]
    
    WaitQuery -->|RX: Timestamp Query| RespondQuery[TX: Timestamp Response<br/>Timestamps]
    RespondQuery --> WaitQuery
    
    WaitQuery -->|RX: Set Sleep Mode<br/>+ Ambas dist > 0| SleepSuccess[SLEEP_RECIVED<br/>Sleep 15 segundos]
    WaitQuery -->|Timeout 1000ms<br/>Sin datos completos| SleepFail[SLEEP_NOT_RECIVED<br/>Sleep 500 ms]
    
    SleepSuccess -->|Wake Up| WaitFirst
    SleepFail -->|Wake Up| WaitFirst
    SleepDefault -->|Wake Up| WaitFirst
    
    WaitFirst -.->|Comando especial| ShipMode[SHIP_MODE<br/>Apagado profundo]
    ShipMode --> End([Off])
    
    style WaitFirst fill:#87CEEB
    style WaitQuery fill:#90EE90
    style SleepSuccess fill:#98FB98
    style SleepFail fill:#FFE4B5
    style SleepDefault fill:#F0E68C
    style ShipMode fill:#D3D3D3
```

### Explicación del Flujo:

1. **WAIT_FIRST_DETECTION**: Espera ser descubierto por sniffer
2. **WAIT_TIMESTAMP_QUERY**: Responde a queries de ranging
3. **SLEEP_RECIVED**: Sleep largo (15s) tras éxito con ambas distancias
4. **SLEEP_NOT_RECIVED**: Sleep corto (500ms) si falló o timeout
5. **DEFAULT SLEEP**: Sleep intermedio (1000ms) cuando no recibe ninguna señal
6. **SHIP_MODE**: Apagado profundo (opcional)

---

## 3. Comparación: Sniffer vs Tag

```mermaid
flowchart LR
    subgraph Sniffer["🔍 SNIFFER (Activo)"]
        S1[DISCOVERY<br/>Busca]
        S2[QUERY<br/>Alterna A/B]
        S3[END<br/>Guarda]
    end
    
    subgraph Tag["📱 TAG (Reactivo)"]
        T1[WAIT_FIRST<br/>Escucha]
        T2[WAIT_QUERY<br/>Responde]
        T3[SLEEP<br/>Duerme]
    end
    
    S1 -.->|Discovery Poll| T1
    T1 -.->|Response| S1
    S2 -.->|Timestamp Query| T2
    T2 -.->|Response| S2
    S3 -.->|Sleep Command| T2
    T2 -.->|ACK| S3
    T3 -.->|Wake up| T1
    
    S1 --> S2 --> S3 --> S1
    T1 --> T2 --> T3 --> T1
    
    style Sniffer fill:#E6F3FF
    style Tag fill:#FFE6F0
```

## 4. Diagrama de Secuencia Completo

```mermaid
sequenceDiagram
    participant S as Sniffer<br/>(TAG_DISCOVERY)
    participant SA as Sniffer Antena A
    participant SB as Sniffer Antena B
    participant T as Tag<br/>(WAIT_FIRST_DETECTION)
    
    Note over S: Antena A activa
    S->>SA: TX Discovery Poll
    SA->>T: [TAG_ID_QUERY][state][sleep][dev]
    
    T->>SA: RX Discovery Response
    SA->>S: [cmd][id][ts_rx(offset5)][ts_tx(offset9)][batt]
    
    Note over S: Estado → TAG_SEND_TIMESTAMP_QUERY
    Note over S: Calcula distancia_A (primera lectura)
    Note over S: Switch a Antena B
    Note over T: Estado → WAIT_TIMESTAMPT_QUERY
    
    loop Queries hasta completar lecturas (query_timeout = 1000 ms)
        alt Antena B necesita lecturas
            Note over S: Antena B activa
            S->>SB: TX Timestamp Query
            SB->>T: [TAG_TIMESTAMP_QUERY][id][dA][dB]
            T->>SB: RX Timestamp Response
            SB->>S: [cmd][ts_rx(offset1)][ts_tx(offset5)]
            Note over S: distance_B.save()
        else Antena A necesita lecturas
            Note over S: Switch a Antena A
            S->>SA: TX Timestamp Query
            SA->>T: [TAG_TIMESTAMP_QUERY][id][dA][dB]
            T->>SA: RX Timestamp Response
            SA->>S: [cmd][ts_rx(offset1)][ts_tx(offset5)]
            Note over S: distance_A.save()
        end
        
        alt Lecturas completas
            Note over S: readings >= total
            Note over S: Estado → TAG_END_READINGS
        else Timeout
            Note over S: query_timeout expiró
            Note over S: Guarda datos (⚠️ pueden estar incompletos)
        else Error CRC
            Note over S: distance_ptr->error_crc_increment()
            Note over S: Reintenta
        end
    end
    
    S->>T: TX Set Sleep Mode
    T->>S: RX Final Response + ACK
    
    Note over S: Estado → TAG_DISCOVERY
    Note over S: save_two_maps_and_clear_tag()
    Note over S: Busca próximo tag
    
    alt Tag con lecturas exitosas
        Note over T: distance_a > 0 && distance_b > 0
        Note over T: Estado → TAG_SLEEP_RECIVED
        Note over T: Sleep 15000 ms
    else Tag sin lecturas completas
        Note over T: Estado → TAG_SLEEP
        Note over T: Sleep 500 ms
    end
```

## 5. Diagrama de Decisión: Alternancia de Antenas

```mermaid
flowchart TD
    Start([switch_hw_timestamp_query]) --> CheckAntenna{Antena actual?}
    
    CheckAntenna -->|Antena A| CheckB[¿dist_B.counter < total/2?]
    CheckAntenna -->|Antena B| CheckA[¿dist_A.counter < total/2?]
    
    CheckB -->|Sí| SwitchToB[Cambiar a Antena B]
    CheckB -->|No| CheckA2[¿dist_A.counter < total/2?]
    
    CheckA -->|Sí| SwitchToA[Cambiar a Antena A]
    CheckA -->|No| KeepB[Mantener Antena B]
    
    CheckA2 -->|Sí| SwitchToA
    CheckA2 -->|No| KeepA[Mantener Antena A]
    
    SwitchToB --> End([Continúa query])
    SwitchToA --> End
    KeepA --> End
    KeepB --> End
    
    style SwitchToB fill:#90EE90
    style SwitchToA fill:#87CEEB
    style KeepA fill:#FFE4B5
    style KeepB fill:#FFE4B5
```

---

## Notas Importantes:

1. **query_timeout crítico:** Ambos equipos (sniffer y tag) tienen 1000 ms para completar el ranging.

2. **Offsets diferentes:** Discovery usa offsets 5/9, queries usan 1/5 por estructura de frame distinta.

3. **Problema de guardado:** El código actual guarda tags incluso si solo una antena tiene lecturas, invalidando la triangulación.

4. **Alternancia inteligente:** `switch_hw_timestamp_query()` balancea lecturas entre antenas para asegurar datos de ambas.

5. **Sleep diferenciado:** Tag duerme 500 ms si falla, 15000 ms si tiene éxito.

6. **TAG_ONE_DETECTION:** Modo especial que omite queries adicionales, perdiendo capacidad de triangulación.
