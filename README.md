# avm-emon

[![pipeline status](https://git.avmenergo.ru/avm-energo/avm-emon/badges/main/pipeline.svg)](https://git.avmenergo.ru/avm-energo/avm-emon/-/pipelines)
[![latest release](https://git.avmenergo.ru/avm-energo/avm-emon/-/badges/release.svg)](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases)

Русская версия: [README-ru.md](README-ru.md)

**AVM EMON** is a simple GUI client for reading the **EM-3.1KM** (Energomonitor device by MARS-Energo) power-quality meter over USB or RS-232.

This software was made by AVM-Energo LLC with the help of Claude.

## Download

Latest release:

- **Windows** (installer): [avm-emon-setup.exe](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases/permalink/latest/downloads/avm-emon-setup.exe)
- **Linux, Debian/Ubuntu**: [avm-emon.deb](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases/permalink/latest/downloads/avm-emon.deb)
- **Linux, RPM** (Fedora/RED OS): [avm-emon.rpm](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases/permalink/latest/downloads/avm-emon.rpm)

All releases: [https://git.avmenergo.ru/avm-energo/avm-emon/-/releases](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases)

## Features

- Connection: port picker (FTDI 0403:6001 auto-highlighted), link type (USB / RS-232), connect/disconnect.
- Measurements: U / I (RMS) / I-U angle per phase A (yellow) / B (green) / C (red) and frequency, refreshed continuously. Values are shown as the device reports them (the range selection does not rescale them).
- Ranges: comboboxes for the current sensor type (transformer / clamp / UPTT), the current range and the voltage range, filled from the list the device reports; the selection follows the range the device is actually using.
- Vector diagram of the measured voltages and currents (libavm-vectorwidget). Phase A is the reference; the voltage angles come from the measured angles between the phase voltages.
- All device calls run on a worker thread, so the window never freezes while the meter answers.

## Build

```sh
cmake -B build -S . -DCMAKE_PREFIX_PATH=<qt>
cmake --build build
./build/avm-emon
```

Uses an installed libavm-emon and libavm-vectorwidget, else `../libavm-emon` / `../libavm-vectorwidget`, else fetches them from git (https://git.avmenergo.ru/avm-energo/libavm-emon.git, https://git.avmenergo.ru/avm-energo/libavm-vectorwidget.git).
