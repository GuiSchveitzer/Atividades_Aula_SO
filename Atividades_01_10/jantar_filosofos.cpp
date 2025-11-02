#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <atomic>

using namespace std;

enum Estado { PENSANDO, COM_FOME, COMENDO };

struct Filosofo {
    int id;
    Estado estado;
    int refeicoes;
};

int N; // número de filósofos
int duracaoSimulacao; // em segundos
int tempoPensarMin, tempoPensarMax;
int tempoComerMin, tempoComerMax;

vector<mutex> garfos;     // um mutex para cada garfo
vector<Filosofo> filosofos;
atomic<bool> rodando(true);

// Função auxiliar para gerar tempo aleatório entre min e max (ms)
int tempoAleatorio(int min, int max) {
    return min + rand() % (max - min + 1);
}

void pensar(int id) {
    filosofos[id].estado = PENSANDO;
    this_thread::sleep_for(chrono::milliseconds(tempoAleatorio(tempoPensarMin, tempoPensarMax)));
}

void comer(int id) {
    filosofos[id].estado = COMENDO;
    this_thread::sleep_for(chrono::milliseconds(tempoAleatorio(tempoComerMin, tempoComerMax)));
    filosofos[id].refeicoes++;
}

// Lógica do filósofo
void filosofo(int id) {
    int esquerda = id;
    int direita = (id + 1) % N;

    while (rodando) {
        pensar(id);

        filosofos[id].estado = COM_FOME;

        // Estratégia: sempre pegar o garfo de menor índice primeiro (para evitar deadlock)
        int primeiro = min(esquerda, direita);
        int segundo = max(esquerda, direita);

        unique_lock<mutex> lock1(garfos[primeiro]);
        unique_lock<mutex> lock2(garfos[segundo]);

        comer(id);
    }
}

// Impressão do estado do sistema
void monitor() {
    using namespace chrono_literals;
    while (rodando) {
        this_thread::sleep_for(1s);

        // Estado dos garfos
        cout << "Garfos: ";
        for (int i = 0; i < N; i++) {
            if (garfos[i].try_lock()) {
                cout << "[O]";
                garfos[i].unlock();
            } else {
                cout << "[X]";
            }
        }
        cout << endl;

        // Estado dos filósofos
        for (int i = 0; i < N; i++) {
            string st;
            if (filosofos[i].estado == PENSANDO) st = "PENS";
            else if (filosofos[i].estado == COM_FOME) st = "FOME";
            else st = "COME";

            cout << "F" << i << ":" << st << " | ";
        }
        cout << endl;

        // Contadores
        for (int i = 0; i < N; i++) {
            cout << "F" << i << " comeu: " << filosofos[i].refeicoes << " | ";
        }
        cout << endl << "----------------------------------------" << endl;
    }
}

int main() {
    // Inicializa gerador de números aleatórios
    srand(time(nullptr));
    
    // Entrada de parâmetros
    cout << "Digite o numero de filosofos: ";
    cin >> N;
    cout << "Digite a duracao da simulacao (s): ";
    cin >> duracaoSimulacao;
    cout << "Digite tempo de pensar (min ms max ms): ";
    cin >> tempoPensarMin >> tempoPensarMax;
    cout << "Digite tempo de comer (min ms max ms): ";
    cin >> tempoComerMin >> tempoComerMax;

    // Inicializa estruturas
    garfos = vector<mutex>(N);
    filosofos.resize(N);
    for (int i = 0; i < N; i++) {
        filosofos[i] = {i, PENSANDO, 0};
    }

    // Cria threads
    vector<thread> threads;
    for (int i = 0; i < N; i++) {
        threads.emplace_back(filosofo, i);
    }
    thread tmonitor(monitor);

    // Roda por tempo determinado
    this_thread::sleep_for(chrono::seconds(duracaoSimulacao));
    rodando = false;

    // Aguarda encerramento
    for (auto &t : threads) t.join();
    tmonitor.join();

    // Estatísticas finais
    cout << "\nResumo Final:\n";
    for (int i = 0; i < N; i++) {
        cout << "Filosofo " << i << " comeu " << filosofos[i].refeicoes << " vezes.\n";
    }

    return 0;
}