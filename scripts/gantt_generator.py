import os
import re
import argparse
import matplotlib.pyplot as plt
import matplotlib.patches as patches

def parse_txt(filepath):
    """Lê o arquivo de saída e extrai as informações das tarefas."""
    # Expressão regular para capturar as informações da linha
    # Exemplo da linha: Task 0: Machine 0, Duration 29, Job ID 0, Start Time 0, End Time 29
    task_pattern = re.compile(
        r"Task\s+(\d+):\s+Machine\s+(\d+),\s+Duration\s+(\d+),\s+Job ID\s+(\d+),\s+Start Time\s+(\d+),\s+End Time\s+(\d+)"
    )
    
    tasks = []
    with open(filepath, 'r') as f:
        for line in f:
            match = task_pattern.search(line)
            if match:
                tasks.append({
                    'task_id': int(match.group(1)),
                    'machine': int(match.group(2)),
                    'duration': int(match.group(3)),
                    'job_id': int(match.group(4)),
                    'start': int(match.group(5)),
                    'end': int(match.group(6))
                })
    return tasks

def generate_gantt(filepath, output_dir):
    """Gera, salva e exibe o gráfico de Gantt."""
    tasks = parse_txt(filepath)
    if not tasks:
        print(f"Nenhuma tarefa encontrada no arquivo {filepath}.")
        return
        
    filename = os.path.basename(filepath)
    name_without_ext = os.path.splitext(filename)[0]
    
    # Determina o número de máquinas e os jobs existentes
    machines = sorted(list(set(t['machine'] for t in tasks)))
    jobs = sorted(list(set(t['job_id'] for t in tasks)))
    
    # Mapeamento de cores (Paleta com 20 cores distintas)
    cmap = plt.get_cmap('tab20')
    colors = {job: cmap(i % 20) for i, job in enumerate(jobs)}
    
    # Configuração da figura
    fig, ax = plt.subplots(figsize=(16, 8))
    
    # Rastrear quais jobs já foram adicionados na legenda para não duplicar
    added_to_legend = set()
    
    for task in tasks:
        m = task['machine']
        start = task['start']
        duration = task['duration']
        job = task['job_id']
        t_id = task['task_id']
        
        # Desenha a barra da tarefa
        ax.barh(y=m, width=duration, left=start, height=0.8, color=colors[job], edgecolor='black', alpha=0.8)
        
        # Texto dentro da barra (J: Job, T: Task)
        center_x = start + (duration / 2)
        center_y = m
        ax.text(center_x, center_y, f"J{job}\nT{t_id}", ha='center', va='center', color='black', fontsize=7, clip_on=True)
        
        # Adiciona na legenda (apenas 1 vez por Job)
        if job not in added_to_legend:
            added_to_legend.add(job)
            
    # Configurações dos Eixos
    ax.set_yticks(machines)
    ax.set_yticklabels([f"Máquina {m}" for m in machines])
    ax.set_xlabel("Unidades de Tempo")
    ax.set_ylabel("Máquinas")
    ax.set_title(f"Gráfico de Gantt - {name_without_ext}")
    
    # Inverte o eixo Y para a Máquina 0 ficar no topo (padrão de cronogramas)
    ax.invert_yaxis() 
    ax.grid(axis='x', linestyle='--', alpha=0.5)
    
    # Criar e posicionar legenda fora do gráfico
    handles = [patches.Patch(color=colors[j], label=f"Job {j}") for j in jobs]
    ax.legend(handles=handles, title="Identificação dos Jobs", bbox_to_anchor=(1.01, 1), loc='upper left')
    
    # Ajusta o layout para a legenda não ficar cortada
    plt.tight_layout()
    
    # Garante que o diretório de saída exista e salva a imagem
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, f"{name_without_ext}.png")
    plt.savefig(output_path, dpi=300)
    print(f"Gráfico salvo com sucesso em: {output_path}")
    
    # Exibe o gráfico na tela
    plt.show()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Gera o Gráfico de Gantt a partir do .txt do escalonamento.")
    parser.add_argument("filepath", help="Caminho para o arquivo .txt gerado pelo código em C")
    
    # Como o script estará em scripts/, configuramos o padrão para salvar em ../out/gantt
    default_outdir = os.path.join(os.path.dirname(__file__), '..', 'out', 'gantt')
    parser.add_argument("--outdir", default=default_outdir, help="Diretório para salvar a imagem .png")
    
    args = parser.parse_args()
    
    # Transforma em caminhos absolutos e normaliza
    filepath = os.path.abspath(args.filepath)
    outdir = os.path.abspath(args.outdir)
    
    generate_gantt(filepath, outdir)