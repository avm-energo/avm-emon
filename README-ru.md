# avm-emon

[![pipeline status](https://git.avmenergo.ru/avm-energo/avm-emon/badges/master/pipeline.svg)](https://git.avmenergo.ru/avm-energo/avm-emon/-/pipelines)
[![latest release](https://git.avmenergo.ru/avm-energo/avm-emon/-/badges/release.svg)](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases)

English: [README.md](README.md)

**AVM EMON** — простой графический (GUI) клиент для чтения показаний измерителя показателей качества электроэнергии **ЭМ-3.1КМ** (Энергомонитор производства ООО "МАРС-Энерго") по USB или RS-232.

Программа разработана ООО «АВМ-Энерго» с помощью Claude.

## Скачать

Последний релиз:

- **Windows** (установщик): [avm-emon-setup.exe](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases/permalink/latest/downloads/avm-emon-setup.exe)
- **Linux, Debian/Ubuntu**: [avm-emon.deb](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases/permalink/latest/downloads/avm-emon.deb)
- **Linux, RPM** (Fedora/RED OS): [avm-emon.rpm](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases/permalink/latest/downloads/avm-emon.rpm)

Все релизы: [https://git.avmenergo.ru/avm-energo/avm-emon/-/releases](https://git.avmenergo.ru/avm-energo/avm-emon/-/releases)

## Возможности

- Подключение: выбор порта (FTDI 0403:6001 подсвечивается автоматически), тип связи (USB / RS-232), подключение/отключение.
- Измерения: U / I (действующие) / угол I-U по каждой фазе A (жёлтая) / B (зелёная) / C (красная) и частота, обновляются непрерывно. Значения откалиброваны так, чтобы совпадать с показаниями передней панели прибора.
- Диапазоны: выпадающие списки типа датчика тока (трансформатор / клещи / УПТТ), диапазона тока и диапазона напряжения; списки заполняются из данных, которые сообщает прибор, а выбор следует за диапазоном, который прибор реально использует.
- Все обращения к прибору выполняются в рабочем потоке, поэтому окно не «зависает», пока измеритель отвечает.

## Сборка

```sh
cmake -B build -S . -DCMAKE_PREFIX_PATH=<qt>
cmake --build build
./build/avm-emon
```

Используется установленная библиотека `libavm-emon`, иначе `../libavm-emon`, иначе она загружается из git (https://git.avmenergo.ru/avm-energo/libavm-emon.git).