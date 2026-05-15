#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QSizePolicy>
#include <QHeaderView>
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QFontDatabase>
#include <QFrame>
#include <QPalette>
#include <QScrollArea>
#include <iomanip>
#include <sstream>

// ─── helpers ────────────────────────────────────────────────
static QString fmtDouble(double v, int prec = 2) {
    return QString::number(v, 'f', prec);
}
static QFrame* hLine() {
    auto* f = new QFrame;
    f->setFrameShape(QFrame::HLine);
    f->setObjectName("hline");
    return f;
}

// ─── constructor ────────────────────────────────────────────
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("StegoLab — Модуль стеганографии BMP [LSB + Контейнер]");
    setMinimumSize(1100, 720);
    resize(1280, 800);

    // Open / create DB next to executable
    QString dbPath = QDir::currentPath() + "/stego_history.db";
    m_db = std::make_unique<Database>(dbPath);
    m_db->open();

    setupUI();
    applyStyles();
}

MainWindow::~MainWindow() {}

// ─── UI setup ────────────────────────────────────────────────
void MainWindow::setupUI() {
    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName("mainTabs");

    auto* embedTab    = new QWidget;
    auto* extractTab  = new QWidget;
    auto* containerTab= new QWidget;
    auto* dbTab       = new QWidget;

    setupEmbedTab(embedTab);
    setupExtractTab(extractTab);
    setupContainerTab(containerTab);
    setupDatabaseTab(dbTab);

    m_tabs->addTab(embedTab,     "🔒  Внедрение (LSB)");
    m_tabs->addTab(extractTab,   "🔓  Извлечение (LSB)");
    m_tabs->addTab(containerTab, "📦  Контейнерный метод");
    m_tabs->addTab(dbTab,        "🗄️  История изображений");

    setCentralWidget(m_tabs);

    m_statusBar = new QStatusBar(this);
    setStatusBar(m_statusBar);
    m_statusBar->showMessage("Готов к работе");
}

// ─── EMBED TAB ───────────────────────────────────────────────
void MainWindow::setupEmbedTab(QWidget* tab) {
    auto* root = new QHBoxLayout(tab);
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(16);

    // LEFT: controls
    auto* leftCol = new QVBoxLayout;
    leftCol->setSpacing(10);

    // Buttons row
    auto* btnRow = new QHBoxLayout;
    m_importBtn  = new QPushButton("Импорт BMP");
    m_loadMsgBtn = new QPushButton("Загрузить TXT");
    m_embedBtn   = new QPushButton("Внедрить");
    m_saveBtn    = new QPushButton("Сохранить");
    m_importBtn->setObjectName("primaryBtn");
    m_embedBtn->setObjectName("accentBtn");
    m_saveBtn->setObjectName("successBtn");
    btnRow->addWidget(m_importBtn);
    btnRow->addWidget(m_loadMsgBtn);
    btnRow->addWidget(m_embedBtn);
    btnRow->addWidget(m_saveBtn);
    leftCol->addLayout(btnRow);

    // Method selector
    auto* methodRow = new QHBoxLayout;
    auto* methodLbl = new QLabel("Метод:");
    m_methodCombo   = new QComboBox;
    m_methodCombo->addItems({"LSB-1 бит", "LSB-2 бита", "LSB-4 бита"});
    m_lsbBits = new QSpinBox;
    m_lsbBits->setRange(1,4);
    m_lsbBits->setValue(1);
    m_lsbBits->setVisible(false); // hidden, synced with combo
    methodRow->addWidget(methodLbl);
    methodRow->addWidget(m_methodCombo);
    methodRow->addStretch();
    leftCol->addLayout(methodRow);
    leftCol->addWidget(hLine());

    // Message input label
    auto* msgLbl = new QLabel("Сообщение для внедрения:");
    msgLbl->setObjectName("sectionLabel");
    leftCol->addWidget(msgLbl);

    // Message text area
    m_messageEdit = new QTextEdit;
    m_messageEdit->setPlaceholderText("Введите текст здесь или загрузите из TXT файла…");
    m_messageEdit->setObjectName("msgEdit");
    m_messageEdit->setMinimumHeight(120);
    leftCol->addWidget(m_messageEdit);

    leftCol->addWidget(hLine());

    // Metrics output
    auto* metricsLbl = new QLabel("Метрики качества:");
    metricsLbl->setObjectName("sectionLabel");
    leftCol->addWidget(metricsLbl);
    m_metricsLabel = new QLabel("—");
    m_metricsLabel->setObjectName("metricsBox");
    m_metricsLabel->setWordWrap(true);
    m_metricsLabel->setMinimumHeight(80);
    leftCol->addWidget(m_metricsLabel);
    leftCol->addStretch();

    // RIGHT: preview + info
    auto* rightCol = new QVBoxLayout;
    rightCol->setSpacing(10);

    m_embedPreview = new QLabel("Предпросмотр\nизображения");
    m_embedPreview->setAlignment(Qt::AlignCenter);
    m_embedPreview->setObjectName("imagePreview");
    m_embedPreview->setMinimumSize(340, 260);
    m_embedPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rightCol->addWidget(m_embedPreview);

    m_embedInfoLabel = new QLabel("—");
    m_embedInfoLabel->setObjectName("infoBox");
    m_embedInfoLabel->setWordWrap(true);
    m_embedInfoLabel->setMinimumHeight(80);
    rightCol->addWidget(m_embedInfoLabel);

    root->addLayout(leftCol, 3);
    root->addLayout(rightCol, 2);

    // Connections
    connect(m_importBtn,    &QPushButton::clicked, this, &MainWindow::onImportImage);
    connect(m_loadMsgBtn,   &QPushButton::clicked, this, &MainWindow::onLoadMessageFromFile);
    connect(m_embedBtn,     &QPushButton::clicked, this, &MainWindow::onEmbedMessage);
    connect(m_saveBtn,      &QPushButton::clicked, this, &MainWindow::onSaveResult);
    connect(m_methodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMethodChanged);
}

// ─── EXTRACT TAB ────────────────────────────────────────────
void MainWindow::setupExtractTab(QWidget* tab) {
    auto* root = new QHBoxLayout(tab);
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(16);

    // LEFT
    auto* leftCol = new QVBoxLayout;
    leftCol->setSpacing(10);

    auto* btnRow = new QHBoxLayout;
    m_importStegBtn  = new QPushButton("Импорт BMP");
    m_extractBtn     = new QPushButton("Извлечь");
    m_saveExtractedBtn = new QPushButton("Сохранить TXT");
    m_importStegBtn->setObjectName("primaryBtn");
    m_extractBtn->setObjectName("accentBtn");
    m_saveExtractedBtn->setObjectName("successBtn");
    btnRow->addWidget(m_importStegBtn);
    btnRow->addWidget(m_extractBtn);
    btnRow->addWidget(m_saveExtractedBtn);
    leftCol->addLayout(btnRow);

    auto* methodRow = new QHBoxLayout;
    auto* ml = new QLabel("Метод:");
    m_extractMethodCombo = new QComboBox;
    m_extractMethodCombo->addItems({"LSB-1 бит", "LSB-2 бита", "LSB-4 бита"});
    m_extractLsbBits = new QSpinBox;
    m_extractLsbBits->setRange(1,4);
    m_extractLsbBits->setValue(1);
    m_extractLsbBits->setVisible(false);
    methodRow->addWidget(ml);
    methodRow->addWidget(m_extractMethodCombo);
    methodRow->addStretch();
    leftCol->addLayout(methodRow);
    leftCol->addWidget(hLine());

    auto* extractedLbl = new QLabel("Извлечённое сообщение:");
    extractedLbl->setObjectName("sectionLabel");
    leftCol->addWidget(extractedLbl);

    m_extractedText = new QTextEdit;
    m_extractedText->setReadOnly(true);
    m_extractedText->setObjectName("msgEdit");
    m_extractedText->setPlaceholderText("Здесь появится расшифрованное сообщение…");
    m_extractedText->setMinimumHeight(200);
    leftCol->addWidget(m_extractedText);
    leftCol->addStretch();

    // RIGHT
    auto* rightCol = new QVBoxLayout;
    rightCol->setSpacing(10);

    m_extractPreview = new QLabel("Предпросмотр\nизображения");
    m_extractPreview->setAlignment(Qt::AlignCenter);
    m_extractPreview->setObjectName("imagePreview");
    m_extractPreview->setMinimumSize(340, 260);
    m_extractPreview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    rightCol->addWidget(m_extractPreview);

    m_extractInfoLabel = new QLabel("—");
    m_extractInfoLabel->setObjectName("infoBox");
    m_extractInfoLabel->setWordWrap(true);
    m_extractInfoLabel->setMinimumHeight(80);
    rightCol->addWidget(m_extractInfoLabel);

    root->addLayout(leftCol, 3);
    root->addLayout(rightCol, 2);

    connect(m_importStegBtn,   &QPushButton::clicked, this, &MainWindow::onImportStegImage);
    connect(m_extractBtn,      &QPushButton::clicked, this, &MainWindow::onExtractMessage);
    connect(m_saveExtractedBtn,&QPushButton::clicked, this, &MainWindow::onSaveExtractedToFile);
    connect(m_extractMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this](int idx){ m_extractLsbBits->setValue(idx == 0 ? 1 : idx == 1 ? 2 : 4); });
}

// ─── CONTAINER TAB ──────────────────────────────────────────
void MainWindow::setupContainerTab(QWidget* tab) {
    auto* root = new QVBoxLayout(tab);
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(12);

    // Top: image selector
    auto* topRow = new QHBoxLayout;
    m_selectContainerImgBtn = new QPushButton("Выбрать BMP-контейнер");
    m_selectContainerImgBtn->setObjectName("primaryBtn");
    topRow->addWidget(m_selectContainerImgBtn);
    topRow->addStretch();
    root->addLayout(topRow);

    m_containerPreview = new QLabel("Предпросмотр контейнера");
    m_containerPreview->setAlignment(Qt::AlignCenter);
    m_containerPreview->setObjectName("imagePreview");
    m_containerPreview->setFixedHeight(200);
    root->addWidget(m_containerPreview);

    m_containerInfoLabel = new QLabel("—");
    m_containerInfoLabel->setObjectName("infoBox");
    m_containerInfoLabel->setWordWrap(true);
    root->addWidget(m_containerInfoLabel);

    root->addWidget(hLine());

    // Archive row
    auto* arcRow = new QHBoxLayout;
    auto* arcLbl = new QLabel("ZIP/RAR архив:");
    m_archivePathEdit = new QLineEdit;
    m_archivePathEdit->setPlaceholderText("Путь к архиву…");
    m_archivePathEdit->setReadOnly(true);
    m_selectArchiveBtn = new QPushButton("Обзор…");
    m_selectArchiveBtn->setObjectName("primaryBtn");
    arcRow->addWidget(arcLbl);
    arcRow->addWidget(m_archivePathEdit, 1);
    arcRow->addWidget(m_selectArchiveBtn);
    root->addLayout(arcRow);

    // Action buttons
    auto* actRow = new QHBoxLayout;
    m_embedContainerBtn   = new QPushButton("📥  Внедрить архив в изображение");
    m_extractContainerBtn = new QPushButton("📤  Извлечь архив из изображения");
    m_embedContainerBtn->setObjectName("accentBtn");
    m_extractContainerBtn->setObjectName("successBtn");
    actRow->addWidget(m_embedContainerBtn);
    actRow->addWidget(m_extractContainerBtn);
    root->addLayout(actRow);
    root->addStretch();

    connect(m_selectContainerImgBtn, &QPushButton::clicked, this, &MainWindow::onSelectContainerImage);
    connect(m_selectArchiveBtn,      &QPushButton::clicked, this, &MainWindow::onSelectArchive);
    connect(m_embedContainerBtn,     &QPushButton::clicked, this, &MainWindow::onEmbedContainer);
    connect(m_extractContainerBtn,   &QPushButton::clicked, this, &MainWindow::onExtractContainer);
}

// ─── DATABASE TAB ────────────────────────────────────────────
void MainWindow::setupDatabaseTab(QWidget* tab) {
    auto* root = new QVBoxLayout(tab);
    root->setContentsMargins(16,16,16,16);
    root->setSpacing(10);

    auto* topRow = new QHBoxLayout;
    m_refreshDBBtn = new QPushButton("Обновить");
    m_deleteDBBtn  = new QPushButton("Удалить запись");
    m_refreshDBBtn->setObjectName("primaryBtn");
    m_deleteDBBtn->setObjectName("dangerBtn");
    topRow->addWidget(m_refreshDBBtn);
    topRow->addWidget(m_deleteDBBtn);
    topRow->addStretch();
    root->addLayout(topRow);

    m_dbTable = new QTableWidget;
    m_dbTable->setColumnCount(9);
    m_dbTable->setHorizontalHeaderLabels({"ID","Файл","Разрешение","Глубина","PSNR","MSE","Ёмкость (б)","Метод","Дата"});
    m_dbTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_dbTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_dbTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dbTable->setAlternatingRowColors(true);
    m_dbTable->setObjectName("dbTable");
    root->addWidget(m_dbTable, 3);

    m_dbDetailText = new QTextEdit;
    m_dbDetailText->setReadOnly(true);
    m_dbDetailText->setObjectName("msgEdit");
    m_dbDetailText->setMaximumHeight(120);
    m_dbDetailText->setPlaceholderText("Детали выбранной записи…");
    root->addWidget(m_dbDetailText, 1);

    connect(m_refreshDBBtn, &QPushButton::clicked, this, &MainWindow::onRefreshDB);
    connect(m_deleteDBBtn,  &QPushButton::clicked, this, &MainWindow::onDeleteDBRecord);
    connect(m_dbTable, &QTableWidget::cellClicked, this, &MainWindow::onDBRowSelected);

    onRefreshDB();
}

// ─── STYLES ──────────────────────────────────────────────────
void MainWindow::applyStyles() {
    setStyleSheet(R"(
        QMainWindow {
            background: #0f1117;
        }
        QWidget {
            background: #0f1117;
            color: #e2e8f0;
            font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif;
            font-size: 13px;
        }
        QTabWidget::pane {
            border: 1px solid #1e293b;
            background: #0f1117;
            border-radius: 6px;
        }
        QTabBar::tab {
            background: #1e293b;
            color: #94a3b8;
            padding: 10px 20px;
            margin-right: 2px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-size: 13px;
            font-weight: 500;
        }
        QTabBar::tab:selected {
            background: #0f172a;
            color: #38bdf8;
            border-bottom: 2px solid #38bdf8;
        }
        QTabBar::tab:hover:!selected {
            background: #1e293b;
            color: #cbd5e1;
        }
        QPushButton {
            border-radius: 5px;
            padding: 7px 16px;
            font-weight: 600;
            font-size: 12px;
        }
        QPushButton#primaryBtn {
            background: #1e293b;
            color: #94a3b8;
            border: 1px solid #334155;
        }
        QPushButton#primaryBtn:hover {
            background: #334155;
            color: #e2e8f0;
        }
        QPushButton#accentBtn {
            background: #0369a1;
            color: #f0f9ff;
            border: none;
        }
        QPushButton#accentBtn:hover {
            background: #0284c7;
        }
        QPushButton#successBtn {
            background: #065f46;
            color: #d1fae5;
            border: none;
        }
        QPushButton#successBtn:hover {
            background: #047857;
        }
        QPushButton#dangerBtn {
            background: #7f1d1d;
            color: #fee2e2;
            border: none;
        }
        QPushButton#dangerBtn:hover {
            background: #991b1b;
        }
        QTextEdit#msgEdit {
            background: #1e293b;
            color: #e2e8f0;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 8px;
            font-family: "Consolas","Courier New", monospace;
            font-size: 13px;
            selection-background-color: #0369a1;
        }
        QLabel#imagePreview {
            background: #1e293b;
            border: 1px dashed #334155;
            border-radius: 8px;
            color: #475569;
            font-size: 13px;
        }
        QLabel#infoBox {
            background: #162032;
            border: 1px solid #1e3a5f;
            border-radius: 6px;
            color: #7dd3fc;
            padding: 8px 12px;
            font-family: "Consolas","Courier New", monospace;
            font-size: 12px;
        }
        QLabel#metricsBox {
            background: #0c2312;
            border: 1px solid #14532d;
            border-radius: 6px;
            color: #86efac;
            padding: 8px 12px;
            font-family: "Consolas","Courier New", monospace;
            font-size: 12px;
        }
        QLabel#sectionLabel {
            color: #94a3b8;
            font-size: 11px;
            font-weight: 600;
            letter-spacing: 0.08em;
            text-transform: uppercase;
        }
        QFrame#hline {
            color: #1e293b;
        }
        QComboBox {
            background: #1e293b;
            color: #e2e8f0;
            border: 1px solid #334155;
            border-radius: 5px;
            padding: 5px 10px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox QAbstractItemView {
            background: #1e293b;
            color: #e2e8f0;
            selection-background-color: #0369a1;
        }
        QTableWidget#dbTable {
            background: #0f172a;
            color: #cbd5e1;
            border: 1px solid #1e293b;
            gridline-color: #1e293b;
            selection-background-color: #1e3a5f;
            alternate-background-color: #111827;
        }
        QTableWidget#dbTable QHeaderView::section {
            background: #1e293b;
            color: #94a3b8;
            padding: 6px;
            border: none;
            font-weight: 600;
            font-size: 11px;
        }
        QLineEdit {
            background: #1e293b;
            color: #e2e8f0;
            border: 1px solid #334155;
            border-radius: 5px;
            padding: 6px 10px;
        }
        QSpinBox {
            background: #1e293b;
            color: #e2e8f0;
            border: 1px solid #334155;
            border-radius: 5px;
            padding: 4px 8px;
        }
        QStatusBar {
            background: #0a0f1a;
            color: #475569;
            border-top: 1px solid #1e293b;
            font-size: 11px;
        }
        QScrollBar:vertical {
            background: #0f172a;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: #334155;
            border-radius: 5px;
            min-height: 20px;
        }
    )");
}

// ─── SLOTS ───────────────────────────────────────────────────
void MainWindow::onImportImage() {
    QString path = QFileDialog::getOpenFileName(this, "Открыть BMP изображение",
                                                 QDir::homePath(), "BMP Files (*.bmp)");
    if (path.isEmpty()) return;
    m_currentImagePath = path;
    updateImagePreview(path, m_embedPreview);

    StegMetrics m = Steganography::calcMetrics(path.toStdString(), path.toStdString());
    QString info = QString("Файл: %1\nРазрешение: %2 × %3\nГлубина: %4 бит\nЁмкость (LSB-1): %5 байт")
        .arg(QFileInfo(path).fileName())
        .arg(m.width).arg(m.height).arg(m.bitDepth).arg(m.maxCapacityBytes);
    m_embedInfoLabel->setText(info);
    m_statusBar->showMessage("Изображение загружено: " + path);
}

void MainWindow::onLoadMessageFromFile() {
    QString path = QFileDialog::getOpenFileName(this, "Загрузить TXT",
                                                 QDir::homePath(), "Text Files (*.txt);;All (*.*)");
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл.");
        return;
    }
    QTextStream in(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif

    m_messageEdit->setPlainText(in.readAll());
    m_statusBar->showMessage("Сообщение загружено из: " + path);
}

void MainWindow::onMethodChanged(int idx) {
    int bits = (idx == 0) ? 1 : (idx == 1) ? 2 : 4;
    m_lsbBits->setValue(bits);
}

void MainWindow::onEmbedMessage() {
    if (m_currentImagePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Сначала загрузите BMP изображение.");
        return;
    }
    QString msg = m_messageEdit->toPlainText();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите сообщение для внедрения.");
        return;
    }

    int bits = m_lsbBits->value();
    // Save to temp
    QString tmpPath = QDir::currentPath() + "/stego_temp_output.bmp";

    bool ok = Steganography::embedLSB(
        m_currentImagePath.toStdString(),
        tmpPath.toStdString(),
        msg.toStdString(),
        bits);

    if (!ok) {
        QMessageBox::critical(this, "Ошибка", "Не удалось внедрить сообщение.\nВозможно, изображение слишком мало.");
        return;
    }

    m_stegImagePath = tmpPath;
    updateImagePreview(tmpPath, m_embedPreview);

    // Metrics
    StegMetrics m = Steganography::calcMetrics(
        m_currentImagePath.toStdString(), tmpPath.toStdString(), msg.size());

    QString metricsStr = QString(
        "MSE:  %1\nPSNR: %2 дБ\nВнедрено: %3 байт\nМаксимум: %4 байт\nМетод: LSB-%5")
        .arg(fmtDouble(m.mse, 4))
        .arg(fmtDouble(m.psnr, 2))
        .arg(m.usedBytes)
        .arg(m.maxCapacityBytes)
        .arg(bits);
    m_metricsLabel->setText(metricsStr);
    m_statusBar->showMessage("Сообщение успешно внедрено.");

    // Save to DB
    ImageRecord rec;
    rec.filePath  = tmpPath;
    rec.fileName  = QFileInfo(tmpPath).fileName();
    rec.width     = m.width;
    rec.height    = m.height;
    rec.bitDepth  = m.bitDepth;
    rec.psnr      = m.psnr;
    rec.mse       = m.mse;
    rec.capacity  = m.maxCapacityBytes;
    rec.method    = "LSB-" + QString::number(bits);
    rec.notes     = "Внедрено " + QString::number(msg.size()) + " байт";
    m_db->insertRecord(rec);
}

void MainWindow::onSaveResult() {
    if (m_stegImagePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Сначала выполните внедрение.");
        return;
    }
    QString dst = QFileDialog::getSaveFileName(this, "Сохранить стегано-изображение",
                                                QDir::homePath() + "/stego_output.bmp",
                                                "BMP (*.bmp)");
    if (dst.isEmpty()) return;
    QFile::copy(m_stegImagePath, dst);
    m_statusBar->showMessage("Сохранено: " + dst);
    QMessageBox::information(this, "Готово", "Файл сохранён:\n" + dst);
}

// ─── EXTRACT SLOTS ───────────────────────────────────────────
void MainWindow::onImportStegImage() {
    QString path = QFileDialog::getOpenFileName(this, "Открыть стего-изображение BMP",
                                                 QDir::homePath(), "BMP Files (*.bmp)");
    if (path.isEmpty()) return;
    m_stegImagePath = path;
    updateImagePreview(path, m_extractPreview);

    StegMetrics m = Steganography::calcMetrics(path.toStdString(), path.toStdString());
    QString info = QString("Файл: %1\nРазрешение: %2 × %3\nГлубина: %4 бит")
        .arg(QFileInfo(path).fileName())
        .arg(m.width).arg(m.height).arg(m.bitDepth);
    m_extractInfoLabel->setText(info);
    m_statusBar->showMessage("Загружено стего-изображение: " + path);
}

void MainWindow::onExtractMessage() {
    if (m_stegImagePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Сначала загрузите стего-изображение.");
        return;
    }
    int bits = m_extractLsbBits->value();
    auto er = Steganography::extractLSB(m_stegImagePath.toStdString(), bits);
    if (!er.dataFound) {
        m_extractedText->setPlainText("[Скрытое сообщение не обнаружено]");
        m_statusBar->showMessage("Скрытых данных не найдено.");
    } else if (!er.integrityOk) {
        m_extractedText->setPlainText(QString::fromStdString(er.data));
        QMessageBox::warning(this, "Нарушение целостности",
            "Извлечённые данные не прошли проверку CRC32.\n"
            "Изображение было модифицировано после внедрения.");
        m_statusBar->showMessage("⚠ Целостность данных нарушена!");
    } else {
        m_extractedText->setPlainText(QString::fromStdString(er.data));
        m_statusBar->showMessage("Извлечение завершено. CRC32: OK.");
    }
}

void MainWindow::onSaveExtractedToFile() {
    QString text = m_extractedText->toPlainText();
    if (text.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Нет извлечённого текста для сохранения.");
        return;
    }
    QString dst = QFileDialog::getSaveFileName(this, "Сохранить сообщение",
                                                QDir::homePath() + "/extracted_message.txt",
                                                "Text Files (*.txt)");
    if (dst.isEmpty()) return;
    QFile f(dst);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&f);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        out.setEncoding(QStringConverter::Utf8);
#else
        out.setCodec("UTF-8");
#endif

        out << text;
    }
    m_statusBar->showMessage("Сообщение сохранено: " + dst);
    QMessageBox::information(this, "Готово", "Файл сохранён:\n" + dst);
}

// ─── CONTAINER SLOTS ─────────────────────────────────────────
void MainWindow::onSelectContainerImage() {
    QString path = QFileDialog::getOpenFileName(this, "Выбрать BMP контейнер",
                                                 QDir::homePath(), "BMP Files (*.bmp)");
    if (path.isEmpty()) return;
    m_containerImagePath = path;
    updateImagePreview(path, m_containerPreview);
    m_containerInfoLabel->setText("Контейнер: " + QFileInfo(path).fileName() +
                                   "\nРазмер: " + QString::number(QFileInfo(path).size()) + " байт");
    m_statusBar->showMessage("Контейнер выбран: " + path);
}

void MainWindow::onSelectArchive() {
    QString path = QFileDialog::getOpenFileName(this, "Выбрать архив",
                                                 QDir::homePath(),
                                                 "Archives (*.zip *.rar *.7z *.tar);;All (*.*)");
    if (path.isEmpty()) return;
    m_archivePath = path;
    m_archivePathEdit->setText(path);
    m_statusBar->showMessage("Архив выбран: " + path);
}

void MainWindow::onEmbedContainer() {
    if (m_containerImagePath.isEmpty() || m_archivePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите изображение-контейнер и архив.");
        return;
    }
    QString dst = QFileDialog::getSaveFileName(this, "Сохранить контейнер",
                                                QDir::homePath() + "/container_output.bmp",
                                                "BMP (*.bmp)");
    if (dst.isEmpty()) return;

    bool ok = Steganography::embedContainer(
        m_containerImagePath.toStdString(),
        m_archivePath.toStdString(),
        dst.toStdString());

    if (ok) {
        m_containerImagePath = dst;
        updateImagePreview(dst, m_containerPreview);

        StegMetrics m = Steganography::calcMetrics(dst.toStdString(), dst.toStdString());
        m_containerInfoLabel->setText(
            QString("Контейнер: %1\nРазмер: %2 байт\nАрхив: %3")
                .arg(QFileInfo(dst).fileName())
                .arg(QFileInfo(dst).size())
                .arg(QFileInfo(m_archivePath).fileName()));

        m_statusBar->showMessage("Архив внедрён в изображение: " + dst);
        QMessageBox::information(this, "Готово",
            "Архив успешно внедрён!\nФайл: " + dst +
            "\nРазмер: " + QString::number(QFileInfo(dst).size()) + " байт");

        // Record in DB
        ImageRecord rec;
        rec.filePath = dst;
        rec.fileName = QFileInfo(dst).fileName();
        rec.width    = m.width;
        rec.height   = m.height;
        rec.bitDepth = m.bitDepth;
        rec.method   = "Container";
        rec.notes    = "Архив: " + QFileInfo(m_archivePath).fileName();
        m_db->insertRecord(rec);
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось внедрить архив.");
    }
}

void MainWindow::onExtractContainer() {
    if (m_containerImagePath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите изображение-контейнер.");
        return;
    }
    QString dst = QFileDialog::getSaveFileName(this, "Сохранить извлечённый архив",
                                                QDir::homePath() + "/extracted_archive.zip",
                                                "Archives (*.zip *.rar);;All (*.*)");
    if (dst.isEmpty()) return;

    bool ok = Steganography::extractContainer(
        m_containerImagePath.toStdString(),
        dst.toStdString());

    if (ok) {
        m_statusBar->showMessage("Архив извлечён: " + dst);
        QMessageBox::information(this, "Готово", "Архив успешно извлечён:\n" + dst);
    } else {
        QMessageBox::critical(this, "Ошибка",
            "Скрытый архив не найден в данном изображении.");
    }
}

// ─── DB SLOTS ────────────────────────────────────────────────
void MainWindow::onRefreshDB() {
    auto records = m_db->getAllRecords();
    m_dbTable->setRowCount(static_cast<int>(records.size()));
    int row = 0;
    for (const auto& r : records) {
        m_dbTable->setItem(row, 0, new QTableWidgetItem(QString::number(r.id)));
        m_dbTable->setItem(row, 1, new QTableWidgetItem(r.fileName));
        m_dbTable->setItem(row, 2, new QTableWidgetItem(QString("%1×%2").arg(r.width).arg(r.height)));
        m_dbTable->setItem(row, 3, new QTableWidgetItem(QString::number(r.bitDepth) + " бит"));
        m_dbTable->setItem(row, 4, new QTableWidgetItem(fmtDouble(r.psnr)));
        m_dbTable->setItem(row, 5, new QTableWidgetItem(fmtDouble(r.mse,4)));
        m_dbTable->setItem(row, 6, new QTableWidgetItem(QString::number(r.capacity)));
        m_dbTable->setItem(row, 7, new QTableWidgetItem(r.method));
        m_dbTable->setItem(row, 8, new QTableWidgetItem(r.timestamp));
        ++row;
    }
}

void MainWindow::onDeleteDBRecord() {
    int row = m_dbTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Ошибка", "Выберите запись для удаления.");
        return;
    }
    auto* idItem = m_dbTable->item(row, 0);
    if (!idItem) return;
    int id = idItem->text().toInt();
    m_db->deleteRecord(id);
    onRefreshDB();
    m_statusBar->showMessage("Запись удалена.");
}

void MainWindow::onDBRowSelected(int row, int /*col*/) {
    auto* idItem = m_dbTable->item(row, 0);
    if (!idItem) return;
    int id = idItem->text().toInt();
    auto rec = m_db->getRecord(id);
    QString detail = QString(
        "ID: %1 | Файл: %2\nПуть: %3\n"
        "Разрешение: %4×%5 | Глубина: %6 бит\n"
        "PSNR: %7 дБ | MSE: %8\nЁмкость: %9 байт | Метод: %10\n"
        "Дата: %11\nЗаметки: %12")
        .arg(rec.id)
        .arg(rec.fileName)
        .arg(rec.filePath)
        .arg(rec.width).arg(rec.height).arg(rec.bitDepth)
        .arg(fmtDouble(rec.psnr)).arg(fmtDouble(rec.mse, 5))
        .arg(rec.capacity)
        .arg(rec.method)
        .arg(rec.timestamp)
        .arg(rec.notes);
    m_dbDetailText->setPlainText(detail);
}

// ─── HELPERS ─────────────────────────────────────────────────
void MainWindow::updateImagePreview(const QString& path, QLabel* label) {
    QPixmap pix(path);
    if (pix.isNull()) {
        label->setText("Не удалось загрузить изображение");
        return;
    }
    QSize sz = label->size().isEmpty() ? QSize(320,240) : label->size();
    label->setPixmap(pix.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}


