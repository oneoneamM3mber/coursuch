#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QUuid>

Database::Database(const QString& dbPath)
    : m_dbPath(dbPath)
    , m_connName("stegodb_" + QUuid::createUuid().toString())
{}

Database::~Database() { close(); }

bool Database::open() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connName);
    db.setDatabaseName(m_dbPath);
    if (!db.open()) {
        qWarning() << "DB open error:" << db.lastError().text();
        return false;
    }
    m_open = true;
    return createTables();
}

void Database::close() {
    if (m_open) {
        QSqlDatabase::database(m_connName).close();
        QSqlDatabase::removeDatabase(m_connName);
        m_open = false;
    }
}

bool Database::isOpen() const { return m_open; }

bool Database::createTables() {
    QSqlQuery q(QSqlDatabase::database(m_connName));
    return q.exec(R"(
        CREATE TABLE IF NOT EXISTS images (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            filepath  TEXT NOT NULL,
            filename  TEXT NOT NULL,
            width     INTEGER DEFAULT 0,
            height    INTEGER DEFAULT 0,
            bitdepth  INTEGER DEFAULT 24,
            psnr      REAL DEFAULT 0,
            mse       REAL DEFAULT 0,
            capacity  INTEGER DEFAULT 0,
            method    TEXT DEFAULT '',
            ts        TEXT,
            notes     TEXT DEFAULT ''
        )
    )");
}

bool Database::insertRecord(const ImageRecord& rec) {
    QSqlQuery q(QSqlDatabase::database(m_connName));
    q.prepare(R"(
        INSERT INTO images (filepath,filename,width,height,bitdepth,psnr,mse,capacity,method,ts,notes)
        VALUES (:fp,:fn,:w,:h,:bd,:psnr,:mse,:cap,:method,:ts,:notes)
    )");
    q.bindValue(":fp",     rec.filePath);
    q.bindValue(":fn",     rec.fileName);
    q.bindValue(":w",      rec.width);
    q.bindValue(":h",      rec.height);
    q.bindValue(":bd",     rec.bitDepth);
    q.bindValue(":psnr",   rec.psnr);
    q.bindValue(":mse",    rec.mse);
    q.bindValue(":cap",    (qlonglong)rec.capacity);
    q.bindValue(":method", rec.method);
    q.bindValue(":ts",     QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    q.bindValue(":notes",  rec.notes);
    if (!q.exec()) { qWarning() << "insert error:" << q.lastError().text(); return false; }
    return true;
}

bool Database::deleteRecord(int id) {
    QSqlQuery q(QSqlDatabase::database(m_connName));
    q.prepare("DELETE FROM images WHERE id=:id");
    q.bindValue(":id", id);
    return q.exec();
}

bool Database::updateNotes(int id, const QString& notes) {
    QSqlQuery q(QSqlDatabase::database(m_connName));
    q.prepare("UPDATE images SET notes=:n WHERE id=:id");
    q.bindValue(":n", notes);
    q.bindValue(":id", id);
    return q.exec();
}

QVector<ImageRecord> Database::getAllRecords() {
    QVector<ImageRecord> result;
    QSqlQuery q(QSqlDatabase::database(m_connName));
    if (!q.exec("SELECT id,filepath,filename,width,height,bitdepth,psnr,mse,capacity,method,ts,notes FROM images ORDER BY id DESC")) {
        qWarning() << "getAllRecords error:" << q.lastError().text();
        return result;
    }
    while (q.next()) {
        ImageRecord r;
        r.id        = q.value(0).toInt();
        r.filePath  = q.value(1).toString();
        r.fileName  = q.value(2).toString();
        r.width     = q.value(3).toInt();
        r.height    = q.value(4).toInt();
        r.bitDepth  = q.value(5).toInt();
        r.psnr      = q.value(6).toDouble();
        r.mse       = q.value(7).toDouble();
        r.capacity  = q.value(8).toLongLong();
        r.method    = q.value(9).toString();
        r.timestamp = q.value(10).toString();
        r.notes     = q.value(11).toString();
        result.append(r);
    }
    return result;
}

ImageRecord Database::getRecord(int id) {
    ImageRecord r;
    QSqlQuery q(QSqlDatabase::database(m_connName));
    q.prepare("SELECT id,filepath,filename,width,height,bitdepth,psnr,mse,capacity,method,ts,notes FROM images WHERE id=:id");
    q.bindValue(":id", id);
    if (q.exec() && q.next()) {
        r.id        = q.value(0).toInt();
        r.filePath  = q.value(1).toString();
        r.fileName  = q.value(2).toString();
        r.width     = q.value(3).toInt();
        r.height    = q.value(4).toInt();
        r.bitDepth  = q.value(5).toInt();
        r.psnr      = q.value(6).toDouble();
        r.mse       = q.value(7).toDouble();
        r.capacity  = q.value(8).toLongLong();
        r.method    = q.value(9).toString();
        r.timestamp = q.value(10).toString();
        r.notes     = q.value(11).toString();
    }
    return r;
}
