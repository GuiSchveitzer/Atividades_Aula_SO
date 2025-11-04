Windows: #Remove-Item -ErrorAction SilentlyContinue main.o, fat16.o, fat16manager.exe; Write-Host "Arquivos compilados removidos." -ForegroundColor Green

Linux: cd /workspaces/Atividades_Aula_SO/atividade_02 && rm -f main.o fat16.o fat16manager.exe fat16manager && echo "Arquivos compilados removidos."


cd /workspaces/Testes-Aula-SO/Trabalho_M2 && g++ -std=c++11 -Wall -Wextra -O2 -o fat16manager main.cpp fat16.cpp
g++ -std=c++11 -Wall -Wextra -O2 -o fat16manager.exe main.cpp fat16.cpp

.\fat16manager.exe disco2.img
./fat16manager disco1.img