# Курсовая работа

**Тема:** Разработка программного модуля стеганографии изображений с поддержкой LSB-метода и контейнерного внедрения данных (C++)

## Функциональные возможности

- **LSB-стеганография** — внедрение текстовых сообщений в BMP-изображения с глубиной 1, 2 или 4 бита на пиксель
- **Контейнерный метод** — внедрение архивов (ZIP/RAR/7z) в BMP-файлы
- **Контроль целостности** — CRC32 для обнаружения модификации изображения после внедрения
- **Метрики качества** — расчёт MSE и PSNR
- **База данных** — SQLite для хранения истории операций
- **GUI** — графический интерфейс на Qt Widgets

## Сборка

### Зависимости

| Компонент | Версия |
|-----------|--------|
| CMake | 3.16+ |
| GCC/MinGW | 10+ / MinGW 8.1+ |
| Qt | 5.15+ или 6.2+ (Widgets, Sql, Test) |
| SQLite | Встроенная в Qt |

### Windows (MinGW)

```
git clone https://github.com/oneoneamM3mber/coursuch
cd coursuch
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/mingw_64"
mingw32-make -j4
```

Или запустить `ЗАПУСТИТЬ_СБОРКУ.bat`

### Linux

```
sudo apt install cmake build-essential qtbase5-dev qt5-qmake
mkdir build && cd build
cmake .. && make -j4
```

## Структура проекта

```
├── CMakeLists.txt
├── src/              — исходные файлы
├── include/          — заголовочные файлы
├── tests/            — модульные тесты (QtTest)
├── docs/             — документация
├── resources/        — тестовые BMP-изображения
└── build_scripts/    — скрипты сборки
```

## Тестирование

```
cmake --build . --target StegoLab_tests
ctest --output-on-failure
```

Тесты: LSB roundtrip, CRC32 integrity, container, capacity, metrics.

## Алгоритмы

**LSB:** N младших бит пикселя заменяются битами сообщения. Формат внедрения:

```
[4 байта: длина][4 байта: CRC32][данные сообщения]
```

**Контейнерный:** данные архива дописываются в конец BMP после маркера `STEG_CONTAINER`.

## Паттерны проектирования

- **Strategy** — LSB-1/2/4 и Container
- **Facade** — MainWindow как единый интерфейс
- **Repository** — Database для SQLite
