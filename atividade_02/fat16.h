#ifndef FAT16_H
#define FAT16_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>

// O pragma pack é usado para garantir que as estruturas sejam alinhadas byte a byte
// Isso é crucial para ler corretamente os dados binários do sistema de arquivos FAT16
// Estrutura do Boot Sector do FAT16
#pragma pack(push, 1)
struct BootSector {
    uint8_t  jmpBoot[3];           // Jump instruction
    char     OEMName[8];           // OEM Name
    uint16_t bytesPerSector;       // Bytes por setor (geralmente 512)
    uint8_t  sectorsPerCluster;    // Setores por cluster
    uint16_t reservedSectors;      // Setores reservados
    uint8_t  numFATs;              // Número de FATs (geralmente 2)
    uint16_t rootEntryCount;       // Número de entradas no diretório raiz
    uint16_t totalSectors16;       // Total de setores (se < 65536)
    uint8_t  mediaType;            // Tipo de mídia
    uint16_t sectorsPerFAT;        // Setores por FAT
    uint16_t sectorsPerTrack;      // Setores por trilha
    uint16_t numHeads;             // Número de cabeças
    uint32_t hiddenSectors;        // Setores ocultos
    uint32_t totalSectors32;       // Total de setores (se >= 65536)
    uint8_t  driveNumber;          // Número do drive
    uint8_t  reserved1;            // Reservado
    uint8_t  bootSignature;        // Assinatura de boot (0x29)
    uint32_t volumeID;             // ID do volume
    char     volumeLabel[11];      // Label do volume
    char     fsType[8];            // Tipo do sistema de arquivos
};
#pragma pack(pop)

// Estrutura de entrada do diretório
#pragma pack(push, 1)
struct DirectoryEntry {
    char     fileName[8];          // Nome do arquivo (8 caracteres)
    char     extension[3];         // Extensão (3 caracteres)
    uint8_t  attributes;           // Atributos do arquivo
    uint8_t  reserved;             // Reservado
    uint8_t  creationTimeTenth;    // Criação em décimos de segundo
    uint16_t creationTime;         // Hora de criação
    uint16_t creationDate;         // Data de criação
    uint16_t lastAccessDate;       // Data do último acesso
    uint16_t firstClusterHigh;     // Parte alta do primeiro cluster (FAT32)
    uint16_t lastModifiedTime;     // Hora da última modificação
    uint16_t lastModifiedDate;     // Data da última modificação
    uint16_t firstClusterLow;      // Parte baixa do primeiro cluster
    uint32_t fileSize;             // Tamanho do arquivo em bytes
};
#pragma pack(pop)

// Atributos de arquivo
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  0x0F

// Valores especiais da FAT
#define FAT_FREE_CLUSTER    0x0000
#define FAT_BAD_CLUSTER     0xFFF7
#define FAT_EOF_MARKER      0xFFF8  // Qualquer valor >= 0xFFF8 indica EOF

// Classe para gerenciar o sistema de arquivos FAT16
class FAT16Manager {
private:
    std::string imageFileName;
    std::fstream imageFile;
    BootSector bootSector;
    std::vector<uint16_t> fat;
    std::vector<DirectoryEntry> rootDirectory;
    
    uint32_t fatStartSector;
    uint32_t rootDirStartSector;
    uint32_t dataStartSector;
    uint32_t rootDirSectors;
    
    bool loadBootSector();
    bool loadFAT();
    bool loadRootDirectory();
    void saveFAT();
    void saveRootDirectory();
    
    uint32_t getClusterOffset(uint16_t cluster);
    std::string getFileName(const DirectoryEntry& entry);
    void setFileName(DirectoryEntry& entry, const std::string& name);
    std::string formatDate(uint16_t date);
    std::string formatTime(uint16_t time);
    
    uint16_t findFreeCluster();
    DirectoryEntry* findFileEntry(const std::string& fileName);
    int findFreeDirectoryEntry();
    
public:
    FAT16Manager(const std::string& imagePath);
    ~FAT16Manager();
    
    bool initialize();
    void listFiles();
    void showFileContent(const std::string& fileName);
    void showFileAttributes(const std::string& fileName);
    bool renameFile(const std::string& oldName, const std::string& newName);
    bool deleteFile(const std::string& fileName);
    bool createFile(const std::string& sourcePath, const std::string& destName);
};

#endif // FAT16_H