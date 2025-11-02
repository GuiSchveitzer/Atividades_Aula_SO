#include "fat16.h"
#include <iostream>
#include <limits>
using namespace std;

void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

void showMenu() {
    cout << "\n-----------------------------------------------\n";
    cout << "|   GERENCIADOR DE SISTEMA DE ARQUIVOS FAT16     |\n";
    cout << "|------------------------------------------------|\n";
    cout << "| 1. Listar conteudo do disco                    |\n";
    cout << "| 2. Mostrar conteudo de um arquivo              |\n";
    cout << "| 3. Exibir atributos de um arquivo              |\n";
    cout << "| 4. Renomear um arquivo                         |\n";
    cout << "| 5. Apagar um arquivo                           |\n";
    cout << "| 6. Criar/Inserir um novo arquivo               |\n";
    cout << "| 0. Sair                                        |\n";
    cout << "|------------------------------------------------|\n";
    cout << "Escolha uma opçao: ";
}

int main(int argc, char* argv[]) {
    string imagePath;

    // Verificar se o caminho da imagem foi fornecido como argumento
    if (argc > 1) {
        imagePath = argv[1];
    } else {
        cout << "Digite o caminho para a imagem do disco FAT16: ";
        getline(cin, imagePath);
    }
    
    // Criar gerenciador FAT16
    FAT16Manager fat16(imagePath);
    
    // Inicializar
    if (!fat16.initialize()) {
        cerr << "\nFalha ao inicializar o sistema de arquivos FAT16." << endl;
        return 1;
    }

    cout << "\nSistema de arquivos FAT16 carregado com sucesso!\n";

    // Loop do menu
    int option;
    bool running = true;
    
    while (running) {
        showMenu();

        if (!(cin >> option)) {
            clearInputBuffer();
            cout << "\nOpcao invalida! Digite um numero.\n";
            continue;
        }
        clearInputBuffer();
        
        switch (option) {
            case 1: {
                // Listar conteúdo do disco
                fat16.listFiles();
                break;
            }
            
            case 2: {
                // Mostrar conteúdo de um arquivo
                string fileName;
                cout << "\nDigite o nome do arquivo: ";
                getline(cin, fileName);

                if (!fileName.empty()) {
                    fat16.showFileContent(fileName);
                } else {
                    cout << "Nome de arquivo invalido.\n";
                }
                break;
            }
            
            case 3: {
                // Exibir atributos de um arquivo
                string fileName;
                cout << "\nDigite o nome do arquivo: ";
                getline(cin, fileName);

                if (!fileName.empty()) {
                    fat16.showFileAttributes(fileName);
                } else {
                    cout << "Nome de arquivo inválido.\n";
                }
                break;
            }
            
            case 4: {
                // Renomear um arquivo
                string oldName, newName;
                cout << "\nDigite o nome atual do arquivo: ";
                getline(cin, oldName);
                cout << "Digite o novo nome do arquivo: ";
                getline(cin, newName);

                if (!oldName.empty() && !newName.empty()) {
                    fat16.renameFile(oldName, newName);
                } else {
                    cout << "Nomes de arquivo inválidos.\n";
                }
                break;
            }
            
            case 5: {
                // Apagar um arquivo
                string fileName;
                cout << "\nDigite o nome do arquivo a ser apagado: ";
                getline(cin, fileName);

                if (!fileName.empty()) {
                    cout << "Tem certeza que deseja apagar '" << fileName << "'? (s/n): ";
                    char confirm;
                    cin >> confirm;
                    clearInputBuffer();
                    
                    if (confirm == 's' || confirm == 'S') {
                        fat16.deleteFile(fileName);
                    } else {
                        cout << "Operação cancelada.\n";
                    }
                } else {
                    cout << "Nome de arquivo inválido.\n";
                }
                break;
            }
            
            case 6: {
                // Criar/Inserir um novo arquivo
                string sourcePath, destName;
                cout << "\nDigite o caminho do arquivo externo: ";
                getline(cin, sourcePath);
                cout << "Digite o nome do arquivo no disco FAT16: ";
                getline(cin, destName);

                if (!sourcePath.empty() && !destName.empty()) {
                    fat16.createFile(sourcePath, destName);
                } else {
                    cout << "Caminhos inválidos.\n";
                }
                break;
            }
            
            case 0: {
                // Sair
                cout << "\nEncerrando o programa...\n";
                running = false;
                break;
            }
            
            default: {
                cerr << "\nOpção inválida! Escolha uma opção entre 0 e 6.\n";
                break;
            }
        }
        
        if (running && option != 0) {
            cout << "\nPressione ENTER para continuar...";
            cin.get();
        }
    }
    
    return 0;
}
