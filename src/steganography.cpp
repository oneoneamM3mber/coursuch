#include "steganography.h"
#include <QFile>
#include <cmath>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <array>

// ============================================================
// CRC32 — табличный (полином 0xEDB88320)
// ============================================================
static const std::array<uint32_t, 256> CRC32_TABLE = []() {
    std::array<uint32_t, 256> t{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ (0xEDB88320 & ~((crc & 1) - 1));
        t[i] = crc;
    }
    return t;
}();

uint32_t Steganography::crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i)
        crc = CRC32_TABLE[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFF;
}

// ============================================================
// BMP I/O
// ============================================================
bool Steganography::readBMP(const std::string& path,
                             BMPFileHeader& fh, BMPInfoHeader& ih,
                             std::vector<uint8_t>& pixels)
{
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::ReadOnly)) return false;
    if (f.read(reinterpret_cast<char*>(&fh), sizeof(fh)) != sizeof(fh)) return false;
    if (f.read(reinterpret_cast<char*>(&ih), sizeof(ih)) != sizeof(ih)) return false;
    if (fh.bfType != 0x4D42) return false; // not BMP
    if (!f.seek(fh.bfOffBits)) return false;
    size_t pixSize = fh.bfSize - fh.bfOffBits;
    pixels.resize(pixSize);
    if (f.read(reinterpret_cast<char*>(pixels.data()), pixSize) != pixSize) return false;
    return true;
}

bool Steganography::writeBMP(const std::string& path,
                              const BMPFileHeader& fh, const BMPInfoHeader& ih,
                              const std::vector<uint8_t>& pixels)
{
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::WriteOnly)) return false;
    if (f.write(reinterpret_cast<const char*>(&fh), sizeof(fh)) != sizeof(fh)) return false;
    if (f.write(reinterpret_cast<const char*>(&ih), sizeof(ih)) != sizeof(ih)) return false;
    // Write any extra header bytes between BMPInfoHeader and pixel data
    size_t extraBytes = fh.bfOffBits - sizeof(fh) - sizeof(ih);
    if (extraBytes > 0) {
        QByteArray extra(extraBytes, '\0');
        if (f.write(extra) != extraBytes) return false;
    }
    if (f.write(reinterpret_cast<const char*>(pixels.data()), pixels.size()) != pixels.size()) return false;
    return true;
}

std::vector<uint8_t> Steganography::readFile(const std::string& path)
{
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::ReadOnly)) return {};
    QByteArray ba = f.readAll();
    return std::vector<uint8_t>(ba.begin(), ba.end());
}

bool Steganography::writeFile(const std::string& path, const std::vector<uint8_t>& data)
{
    QFile f(QString::fromStdString(path));
    if (!f.open(QIODevice::WriteOnly)) return false;
    qint64 written = f.write(reinterpret_cast<const char*>(data.data()), data.size());
    return written == static_cast<qint64>(data.size());
}

// ============================================================
// LSB EMBED
// ============================================================
bool Steganography::embedLSB(const std::string& srcPath,
                               const std::string& dstPath,
                               const std::string& message,
                               int lsbBits)
{
    if (lsbBits < 1 || lsbBits > 4) return false;

    BMPFileHeader fh;
    BMPInfoHeader ih;
    std::vector<uint8_t> pixels;
    if (!readBMP(srcPath, fh, ih, pixels)) return false;

    // Encode: 4 bytes length + 4 bytes CRC32 + message bytes
    uint32_t msgLen = static_cast<uint32_t>(message.size());
    std::vector<uint8_t> lenBytes(4);
    lenBytes[0] = (msgLen >> 24) & 0xFF;
    lenBytes[1] = (msgLen >> 16) & 0xFF;
    lenBytes[2] = (msgLen >> 8)  & 0xFF;
    lenBytes[3] = (msgLen)       & 0xFF;

    // CRC32 over (length bytes + message)
    std::vector<uint8_t> crcInput = lenBytes;
    crcInput.insert(crcInput.end(), message.begin(), message.end());
    uint32_t crcVal = crc32(crcInput.data(), crcInput.size());

    std::vector<uint8_t> payload(8 + msgLen);
    std::memcpy(&payload[0], lenBytes.data(), 4);
    payload[4] = (crcVal >> 24) & 0xFF;
    payload[5] = (crcVal >> 16) & 0xFF;
    payload[6] = (crcVal >> 8)  & 0xFF;
    payload[7] = (crcVal)       & 0xFF;
    for (size_t i = 0; i < msgLen; ++i)
        payload[8 + i] = static_cast<uint8_t>(message[i]);

    // Check capacity
    size_t totalBits = pixels.size() * lsbBits;
    size_t neededBits = payload.size() * 8;
    if (neededBits > totalBits) return false;

    uint8_t mask = static_cast<uint8_t>((1 << lsbBits) - 1);
    size_t pixIdx = 0;
    int bitBuf = 0;
    int bitsLeft = 0;

    for (size_t byteIdx = 0; byteIdx < payload.size(); ++byteIdx) {
        bitBuf = (bitBuf << 8) | payload[byteIdx];
        bitsLeft += 8;
        while (bitsLeft >= lsbBits && pixIdx < pixels.size()) {
            bitsLeft -= lsbBits;
            uint8_t chunk = static_cast<uint8_t>((bitBuf >> bitsLeft) & mask);
            pixels[pixIdx] = (pixels[pixIdx] & ~mask) | chunk;
            ++pixIdx;
        }
    }
    // flush remaining bits
    if (bitsLeft > 0 && pixIdx < pixels.size()) {
        int pad = lsbBits - bitsLeft;
        uint8_t chunk = static_cast<uint8_t>((bitBuf << pad) & mask);
        pixels[pixIdx] = (pixels[pixIdx] & ~mask) | chunk;
    }

    return writeBMP(dstPath, fh, ih, pixels);
}

// ============================================================
// LSB EXTRACT
// ============================================================
ExtractResult Steganography::extractLSB(const std::string& srcPath, int lsbBits)
{
    ExtractResult result;
    if (lsbBits < 1 || lsbBits > 4) { result.dataFound = false; return result; }

    BMPFileHeader fh;
    BMPInfoHeader ih;
    std::vector<uint8_t> pixels;
    if (!readBMP(srcPath, fh, ih, pixels)) { result.dataFound = false; return result; }

    uint8_t mask = static_cast<uint8_t>((1 << lsbBits) - 1);

    auto extractBytes = [&](size_t count, size_t& pixIdx) -> std::vector<uint8_t> {
        std::vector<uint8_t> r;
        int bitBuf = 0;
        int bitsAccum = 0;
        while (r.size() < count && pixIdx < pixels.size()) {
            bitBuf = (bitBuf << lsbBits) | (pixels[pixIdx] & mask);
            bitsAccum += lsbBits;
            ++pixIdx;
            if (bitsAccum >= 8) {
                bitsAccum -= 8;
                r.push_back(static_cast<uint8_t>((bitBuf >> bitsAccum) & 0xFF));
            }
        }
        return r;
    };

    size_t pixIdx = 0;
    auto lenBytes = extractBytes(4, pixIdx);
    if (lenBytes.size() < 4) { result.dataFound = false; return result; }

    uint32_t msgLen = ((uint32_t)lenBytes[0] << 24) |
                      ((uint32_t)lenBytes[1] << 16) |
                      ((uint32_t)lenBytes[2] << 8)  |
                      ((uint32_t)lenBytes[3]);

    // Sanity check: max possible message bytes given LSB bits
    uint32_t maxPossible = static_cast<uint32_t>((pixels.size() * lsbBits) / 8);
    if (msgLen > maxPossible - 8) { result.dataFound = false; return result; }

    // Extract CRC (4 bytes)
    auto crcBytes = extractBytes(4, pixIdx);
    if (crcBytes.size() < 4) { result.dataFound = false; return result; }
    uint32_t storedCrc = ((uint32_t)crcBytes[0] << 24) |
                         ((uint32_t)crcBytes[1] << 16) |
                         ((uint32_t)crcBytes[2] << 8)  |
                         ((uint32_t)crcBytes[3]);

    // Extract message data
    auto msgBytes = extractBytes(msgLen, pixIdx);
    if (msgBytes.size() < msgLen) { result.dataFound = false; return result; }
    result.data = std::string(msgBytes.begin(), msgBytes.end());

    // Verify CRC32
    std::vector<uint8_t> crcInput = lenBytes;
    crcInput.insert(crcInput.end(), msgBytes.begin(), msgBytes.end());
    uint32_t computedCrc = crc32(crcInput.data(), crcInput.size());
    result.integrityOk = (computedCrc == storedCrc);

    return result;
}

// ============================================================
// CONTAINER EMBED (append ZIP to BMP)
// ============================================================
bool Steganography::embedContainer(const std::string& imagePath,
                                    const std::string& archivePath,
                                    const std::string& dstPath)
{
    auto imgData = readFile(imagePath);
    auto arcData = readFile(archivePath);
    if (imgData.empty() || arcData.empty()) return false;

    // Check if image already has embedded container data; if so, strip it
    imgData = stripContainer(imgData);

    // Marker: "STEG_CONTAINER" + 8-byte archive size
    const std::string marker = "STEG_CONTAINER";
    uint64_t arcSize = arcData.size();

    std::vector<uint8_t> combined;
    combined.insert(combined.end(), imgData.begin(), imgData.end());
    combined.insert(combined.end(), marker.begin(), marker.end());
    // Write size as little-endian 8 bytes
    for (int i = 0; i < 8; ++i)
        combined.push_back(static_cast<uint8_t>((arcSize >> (8 * i)) & 0xFF));
    combined.insert(combined.end(), arcData.begin(), arcData.end());

    return writeFile(dstPath, combined);
}

// ============================================================
// CONTAINER EXTRACT
// ============================================================
std::vector<uint8_t> Steganography::extractContainerData(const std::vector<uint8_t>& data)
{
    const std::string marker = "STEG_CONTAINER";
    if (data.size() < marker.size() + 8) return {};

    size_t markerPos = std::string::npos;
    // Search backwards — validate each candidate
    size_t start = (data.size() >= marker.size() + 8)
                       ? data.size() - marker.size() - 8
                       : 0;
    for (size_t i = start; i != (size_t)-1; --i) {
        if (std::memcmp(data.data() + i, marker.data(), marker.size()) == 0) {
            // Validate: next 8 bytes encode the size, and remaining data must match
            size_t sizePos = i + marker.size();
            if (sizePos + 8 > data.size()) continue;

            uint64_t arcSize = 0;
            for (int b = 0; b < 8; ++b)
                arcSize |= (uint64_t)data[sizePos + b] << (8 * b);

            if (sizePos + 8 + arcSize == data.size()) {
                markerPos = i;
                break;
            }
        }
    }
    if (markerPos == std::string::npos) return {};

    size_t sizePos = markerPos + marker.size();
    uint64_t arcSize = 0;
    for (int i = 0; i < 8; ++i)
        arcSize |= (uint64_t)data[sizePos + i] << (8 * i);

    size_t arcStart = sizePos + 8;
    return std::vector<uint8_t>(data.begin() + arcStart,
                                 data.begin() + arcStart + arcSize);
}

std::vector<uint8_t> Steganography::stripContainer(const std::vector<uint8_t>& data)
{
    const std::string marker = "STEG_CONTAINER";
    if (data.size() < marker.size() + 8) return data;

    size_t start = data.size() - marker.size() - 8;
    for (size_t i = start; i != (size_t)-1; --i) {
        if (std::memcmp(data.data() + i, marker.data(), marker.size()) == 0) {
            size_t sizePos = i + marker.size();
            if (sizePos + 8 > data.size()) continue;
            uint64_t arcSize = 0;
            for (int b = 0; b < 8; ++b)
                arcSize |= (uint64_t)data[sizePos + b] << (8 * b);
            if (sizePos + 8 + arcSize == data.size()) {
                // Return image data without the appended container
                return std::vector<uint8_t>(data.begin(), data.begin() + i);
            }
        }
    }
    return data;
}

bool Steganography::extractContainer(const std::string& imagePath,
                                      const std::string& outArchivePath)
{
    auto data = readFile(imagePath);
    if (data.empty()) return false;

    auto arcData = extractContainerData(data);
    if (arcData.empty()) return false;

    return writeFile(outArchivePath, arcData);
}

// ============================================================
// METRICS
// ============================================================
StegMetrics Steganography::calcMetrics(const std::string& origPath,
                                        const std::string& stegPath,
                                        long usedBytes)
{
    StegMetrics m{};
    m.filePath = stegPath;

    BMPFileHeader fh1, fh2;
    BMPInfoHeader ih1, ih2;
    std::vector<uint8_t> p1, p2;

    if (!readBMP(origPath, fh1, ih1, p1)) return m;
    if (!readBMP(stegPath, fh2, ih2, p2)) return m;

    m.width    = ih2.biWidth;
    m.height   = std::abs(ih2.biHeight);
    m.bitDepth = ih2.biBitCount;
    m.usedBytes = usedBytes;

    size_t n = std::min(p1.size(), p2.size());
    if (n == 0) { m.mse = 0.0; m.psnr = 100.0; return m; }

    double sumSq = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double diff = (double)p1[i] - (double)p2[i];
        sumSq += diff * diff;
    }
    m.mse = sumSq / (double)n;
    if (m.mse > 0.0)
        m.psnr = 10.0 * std::log10(255.0 * 255.0 / m.mse);
    else
        m.psnr = 100.0;

    m.maxCapacityBytes = getCapacity(stegPath, 1);
    return m;
}

long Steganography::getCapacity(const std::string& imagePath, int lsbBits)
{
    BMPFileHeader fh;
    BMPInfoHeader ih;
    std::vector<uint8_t> pixels;
    if (!readBMP(imagePath, fh, ih, pixels)) return 0;
    long cap = static_cast<long>((pixels.size() * lsbBits) / 8) - 8;
    return cap < 0 ? 0 : cap;
}
