# Firmware

PlatformIO/ESP-IDF firmware for the ISOBUS dummy hardware.

## Build

```bash
pio run -e esp32s3_n4
```

## Upload

```bash
pio run -e esp32s3_n4 -t upload
```

## Monitor

```bash
pio device monitor -e esp32s3_n4
```

## Pinned Versions

- `espressif32@6.12.0`
- `framework-espidf@3.50500.0` (`ESP-IDF 5.5.0`)
- `toolchain-xtensa-esp-elf@14.2.0+20241119`
- `AgIsoStack++` at commit `509a920cadff634612e8972158e1c293c9f372c7`

The custom board definition lives in `boards/esp32-s3-wroom-1-n4.json`.
The VT object pool in `object_pool/StandardPool.iop` is embedded at build time.
