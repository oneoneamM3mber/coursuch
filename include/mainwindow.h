#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QSpinBox>
#include <QGroupBox>
#include <QStatusBar>
#include <QProgressBar>
#include <QString>
#include <memory>
#include "database.h"
#include "steganography.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // Embed tab
    void onImportImage();
    void onLoadMessageFromFile();
    void onEmbedMessage();
    void onSaveResult();
    void onMethodChanged(int idx);

    // Extract tab
    void onImportStegImage();
    void onExtractMessage();
    void onSaveExtractedToFile();

    // Container tab
    void onSelectContainerImage();
    void onSelectArchive();
    void onEmbedContainer();
    void onExtractContainer();

    // DB tab
    void onRefreshDB();
    void onDeleteDBRecord();
    void onDBRowSelected(int row, int col);

    // Common
    void updateImagePreview(const QString& path, QLabel* label);

private:
    void setupUI();
    void setupEmbedTab(QWidget* tab);
    void setupExtractTab(QWidget* tab);
    void setupContainerTab(QWidget* tab);
    void setupDatabaseTab(QWidget* tab);
    void applyStyles();

    // State
    QString     m_currentImagePath;
    QString     m_stegImagePath;
    QString     m_containerImagePath;
    QString     m_archivePath;
    std::unique_ptr<Database> m_db;

    // Embed tab widgets
    QLabel*      m_embedPreview;
    QLabel*      m_embedInfoLabel;
    QTextEdit*   m_messageEdit;
    QComboBox*   m_methodCombo;
    QSpinBox*    m_lsbBits;
    QPushButton* m_importBtn;
    QPushButton* m_loadMsgBtn;
    QPushButton* m_embedBtn;
    QPushButton* m_saveBtn;
    QLabel*      m_metricsLabel;

    // Extract tab widgets
    QLabel*      m_extractPreview;
    QLabel*      m_extractInfoLabel;
    QTextEdit*   m_extractedText;
    QComboBox*   m_extractMethodCombo;
    QSpinBox*    m_extractLsbBits;
    QPushButton* m_importStegBtn;
    QPushButton* m_extractBtn;
    QPushButton* m_saveExtractedBtn;

    // Container tab
    QLabel*      m_containerPreview;
    QLabel*      m_containerInfoLabel;
    QLineEdit*   m_archivePathEdit;
    QPushButton* m_selectContainerImgBtn;
    QPushButton* m_selectArchiveBtn;
    QPushButton* m_embedContainerBtn;
    QPushButton* m_extractContainerBtn;

    // DB tab
    QTableWidget* m_dbTable;
    QPushButton*  m_refreshDBBtn;
    QPushButton*  m_deleteDBBtn;
    QTextEdit*    m_dbDetailText;

    QTabWidget*  m_tabs;
    QStatusBar*  m_statusBar;
};
