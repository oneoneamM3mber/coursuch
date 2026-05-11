#include <QtTest>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include "../include/steganography.h"

class TestSteganography : public QObject {
    Q_OBJECT

private:
    QTemporaryDir m_tempDir;

    // Generate a small solid-color BMP (24-bit, width×height)
    QString createTestBmp(int width, int height, uint8_t r = 128, uint8_t g = 128, uint8_t b = 128) {
        QString path = m_tempDir.path() + QString("/test_%1x%2.bmp").arg(width).arg(height);

        uint32_t rowSize = ((width * 3 + 3) / 4) * 4;
        uint32_t pixelDataSize = rowSize * height;
        uint32_t fileSize = 14 + 40 + pixelDataSize;
        uint32_t offBits = 14 + 40;

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) return {};

        // BMP file header
        uint16_t bfType = 0x4D42;
        f.write(reinterpret_cast<const char*>(&bfType), 2);
        f.write(reinterpret_cast<const char*>(&fileSize), 4);
        uint32_t reserved = 0;
        f.write(reinterpret_cast<const char*>(&reserved), 4);
        f.write(reinterpret_cast<const char*>(&offBits), 4);

        // BMP info header (40 bytes)
        uint32_t biSize = 40;
        int32_t biWidth = width;
        int32_t biHeight = height;
        uint16_t biPlanes = 1;
        uint16_t biBitCount = 24;
        uint32_t biCompression = 0;
        uint32_t biSizeImage = pixelDataSize;
        int32_t biXPelsPerMeter = 2835;
        int32_t biYPelsPerMeter = 2835;
        uint32_t biClrUsed = 0;
        uint32_t biClrImportant = 0;

        f.write(reinterpret_cast<const char*>(&biSize), 4);
        f.write(reinterpret_cast<const char*>(&biWidth), 4);
        f.write(reinterpret_cast<const char*>(&biHeight), 4);
        f.write(reinterpret_cast<const char*>(&biPlanes), 2);
        f.write(reinterpret_cast<const char*>(&biBitCount), 2);
        f.write(reinterpret_cast<const char*>(&biCompression), 4);
        f.write(reinterpret_cast<const char*>(&biSizeImage), 4);
        f.write(reinterpret_cast<const char*>(&biXPelsPerMeter), 4);
        f.write(reinterpret_cast<const char*>(&biYPelsPerMeter), 4);
        f.write(reinterpret_cast<const char*>(&biClrUsed), 4);
        f.write(reinterpret_cast<const char*>(&biClrImportant), 4);

        // Pixel data — bottom-up, BGR
        std::vector<uint8_t> row(rowSize, 0);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                row[x * 3 + 0] = b;
                row[x * 3 + 1] = g;
                row[x * 3 + 2] = r;
            }
            f.write(reinterpret_cast<const char*>(row.data()), row.size());
        }

        f.close();
        return path;
    }

private slots:
    void testLSBRoundtrip_data() {
        QTest::addColumn<int>("lsbBits");
        QTest::addColumn<QString>("message");

        QString longMsg(500, 'A');

        QTest::newRow("LSB-1 short")     << 1 << QString("Hello, StegoLab!");
        QTest::newRow("LSB-1 long")      << 1 << longMsg;
        QTest::newRow("LSB-2 short")     << 2 << QString("LSB-2 test message");
        QTest::newRow("LSB-2 long")      << 2 << longMsg;
        QTest::newRow("LSB-4 short")     << 4 << QString("LSB-4 works!");
        QTest::newRow("LSB-4 long")      << 4 << longMsg;
        QTest::newRow("LSB-1 empty msg") << 1 << QString("");
        QTest::newRow("LSB-1 cyrillic")  << 1 << QString("Привет, Стеголаб!");
    }

    void testLSBRoundtrip() {
        QFETCH(int, lsbBits);
        QFETCH(QString, message);

        QString srcPath = createTestBmp(100, 100);
        QVERIFY(!srcPath.isEmpty());

        QString dstPath = m_tempDir.path() + "/output.bmp";

        bool ok = Steganography::embedLSB(
            srcPath.toStdString(), dstPath.toStdString(),
            message.toStdString(), lsbBits);
        QVERIFY2(ok, "embedLSB failed");

        if (message.isEmpty()) {
            // Embedding empty message may succeed or fail — skip extract
            return;
        }

        auto result = Steganography::extractLSB(dstPath.toStdString(), lsbBits);
        QVERIFY(result.dataFound);
        QVERIFY(result.integrityOk);
        QCOMPARE(QString::fromStdString(result.data), message);
    }

    void testCRCIntegrityFailure() {
        QString srcPath = createTestBmp(50, 50);
        QVERIFY(!srcPath.isEmpty());

        QString dstPath = m_tempDir.path() + "/stego.bmp";

        const QString msg = "Integrity check message";
        bool ok = Steganography::embedLSB(
            srcPath.toStdString(), dstPath.toStdString(),
            msg.toStdString(), 1);
        QVERIFY(ok);

        // Tamper: flip one bit in the file
        QFile f(dstPath);
        QVERIFY(f.open(QIODevice::ReadWrite));
        QByteArray data = f.readAll();
        QVERIFY(data.size() > 100);
        data[100] ^= 0x01; // flip one bit somewhere in pixel data
        f.seek(0);
        f.write(data);
        f.close();

        auto result = Steganography::extractLSB(dstPath.toStdString(), 1);
        QVERIFY(result.dataFound);
        QVERIFY(!result.integrityOk);
    }

    void testContainerRoundtrip() {
        QString imgPath = createTestBmp(10, 10);
        QVERIFY(!imgPath.isEmpty());

        // Create a small "archive" (text file)
        QString arcPath = m_tempDir.path() + "/test_archive.zip";
        QFile arc(arcPath);
        QVERIFY(arc.open(QIODevice::WriteOnly));
        arc.write("FAKE_ZIP_CONTENT_12345");
        arc.close();

        QString dstPath = m_tempDir.path() + "/container.bmp";
        bool ok = Steganography::embedContainer(
            imgPath.toStdString(), arcPath.toStdString(),
            dstPath.toStdString());
        QVERIFY2(ok, "embedContainer failed");

        QString outPath = m_tempDir.path() + "/extracted.zip";
        ok = Steganography::extractContainer(dstPath.toStdString(), outPath.toStdString());
        QVERIFY2(ok, "extractContainer failed");

        QFile out(outPath);
        QVERIFY(out.open(QIODevice::ReadOnly));
        QByteArray extracted = out.readAll();
        QCOMPARE(extracted, QByteArray("FAKE_ZIP_CONTENT_12345"));
    }

    void testGetCapacity() {
        QString path = createTestBmp(100, 100); // 100×100×3 = 30000 bytes
        QVERIFY(!path.isEmpty());

        // LSB-1: (30000 * 1) / 8 - 8 = 3742
        long cap1 = Steganography::getCapacity(path.toStdString(), 1);
        QCOMPARE(cap1, 3742L);

        // LSB-2: (30000 * 2) / 8 - 8 = 7492
        long cap2 = Steganography::getCapacity(path.toStdString(), 2);
        QCOMPARE(cap2, 7492L);

        // LSB-4: (30000 * 4) / 8 - 8 = 14992
        long cap4 = Steganography::getCapacity(path.toStdString(), 4);
        QCOMPARE(cap4, 14992L);
    }

    void testCalcMetrics() {
        QString origPath = createTestBmp(10, 10, 128, 128, 128);
        QVERIFY(!origPath.isEmpty());

        QString stegPath = m_tempDir.path() + "/metrics_stego.bmp";

        // Embed a small message
        bool ok = Steganography::embedLSB(
            origPath.toStdString(), stegPath.toStdString(),
            "MetricsTest", 1);
        QVERIFY(ok);

        auto m = Steganography::calcMetrics(
            origPath.toStdString(), stegPath.toStdString(), 11);

        QCOMPARE(m.usedBytes, 11L);
        QCOMPARE(m.width, 10);
        QCOMPARE(m.height, 10);
        QCOMPARE(m.bitDepth, 24);
        QVERIFY(m.mse > 0.0);
        QVERIFY(m.psnr > 40.0); // LSB-1 should be high quality
        QVERIFY(m.maxCapacityBytes > 0);
    }
};

QTEST_MAIN(TestSteganography)
#include "test_steganography.moc"
