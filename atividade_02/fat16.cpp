#include "fat16.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <ctime>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

using namespace std;

// Construtor da classe FAT16Manager
FAT16Manager::FAT16Manager(const string& imagePath) : imageFileName(imagePath) {
    fatStartSector = 0;
    rootDirStartSector = 0;
    dataStartSector = 0;
    rootDirSectors = 0;
}

// Destrutor da classe FAT16Manager
FAT16Manager::~FAT16Manager() {
    if (imageFile.is_open()) {
        imageFile.close();
    }
}

// Inicializa o gerenciador FAT16
// Monta o sistema de arquivos, carregando as estruturas de controle na memória
// Similar ao processo de montagem (mount) de um disco em sistemas Unix/Linux
bool FAT16Manager::initialize() {
    // Abre o arquivo de imagem em modo binário (leitura e escrita)
    // Simula a abertura de um dispositivo de bloco pelo driver de disco
    imageFile.open(imageFileName, ios::in | ios::out | ios::binary);
    if (!imageFile.is_open()) {
        cerr << "Erro: Não foi possível abrir a imagem do disco: " << imageFileName << endl;
        return false;
    }
    
    // Carrega o Boot Sector (setor 0)
    // Lê os metadados do sistema de arquivos (similar ao superbloco em ext4)
    if (!loadBootSector()) {
        cerr << "Erro: Falha ao carregar o Boot Sector" << endl;
        return false;
    }
    
    // Carrega a FAT (File Allocation Table) na memória
    // Estrutura de alocação que mapeia clusters livres e ocupados (similar ao bitmap de blocos)
    if (!loadFAT()) {
        cerr << "Erro: Falha ao carregar a FAT" << endl;
        return false;
    }
    
    // Carrega o diretório raiz na memória
    // Carrega a tabela de inodes/entradas de diretório para acesso rápido
    if (!loadRootDirectory()) {
        cerr << "Erro: Falha ao carregar o diretório raiz" << endl;
        return false;
    }
    
    return true;
}

// Carrega o Boot Sector (primeiro setor do disco)
// Equivalente à leitura do superbloco - contém metadados essenciais do sistema de arquivos
bool FAT16Manager::loadBootSector() {
    // Posiciona no início do disco (setor 0, byte 0)
    imageFile.seekg(0, ios::beg);
    imageFile.read(reinterpret_cast<char*>(&bootSector), sizeof(BootSector));
    
    if (!imageFile.good()) {
        return false;
    }
    
    // Calcula as posições dos setores importantes no disco
    // Layout do disco FAT16 = [Boot Sector][FATs][Root Dir][Data Area]
    
    // Setor onde começa a FAT (normalmente setor 1)
    fatStartSector = bootSector.reservedSectors;
    
    // Setor do diretório raiz = após as FATs (geralmente 2 cópias para redundância)
    rootDirStartSector = fatStartSector + (bootSector.numFATs * bootSector.sectorsPerFAT);
    
    // Quantos setores o diretório raiz ocupa (cada entrada tem 32 bytes)
    rootDirSectors = ((bootSector.rootEntryCount * 32) + (bootSector.bytesPerSector - 1)) / bootSector.bytesPerSector;
    
    // Setor onde começa a área de dados (clusters 2 em diante)
    dataStartSector = rootDirStartSector + rootDirSectors;
    
    return true;
}

// Carrega a FAT (File Allocation Table) do disco para a memória
bool FAT16Manager::loadFAT() {
    // Calcula o tamanho total da FAT em bytes
    uint32_t fatSize = bootSector.sectorsPerFAT * bootSector.bytesPerSector;
    
    // Número de entradas na FAT (cada entrada = 16 bits = 2 bytes)
    uint32_t fatEntries = fatSize / 2;
    
    // Aloca memória para a FAT
    fat.resize(fatEntries);

    // Posiciona no início da FAT e lê todo o conteúdo
    // Carrega a tabela de alocação na RAM para acesso rápido (cache)
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

// Salva a FAT da memória de volta para o disco
// Atualiza TODAS as cópias da FAT para garantir redundância e recuperação
void FAT16Manager::saveFAT() {
    uint32_t fatSize = bootSector.sectorsPerFAT * bootSector.bytesPerSector;
    
    // Atualiza todas as cópias da FAT (geralmente 2 para redundância)
    // Se uma FAT ficar corrompida, a outra pode ser usada para recuperação
    for (int i = 0; i < bootSector.numFATs; i++) {
        uint32_t fatOffset = (fatStartSector + i * bootSector.sectorsPerFAT) * bootSector.bytesPerSector;
        imageFile.seekp(fatOffset, ios::beg);
        imageFile.write(reinterpret_cast<const char*>(fat.data()), fatSize);
    }
    // Força a escrita no disco
    imageFile.flush();
}

void FAT16Manager::saveRootDirectory() {
    uint32_t rootDirSize = bootSector.rootEntryCount * sizeof(DirectoryEntry);
    
    imageFile.seekp(rootDirStartSector * bootSector.bytesPerSector, ios::beg);
    imageFile.write(reinterpret_cast<const char*>(rootDirectory.data()), rootDirSize);
    imageFile.flush();
}

// Calcula o offset (deslocamento) em bytes de um cluster no disco
uint32_t FAT16Manager::getClusterOffset(uint16_t cluster) {
    uint32_t firstSectorOfCluster = dataStartSector + (cluster - 2) * bootSector.sectorsPerCluster;
    
    // Converte setor para offset em bytes
    return firstSectorOfCluster * bootSector.bytesPerSector;
}

//Extrai e reconstrói o nome do arquivo do formato interno FAT16 para string legível
//Ex: "FILE    TXT" -> "FILE.TXT"
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

// Define o nome de arquivo em uma entrada de diretório
// Implementa o formato 8.3 do FAT16 (8 caracteres para o nome, 3 para a extensão)
void FAT16Manager::setFileName(DirectoryEntry& entry, const string& name) {
    // Inicializa com espaços (padrão FAT16)
    memset(entry.fileName, ' ', 8);
    memset(entry.extension, ' ', 3);
    
    size_t dotPos = name.find('.');
    
    if (dotPos == string::npos) {
        // Arquivo sem extensão
        for (size_t i = 0; i < min(name.length(), size_t(8)); i++) {
            // FAT16 armazena nomes em MAIÚSCULAS (case-insensitive)
            entry.fileName[i] = toupper(name[i]);
        }
    } else {
        // Separa nome base e extensão
        string baseName = name.substr(0, dotPos);
        string ext = name.substr(dotPos + 1);
        
        // Copia nome base (até 8 caracteres)
        for (size_t i = 0; i < min(baseName.length(), size_t(8)); i++) {
            entry.fileName[i] = toupper(baseName[i]);
        }

        // Copia extensão (até 3 caracteres)
        for (size_t i = 0; i < min(ext.length(), size_t(3)); i++) {
            entry.extension[i] = toupper(ext[i]);
        }
    }
}

// Formata a data armazenada em formato FAT16 para string legível "DD/MM/AAAA"
string FAT16Manager::formatDate(uint16_t date) {
    int day = date & 0x1F;
    int month = (date >> 5) & 0x0F;
    int year = ((date >> 9) & 0x7F) + 1980;
    
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d", day, month, year);
    return string(buffer);
}

// Formata o tempo armazenado em formato FAT16 para string legível "HH:MM:SS"
string FAT16Manager::formatTime(uint16_t time) {
    int second = (time & 0x1F) * 2;
    int minute = (time >> 5) & 0x3F;
    int hour = (time >> 11) & 0x1F;
    
    char buffer[9];
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hour, minute, second);
    return string(buffer);
}

// Procura um cluster livre na FAT
// Algoritmo de alocação First-Fit - encontra o primeiro bloco livre
// Similar ao gerenciamento de memória e alocação de blocos em disco
uint16_t FAT16Manager::findFreeCluster() {
    // Clusters começam em 2 (0 e 1 são reservados pelo sistema)
    for (size_t i = 2; i < fat.size(); i++) {
        if (fat[i] == FAT_FREE_CLUSTER) {  // 0x0000 indica cluster livre
            return i;
        }
    }
    //Retorna 0 se não há espaço livre (disco cheio)
    return 0;
}

// Busca uma entrada de arquivo no diretório raiz do FAT16 (busca dos metadados)
// Implementa a operação de lookup (busca) em sistema de arquivos
// Percorre linearmente o diretório (O(n) - não há índice ou hash)
DirectoryEntry* FAT16Manager::findFileEntry(const string& fileName) {
    // Converte para maiúsculas (FAT16 é case-insensitive)
    string upperName = fileName;
    transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);

    for (auto& entry : rootDirectory) {
        // 0x00: fim do diretório (não há mais entradas válidas após este ponto)
        if (entry.fileName[0] == 0x00) break;
        
        // 0xE5: entrada deletada (espaço pode ser reutilizado)
        // Implementa deleção "soft" - dados ainda existem até serem sobrescritos
        if (static_cast<uint8_t>(entry.fileName[0]) == 0xE5) continue;
        
        // Ignora entradas de volume label
        if (entry.attributes & ATTR_VOLUME_ID) continue;
        
        // Compara nomes (case-insensitive)
        string entryName = getFileName(entry);
        transform(entryName.begin(), entryName.end(), entryName.begin(), ::toupper);

        if (entryName == upperName) {
            return &entry;  // Arquivo encontrado
        }
    }
    return nullptr;  // Arquivo não encontrado
}

// Procura uma entrada de diretório livre no diretório raiz
// Retorna o índice da entrada livre ou -1 se não houver espaço
int FAT16Manager::findFreeDirectoryEntry() {
    for (size_t i = 0; i < rootDirectory.size(); i++) {
        if (rootDirectory[i].fileName[0] == 0x00 || static_cast<uint8_t>(rootDirectory[i].fileName[0]) == 0xE5) {
            return i;
        }
    }
    return -1;
}

// Lista os arquivos no diretório raiz do FAT16
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

// Exibe o conteúdo de um arquivo
// Implementa a operação de leitura sequencial de arquivo
// Segue a cadeia de clusters na FAT
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

    // Obtém o primeiro cluster do arquivo (início da cadeia)
    uint16_t cluster = entry->firstClusterLow;
    uint32_t remainingBytes = entry->fileSize;
    uint32_t clusterSize = bootSector.sectorsPerCluster * bootSector.bytesPerSector;

    vector<char> buffer(clusterSize);

    // Percorre a linked list de clusters na FAT
    // Cada cluster aponta para o próximo até encontrar EOF (0xFFF8-0xFFFF)
    while (cluster >= 2 && cluster < FAT_EOF_MARKER && remainingBytes > 0) {
        // Calcula onde está este cluster no disco
        uint32_t offset = getClusterOffset(cluster);
        uint32_t bytesToRead = min(remainingBytes, clusterSize);

        // Lê o conteúdo do cluster
        imageFile.seekg(offset, ios::beg);
        imageFile.read(buffer.data(), bytesToRead);
        
        // Exibe o conteúdo
        for (uint32_t i = 0; i < bytesToRead; i++) {
            cout << buffer[i];
        }
        
        remainingBytes -= bytesToRead;
        
        // SO: Busca o próximo cluster na cadeia (follow the pointer)
        cluster = fat[cluster];
    }

    cout << "\n========================================\n" << endl;
}

// Exibe os atributos e metadados de um arquivo
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
    cout << "  Criação:           " << formatDate(entry->creationDate) << " " << formatTime(entry->creationTime) << endl;
    cout << "  Última modificação: " << formatDate(entry->lastModifiedDate) << " " << formatTime(entry->lastModifiedTime) << endl;
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

// Renomeia um arquivo no sistema de arquivos FAT16
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

// Remove um arquivo do sistema de arquivos
// Implementa a operação de deleção (unlink)
// Libera os clusters na FAT e marca a entrada do diretório como deletada
bool FAT16Manager::deleteFile(const string& fileName) {
    DirectoryEntry* entry = findFileEntry(fileName);
    
    if (!entry) {
        cerr << "Erro: Arquivo '" << fileName << "' não encontrado." << endl;
        return false;
    }
    
    // Percorre a cadeia de clusters e marca cada um como livre
    // Libera os blocos para reutilização (dealocação)
    uint16_t cluster = entry->firstClusterLow;
    while (cluster >= 2 && cluster < FAT_EOF_MARKER) {
        uint16_t nextCluster = fat[cluster];  // Salva o próximo antes de limpar
        fat[cluster] = FAT_FREE_CLUSTER;      // Marca como livre (0x0000)
        cluster = nextCluster;
    }
    
    // Marca a entrada do diretório como deletada (soft delete)
    // 0xE5 no primeiro byte indica que o espaço pode ser reutilizado
    // Os dados ainda existem no disco até serem sobrescritos
    entry->fileName[0] = static_cast<char>(0xE5);
    
    // Persiste as mudanças no disco
    saveFAT();
    saveRootDirectory();

    cout << "Arquivo '" << fileName << "' removido com sucesso." << endl;
    return true;
}

// Cria um novo arquivo no sistema FAT16 copiando de um arquivo externo
// Implementa as operações de create + write
// Envolve: alocação de clusters, criação de entrada de diretório, e escrita de dados
bool FAT16Manager::createFile(const string& sourcePath, const string& destName) {
    // Abre o arquivo fonte (do sistema de arquivos hospedeiro)
    ifstream sourceFile(sourcePath, ios::binary);
    if (!sourceFile.is_open()) {
        cerr << "Erro: Não foi possível abrir o arquivo fonte: " << sourcePath << endl;
        return false;
    }
    
    // Descobre o tamanho do arquivo
    sourceFile.seekg(0, ios::end);
    uint32_t fileSize = sourceFile.tellg();
    sourceFile.seekg(0, ios::beg);
    
    // Verifica se já existe arquivo com este nome (nomes devem ser únicos)
    if (findFileEntry(destName)) {
        cerr << "Erro: Já existe um arquivo com o nome '" << destName << "'." << endl;
        sourceFile.close();
        return false;
    }
    
    // Valida o nome do arquivo (formato 8.3)
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
    
    // Calcula quantos clusters são necessários para armazenar o arquivo
    // Unidade de alocação = cluster
    uint32_t clusterSize = bootSector.sectorsPerCluster * bootSector.bytesPerSector;
    uint32_t clustersNeeded = (fileSize + clusterSize - 1) / clusterSize;  // Arredonda para cima
    
    if (clustersNeeded == 0 && fileSize > 0) {
        clustersNeeded = 1;
    }

    // FASE DE ALOCAÇÃO - Reserva clusters livres no disco
    // Implementa alocação não-contígua (linked allocation)
    vector<uint16_t> allocatedClusters;
    for (uint32_t i = 0; i < clustersNeeded; i++) {
        uint16_t cluster = findFreeCluster();
        if (cluster == 0) {
            // Disco cheio - faz rollback da alocação
            cerr << "Erro: Não há espaço suficiente no disco." << endl;

            for (uint16_t c : allocatedClusters) {
                fat[c] = FAT_FREE_CLUSTER;  // Libera clusters já alocados
            }
            sourceFile.close();
            return false;
        }
        allocatedClusters.push_back(cluster);
        fat[cluster] = FAT_EOF_MARKER;  // Marca temporariamente como EOF
    }
    
    // Constrói a cadeia de clusters na FAT
    // Cada cluster aponta para o próximo, exceto o último que tem EOF
    for (size_t i = 0; i < allocatedClusters.size() - 1; i++) {
        fat[allocatedClusters[i]] = allocatedClusters[i + 1];  // cluster[i] -> cluster[i+1]
    }
    if (!allocatedClusters.empty()) {
        fat[allocatedClusters.back()] = FAT_EOF_MARKER;
    }

    // FASE DE ESCRITA - Copia dados do arquivo fonte para os clusters alocados
    // Operação de leitura e escrita em blocos
    vector<char> buffer(clusterSize);
    for (uint16_t cluster : allocatedClusters) {
        uint32_t bytesToRead = min(fileSize, clusterSize);
        
        // Lê dados do arquivo fonte
        sourceFile.read(buffer.data(), bytesToRead);
        uint32_t bytesRead = sourceFile.gcount();
        
        // Preenche o resto do cluster com zeros (padding)
        // Clusters são sempre escritos completos por questões de alinhamento
        if (bytesRead < clusterSize) {
            memset(buffer.data() + bytesRead, 0, clusterSize - bytesRead);
        }
        
        // Escreve o cluster no disco
        uint32_t offset = getClusterOffset(cluster);
        imageFile.seekp(offset, ios::beg);
        imageFile.write(buffer.data(), clusterSize);
        
        fileSize -= bytesRead;
    }
    
    sourceFile.close();
    
    // FASE DE METADADOS - Cria a entrada de diretório
    // Contém informações sobre o arquivo: nome, tamanho, datas, atributos, primeiro cluster
    DirectoryEntry& newEntry = rootDirectory[freeEntryIndex];
    memset(&newEntry, 0, sizeof(DirectoryEntry));
    
    // Define o nome no formato 8.3
    setFileName(newEntry, destName);

    // Define atributos do arquivo (permissões simplificadas)
    // Archive bit indica que o arquivo foi modificado (para backup)
    newEntry.attributes = ATTR_ARCHIVE;

    // Preserva atributos do arquivo original (mapeamento de permissões)
    // Traduz permissões do sistema hospedeiro para atributos FAT16
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(sourcePath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES) {
        if (attrs & FILE_ATTRIBUTE_READONLY) {
            newEntry.attributes |= ATTR_READ_ONLY;  // Somente leitura
            std::cout << "Arquivo marcado como somente leitura." << std::endl;
        }
        if (attrs & FILE_ATTRIBUTE_HIDDEN) {
            newEntry.attributes |= ATTR_HIDDEN;  // Oculto
        }
        if (attrs & FILE_ATTRIBUTE_SYSTEM) {
            newEntry.attributes |= ATTR_SYSTEM;  // Arquivo de sistema
        }
    }
#else
    // Para sistemas Unix/Linux - mapeia permissões POSIX para atributos FAT16
    struct stat fileStat;
    if (stat(sourcePath.c_str(), &fileStat) == 0) {
        // Se o arquivo não tem permissão de escrita para o dono (chmod -w)
        if (!(fileStat.st_mode & S_IWUSR)) {
            newEntry.attributes |= ATTR_READ_ONLY;
            std::cout << "Arquivo marcado como somente leitura." << std::endl;
        }
    }
#endif

    // Define timestamps (metadados temporais)
    // FAT16 tem precisão de 2 segundos para tempo de modificação
    time_t now = ::time(nullptr);
    struct tm* timeInfo = localtime(&now);
    
    // Formato de data FAT: bits 15-9: ano , bits 8-5: mês, bits 4-0: dia
    uint16_t date = ((timeInfo->tm_year - 80) << 9) | ((timeInfo->tm_mon + 1) << 5) | timeInfo->tm_mday;
    // Formato de hora FAT: bits 15-11: hora, bits 10-5: minuto, bits 4-0: segundo/2
    uint16_t timeVal = (timeInfo->tm_hour << 11) | (timeInfo->tm_min << 5) | (timeInfo->tm_sec / 2);
    
    newEntry.creationDate = date;
    newEntry.creationTime = timeVal;
    newEntry.lastModifiedDate = date;
    newEntry.lastModifiedTime = timeVal;
    newEntry.lastAccessDate = date;

    // Define o tamanho exato do arquivo
    ifstream sizeCheck(sourcePath, ios::binary | ios::ate);
    newEntry.fileSize = sizeCheck.tellg();
    sizeCheck.close();
    
    // Aponta para o primeiro cluster da cadeia (entrada da linked list)
    // Este é o ponto de partida para ler o arquivo
    newEntry.firstClusterLow = allocatedClusters.empty() ? 0 : allocatedClusters[0];
    newEntry.firstClusterHigh = 0;  // FAT16 usa apenas os 16 bits baixos
    
    // Persiste todas as mudanças no disco
    saveFAT();              // Atualiza a tabela de alocação
    saveRootDirectory();    // Atualiza o diretório raiz

    cout << "Arquivo '" << destName << "' criado com sucesso (" << newEntry.fileSize << " bytes)." << endl;
    return true;
}