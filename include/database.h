#pragma once
#include <QString>
#include <QVector>

struct ImageRecord {
    int     id        = 0;
    QString filePath;
    QString fileName;
    int     width     = 0;
    int     height    = 0;
    int     bitDepth  = 24;
    double  psnr      = 0.0;
    double  mse       = 0.0;
    long    capacity  = 0;
    QString method;
    QString timestamp;
    QString notes;
};

class Database {
public:
    explicit Database(const QString& dbPath);
    ~Database();

    bool open();
    void close();
    bool isOpen() const;

    bool insertRecord(const ImageRecord& rec);
    bool deleteRecord(int id);
    QVector<ImageRecord> getAllRecords();
    ImageRecord getRecord(int id);
    bool updateNotes(int id, const QString& notes);

private:
    QString m_dbPath;
    QString m_connName;
    bool    m_open = false;
    bool    createTables();
};
