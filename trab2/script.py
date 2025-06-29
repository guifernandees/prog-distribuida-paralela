#!/usr/bin/env python3
"""
Script de automação de experimentos para MPI Matrix Multiplication
utilizando códigos instrumentados (*.c -> *_inst). Gera um CSV com tempos
total, de comunicação e de computação para cada versão, tamanho de matriz e
número de processos.
"""

import os
import subprocess
import csv
import re
import sys

# 1) Trava OpenMP a uma thread
os.environ['OMP_NUM_THREADS'] = '1'

# 2) Coleta SLURM_JOB_ID e gera machinefile
job_id = os.environ.get('SLURM_JOB_ID')
if not job_id:
    print("Erro: SLURM_JOB_ID não encontrado. Rode este script dentro de um job SLURM.", file=sys.stderr)
    sys.exit(1)

machinefile = f"nodes.{job_id}"
try:
    # Criar machinefile com lista de nós alocados
    subprocess.run(
        f"srun -l hostname | sort -n | awk '{{print $2}}' > {machinefile}",
        shell=True,
        check=True
    )
    print(f"Machinefile gerado em: {machinefile}")
except subprocess.CalledProcessError as e:
    print(f"Erro ao gerar machinefile: {e}", file=sys.stderr)
    sys.exit(1)

# 3) Mapeamento das versões instrumentadas
executables = {
    'collective':     './mpi_coletiva',
    'p2p_block':      './mpi_p2p_bloqueante',
    'p2p_nb':         './mpi_p2p_naobloqueante'
}

# 4) Configurações de teste
matrix_sizes   = [500, 1000, 2000, 4000]
process_counts = [2, 4, 8]

# 5) Arquivo de saída e regex de parsing
output_csv   = 'resultados_desempenho.csv'
time_pattern = re.compile(
    r"Total exec:\s*([0-9.]+)\s+Comm:\s*([0-9.]+)\s+Comp:\s*([0-9.]+)"
)

# 6) Cabeçalho do CSV
with open(output_csv, mode='w', newline='') as csvfile:
    writer = csv.writer(csvfile)
    writer.writerow(['version', 'N', 'p', 'total_time', 'comm_time', 'comp_time'])

    # 7) Loop de experimentos
    for version, exe in executables.items():
        for N in matrix_sizes:
            for p in process_counts:
                cmd = [
                    'mpirun',
                    '-np', str(p),
                    '-machinefile', machinefile,
                    '--mca', '^openib',
                    '--mca', 'btl_tcp_if_include', 'eno2',
                    '--bind-to', 'none',
                    exe, str(N)
                ]
                print(f"[{version}] N={N:4d}, p={p:2d}  →  cmd: {' '.join(cmd)}")
                try:
                    output = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
                except subprocess.CalledProcessError as e:
                    print(f"  ERRO ao executar: {e.output.decode().strip()}", file=sys.stderr)
                    continue

                # 8) Extrai tempos da saída
                text = output.decode()
                m = time_pattern.search(text)
                if not m:
                    print(f"  ERRO parsing tempos:\n{text}", file=sys.stderr)
                    continue

                total_time, comm_time, comp_time = m.groups()
                writer.writerow([version, N, p, total_time, comm_time, comp_time])

print(f"\n✓ Resultados gravados em '{output_csv}'")
