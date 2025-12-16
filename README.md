# fw-SmartTag

Firmware for the SmartTag UWB-based indoor localization system, consisting of personal tags and sniffer devices for real-time positioning.

## 🎯 System Overview

```mermaid
block-beta
  columns 3
  
  block:Tags:1
    columns 1
    Tag1["Personal Tag<br/>STM32U535"]
    Tag2["Personal Tag<br/>STM32U535"]
    TagN["Personal Tag<br/>STM32U535"]
  end
  
  space
  
  block:Sniffer:1
    columns 1
    SnifferDev["Sniffer Device<br/>STM32G474"]
    UWB["DW3000 UWB<br/>DS-TWR Protocol"]
    LoRaModule["LoRa SX1278<br/>Long Range"]
  end
  
  Tags--"UWB Ranging<br/>38m max"-->Sniffer
  space:3
  Sniffer--"Distance Data<br/>LoRa 915MHz"-->Server["Backend Server"]
  
  classDef tag fill:#4A90E2,stroke:#2E5C8A,color:#fff
  classDef sniffer fill:#50C878,stroke:#2E7D4E,color:#fff
  classDef server fill:#F39C12,stroke:#C87F0A,color:#fff
  
  class Tag1,Tag2,TagN tag
  class SnifferDev,UWB,LoRaModule sniffer
  class Server server
```

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
- LoRa transmission (SX1278, 915MHz)
- 8 configurable UWB timeouts
- EEPROM configuration storage
- UART command interface
- Supports up to 255 tags per sniffer

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

**Latest Release:** v1.0.0  
**Repository:** https://github.com/uqomm/fw-SmartTag
