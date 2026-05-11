#pragma once
#include <string>
#include <vector>
#include <cstdint>

// BMP File Structures
#pragma pack(push, 1)
struct BMPFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BMPInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

struct ExtractResult {
    std::string data;
    bool        integrityOk = true;
    bool        dataFound   = true;
};

struct StegMetrics {
    double mse;
    double psnr;
    long   maxCapacityBytes;
    long   usedBytes;
    int    width;
    int    height;
    int    bitDepth;
    std::string filePath;
};

class Steganography {
public:
    // LSB embed: embeds message into BMP image using N LSBs (1-4)
    static bool embedLSB(const std::string& srcPath,
                         const std::string& dstPath,
                         const std::string& message,
                         int lsbBits = 1);

    // LSB extract: extracts hidden message from BMP image
    static ExtractResult extractLSB(const std::string& srcPath, int lsbBits = 1);

    // Container method: appends archive to image file
    static bool embedContainer(const std::string& imagePath,
                               const std::string& archivePath,
                               const std::string& dstPath);

    // Container extract: extracts appended archive from image
    static bool extractContainer(const std::string& imagePath,
                                 const std::string& outArchivePath);

    // Metrics calculation
    static StegMetrics calcMetrics(const std::string& origPath,
                                   const std::string& stegPath,
                                   long usedBytes = 0);

    // Get max capacity for image
    static long getCapacity(const std::string& imagePath, int lsbBits = 1);

    // Helper: extract container data from raw bytes (used by embed & extract)
    static std::vector<uint8_t> extractContainerData(const std::vector<uint8_t>& data);

    // Helper: strip any appended container from image bytes, return clean BMP
    static std::vector<uint8_t> stripContainer(const std::vector<uint8_t>& data);

    // Helper: read file to vector
    static std::vector<uint8_t> readFile(const std::string& path);

    // Helper: write vector to file
    static bool writeFile(const std::string& path, const std::vector<uint8_t>& data);

private:
    static bool readBMP(const std::string& path,
                        BMPFileHeader& fh, BMPInfoHeader& ih,
                        std::vector<uint8_t>& pixels);
    static bool writeBMP(const std::string& path,
                         const BMPFileHeader& fh, const BMPInfoHeader& ih,
                         const std::vector<uint8_t>& pixels);
    static uint32_t crc32(const uint8_t* data, size_t len);
};
