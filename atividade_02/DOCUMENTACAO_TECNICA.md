# Documentação Técnica - Gerenciador FAT16

## Visão Geral

Este documento descreve os detalhes técnicos da implementação do gerenciador de sistema de arquivos FAT16 em C++.

## Arquitetura do Sistema

### Estrutura de Dados

#### 1. Boot Sector (512 bytes)
```cpp
struct BootSector {
    uint8_t  jmpBoot[3];           // 0x00: Jump instruction
    char     OEMName[8];           // 0x03: Nome OEM
    uint16_t bytesPerSector;       // 0x0B: Bytes por setor (512)
    uint8_t  sectorsPerCluster;    // 0x0D: Setores por cluster
    uint16_t reservedSectors;      // 0x0E: Setores reservados
    uint8_t  numFATs;              // 0x10: Número de FATs (2)
    uint16_t rootEntryCount;       // 0x11: Entradas no diretório raiz
    uint16_t totalSectors16;       // 0x13: Total de setores
    uint8_t  mediaType;            // 0x15: Tipo de mídia
    uint16_t sectorsPerFAT;        // 0x16: Setores por FAT
    uint16_t sectorsPerTrack;      // 0x18: Setores por trilha
    uint16_t numHeads;             // 0x1A: Número de cabeças
    uint32_t hiddenSectors;        // 0x1C: Setores ocultos
    uint32_t totalSectors32;       // 0x20: Total (se >= 65536)
    // ... campos adicionais
};
```

**Campos Importantes:**
- `bytesPerSector`: Geralmente 512 bytes
- `sectorsPerCluster`: Varia (1, 2, 4, 8, 16, 32, 64)
- `numFATs`: Tipicamente 2 (redundância)
- `rootEntryCount`: Geralmente 512 (16 KB)
- `sectorsPerFAT`: Tamanho da FAT em setores

#### 2. Directory Entry (32 bytes)
```cpp
struct DirectoryEntry {
    char     fileName[8];          // 0x00: Nome (8 chars)
    char     extension[3];         // 0x08: Extensão (3 chars)
    uint8_t  attributes;           // 0x0B: Atributos
    uint8_t  reserved;             // 0x0C: Reservado
    uint8_t  creationTimeTenth;    // 0x0D: Centésimos
    uint16_t creationTime;         // 0x0E: Hora criação
    uint16_t creationDate;         // 0x10: Data criação
    uint16_t lastAccessDate;       // 0x12: Data último acesso
    uint16_t firstClusterHigh;     // 0x14: Cluster alto (FAT32)
    uint16_t lastModifiedTime;     // 0x16: Hora modificação
    uint16_t lastModifiedDate;     // 0x18: Data modificação
    uint16_t firstClusterLow;      // 0x1A: Cluster baixo
    uint32_t fileSize;             // 0x1C: Tamanho em bytes
};
```

**Atributos (byte 0x0B):**
```
0x01 - ATTR_READ_ONLY  (somente leitura)
0x02 - ATTR_HIDDEN     (oculto)
0x04 - ATTR_SYSTEM     (sistema)
0x08 - ATTR_VOLUME_ID  (label de volume)
0x10 - ATTR_DIRECTORY  (diretório)
0x20 - ATTR_ARCHIVE    (arquivo)
0x0F - ATTR_LONG_NAME  (nome longo)
```

**Formato de Data (16 bits):**
```
Bits 15-9: Ano (0 = 1980, 127 = 2107)
Bits 8-5:  Mês (1-12)
Bits 4-0:  Dia (1-31)
```

**Formato de Hora (16 bits):**
```
Bits 15-11: Hora (0-23)
Bits 10-5:  Minuto (0-59)
Bits 4-0:   Segundo/2 (0-29)
```

#### 3. File Allocation Table (FAT)
```
Cada entrada: 16 bits (2 bytes)

Valores especiais:
0x0000        = Cluster livre
0x0001        = Reservado
0x0002-0xFFEF = Próximo cluster
0xFFF0-0xFFF6 = Reservado
0xFFF7        = Cluster defeituoso
0xFFF8-0xFFFF = Fim da cadeia (EOF)
```

### Layout do Disco

```
+------------------+  Offset 0
| Boot Sector      |  512 bytes (1 setor)
+------------------+
| Setores          |
| Reservados       |  Variável
+------------------+
| FAT #1           |  sectorsPerFAT setores
+------------------+
| FAT #2           |  sectorsPerFAT setores (cópia)
+------------------+
| Diretório Raiz   |  rootEntryCount * 32 bytes
|                  |  (geralmente 32 setores = 16 KB)
+------------------+
| Área de Dados    |  Clusters 2, 3, 4, ...
| Cluster 2        |
| Cluster 3        |
| ...              |
+------------------+
```

### Cálculo de Offsets

#### Offset do Boot Sector
```cpp
uint32_t bootSectorOffset = 0;
```

#### Offset da FAT
```cpp
uint32_t fatOffset = bootSector.reservedSectors * bootSector.bytesPerSector;
```

#### Offset do Diretório Raiz
```cpp
uint32_t rootDirOffset = fatOffset + 
    (bootSector.numFATs * bootSector.sectorsPerFAT * bootSector.bytesPerSector);
```

#### Offset da Área de Dados
```cpp
uint32_t rootDirSectors = 
    ((bootSector.rootEntryCount * 32) + (bootSector.bytesPerSector - 1)) / 
    bootSector.bytesPerSector;

uint32_t dataOffset = rootDirOffset + 
    (rootDirSectors * bootSector.bytesPerSector);
```

#### Offset de um Cluster Específico
```cpp
// Clusters começam em 2
uint32_t clusterOffset(uint16_t cluster) {
    uint32_t firstSector = dataStartSector + (cluster - 2) * sectorsPerCluster;
    return firstSector * bytesPerSector;
}
```

## Algoritmos Principais

### 1. Inicialização

```
1. Abrir arquivo de imagem em modo binário
2. Ler Boot Sector (primeiros 512 bytes)
3. Calcular offsets importantes:
   - Início da FAT
   - Início do diretório raiz
   - Início da área de dados
4. Carregar FAT na memória
5. Carregar diretório raiz na memória
```

**Complexidade:** O(n) onde n é o tamanho da FAT + diretório

### 2. Listar Arquivos

```
1. Para cada entrada no diretório raiz:
   a. Se fileName[0] == 0x00: fim das entradas, parar
   b. Se fileName[0] == 0xE5: entrada deletada, pular
   c. Se attributes & ATTR_VOLUME_ID: label, pular
   d. Se attributes & ATTR_DIRECTORY: subdiretório, pular
   e. Extrair nome e extensão
   f. Exibir nome e tamanho
```

**Complexidade:** O(n) onde n é o número de entradas no diretório

### 3. Ler Conteúdo de Arquivo

```
1. Buscar entrada do arquivo no diretório
2. Obter primeiro cluster (firstClusterLow)
3. Obter tamanho do arquivo
4. ENQUANTO cluster válido E bytes restantes > 0:
   a. Calcular offset do cluster
   b. Ler dados do cluster
   c. Exibir/processar dados
   d. Atualizar bytes restantes
   e. Obter próximo cluster da FAT: cluster = FAT[cluster]
5. Se próximo cluster >= 0xFFF8: fim do arquivo
```

**Complexidade:** O(k) onde k é o número de clusters do arquivo

### 4. Exibir Atributos

```
1. Buscar entrada do arquivo no diretório
2. Extrair e decodificar:
   a. Data de criação (bits 15-9: ano, 8-5: mês, 4-0: dia)
   b. Hora de criação (bits 15-11: hora, 10-5: min, 4-0: seg/2)
   c. Data de modificação
   d. Hora de modificação
   e. Data de último acesso
3. Verificar bits de atributos:
   a. Bit 0: somente leitura
   b. Bit 1: oculto
   c. Bit 2: sistema
   d. Bit 5: arquivo
4. Exibir informações formatadas
```

**Complexidade:** O(n) onde n é o número de entradas no diretório

### 5. Renomear Arquivo

```
1. Buscar entrada do arquivo no diretório
2. Verificar se novo nome já existe
3. Validar formato 8.3:
   a. Separar nome e extensão pelo ponto
   b. Nome: máximo 8 caracteres
   c. Extensão: máximo 3 caracteres
4. Converter nome para maiúsculas
5. Preencher campos fileName e extension com espaços
6. Copiar nome (padding com espaços)
7. Copiar extensão (padding com espaços)
8. Atualizar data/hora de modificação
9. Salvar diretório raiz no disco
```

**Complexidade:** O(n) onde n é o número de entradas no diretório

### 6. Apagar Arquivo

```
1. Buscar entrada do arquivo no diretório
2. Obter primeiro cluster (firstClusterLow)
3. Percorrer cadeia de clusters:
   a. Salvar próximo cluster: next = FAT[cluster]
   b. Liberar cluster atual: FAT[cluster] = 0x0000
   c. cluster = next
   d. Se cluster >= 0xFFF8: fim da cadeia
4. Marcar entrada como deletada: fileName[0] = 0xE5
5. Salvar FAT (todas as cópias)
6. Salvar diretório raiz
```

**Complexidade:** O(k + n) onde k é o número de clusters e n é o tamanho da FAT

### 7. Criar Arquivo (mais complexo)

```
1. Abrir arquivo fonte externo
2. Obter tamanho do arquivo
3. Verificar se nome já existe no diretório
4. Validar formato 8.3 do nome
5. Encontrar entrada livre no diretório:
   a. Procurar fileName[0] == 0x00 ou 0xE5
6. Calcular clusters necessários:
   clusters = (tamanho + clusterSize - 1) / clusterSize
7. Alocar clusters:
   a. Para cada cluster necessário:
      - Buscar cluster livre na FAT (valor 0x0000)
      - Adicionar à lista de alocados
      - Marcar temporariamente como EOF (0xFFFF)
8. Encadear clusters na FAT:
   a. FAT[cluster[i]] = cluster[i+1]
   b. FAT[cluster[último]] = 0xFFFF (EOF)
9. Escrever dados:
   a. Para cada cluster alocado:
      - Ler bloco do arquivo fonte
      - Calcular offset do cluster
      - Escrever dados no cluster
      - Preencher resto com zeros se necessário
10. Criar entrada no diretório:
    a. Definir nome e extensão
    b. Definir atributos (ATTR_ARCHIVE)
    c. Calcular data/hora atual
    d. Definir tamanho do arquivo
    e. Definir primeiro cluster
11. Salvar FAT (todas as cópias)
12. Salvar diretório raiz
```

**Complexidade:** O(k * m + n) onde:
- k = número de clusters necessários
- m = tamanho do cluster
- n = tamanho da FAT

## Tratamento de Erros

### Cenários Tratados

1. **Arquivo de imagem não encontrado**
   - Verificação: `imageFile.is_open()`
   - Ação: Retornar erro, exibir mensagem

2. **Arquivo não encontrado no diretório**
   - Verificação: `findFileEntry() == nullptr`
   - Ação: Exibir mensagem de erro

3. **Nome duplicado**
   - Verificação: `findFileEntry(newName) != nullptr`
   - Ação: Impedir operação, exibir mensagem

4. **Nome inválido (formato 8.3)**
   - Verificação: Comprimento de nome/extensão
   - Ação: Rejeitar, exibir formato correto

5. **Disco cheio**
   - Verificação: `findFreeCluster() == 0`
   - Ação: Liberar clusters alocados, retornar erro

6. **Diretório cheio**
   - Verificação: `findFreeDirectoryEntry() == -1`
   - Ação: Retornar erro, sugerir remover arquivos

7. **Erro de leitura/escrita**
   - Verificação: `imageFile.good()`
   - Ação: Retornar erro, verificar permissões

## Otimizações Implementadas

### 1. Carregamento em Memória
- FAT completa carregada uma vez
- Diretório raiz carregado uma vez
- Evita múltiplas operações de I/O

### 2. Escrita Eficiente
- Atualização de todas as cópias da FAT simultaneamente
- Flush explícito para garantir persistência

### 3. Busca de Clusters Livres
- Busca linear a partir do cluster 2
- Para no primeiro cluster livre encontrado
- Pode ser otimizado com bitmap ou lista de livres

### 4. Validação Antecipada
- Verifica condições de erro antes de alocar recursos
- Evita operações parciais que precisam ser revertidas

## Limitações Conhecidas

1. **Sem suporte a subdiretórios**
   - Apenas diretório raiz é manipulado
   - Conforme especificação do trabalho

2. **Sem suporte a Long File Names (LFN)**
   - Apenas formato 8.3 tradicional
   - Nomes longos ignorados

3. **Fragmentação**
   - Não há desfragmentação
   - Clusters alocados sequencialmente quando possível

4. **Tamanho máximo de arquivo**
   - Limitado pelo número de clusters disponíveis
   - FAT16: máximo 2 GB com clusters de 32 KB

5. **Case-insensitive mas preserva maiúsculas**
   - Busca é case-insensitive
   - Armazenamento é sempre em maiúsculas (padrão FAT)

## Considerações de Portabilidade

### Endianness
- FAT16 usa little-endian
- Estruturas definidas com `#pragma pack(push, 1)`
- Campos multi-byte lidos diretamente

### Alinhamento
- `#pragma pack` garante estruturas sem padding
- Essencial para correspondência binária exata

### Tamanhos de Tipos
- Uso de tipos fixos: `uint8_t`, `uint16_t`, `uint32_t`
- Garante portabilidade entre plataformas

## Referências Técnicas

### Especificações
- Microsoft FAT Specification
- FAT File System Design Guide

### Valores de Referência
- Setor: 512 bytes (padrão)
- Cluster: 512, 1K, 2K, 4K, 8K, 16K, 32K bytes
- Diretório raiz: 512 entradas (16 KB)
- Entrada de diretório: 32 bytes
- Entrada FAT: 2 bytes (FAT16)

### Ferramentas de Teste
- `mount` (Linux)
- `hexdump` / `xxd`
- `fsck.fat` (verificação)
- `mkfs.fat` (criação)

## Fluxograma de Operações Críticas

### Criação de Arquivo
```
Início
  |
  V
Abrir arquivo fonte
  |
  V
Validar nome
  |
  V
Buscar entrada livre -----> [Não encontrou] --> Erro: Diretório cheio
  |
  V
Calcular clusters necessários
  |
  V
Loop: Alocar clusters
  |
  V
[Cluster livre?] --Não--> Liberar alocados --> Erro: Disco cheio
  |
  Sim
  V
Encadear clusters na FAT
  |
  V
Loop: Escrever dados
  |
  V
Criar entrada no diretório
  |
  V
Salvar FAT e diretório
  |
  V
Sucesso
```

### Remoção de Arquivo
```
Início
  |
  V
Buscar arquivo -----> [Não encontrou] --> Erro: Arquivo não existe
  |
  V
Confirmar remoção
  |
  V
Loop: Percorrer cadeia de clusters
  |
  V
Liberar cada cluster (FAT[i] = 0x0000)
  |
  V
Marcar entrada como deletada (0xE5)
  |
  V
Salvar FAT e diretório
  |
  V
Sucesso
```

---

**Última atualização:** Novembro 2025
