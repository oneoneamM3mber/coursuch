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

### Windows (MinGW) — один клик

```
Двойной клик на: ЗАПУСТИТЬ_СБОРКУ.bat
```

### Windows (MinGW) — вручную

```
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:/Qt/6.11.0/mingw_64"
mingw32-make -j4
windeployqt StegoLab.exe
```

### Linux

```
sudo apt install cmake build-essential qtbase5-dev
mkdir build && cd build
cmake .. && make -j4
```

### Запуск готового exe

```
run.bat
```

## Структура проекта

```
├── CMakeLists.txt
├── ЗАПУСТИТЬ_СБОРКУ.bat  — сборка и запуск (Windows)
├── run.bat                — запуск готового exe
├── src/                   — исходные файлы
├── include/               — заголовочные файлы
├── tests/                 — модульные тесты (QtTest)
├── docs/                  — документация
├── resources/             — тестовые BMP-изображения
├── build_scripts/         — скрипт сборки для Linux
├── dist/                  — собранный exe + Qt DLL (готов к запуску)
└── .gitignore
```

## Тестирование

```
cmake --build . --target StegoLab_tests
ctest --output-on-failure
```

Тесты: LSB roundtrip, CRC32 integrity, container, capacity, metrics.
