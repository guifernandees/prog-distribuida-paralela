#!/usr/bin/env python3
"""
Script de automação de experimentos para MPI Matrix Multiplication
Gera um CSV com tempos total, de comunicação e de computação
para cada versão do algoritmo, tamanho de matriz e número de processos.
"""
import subprocess
import csv
import re
import sys

# Caminhos para os executáveis MPI (ajuste conforme seu diretório)
executables = {
    'collective': './mpi_coletiva',
    'p2p_block': './mpi_p2p_bloqueante',
    'p2p_nb': './mpi_p2p_naobloqueante'
}

# Tamanhos de matriz e número de processos a testar
matrix_sizes = [500, 1000, 2000, 4000]
process_counts = [2, 4, 8]

# Arquivo de saída
output_csv = 'resultados_desempenho.csv'

# Regex para extrair tempos: "Total exec: X  Comm: Y  Comp: Z"
time_pattern = re.compile(
    r"Total exec:\s*([0-9.]+)\s+Comm:\s*([0-9.]+)\s+Comp:\s*([0-9.]+)"
)

with open(output_csv, mode='w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(['version', 'N', 'p', 'total_time', 'comm_time', 'comp_time'])

    for version, exe in executables.items():
        for N in matrix_sizes:
            for p in process_counts:
                cmd = ['mpirun', '-np', str(p), exe, str(N)]
                print(f"Executando {version}, N={N}, p={p}...")
                try:
                    output = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
                except subprocess.CalledProcessError as e:
                    print(f"Erro ao executar {cmd}: {e.output.decode()}", file=sys.stderr)
                    continue

                text = output.decode()
                match = time_pattern.search(text)
                if not match:
                    print(f"Não foi possível extrair tempos de:\n{text}", file=sys.stderr)
                    continue

                total_time, comm_time, comp_time = match.groups()
                writer.writerow([version, N, p, total_time, comm_time, comp_time])

print(f"Resultados gravados em '{output_csv}'")
