# Technical Notes - DW3000 UWB System

## Note 1: Antenna Calibration

The sum of the values is the **TX to RX antenna delay**, experimentally determined by a calibration process. Here we use a hard coded typical value, but in a real application, each device should have its own antenna delay properly calibrated to get the best possible precision when performing range measurements.

---

## Note 2: UWB Message Format

The messages here are similar to those used in the **DecaRanging ARM application** (shipped with EVK1000 kit). They comply with the **IEEE 802.15.4** standard MAC data frame encoding and follow the **ISO/IEC:24730-62:2013** standard.

### Message Types

**Poll Message**
- Sent by the initiator to trigger the ranging exchange

**Response Message**
- Sent by the responder allowing the initiator to go on with the process

**Final Message**
- Sent by the initiator to complete the exchange and provide all information needed by the responder to compute the time-of-flight (distance) estimate

### Frame Structure (First 10 bytes - Common to all)

| Bytes | Field | Description |
|-------|-------|-------------|
| 0-1 | Frame Control | `0x8841` (data frame using 16-bit addressing) |
| 2 | Sequence Number | Incremented for each new frame |
| 3-4 | PAN ID | `0xDECA` |
| 5-6 | Destination Address | See NOTE 3 below |
| 7-8 | Source Address | See NOTE 3 below |
| 9 | Function Code | Specific values to indicate message type in ranging process |

### Message-Specific Data

**Poll Message:**
- No additional data

**Response Message:**
- Byte 10: Activity Code (`0x02` = proceed with ranging exchange)
- Bytes 11-12: Activity Parameter (not used for code 0x02)

**Final Message:**
- Bytes 10-13: Poll message transmission timestamp
- Bytes 14-17: Response message reception timestamp
- Bytes 18-21: Final message transmission timestamp

**Note:** All messages end with a **2-byte checksum** automatically set by DW IC.

---

## Note 3: Device Addressing

Source and destination addresses are **hard coded constants** in this example to keep it simple. In a real product:
- Every device should have a **unique ID**
- **16-bit addressing** is used to keep messages as short as possible
- Short addresses must be defined after an exchange of specific messages with each device participating in the ranging

---

## Note 4: Frame Delays and Synchronization

Delays between frames have been chosen to ensure:
- ✅ Proper synchronization of transmission and reception
- ✅ Correct accuracy of computed distance

### Timing Diagram

```
Initiator:   |Poll TX| ..... |Resp RX| ........ |Final TX|
Responder:   |Poll RX| ..... |Resp TX| ........ |Final RX|
                ^|P|                                    - Poll TX/RX time
                        ^|R|                            - Resp TX/RX time
                                           ^|F|         - Final TX/RX time
```

### Delay Parameters

| Parameter | Symbol | Description |
|-----------|--------|-------------|
| `POLL_TX_TO_RESP_RX_DLY_UUS` | `<--TDLY-->` | Poll TX to Resp RX delay |
| `RESP_RX_TIMEOUT_UUS` | `<-RLEN->` | Response reception timeout |
| `POLL_RX_TO_RESP_TX_DLY_UUS` | `<----RDLY----->` | How quickly responder can turn around |
| `RESP_TX_TO_FINAL_RX_DLY_UUS` | `<--T2DLY-->` | Resp TX to Final RX delay |
| `FINAL_RX_TIMEOUT_UUS` | `<-FLEN--->` | Final reception timeout |
| `RESP_RX_TO_FINAL_TX_DLY_UUS` | `<----RDLY----->` | How quickly initiator can turn around |

### Configuration Examples

**Example 1: SPI @ 18 MHz (Default)**
- Frame length: ~190 µs
- `POLL_RX_TO_RESP_TX_DLY_UUS`: 400 µs
- `RESP_RX_TO_FINAL_TX_DLY_UUS`: 400 µs
- TXtoRX delays: 210 µs

> Can be reduced further by using interrupts or code optimizations/faster SPI

**Example 2: SPI @ 4.5 MHz**
- Frame length: ~190 µs
- `POLL_RX_TO_RESP_TX_DLY_UUS`: 550 µs
- `RESP_RX_TO_FINAL_TX_DLY_UUS`: 600 µs
- TXtoRX delays: 360 and 410 µs respectively

---

## Note 5: Reception Timeouts

This timeout is for **complete reception of a frame**, meaning timeout duration must account for the **expected frame length**.

At **6.81 Mbps** data rate with frame lengths of ~190 µs, the typical value is **around 200 µs**.

---

## Note 6: Maximum SPI Speed

In this example, the DW IC is put into **IDLE state** after calling `dwt_initialise()`.

This means that a **fast SPI rate of up to 38 MHz** can be used thereafter.

---

## Note 7: Preamble Timeout

The **preamble timeout** allows the receiver to **stop listening** when preamble is not starting.

### Use Cases

- **Out of range**: Responder is beyond reception range
- **No response**: Responder did not receive the message
- **Power savings**: Avoids wasting power listening for a message that won't come

### Recommendations

| Scenario | PAC Minimum | Description |
|----------|------------|-------------|
| Short range | 5 PAC | Short range applications |
| Long range | 50-80% of preamble length | Challenging long range, NLOS, or noisy environments |

---

## Note 8: TX Power and Bandwidth Configuration

In a real application, for **optimum performance** within regulatory limits, it may be necessary to:

- Set **TX pulse bandwidth**
- Configure **TX power**
- Use API: `dwt_configuretxrf()`
- Use **per-device calibrated values** saved in:
  - Target system
  - DW IC OTP memory

---

## Note 9: TX Data Writing

`dwt_writetxdata()` takes the **full message size** as parameter but only copies **(size - 2) bytes**.

### Details

- The **2-byte checksum** is automatically appended by the DW IC
- Variable could be **2 bytes shorter** without losing data
- However, `sizeof()` would still report full length

---

## Note 10: Polling vs Interrupt Operation

We use **polled mode** here to keep the example simple.

### Alternatives

- **Polling**: Simple, sequential operation
- **Interrupts**: All status events can generate interrupts

### Accessing STATUS Register

The **STATUS register** is **5 bytes long**, but events are in the first bytes:
- Use simple API: `dwt_read32bitreg()`
- Instead of reading all 5 bytes

---

## Note 11: Final TX Timestamp Calculation

To send the **final TX timestamp** in the final message:
- Calculate it **in advance** (don't read from register)
- Add desired response delay to RX timestamp

### Resolution Details

- **Timestamps**: Expressed in **device time units**
- **Delayed TX resolution**: **512 device time units**
- **Operation**: Lower **9 bits must be zeroed** (512-alignment)
- **Encoding**: Encode **40-bit value** in **32-bit words** by shifting 8 zero bits

---

## Note 12: Timestamp Calculation Precision

The **high order byte** of each **40-bit timestamp** is **discarded**.

### Justification

- Timestamps **not separated** by more than 2³² device time units
- Equals approximately **67 ms**
- Round-trip delay calculation can use **32-bit subtraction**
- Sufficient precision for time-of-flight computation

---

## Note 13: dwt_starttx() Error Handling

On DWK3000 platform with provided `RESP_RX_TO_FINAL_TX_DLY`, `dwt_starttx()` is always successful.

### Error Case

If delay is **too short** or code flow is interrupted:
- `dwt_starttx()` may issue **too late** for configured start time
- Code **stuck waiting** for event that never comes

### Solution

```cpp
// Handle late transmission condition
if (dwt_starttx(DWT_START_TX_DELAYED) != DWT_SUCCESS) {
    // Abandon ranging exchange, retry after 1 second
    tag_status = TAG_TX_LATE;
    retry_time = HAL_GetTick() + 1000;
}
```

**Note:** Companion "responder" example (ex_05b) should timeout awaiting "final" and prepare for next poll.

---

## Note 14: Additional Resources

For more information, refer to:

- **DecaRanging ARM Application**
  - Distributed with EVK1000 product
  - Additional practical usage examples

- **DW IC API Guide**
  - Details on DW IC driver functions

- **DW IC User Manual**
  - Complete chip specifications

---

## Quick Reference

### Default Configuration
```cpp
Channel: 5 (6.5 GHz)
Data Rate: 6.8 Mbps
PRF: 64 MHz
Preamble: 64 symbols
PAC: 8 symbols
```

### Typical Timeouts (6.8 Mbps)
```cpp
POLL_TX_TO_RESP_RX_DLY_UUS: 700 µs
RESP_RX_TIMEOUT_UUS: 600 µs
POLL_RX_TO_RESP_TX_DLY_UUS: 400 µs
RESP_RX_TO_FINAL_TX_DLY_UUS: 400 µs
```

### Critical Parameters
```cpp
PRE_TIMEOUT: 8 PAC
SFD Timeout: (1025 + 1 + 8 - 32) = 1002
Antenna Delay: Device calibrated
```
