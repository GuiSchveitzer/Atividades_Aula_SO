#include "fat16.h"
#include <ctime>
#include <algorithm>
#include <cctype>

using namespace std;

FAT16Manager::FAT16Manager(const string& imagePath) : imageFileName(imagePath) {
    fatStartSector = 0;
    rootDirStartSector = 0;
    dataStartSector = 0;
    rootDirSectors = 0;
}

FAT16Manager::~FAT16Manager() {
    if (imageFile.is_open()) {
        imageFile.close();
    }
}

bool FAT16Manager::initialize() {
    imageFile.open(imageFileName, ios::in | ios::out | ios::binary);
    if (!imageFile.is_open()) {
        cerr << "Erro: Não foi possível abrir a imagem do disco: " << imageFileName << endl;
        return false;
    }
    
    if (!loadBootSector()) {
        cerr << "Erro: Falha ao carregar o Boot Sector" << endl;
        return false;
    }
    
    if (!loadFAT()) {
        cerr << "Erro: Falha ao carregar a FAT" << endl;
        return false;
    }
    
    if (!loadRootDirectory()) {
        cerr << "Erro: Falha ao carregar o diretório raiz" << endl;
        return false;
    }
    
    return true;
}

bool FAT16Manager::loadBootSector() {
    imageFile.seekg(0, ios::beg);
    imageFile.read(reinterpret_cast<char*>(&bootSector), sizeof(BootSector));
    
    if (!imageFile.good()) {
        return false;
    }
    
    // Calcular setores importantes
    fatStartSector = bootSector.reservedSectors;
    rootDirStartSector = fatStartSector + (bootSector.numFATs * bootSector.sectorsPerFAT);
    rootDirSectors = ((bootSector.rootEntryCount * 32) + (bootSector.bytesPerSector - 1)) / bootSector.bytesPerSector;
    dataStartSector = rootDirStartSector + rootDirSectors;
    
    return true;
}

bool FAT16Manager::loadFAT() {
    uint32_t fatSize = bootSector.sectorsPerFAT * bootSector.bytesPerSector;
    uint32_t fatEntries = fatSize / 2;
    
    fat.resize(fatEntries);

    imageFile.seekg(fatStartSector * bootSector.bytesPerSector, ios::beg);
    imageFile.read(reinterpret_cast<char*>(fat.data()), fatSize);
    
    return imageFile.good();
}

bool FAT16Manager::loadRootDirectory() {
    uint32_t rootDirSize = bootSector.rootEntryCount * sizeof(DirectoryEntry);
    rootDirectory.resize(bootSector.rootEntryCount);
    
    imageFile.seekg(rootDirStartSector * bootSector.bytesPerSector, ios::beg);
    imageFile.read(reinterpret_cast<char*>(rootDirectory.data()), rootDirSize);
    
    return imageFile.good();
}

void FAT16Manager::saveFAT() {
    uint32_t fatSize = bootSector.sectorsPerFAT * bootSector.bytesPerSector;
    
    for (int i = 0; i < bootSector.numFATs; i++) {
        uint32_t fatOffset = (fatStartSector + i * bootSector.sectorsPerFAT) * bootSector.bytesPerSector;
        imageFile.seekp(fatOffset, ios::beg);
        imageFile.write(reinterpret_cast<const char*>(fat.data()), fatSize);
    }
    imageFile.flush();
}

void FAT16Manager::saveRootDirectory() {
    uint32_t rootDirSize = bootSector.rootEntryCount * sizeof(DirectoryEntry);
    
    imageFile.seekp(rootDirStartSector * bootSector.bytesPerSector, ios::beg);
    imageFile.write(reinterpret_cast<const char*>(rootDirectory.data()), rootDirSize);
    imageFile.flush();
}

uint32_t FAT16Manager::getClusterOffset(uint16_t cluster) {
    uint32_t firstSectorOfCluster = dataStartSector + (cluster - 2) * bootSector.sectorsPerCluster;
    return firstSectorOfCluster * bootSector.bytesPerSector;
}

string FAT16Manager::getFileName(const DirectoryEntry& entry) {
    string name;

    for (int i = 0; i < 8 && entry.fileName[i] != ' '; i++) {
        name += entry.fileName[i];
    }
    
    bool hasExtension = false;
    for (int i = 0; i < 3; i++) {
        if (entry.extension[i] != ' ') {
            hasExtension = true;
            break;
        }
    }
    
    if (hasExtension) {
        name += '.';
        for (int i = 0; i < 3 && entry.extension[i] != ' '; i++) {
            name += entry.extension[i];
        }
    }
    
    return name;
}

void FAT16Manager::setFileName(DirectoryEntry& entry, const string& name) {
    memset(entry.fileName, ' ', 8);
    memset(entry.extension, ' ', 3);
    
    size_t dotPos = name.find('.');
    
    if (dotPos == string::npos) {
        for (size_t i = 0; i < min(name.length(), size_t(8)); i++) {
            entry.fileName[i] = toupper(name[i]);
        }
    } else {
        string baseName = name.substr(0, dotPos);
        string ext = name.substr(dotPos + 1);
        
        for (size_t i = 0; i < min(baseName.length(), size_t(8)); i++) {
            entry.fileName[i] = toupper(baseName[i]);
        }

        for (size_t i = 0; i < min(ext.length(), size_t(3)); i++) {
            entry.extension[i] = toupper(ext[i]);
        }
    }
}

string FAT16Manager::formatDate(uint16_t date) {
    int day = date & 0x1F;
    int month = (date >> 5) & 0x0F;
    int year = ((date >> 9) & 0x7F) + 1980;
    
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", day, month, year);
    return string(buffer);
}

string FAT16Manager::formatTime(uint16_t time) {
    int second = (time & 0x1F) * 2;
    int minute = (time >> 5) & 0x3F;
    int hour = (time >> 11) & 0x1F;
    
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour, minute, second);
    return string(buffer);
}

uint16_t FAT16Manager::findFreeCluster() {
    for (size_t i = 2; i < fat.size(); i++) {
        if (fat[i] == FAT_FREE_CLUSTER) {
            return i;
        }
    }
    return 0;
}

DirectoryEntry* FAT16Manager::findFileEntry(const string& fileName) {
    string upperName = fileName;
    transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

    for (auto& entry : rootDirectory) {
        if (entry.fileName[0] == 0x00) break;
        if (static_cast<uint8_t>(entry.fileName[0]) == 0xE5) continue;
        if (entry.attributes & ATTR_VOLUME_ID) continue;
        
        std::string entryName = getFileName(entry);
        std::transform(entryName.begin(), entryName.end(), entryName.begin(), ::toupper);
        
        if (entryName == upperName) {
            return &entry;
        }
    }
    return nullptr;
}

int FAT16Manager::findFreeDirectoryEntry() {
    for (size_t i = 0; i < rootDirectory.size(); i++) {
        if (rootDirectory[i].fileName[0] == 0x00 || static_cast<uint8_t>(rootDirectory[i].fileName[0]) == 0xE5) {
            return i;
        }
    }
    return -1;
}

void FAT16Manager::listFiles() {
    cout << "\n========== CONTEÚDO DO DISCO ==========\n";
    cout << left << setw(20) << "Nome do Arquivo" 
         << right << setw(15) << "Tamanho (bytes)" << endl;
    cout << string(35, '-') << endl;

    int fileCount = 0;
    for (const auto& entry : rootDirectory) {
        if (entry.fileName[0] == 0x00) break;
        if (static_cast<uint8_t>(entry.fileName[0]) == 0xE5) continue;
        if (entry.attributes & ATTR_VOLUME_ID) continue;
        if (entry.attributes & ATTR_DIRECTORY) continue;

        string fileName = getFileName(entry);
        cout << left << setw(20) << fileName 
             << right << setw(15) << entry.fileSize << endl;
        fileCount++;
    }
    
    if (fileCount == 0) {
        cout << "Nenhum arquivo encontrado no diretório raiz." << endl;
    }
    cout << "\nTotal de arquivos: " << fileCount << endl;
    cout << "========================================\n" << endl;
}

void FAT16Manager::showFileContent(const string& fileName) {
    DirectoryEntry* entry = findFileEntry(fileName);
    
    if (!entry) {
        cerr << "Erro: Arquivo '" << fileName << "' não encontrado." << endl;
        return;
    }
    
    if (entry->fileSize == 0) {
        cout << "\nArquivo vazio." << endl;
        return;
    }

    cout << "\n========== CONTEÚDO DO ARQUIVO: " << fileName << " ==========\n";

    uint16_t cluster = entry->firstClusterLow;
    uint32_t remainingBytes = entry->fileSize;
    uint32_t clusterSize = bootSector.sectorsPerCluster * bootSector.bytesPerSector;

    vector<char> buffer(clusterSize);
    
    while (cluster >= 2 && cluster < FAT_EOF_MARKER && remainingBytes > 0) {
        uint32_t offset = getClusterOffset(cluster);
        uint32_t bytesToRead = min(remainingBytes, clusterSize);

        imageFile.seekg(offset, ios::beg);
        imageFile.read(buffer.data(), bytesToRead);
        
        for (uint32_t i = 0; i < bytesToRead; i++) {
            cout << buffer[i];
        }
        
        remainingBytes -= bytesToRead;
        cluster = fat[cluster];
    }

    cout << "\n========================================\n" << endl;
}

void FAT16Manager::showFileAttributes(const string& fileName) {
    DirectoryEntry* entry = findFileEntry(fileName);
    
    if (!entry) {
        cerr << "Erro: Arquivo '" << fileName << "' não encontrado." << endl;
        return;
    }

    cout << "\n========== ATRIBUTOS DO ARQUIVO: " << fileName << " ==========\n";
    cout << "Nome completo: " << getFileName(*entry) << endl;
    cout << "Tamanho: " << entry->fileSize << " bytes" << endl;
    cout << "\nDatas e Horários:" << endl;
    cout << "  Criação:           " << formatDate(entry->creationDate) 
              << " " << formatTime(entry->creationTime) << endl;
    cout << "  Última modificação: " << formatDate(entry->lastModifiedDate) 
              << " " << formatTime(entry->lastModifiedTime) << endl;
    cout << "  Último acesso:      " << formatDate(entry->lastAccessDate) << endl;

    cout << "\nAtributos:" << endl;
    cout << "  Somente leitura: " << ((entry->attributes & ATTR_READ_ONLY) ? "SIM" : "NÃO") << endl;
    cout << "  Oculto:          " << ((entry->attributes & ATTR_HIDDEN) ? "SIM" : "NÃO") << endl;
    cout << "  Arquivo sistema: " << ((entry->attributes & ATTR_SYSTEM) ? "SIM" : "NÃO") << endl;
    cout << "  Arquivo:         " << ((entry->attributes & ATTR_ARCHIVE) ? "SIM" : "NÃO") << endl;

    cout << "\nInformações técnicas:" << endl;
    cout << "  Primeiro cluster: " << entry->firstClusterLow << endl;
    cout << "========================================\n" << endl;
}

bool FAT16Manager::renameFile(const string& oldName, const string& newName) {
    DirectoryEntry* entry = findFileEntry(oldName);
    
    if (!entry) {
        cerr << "Erro: Arquivo '" << oldName << "' não encontrado." << endl;
        return false;
    }
    
    if (findFileEntry(newName)) {
        cerr << "Erro: Já existe um arquivo com o nome '" << newName << "'." << endl;
        return false;
    }
    
    size_t dotPos = newName.find('.');
    if (dotPos != string::npos) {
        string baseName = newName.substr(0, dotPos);
        string ext = newName.substr(dotPos + 1);

        if (baseName.length() > 8 || ext.length() > 3) {
            cerr << "Erro: Nome inválido. Formato: até 8 caracteres.até 3 caracteres" << endl;
            return false;
        }
    } else {
        if (newName.length() > 8) {
            cerr << "Erro: Nome muito longo (máximo 8 caracteres sem extensão)." << endl;
            return false;
        }
    }
    
    setFileName(*entry, newName);
    
    time_t now = ::time(nullptr);
    struct tm* timeInfo = localtime(&now);
    
    uint16_t date = ((timeInfo->tm_year - 80) << 9) | ((timeInfo->tm_mon + 1) << 5) | timeInfo->tm_mday;
    uint16_t timeVal = (timeInfo->tm_hour << 11) | (timeInfo->tm_min << 5) | (timeInfo->tm_sec / 2);
    
    entry->lastModifiedDate = date;
    entry->lastModifiedTime = timeVal;
    
    saveRootDirectory();

    cout << "Arquivo renomeado com sucesso: '" << oldName << "' -> '" << newName << "'" << endl;
    return true;
}

bool FAT16Manager::deleteFile(const string& fileName) {
    DirectoryEntry* entry = findFileEntry(fileName);
    
    if (!entry) {
        cerr << "Erro: Arquivo '" << fileName << "' não encontrado." << endl;
        return false;
    }
    
    uint16_t cluster = entry->firstClusterLow;
    while (cluster >= 2 && cluster < FAT_EOF_MARKER) {
        uint16_t nextCluster = fat[cluster];
        fat[cluster] = FAT_FREE_CLUSTER;
        cluster = nextCluster;
    }
    
    entry->fileName[0] = static_cast<char>(0xE5);
    
    saveFAT();
    saveRootDirectory();

    cout << "Arquivo '" << fileName << "' removido com sucesso." << endl;
    return true;
}

bool FAT16Manager::createFile(const string& sourcePath, const string& destName) {
    ifstream sourceFile(sourcePath, ios::binary);
    if (!sourceFile.is_open()) {
        cerr << "Erro: Não foi possível abrir o arquivo fonte: " << sourcePath << endl;
        return false;
    }
    
    sourceFile.seekg(0, ios::end);
    uint32_t fileSize = sourceFile.tellg();
    sourceFile.seekg(0, ios::beg);
    
    if (findFileEntry(destName)) {
        cerr << "Erro: Já existe um arquivo com o nome '" << destName << "'." << endl;
        sourceFile.close();
        return false;
    }
    
    size_t dotPos = destName.find('.');
    if (dotPos != string::npos) {
        string baseName = destName.substr(0, dotPos);
        string ext = destName.substr(dotPos + 1);

        if (baseName.length() > 8 || ext.length() > 3) {
            cerr << "Erro: Nome inválido. Formato: até 8 caracteres.até 3 caracteres" << endl;
            sourceFile.close();
            return false;
        }
    } else {
        if (destName.length() > 8) {
            cerr << "Erro: Nome muito longo (máximo 8 caracteres sem extensão)." << endl;
            sourceFile.close();
            return false;
        }
    }
    
    int freeEntryIndex = findFreeDirectoryEntry();
    if (freeEntryIndex == -1) {
        cerr << "Erro: Diretório raiz está cheio." << endl;
        sourceFile.close();
        return false;
    }
    
    uint32_t clusterSize = bootSector.sectorsPerCluster * bootSector.bytesPerSector;
    uint32_t clustersNeeded = (fileSize + clusterSize - 1) / clusterSize;
    
    if (clustersNeeded == 0 && fileSize > 0) {
        clustersNeeded = 1;
    }

    vector<uint16_t> allocatedClusters;
    for (uint32_t i = 0; i < clustersNeeded; i++) {
        uint16_t cluster = findFreeCluster();
        if (cluster == 0) {
            cerr << "Erro: Não há espaço suficiente no disco." << endl;

            for (uint16_t c : allocatedClusters) {
                fat[c] = FAT_FREE_CLUSTER;
            }
            sourceFile.close();
            return false;
        }
        allocatedClusters.push_back(cluster);
        fat[cluster] = FAT_EOF_MARKER;
    }
    
    for (size_t i = 0; i < allocatedClusters.size() - 1; i++) {
        fat[allocatedClusters[i]] = allocatedClusters[i + 1];
    }
    if (!allocatedClusters.empty()) {
        fat[allocatedClusters.back()] = FAT_EOF_MARKER;
    }
    
    vector<char> buffer(clusterSize);
    for (uint16_t cluster : allocatedClusters) {
        uint32_t bytesToRead = min(fileSize, clusterSize);
        
        sourceFile.read(buffer.data(), bytesToRead);
        uint32_t bytesRead = sourceFile.gcount();
        
        if (bytesRead < clusterSize) {
            memset(buffer.data() + bytesRead, 0, clusterSize - bytesRead);
        }
        
        uint32_t offset = getClusterOffset(cluster);
        imageFile.seekp(offset, ios::beg);
        imageFile.write(buffer.data(), clusterSize);
        
        fileSize -= bytesRead;
    }
    
    sourceFile.close();
    
    DirectoryEntry& newEntry = rootDirectory[freeEntryIndex];
    memset(&newEntry, 0, sizeof(DirectoryEntry));
    
    setFileName(newEntry, destName);
    newEntry.attributes = ATTR_ARCHIVE;
    
    time_t now = ::time(nullptr);
    struct tm* timeInfo = localtime(&now);
    
    uint16_t date = ((timeInfo->tm_year - 80) << 9) | ((timeInfo->tm_mon + 1) << 5) | timeInfo->tm_mday;
    uint16_t timeVal = (timeInfo->tm_hour << 11) | (timeInfo->tm_min << 5) | (timeInfo->tm_sec / 2);
    
    newEntry.creationDate = date;
    newEntry.creationTime = timeVal;
    newEntry.lastModifiedDate = date;
    newEntry.lastModifiedTime = timeVal;
    newEntry.lastAccessDate = date;

    ifstream sizeCheck(sourcePath, ios::binary | ios::ate);
    newEntry.fileSize = sizeCheck.tellg();
    sizeCheck.close();
    
    newEntry.firstClusterLow = allocatedClusters.empty() ? 0 : allocatedClusters[0];
    newEntry.firstClusterHigh = 0;
    
    saveFAT();
    saveRootDirectory();

    cout << "Arquivo '" << destName << "' criado com sucesso (" << newEntry.fileSize << " bytes)." << endl;
    return true;
}
