#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <atomic>

using namespace std;

enum EstadoBarbeiro { DORME, ATENDE };
enum EstadoCliente { ENTRA, AGUARDA, DESISTE, ATENDIDO };

struct Cliente {
    int id;
    EstadoCliente estado;
};

// Parâmetros globais
int numCadeiras;
int taxaChegadaMin, taxaChegadaMax;  // tempo entre chegadas (ms)
int tempoAtendimentoMin, tempoAtendimentoMax;  // tempo de atendimento (ms)
int duracaoSimulacao;  // em segundos

// Estado da barbearia
EstadoBarbeiro estadoBarbeiro = DORME;
queue<Cliente> filaEspera;
atomic<int> proximoClienteId(1);
atomic<bool> rodando(true);

// Contadores
atomic<int> clientesAtendidos(0);
atomic<int> clientesDesistentes(0);
atomic<int> totalClientesChegaram(0);

// Sincronização
mutex mutexFila;
condition_variable cvBarbeiro;  // para acordar o barbeiro
condition_variable cvCliente;   // para clientes esperarem

// Função auxiliar para gerar tempo aleatório
int tempoAleatorio(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Thread do barbeiro
void barbeiro() {
    while (rodando) {
        unique_lock<mutex> lock(mutexFila);
        
        // Se não há clientes, barbeiro dorme
        if (filaEspera.empty()) {
            estadoBarbeiro = DORME;
            cvBarbeiro.wait(lock, [&] { return !filaEspera.empty() || !rodando; });
        }
        
        if (!rodando) break;
        
        // Barbeiro acorda e atende cliente
        if (!filaEspera.empty()) {
            estadoBarbeiro = ATENDE;
            Cliente cliente = filaEspera.front();
            filaEspera.pop();
            
            lock.unlock();  // Libera o mutex enquanto atende
            
            // Simula tempo de atendimento
            this_thread::sleep_for(chrono::milliseconds(
                tempoAleatorio(tempoAtendimentoMin, tempoAtendimentoMax)
            ));
            
            clientesAtendidos++;
        }
    }
}

// Thread geradora de clientes
void gerarClientes() {
    while (rodando) {
        // Aguarda tempo entre chegadas
        this_thread::sleep_for(chrono::milliseconds(
            tempoAleatorio(taxaChegadaMin, taxaChegadaMax)
        ));
        
        if (!rodando) break;
        
        // Novo cliente chega
        Cliente novoCliente = {proximoClienteId++, ENTRA};
        totalClientesChegaram++;
        
        unique_lock<mutex> lock(mutexFila);
        
        // Verifica se há lugar na fila
        if (filaEspera.size() < numCadeiras) {
            novoCliente.estado = AGUARDA;
            filaEspera.push(novoCliente);
            
            // Acorda o barbeiro se estiver dormindo
            cvBarbeiro.notify_one();
        } else {
            // Sala lotada - cliente desiste
            novoCliente.estado = DESISTE;
            clientesDesistentes++;
        }
    }
}

// Thread do monitor (exibe estado)
void monitor() {
    while (rodando) {
        this_thread::sleep_for(chrono::seconds(1));
        
        unique_lock<mutex> lock(mutexFila);
        
        // Estado do barbeiro
        cout << "Barbeiro: " << (estadoBarbeiro == DORME ? "DORME" : "ATENDE") << endl;
        
        // Estado da fila
        cout << "Fila [";
        for (int i = 0; i < numCadeiras; i++) {
            if (i < filaEspera.size()) {
                cout << "#";
            } else {
                cout << ".";
            }
        }
        cout << "] (" << filaEspera.size() << "/" << numCadeiras << ")" << endl;
        
        // Contadores
        cout << "Atendidos: " << clientesAtendidos << " | ";
        cout << "Desistentes: " << clientesDesistentes << " | ";
        cout << "Em espera: " << filaEspera.size() << " | ";
        cout << "Total chegaram: " << totalClientesChegaram << endl;
        cout << "----------------------------------------" << endl;
        
        lock.unlock();
    }
}

int main() {
    // Inicializa gerador de números aleatórios
    srand(time(nullptr));
    
    // Entrada de parâmetros
    cout << "Digite o numero de cadeiras de espera: ";
    cin >> numCadeiras;
    cout << "Digite taxa de chegada de clientes (min ms max ms): ";
    cin >> taxaChegadaMin >> taxaChegadaMax;
    cout << "Digite tempo de atendimento (min ms max ms): ";
    cin >> tempoAtendimentoMin >> tempoAtendimentoMax;
    cout << "Digite a duracao da simulacao (s): ";
    cin >> duracaoSimulacao;
    
    cout << "\n=== Simulacao da Barbearia ===\n" << endl;
    
    // Cria threads
    thread tBarbeiro(barbeiro);
    thread tGerador(gerarClientes);
    thread tMonitor(monitor);
    
    // Roda por tempo determinado
    this_thread::sleep_for(chrono::seconds(duracaoSimulacao));
    rodando = false;
    
    // Acorda todas as threads para finalizar
    cvBarbeiro.notify_all();
    cvCliente.notify_all();
    
    // Aguarda encerramento
    tBarbeiro.join();
    tGerador.join();
    tMonitor.join();
    
    // Estatísticas finais
    cout << "\n=== Resumo Final ===" << endl;
    cout << "Total de clientes que chegaram: " << totalClientesChegaram << endl;
    cout << "Clientes atendidos: " << clientesAtendidos << endl;
    cout << "Clientes que desistiram: " << clientesDesistentes << endl;
    cout << "Taxa de atendimento: " << 
        (totalClientesChegaram > 0 ? 
         (clientesAtendidos * 100.0 / totalClientesChegaram) : 0) 
        << "%" << endl;
    
    return 0;
}
