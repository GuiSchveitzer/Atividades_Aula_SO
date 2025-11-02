# Relatório: Implementação de Problemas Clássicos de Sistemas Operacionais

## Sumário Executivo

Este relatório apresenta a implementação de dois problemas clássicos de sincronização em Sistemas Operacionais: **Jantar dos Filósofos** e **Barbeiro Dorminhoco**. Ambos demonstram conceitos fundamentais de programação concorrente, sincronização de threads e prevenção de deadlocks.

---

## 1. PROBLEMA DO JANTAR DOS FILÓSOFOS

### 1.1 Contexto do Problema

O problema do Jantar dos Filósofos, proposto por Dijkstra, ilustra desafios clássicos em sistemas concorrentes:
- **Compartilhamento de recursos limitados** (garfos)
- **Prevenção de deadlock**
- **Garantia de progresso** para todas as threads
- **Exclusão mútua** no acesso aos recursos

### 1.2 Arquitetura da Solução

#### **Modelo de Threading:**
```cpp
// Uma thread por filósofo + thread monitor
vector<thread> threads;
for (int i = 0; i < N; i++) {
    threads.emplace_back(filosofo, i);
}
thread tmonitor(monitor);
```

**Análise SO:** Cada filósofo é uma **thread independente**, simulando processos concorrentes que competem por recursos compartilhados.

#### **Representação de Recursos:**
```cpp
vector<mutex> garfos;     // N garfos = N mutex
```

**Análise SO:** Cada garfo é representado por um **mutex**, implementando **exclusão mútua** no acesso ao recurso. Apenas uma thread pode "segurar" um garfo por vez.

### 1.3 Solução do Deadlock

#### **Problema Clássico:**
```
Filósofo 0: pega garfo 0, espera garfo 1
Filósofo 1: pega garfo 1, espera garfo 2
...
Filósofo N-1: pega garfo N-1, espera garfo 0
```
**Resultado:** Deadlock circular - todos esperando indefinidamente.

#### **Solução Implementada:**
```cpp
// Estratégia: sempre pegar garfo de menor índice primeiro
int primeiro = min(esquerda, direita);
int segundo = max(esquerda, direita);

unique_lock<mutex> lock1(garfos[primeiro]);
unique_lock<mutex> lock2(garfos[segundo]);
```

**Análise SO:** Esta é uma implementação da **ordenação de recursos**, uma técnica fundamental para prevenção de deadlock. Garante que todos os filósofos adquirem recursos na mesma ordem, eliminando dependências circulares.

### 1.4 Mecanismos de Sincronização

#### **Controle de Execução:**
```cpp
atomic<bool> rodando(true);
```

**Análise SO:** `atomic<bool>` garante **operações atômicas** em arquiteturas multi-core, prevenindo race conditions no controle de finalização. Essencial para **cache coherency** entre CPUs.

#### **Gestão de Estado:**
```cpp
enum Estado { PENSANDO, COM_FOME, COMENDO };
filosofos[id].estado = COM_FOME;
```

**Análise SO:** Implementa uma **máquina de estados finitos** para cada thread, similar ao gerenciamento de estados de processos no SO (ready, running, blocked).

### 1.5 Monitoramento Non-blocking

```cpp
if (garfos[i].try_lock()) {
    cout << "[O]";  // Garfo livre
    garfos[i].unlock();
} else {
    cout << "[X]";  // Garfo ocupado
}
```

**Análise SO:** `try_lock()` implementa **polling não-bloqueante**, permitindo observação do sistema sem interferir na execução. Técnica comum em sistemas de monitoramento.

### 1.6 Resultados e Métricas

**Exclusão Mútua:** ✅ Verificada - nunca dois filósofos adjacentes comem simultaneamente
**Progresso:** ✅ Todos os filósofos conseguem comer (F0:21, F1:20, F2:20 refeições)
**Equilíbrio:** ✅ Distribuição justa de recursos
**Livre de Deadlock:** ✅ Ordenação de recursos previne dependências circulares

---

## 2. PROBLEMA DO BARBEIRO DORMINHOCO

### 2.1 Contexto do Problema

O Barbeiro Dorminhoco modela um sistema **produtor-consumidor** com:
- **Buffer limitado** (cadeiras de espera)
- **Sincronização assimétrica** (barbeiro vs. clientes)
- **Gestão de capacidade** e overflow
- **Comunicação inter-threads**

### 2.2 Arquitetura da Solução

#### **Modelo de Threading:**
```cpp
thread tBarbeiro(barbeiro);      // Thread consumidora
thread tGerador(gerarClientes);  // Thread produtora
thread tMonitor(monitor);        // Thread observadora
```

**Análise SO:** Implementa o padrão **produtor-consumidor** clássico:
- **Produtor:** Gera clientes (simula chegadas)
- **Consumidor:** Barbeiro atende clientes
- **Buffer:** Fila de espera com capacidade limitada

#### **Buffer Compartilhado:**
```cpp
queue<Cliente> filaEspera;
mutex mutexFila;
```

**Análise SO:** Fila protegida por mutex implementa um **buffer thread-safe**, garantindo integridade dos dados em operações concorrentes de inserção/remoção.

### 2.3 Sincronização com Condition Variables

#### **Problema de Sincronização:**
Como o barbeiro sabe quando há clientes? Como evitar busy-waiting?

#### **Solução Implementada:**
```cpp
// Barbeiro dorme quando não há clientes
if (filaEspera.empty()) {
    estadoBarbeiro = DORME;
    cvBarbeiro.wait(lock, [&] { return !filaEspera.empty() || !rodando; });
}

// Cliente acorda barbeiro
filaEspera.push(novoCliente);
cvBarbeiro.notify_one();
```

**Análise SO:** 
- **`condition_variable`** implementa **wait/notify semantics**, evitando polling desnecessário
- **Barbeiro bloqueia** até ser notificado (economia de CPU)
- **Predicate lambda** previne **spurious wakeups**
- Similar ao mecanismo de **semáforos** em SOs reais

### 2.4 Gestão de Recursos e Overflow

```cpp
if (filaEspera.size() < numCadeiras) {
    filaEspera.push(novoCliente);
    cvBarbeiro.notify_one();
} else {
    clientesDesistentes++;  // Buffer overflow
}
```

**Análise SO:** Implementa **controle de admissão**, técnica fundamental em sistemas com recursos limitados. Simula comportamento real de buffers em SOs quando há sobrecarga.

### 2.5 Liberação Estratégica de Locks

```cpp
unique_lock<mutex> lock(mutexFila);
Cliente cliente = filaEspera.front();
filaEspera.pop();

lock.unlock();  // Libera antes de atender

// Atendimento sem bloquear outras operações
this_thread::sleep_for(chrono::milliseconds(...));
```

**Análise SO:** **Liberação antecipada de mutex** maximiza concorrência. Permite que:
- Novos clientes cheguem durante atendimento
- Monitor continue funcionando
- Sistema não trave durante operações longas

Técnica essencial para **throughput** em sistemas concorrentes.

### 2.6 Métricas e Observabilidade

```cpp
cout << "Fila [###..] (" << filaEspera.size() << "/" << numCadeiras << ")" << endl;
cout << "Atendidos: " << clientesAtendidos << " | Desistentes: " << clientesDesistentes;
```

**Análise SO:** Sistema de **telemetria em tempo real** similar a ferramentas de monitoramento de SO (htop, iostat). Permite análise de:
- **Utilização de recursos** (ocupação da fila)
- **Taxa de throughput** (clientes atendidos)
- **Taxa de perda** (clientes desistentes)

---

## 3. COMPARAÇÃO ENTRE SOLUÇÕES

| Aspecto | Jantar dos Filósofos | Barbeiro Dorminhoco |
|---------|---------------------|-------------------|
| **Padrão** | Recursos Compartilhados | Produtor-Consumidor |
| **Threads** | N filósofos + monitor | 1 barbeiro + 1 gerador + monitor |
| **Sincronização** | Mutex (exclusão mútua) | Mutex + Condition Variables |
| **Desafio Principal** | Prevenção de deadlock | Comunicação eficiente |
| **Técnica Anti-deadlock** | Ordenação de recursos | N/A (sem dependência circular) |
| **Controle de Capacidade** | Implícito (N garfos) | Explícito (buffer limitado) |

---

## 4. CONCEITOS DE SO DEMONSTRADOS

### 4.1 Concorrência e Paralelismo
- **Multi-threading** com `std::thread`
- **Operações atômicas** com `std::atomic`
- **Exclusão mútua** com `std::mutex`

### 4.2 Sincronização
- **Locks** com `unique_lock` (RAII pattern)
- **Condition variables** para wait/notify
- **Prevenção de deadlock** por ordenação

### 4.3 Gestão de Recursos
- **Buffer management** (filas limitadas)
- **Resource allocation** (garfos/cadeiras)
- **Overflow handling** (clientes desistentes)

### 4.4 Observabilidade
- **Non-blocking monitoring** com `try_lock`
- **Real-time metrics** e telemetria
- **State visualization** (estados das threads)

---

## 5. CONCLUSÕES

### 5.1 Eficácia das Soluções
Ambas as implementações demonstram **soluções robustas** para problemas clássicos de SO:
- ✅ **Livre de deadlocks**
- ✅ **Garantia de progresso**
- ✅ **Uso eficiente de recursos**
- ✅ **Observabilidade completa**

### 5.2 Aplicabilidade Prática
Os padrões implementados são **diretamente aplicáveis** em:
- **Sistemas de banco de dados** (lock ordering)
- **Servidores web** (connection pooling)
- **Sistemas embarcados** (resource management)
- **Aplicações real-time** (producer-consumer)

### 5.3 Lessons Learned
1. **Ordenação de recursos** é fundamental para prevenir deadlocks
2. **Condition variables** são superiores a busy-waiting
3. **Liberação antecipada de locks** maximiza throughput
4. **Monitoramento não-bloqueante** é essencial para observabilidade

---

**Palavras-chave:** Sistemas Operacionais, Concorrência, Sincronização, Deadlock, Mutex, Condition Variables, Producer-Consumer, Threading, Resource Management.