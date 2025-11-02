# Gerenciador de Sistema de Arquivos FAT16

Este projeto implementa um gerenciador completo para manipulação de sistemas de arquivos FAT16, desenvolvido em C++ como trabalho da disciplina de Sistemas Operacionais.

## 📋 Funcionalidades Implementadas

O programa implementa **todas as 6 operações** solicitadas:

### 1. Listar Conteúdo do Disco (10%)
- Exibe todos os arquivos do diretório raiz
- Mostra nome e tamanho de cada arquivo
- Conta o total de arquivos no disco

### 2. Listar Conteúdo de um Arquivo (10%)
- Exibe o conteúdo completo de um arquivo
- Lê os dados seguindo a cadeia de clusters na FAT
- Suporta arquivos de qualquer tamanho

### 3. Exibir Atributos de um Arquivo (10%)
- Mostra data/hora de criação e última modificação
- Exibe atributos: somente leitura, oculto, arquivo de sistema
- Informações técnicas adicionais (tamanho, primeiro cluster)

### 4. Renomear um Arquivo (10%)
- Permite trocar o nome de arquivos existentes
- Valida formato 8.3 do FAT16
- Atualiza data de modificação automaticamente

### 5. Apagar/Remover um Arquivo (20%)
- Remove arquivos do diretório raiz
- Libera todos os clusters ocupados na FAT
- Solicita confirmação antes de apagar

### 6. Inserir/Criar um Novo Arquivo (40%)
- Adiciona arquivos externos ao disco FAT16
- Aloca clusters necessários dinamicamente
- Cria entrada no diretório com todas as informações
- Define datas/horas de criação automaticamente

## 🏗️ Estrutura do Projeto

```
Trabalho_M2/
├── fat16.h          # Cabeçalho com estruturas e declarações
├── fat16.cpp        # Implementação das operações FAT16
├── main.cpp         # Programa principal com menu interativo
├── Makefile         # Automação da compilação
├── disco1.img       # Imagem de disco FAT16 de exemplo
├── disco2.img       # Imagem de disco FAT16 de exemplo
└── README.md        # Este arquivo
```

## 🔧 Compilação

### Usando Make (recomendado)

```bash
# Compilar o projeto
make

# Limpar arquivos compilados
make clean

# Recompilar tudo
make rebuild

# Compilar e executar com disco1.img
make run1

# Compilar e executar com disco2.img
make run2

# Ver opções disponíveis
make help
```

### Compilação Manual

```bash
g++ -std=c++11 -Wall -Wextra -O2 -o fat16manager main.cpp fat16.cpp
```

## 🚀 Como Usar

### Execução Básica

```bash
# Executar com imagem de disco
./fat16manager disco1.img

# Ou deixar o programa solicitar o caminho
./fat16manager
```

### Menu Interativo

Ao executar, você verá um menu com as seguintes opções:

```
╔════════════════════════════════════════════════╗
║   GERENCIADOR DE SISTEMA DE ARQUIVOS FAT16    ║
╠════════════════════════════════════════════════╣
║ 1. Listar conteúdo do disco                    ║
║ 2. Mostrar conteúdo de um arquivo              ║
║ 3. Exibir atributos de um arquivo              ║
║ 4. Renomear um arquivo                         ║
║ 5. Apagar um arquivo                           ║
║ 6. Criar/Inserir um novo arquivo               ║
║ 0. Sair                                        ║
╚════════════════════════════════════════════════╝
```

### Exemplos de Uso

#### Listar arquivos no disco
1. Escolha opção 1
2. Veja a lista de arquivos e seus tamanhos

#### Ver conteúdo de um arquivo
1. Escolha opção 2
2. Digite o nome do arquivo (ex: `teste.txt`)
3. O conteúdo será exibido na tela

#### Renomear um arquivo
1. Escolha opção 4
2. Digite o nome atual (ex: `antigo.txt`)
3. Digite o novo nome (ex: `novo.txt`)
4. Formato válido: até 8 caracteres + extensão de até 3 caracteres

#### Adicionar um arquivo ao disco
1. Escolha opção 6
2. Digite o caminho do arquivo no sistema (ex: `/home/user/documento.txt`)
3. Digite o nome que terá no disco FAT16 (ex: `doc.txt`)
4. O arquivo será copiado para o disco

## 📐 Especificações Técnicas

### Estruturas FAT16 Implementadas

- **Boot Sector**: Informações básicas do sistema de arquivos
- **FAT (File Allocation Table)**: Cadeia de alocação de clusters
- **Diretório Raiz**: Entradas de arquivos e suas propriedades
- **Área de Dados**: Conteúdo dos arquivos organizados em clusters

### Características

- Suporte completo ao formato FAT16
- Manipulação correta de clusters encadeados
- Validação de nomes no formato 8.3
- Atualização automática de datas/horas
- Gerenciamento de espaço livre
- Tratamento de erros robusto

### Limitações (conforme especificação)

- Não suporta subdiretórios
- Opera apenas no diretório raiz
- Nomes de arquivo: formato 8.3 (8 caracteres + 3 de extensão)

## 🔍 Detalhes de Implementação

### Classe Principal: FAT16Manager

```cpp
class FAT16Manager {
    // Operações públicas
    bool initialize();                    // Carrega estruturas do disco
    void listFiles();                     // Lista arquivos
    void showFileContent(fileName);       // Mostra conteúdo
    void showFileAttributes(fileName);    // Mostra atributos
    bool renameFile(oldName, newName);    // Renomeia arquivo
    bool deleteFile(fileName);            // Remove arquivo
    bool createFile(source, dest);        // Cria novo arquivo
};
```

### Algoritmo de Criação de Arquivo

1. Valida nome e verifica se já existe
2. Abre arquivo fonte e obtém tamanho
3. Calcula clusters necessários
4. Aloca clusters livres na FAT
5. Encadeia clusters (cria linked list)
6. Escreve dados nos clusters
7. Cria entrada no diretório
8. Salva FAT e diretório no disco

### Algoritmo de Remoção de Arquivo

1. Localiza entrada do arquivo no diretório
2. Percorre cadeia de clusters na FAT
3. Marca cada cluster como livre
4. Marca entrada do diretório como deletada (0xE5)
5. Salva mudanças no disco

## 🧪 Testando o Programa

```bash
# Compilar
make

# Executar com disco de exemplo
./fat16manager disco1.img

# Testar operações básicas
# 1. Listar arquivos existentes
# 2. Ver conteúdo de um arquivo
# 3. Renomear um arquivo
# 4. Criar um arquivo de teste
# 5. Verificar se foi criado corretamente
# 6. Apagar o arquivo teste
```

## 📊 Distribuição de Pontos

| Operação | Peso | Status |
|----------|------|--------|
| Listar conteúdo do disco | 10% | ✅ Implementado |
| Listar conteúdo de arquivo | 10% | ✅ Implementado |
| Exibir atributos | 10% | ✅ Implementado |
| Renomear arquivo | 10% | ✅ Implementado |
| Apagar arquivo | 20% | ✅ Implementado |
| Criar arquivo | 40% | ✅ Implementado |
| **Total** | **100%** | ✅ **Completo** |

## 🛠️ Requisitos do Sistema

- **Compilador**: g++ com suporte a C++11 ou superior
- **Sistema Operacional**: Linux, macOS, Windows (com MinGW/Cygwin)
- **Ferramentas**: make (opcional, para usar Makefile)
- **Espaço**: Mínimo de 1.44 MB para imagens de disco

## 📝 Observações Importantes

1. **Backup**: Sempre faça backup da imagem de disco antes de realizar operações destrutivas
2. **Formato de nome**: Respeite o formato 8.3 do FAT16
3. **Espaço disponível**: Verifique se há espaço suficiente antes de adicionar arquivos grandes
4. **Permissões**: Certifique-se de ter permissão de leitura/escrita na imagem de disco

## 🐛 Solução de Problemas

### Erro ao abrir imagem de disco
- Verifique se o arquivo existe
- Confirme as permissões de leitura/escrita
- Use caminho absoluto se necessário

### Arquivo não encontrado
- Nomes são case-insensitive
- Use formato correto: `NOME.EXT`
- Verifique listagem com opção 1

### Disco cheio
- Use opção 1 para ver espaço usado
- Remova arquivos não utilizados
- Considere usar imagem de disco maior

## 👨‍💻 Desenvolvimento

Este projeto foi desenvolvido seguindo as especificações do sistema de arquivos FAT16, incluindo:

- Estruturas de dados alinhadas com `#pragma pack`
- Leitura/escrita binária com `fstream`
- Manipulação de bits para atributos e datas
- Alocação dinâmica de clusters
- Validação completa de entradas

## 📚 Referências

- Especificação FAT16 da Microsoft
- Documentação do sistema de arquivos FAT
- Material didático da disciplina de Sistemas Operacionais

---

**Desenvolvido como trabalho acadêmico - Sistemas Operacionais**
