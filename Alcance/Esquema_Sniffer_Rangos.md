# Esquema del Sniffer y Rangos de Detección

## Vista Esquemática

El Sniffer está ubicado en el centro de un túnel minero, con antena integrada omnidireccional. Los rangos de detección se extienden lateralmente hasta 38 metros en condiciones óptimas (LOS, 12V, orientación favorable).

### Diagrama Esquemático

```mermaid
graph TD
    A[Sniffer<br/>Antenas Integrada<br/>Altura: 3m] -->|LoRa/LeackyFedder| H[HeadEnd]
    H --> S[Servidor]
    A <-->|UWB| B[Tag 1<br/>Distancia: 38m]
    A <-->|UWB| C[Tag 2<br/>Distancia: 12m]
    A <-->|UWB| D[Tag 3<br/>Distancia: 38m]
    
    B --> E[Túnel Ø3m<br/>LOS Requerido >12m]
    C --> E
    D --> E
    
    style S fill:#9f9,stroke:#333,stroke-width:4px
    style H fill:#ff9,stroke:#333,stroke-width:4px
    style A fill:#f9f,stroke:#333,stroke-width:4px
    style E fill:#bbf,stroke:#333,stroke-width:2px
```

### Descripción

- **Sniffer Central:** Unidad fija con antena UWB integrada internamente.
- **Rango Lateral:** Hasta 38 metros en línea de vista óptima.
- **Condiciones:** Alimentación 12V DC, orientación vertical de antenas.
- **Limitaciones:** Rango se reduce a 12m con orientación desfavorable o NLOS.

**Nota:** Este esquema representa una vista lateral simplificada. El patrón de antena es omnidireccional en el plano horizontal.