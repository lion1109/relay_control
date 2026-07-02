# Relay Control

## Protocol Overview

All commands are fixed 4-byte packets:

| Byte | Description |
|------|-------------|
| 0 | Command prefix (0xA0) |
| 1 | Command / relay index |
| 2 | Parameter |
| 3 | Checksum = (byte0 + byte1 + byte2) & 0xFF |

### Command Summary

| Command | Format | Description |
|--------|--------|-------------|
| Relay control | `A0 rr oo cs` | Switch relay `rr` ON/OFF |
| All relays | `A0 FF oo cs` | Switch all relays |
| Active level | `A0 FE lv cs` | Set active GPIO level |
| Logging | `A0 FC en cs` *(planned)* | Enable/disable logging |
| Timeout | `A0 FD tt cs` *(planned)* | Watchdog timeout configuration |
| GPIO mapping | `A0 FB rg cs` *(planned)* | Map relay `r` to GPIO `g` |

---

## Timeout Feature (Security Watchdog)

The timeout feature is designed as a **fail-safe mechanism** to ensure that all relays are switched off if the controlling sender is no longer active.

### Purpose

In real-world applications (automation, lab equipment, power switching), communication can be interrupted due to:

- USB disconnect
- software crash on host side
- corrupted data stream
- unexpected reset of controlling system

Without a timeout, relays may remain in their last state indefinitely, which can be unsafe.

---

### Behavior

If enabled, the firmware tracks the time since the last valid command was received.

If this time exceeds the configured timeout:

> All relays are automatically switched OFF.

---

### Configuration Command

```
A0 FD tt cs
```

| Value | Meaning |
|------:|--------|
| 0 | timeout disabled (current default behavior) |
| 1–250 | timeout in 1/100 seconds (10 ms steps → up to 2.5 s) |
| 251–254 | reserved |
| 255 | per-command timeout mode (advanced failsafe feature, future) |

---

### Example

```
A0 FD 50 F5
```

→ timeout = 0.5 seconds

---

## GPIO Mapping Feature (Planned)

```
A0 FB rg cs
```

| Byte | Meaning |
|------|--------|
| r | relay number (4 bit) |
| g | GPIO number (4 bit encoded, if g > 10 then g += 9, if g = 15, then delete mapping) |

### Purpose

Allows dynamic mapping of relays to GPIO pins at runtime without recompiling firmware.

This is useful for:

- modular relay boards
- hardware variants
- production calibration
- test setups

---

## ESP32-C3 Super Mini GPIO Availability

The ESP32-C3 Super Mini has **limited but usable GPIO resources**.

### Usable GPIOs for relays

In practice, the following GPIOs are typically usable on ESP32-C3 Super Mini:

- GPIO0
- GPIO1
- GPIO2 is boot sensitive, so do not use it.
- GPIO3
- GPIO4
- GPIO5
- GPIO6
- GPIO7
- GPIO8 may be boot sensitive, so do not use it.
- GPIO9 may be boot sensitive, so do not use it.
- GPIO10
- GPIO20
- GPIO21

👉 Roughly: **10 to 12 GPIOs are realistically usable**

---

### GPIOs that should NOT be used for relays

Some pins are reserved or risky:

#### 1. Bootstrapping pins
These affect boot mode and can prevent startup if pulled incorrectly:

- GPIO9 (boot strapping behavior depending on board)
- GPIO8 (sometimes used for flash voltage configuration)

#### 2. USB / Serial interface pins
On ESP32-C3 Super Mini:

- USB D+ / D- are fixed internally
- USB-JTAG uses internal routing (not GPIO-reassignable)

#### 3. Flash / SPI internal pins
Some pins are internally used for flash and PSRAM:

- GPIO11–GPIO17 (often reserved depending on module variant)

#### 4. Input-only / restricted pins (board dependent)

Some boards expose pins that are:

- input-only
- weak drive capability
- shared with onboard LEDs or functions

---

### Practical recommendation

For relay usage, a **safe GPIO subset** is:

```
GPIO0–GPIO7 + GPIO1O + GPIO20 + GPIO21
```

Always verify against the **specific Super Mini schematic**, because ESP32-C3 modules differ slightly between manufacturers.

---

## GPIO Mapping Safety Note

Dynamic GPIO mapping (A0 FB) must ensure:

- no boot-critical pins are assigned
- no duplicate GPIO assignment
- no overlap with USB or system functions

Invalid mappings should be rejected by firmware.

---