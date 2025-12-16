# fw-SmartTag

Firmware for the SmartTag UWB-based indoor localization system, consisting of personal tags and sniffer devices for real-time positioning.

## 🎯 System Overview

```mermaid
block-beta
  columns 5
  
  block:Tags:1
    columns 1
    Tag1["Personal Tag<br/>STM32U535<br/>🏷️"]
    Tag2["Personal Tag<br/>STM32U535<br/>🏷️"]
    TagN["... More Tags<br/>STM32U535<br/>🏷️"]
  end
  
  space
  
  block:Sniffer:1
    columns 1
    SnifferDev["Sniffer Device<br/>STM32G474<br/>📡"]
    UWB["DW3000 UWB<br/>Dual Chip"]
    LoRaModule["LoRa SX1276<br/>VHF TX"]
  end
  
  space
  
  block:Gateway:1
    columns 1
    GatewayDev["Gateway<br/>LoRa Receiver<br/>🔌"]
    Processor["Data Processing<br/>& Forwarding"]
  end
  
  space
  
  Server["Backend Server<br/>Cloud/Local<br/>☁️"]
  
  Tags--"UWB Ranging<br/>DS-TWR Protocol<br/>38m max"-->Sniffer
  Sniffer--"LoRa VHF<br/>Distance Data<br/>Long Range"-->Gateway
  Gateway--"TCP/IP<br/>HTTP/MQTT<br/>Ethernet/WiFi"-->Server
  
  classDef tag fill:#4A90E2,stroke:#2E5C8A,color:#fff,stroke-width:3px
  classDef sniffer fill:#50C878,stroke:#2E7D4E,color:#fff,stroke-width:3px
  classDef gateway fill:#9B59B6,stroke:#6C3483,color:#fff,stroke-width:3px
  classDef server fill:#F39C12,stroke:#C87F0A,color:#fff,stroke-width:3px
  
  class Tag1,Tag2,TagN tag
  class SnifferDev,UWB,LoRaModule sniffer
  class GatewayDev,Processor gateway
  class Server server
```

**Data Flow:** Tags (UWB) → Sniffer (LoRa VHF) → Gateway (Internet) → Backend Server

## 📦 Projects

### 🏷️ Persona (Personal Tag)
**MCU:** STM32U535VET  
**Role:** UWB responder worn by personnel

**Features:**
- UWB communication with DW3000
- Battery management (BQ25150)
- RGB LED status indicator (WS2812)
- Low-power sleep modes
- I2C & GPIO interfaces

### 📡 Sniffer-Tag (Location Anchor)
**MCU:** STM32G474RET  
**Role:** UWB initiator for distance measurement

**Features:**
- Dual DW3000 UWB chips (DS-TWR protocol)
- LoRa transmission (SX1276, VHF band)
- 8 configurable UWB timeouts
- EEPROM configuration storage
- UART command interface
- Supports up to 255 tags per sniffer
- Gateway communication via LoRa VHF

### 🛠️ Tools
GUI applications for configuration and monitoring:
- `sniffer_tag_gui.py` - UWB timeout configuration
- `gui_tool.py` - Detection history and CSV export
- `jira_manager.py` - Development workflow integration

## 🚀 Quick Start

1. **Clone repository:**
   ```bash
   git clone https://github.com/uqomm/fw-SmartTag.git
   cd fw-SmartTag
   ```

2. **Open in STM32CubeIDE:**
   - Import the desired project (Persona or sniffer-tag)
   - Build for Debug or Release

3. **Flash firmware:**
   - Connect ST-LINK programmer
   - Flash to target device

## 📚 Documentation

Comprehensive technical documentation is available in the `docs/` submodule:
- System architecture and specifications
- UWB timeout configuration guide
- Migration guides (850K baud rate)
- Protocol specifications
- Test reports and analysis

Access docs: `docs/docs/fw-smartTag/`

## 🏗️ Development

- **IDE:** STM32CubeIDE
- **Language:** C/C++
- **HAL:** STM32 HAL Drivers
- **Version Control:** Git with submodules

## 📋 Changelog

See project-specific CHANGELOGs:
- [Sniffer-Tag CHANGELOG](sniffer-tag/CHANGELOG.md)
- [Persona CHANGELOG](Persona/CHANGELOG.md)
- [Tools CHANGELOG](tools/CHANGELOG.md)

## 📄 License

Proprietary - UQOMM

## 🤝 Contributing

1. Create a feature branch
2. Commit changes with descriptive messages
3. Push to branch and create Pull Request
4. Ensure all tests pass before merging

---

**Latest Release:** v1.1.0  
**Repository:** https://github.com/uqomm/fw-SmartTag

## 🔧 Hardware Specifications

### Communication Modules
- **UWB:** Qorvo DW3000 (6.5 GHz band, IEEE 802.15.4z)
- **LoRa:** Semtech SX1276 (VHF band for gateway communication)
- **Range:** UWB up to 38m, LoRa long-range to gateway

---

## 📊 Operation Flow Diagrams

### ONE DETECTION Mode - UWB Ranging Protocol

```mermaid
sequenceDiagram
    participant Sniffer as Sniffer<br/>(STM32G474)
    participant Tag as Personal Tag<br/>(STM32U535)
    
    Note over Sniffer,Tag: DS-TWR (Double-Sided Two-Way Ranging)
    
    Sniffer->>+Tag: POLL Frame
    Note right of Sniffer: TX timestamp: T1
    
    Tag-->>-Sniffer: RESPONSE Frame
    Note left of Tag: RX timestamp: T2<br/>TX timestamp: T3
    
    Sniffer->>Tag: FINAL Frame
    Note right of Sniffer: RX timestamp: T4<br/>TX timestamp: T5
    
    Tag-->>Sniffer: FINAL Response
    Note left of Tag: RX timestamp: T6
    
    rect rgb(200, 230, 255)
    Note over Sniffer: Calculate Distance<br/>TOF = [(T4-T1)-(T3-T2)+(T6-T5)-(T6-T3)]/4<br/>Distance = TOF × c
    end
    
    Note over Sniffer: Store Distance<br/>Tag ID + Distance
```

**ONE DETECTION Features:**
- Single measurement per tag per cycle
- High precision (±10cm according to DW3000 specs)
- Optimized for energy efficiency
- 8 configurable timeouts for reliability at >20m

---

### Sniffer Data Transmission Flow

```mermaid
sequenceDiagram
    participant Tags as Personal Tags<br/>(Multiple)
    participant Sniffer as Sniffer Device<br/>(STM32G474)
    participant LoRa as LoRa Module<br/>(SX1276)
    participant Gateway as Gateway<br/>(VHF Receiver)
    
    rect rgb(220, 240, 255)
    Note over Tags,Sniffer: UWB Detection Phase
    loop For each Tag (up to 255)
        Tags->>Sniffer: UWB DS-TWR Protocol
        Sniffer->>Sniffer: Calculate Distance
        Sniffer->>Sniffer: Store in Memory<br/>(Tag_ID, Distance, Antenna)
    end
    end
    
    rect rgb(240, 255, 220)
    Note over Sniffer,Gateway: LoRa Transmission Phase
    
    Sniffer->>Sniffer: Build Data Packet<br/>Format: [Sniffer_ID | Tag_Count | Data...]
    
    Sniffer->>LoRa: Serial Data via UART
    Note right of Sniffer: Data includes:<br/>- Tag IDs<br/>- Distances<br/>- Antenna info<br/>- Timestamps
    
    LoRa->>Gateway: VHF Transmission
    Note right of LoRa: Long-range<br/>Low power<br/>LoRa modulation
    
    Gateway-->>Gateway: Process & Forward<br/>to Backend Server
    end
    
    Note over Sniffer: Transmission Interval:<br/>Configurable (default ~16s)<br/>Balances data freshness<br/>vs power consumption
```

**Transmission Protocol:**
- **Packet Structure:** Sniffer ID + Tag count + [Tag ID, Distance, Antenna] × N
- **Frequency:** VHF band (configurable)
- **Modulation:** LoRa (high range, low power)
- **Timeout:** 16 seconds LoRa send timeout
- **Error Handling:** Retry mechanism with logging
