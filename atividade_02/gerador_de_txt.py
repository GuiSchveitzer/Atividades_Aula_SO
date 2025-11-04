import os

# Caminho onde o arquivo será salvo
diretorio = "C:/Users/Usuario/Downloads/atividade_02"  # Altere para o diretório que você deseja
nome_arquivo = "arquivo_grande.txt"

# Caminho completo do arquivo
caminho_completo = os.path.join(diretorio, nome_arquivo)

# Tamanho alvo do arquivo em bytes (300 MB)
tamanho_arquivo = 8 * 1024 * 1024  # 300 MB em bytes

# Texto que será repetido para gerar o arquivo
texto = "NESCAU > TODDY\n"  # Pode ser qualquer texto que você quiser

# Verifica se o diretório existe, se não, cria
os.makedirs(diretorio, exist_ok=True)

# Cria o arquivo de 4 MB no diretório especificado
with open(caminho_completo, "w") as f:
    # Calcular o número de repetições necessárias para alcançar o tamanho desejado
    while f.tell() < tamanho_arquivo:
        f.write(texto)
        # Print para verificar se o código está funcionando e progredindo
        if f.tell() % (50 * 1024 * 1024) == 0:  # A cada 50MB
            print(f"Arquivo está com {f.tell() / (1024 * 1024)} MB gerados...")

print(f"Arquivo gerado com sucesso em: {caminho_completo}")
