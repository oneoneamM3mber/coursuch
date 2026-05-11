# StegoLab — Модуль стеганографии BMP изображений
## Курсовая работа: Разработка программного модуля стеганографии изображений с поддержкой LSB-метода и контейнерного внедрения данных (C++)

---

## 📁 Файловая структура проекта

```
StegoLab/
│
├── CMakeLists.txt              ← Главный файл сборки (CMake)
│
├── include/                    ← Заголовочные файлы (.h)
│   ├── steganography.h         ← Интерфейс LSB и контейнерного метода
│   ├── database.h              ← Интерфейс БД (SQLite)
│   └── mainwindow.h            ← Главное окно GUI (Qt)
│
├── src/                        ← Исходные файлы (.cpp)
│   ├── main.cpp                ← Точка входа приложения
│   ├── steganography.cpp       ← Реализация алгоритмов стеганографии
│   ├── database.cpp            ← Работа с базой данных SQLite
│   └── mainwindow.cpp          ← Реализация GUI (Qt Widgets)
│
├── db/                         ← SQLite (используется системная)
│
├── resources/                  ← Ресурсы (иконки, тестовые BMP)
│
└── build_scripts/
    ├── build_windows.bat       ← Скрипт сборки для Windows
    └── build_linux.sh          ← Скрипт сборки для Linux
```

---

## ⚙️ Зависимости

| Компонент | Минимальная версия | Для чего |
|-----------|-------------------|----------|
| CMake     | 3.16+             | Система сборки |
| GCC/MSVC  | GCC 10+ / VS 2019+ | Компилятор C++17 |
| Qt5/Qt6   | Qt 5.15+ или Qt 6.2+ | GUI (Widgets) |
| SQLite3   | 3.35+             | База данных истории |

---

## 🚀 ЗАПУСК НА WINDOWS — MinGW (ваш компилятор)

### Самый простой способ — один файл:
```
Двойной клик на: ЗАПУСТИТЬ_СБОРКУ.bat
```
Скрипт сам найдёт MinGW и Qt, скомпилирует и запустит программу.

### Требования (устанавливаются через Qt Installer одним пакетом):
| Компонент | Где взять | Что выбрать |
|-----------|-----------|-------------|
| Qt 5.15   | qt.io/download | `Qt 5.15.2 → MinGW 8.1.0 64-bit` |
| MinGW     | Qt Installer  | `Tools → MinGW 8.1.0 64-bit` |
| CMake     | Qt Installer  | `Tools → CMake 64-bit` |

### Ручная сборка:
```bat
REM Задать пути (под вашу установку Qt):
set QTDIR=C:\Qt\5.15.2\mingw81_64
set MINGW=C:\Qt\Tools\mingw810_64
set PATH=%QTDIR%\bin;%MINGW%\bin;C:\Qt\Tools\CMake_64\bin;%PATH%

mkdir build_mingw && cd build_mingw
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QTDIR%"
mingw32-make -j4
cd ..

REM Собрать дистрибутив (папку для флешки):
mkdir dist
copy build_mingw\StegoLab.exe dist\
windeployqt --release --no-translations --dir dist dist\StegoLab.exe
```

### Перенос на флешку (для презентации):
После сборки скопировать **папку `dist\`** целиком:
```
dist/
├── StegoLab.exe
├── Qt5Widgets.dll / Qt5Core.dll / Qt5Gui.dll
├── libgcc_s_seh-1.dll       ← MinGW runtime
├── libstdc++-6.dll
├── libwinpthread-1.dll
└── platforms/qwindows.dll
```
> Запускается на **любом Windows 10/11 x64** без установки Qt и MinGW!

---

## 🚀 ЗАПУСК НА WINDOWS (MSVC — альтернатива)

### Шаг 1: Установка Qt

1. Скачать **Qt Online Installer**: https://www.qt.io/download-qt-installer  
2. В установщике выбрать:  
   - `Qt 5.15.2 → MSVC 2019 64-bit` (рекомендуется)  
   - или `Qt 6.6.0 → MSVC 2022 64-bit`
3. Также установить компонент **CMake** (включён в Qt)

### Шаг 2: Установка Visual Studio Build Tools

- Скачать **VS Build Tools 2022** (бесплатно):  
  https://aka.ms/vs/17/release/vs_BuildTools.exe  
- Выбрать: `Desktop development with C++`

### Шаг 3: Сборка проекта

```bat
REM Открыть "x64 Native Tools Command Prompt for VS 2022"
REM (или "Developer Command Prompt")

cd C:\путь\к\StegoLab

REM Установить путь к Qt (изменить под свою версию!):
set QTDIR=C:\Qt\5.15.2\msvc2019_64
set PATH=%QTDIR%\bin;%PATH%

REM Сборка:
build_scripts\build_windows.bat
```

**Или вручную:**
```bat
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64"
cmake --build . --config Release
cd ..
windeployqt build\Release\StegoLab.exe
```

### Шаг 4: Запуск

После сборки запустить: `build\Release\StegoLab.exe`

---

## 🚀 ЗАПУСК НА LINUX (Ubuntu/Debian)

```bash
# Установить зависимости
sudo apt update
sudo apt install -y cmake build-essential qtbase5-dev qt5-qmake libsqlite3-dev

# Собрать
chmod +x build_scripts/build_linux.sh
./build_scripts/build_linux.sh

# Запустить
./build/StegoLab
```

---

## 💾 ПЕРЕНОС НА ФЛЕШКУ ДЛЯ ПРЕЗЕНТАЦИИ

### Вариант A — Перенести исходники + собрать на месте:
1. Скопировать всю папку `StegoLab/` на флешку
2. На компьютере для презентации установить Qt + CMake
3. Запустить `build_windows.bat`

### Вариант B — Перенести готовый EXE (рекомендуется):
После сборки и выполнения `windeployqt` скопировать **всю папку `build\Release\`** на флешку. В ней будет:
```
Release/
├── StegoLab.exe          ← главный файл
├── Qt5Widgets.dll        ← Qt runtime
├── Qt5Core.dll
├── Qt5Gui.dll
├── platforms/            ← платформенные плагины Qt
│   └── qwindows.dll
└── sqlite3.dll           ← (если системная не найдена)
```
> **Важно:** `windeployqt` автоматически соберёт все нужные DLL.  
> Запустить на любом Windows 10/11 x64 без установки чего-либо!

---

## 🔬 Описание алгоритмов (для защиты)

### LSB-метод (Least Significant Bit)
Заменяет N младших бит каждого байта пикселя битами скрываемого сообщения.

```
Оригинальный пиксель R:  10110111  (0xB7)
LSB-1 с битом '1':       10110111  → незаметно
LSB-4 с данными '0101':  10110101  → чуть заметно
```

**Заголовок сообщения:** первые 4 байта кодируют длину (uint32), затем идут байты сообщения.

**Ёмкость при LSB-1:** `(ширина × высота × 3 байта на пиксель) / 8 - 4` байт

| LSB биты | Ёмкость (1920×1080 24-bit) | PSNR (типично) |
|----------|---------------------------|----------------|
| LSB-1    | ~777 КБ                  | ≥ 51 дБ        |
| LSB-2    | ~1.5 МБ                  | ~44 дБ         |
| LSB-4    | ~3.1 МБ                  | ~38 дБ         |

### Контейнерный метод (стеганография файлового контейнера)
Архив (ZIP/RAR/7z) **дописывается в конец** BMP-файла после пиксельных данных. BMP-просмотрщики игнорируют данные после конца изображения.

```
[BMP заголовок][пиксели BMP][маркер "STEG_CONTAINER"][размер архива][данные архива]
```

Преимущества: неограниченный размер внедряемых данных, полное сохранение качества изображения.

---

## 🗄️ База данных

Файл `stego_history.db` (SQLite) создаётся автоматически рядом с EXE.

**Таблица `images`:**

| Поле      | Тип     | Описание |
|-----------|---------|----------|
| id        | INTEGER | Первичный ключ |
| filepath  | TEXT    | Полный путь к файлу |
| filename  | TEXT    | Имя файла |
| width     | INTEGER | Ширина в пикселях |
| height    | INTEGER | Высота в пикселях |
| bitdepth  | INTEGER | Глубина цвета (бит) |
| psnr      | REAL    | PSNR (дБ) |
| mse       | REAL    | MSE |
| capacity  | INTEGER | Ёмкость (байт) |
| method    | TEXT    | Метод: LSB-1/2/4, Container |
| ts        | TEXT    | Дата и время операции |
| notes     | TEXT    | Примечания |

---

## 📊 Метрики качества

**MSE (Mean Squared Error) — среднеквадратическая ошибка:**
```
MSE = (1/N) × Σ(orig_i - steg_i)²
```

**PSNR (Peak Signal-to-Noise Ratio):**
```
PSNR = 10 × log₁₀(255² / MSE)   [дБ]
```

- PSNR > 40 дБ → изменения визуально незаметны
- PSNR > 30 дБ → изменения слабо заметны  
- PSNR < 30 дБ → заметное ухудшение качества

---

## 🎯 Паттерны проектирования (для защиты)

| Паттерн | Где применён | Описание |
|---------|-------------|----------|
| **Strategy** | `Steganography` класс | LSB-1/2/4 и Container — разные стратегии внедрения |
| **Facade** | `MainWindow` | Единый интерфейс, скрывающий embed/extract/analyze |
| **Template Method** | `embedLSB` / `extractLSB` | Общий алгоритм: загрузка → обработка → сохранение |
| **Repository** | `Database` класс | Инкапсуляция доступа к SQLite |

---

## ❓ Часто задаваемые вопросы на защите

**В: Почему BMP, а не JPG/PNG?**  
О: BMP хранит пиксели без сжатия. JPEG — с потерями (уничтожит скрытые биты). PNG использует предсказание, что затрудняет LSB. BMP — идеальный носитель для LSB-стеганографии.

**В: Как обнаружить стеганографию (стегоанализ)?**  
О: Chi-square атака (статистический анализ распределения LSB), RS-анализ (Regular/Singular), визуальный анализ увеличенных фрагментов изображения.

**В: Можно ли зашифровать сообщение перед внедрением?**  
О: Да — шифрование (AES/XOR) делается до вызова `embedLSB()`. В данной реализации использован чистый текст для наглядности курсовой работы.
