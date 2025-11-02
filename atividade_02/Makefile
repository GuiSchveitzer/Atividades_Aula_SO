# Makefile para o Gerenciador FAT16

# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2
LDFLAGS =

# Nome do executável
TARGET = fat16manager

# Arquivos fonte e objeto
SOURCES = main.cpp fat16.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = fat16.h

# Regra padrão
all: $(TARGET)

# Regra para criar o executável
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "Compilação concluída com sucesso!"
	@echo "Execute com: ./$(TARGET) <caminho_para_imagem_disco>"

# Regra para compilar arquivos .cpp em .o
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpeza de arquivos compilados
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Arquivos compilados removidos."

# Recompilar tudo do zero
rebuild: clean all

# Executar com disco1.img
run1: $(TARGET)
	./$(TARGET) disco1.img

# Executar com disco2.img
run2: $(TARGET)
	./$(TARGET) disco2.img

# Ajuda
help:
	@echo "Opções disponíveis:"
	@echo "  make          - Compila o projeto"
	@echo "  make clean    - Remove arquivos compilados"
	@echo "  make rebuild  - Recompila tudo do zero"
	@echo "  make run1     - Compila e executa com disco1.img"
	@echo "  make run2     - Compila e executa com disco2.img"
	@echo "  make help     - Exibe esta mensagem"

.PHONY: all clean rebuild run1 run2 help
