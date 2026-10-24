import os
import re
import argparse
from collections import defaultdict
import matplotlib.pyplot as plt
import matplotlib.patches as patches


def get_job_name(job_id):
    """Converte J0 -> A, J1 -> B, ..., J25 -> Z, J26 -> AA, etc."""
    name = ""
    n = job_id
    while True:
        name = chr(65 + (n % 26)) + name
        n = n // 26 - 1
        if n < 0:
            break
    return name


def parse_txt(filepath):
    """Lê o arquivo de saída e extrai as tarefas e o makespan."""
    task_pattern = re.compile(
        r"Task\s+(\d+):\s+Machine\s+(\d+),\s+Duration\s+(\d+),\s+Job ID\s+(\d+),\s+Start Time\s+(\d+),\s+End Time\s+(\d+)"
    )
    makespan_pattern = re.compile(r"Maximo Makespan:\s+(\d+)")

    tasks = []
    makespan = None

    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            if makespan is None:
                m_match = makespan_pattern.search(line)
                if m_match:
                    makespan = int(m_match.group(1))

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

    if makespan is None and tasks:
        makespan = max(t['end'] for t in tasks)

    return tasks, makespan


def compute_critical_path(tasks, makespan, single_chain=True):
    """
    Identifica as operações pertencentes ao Caminho Crítico.
    - Se single_chain=True (padrão): retorna uma cadeia contínua e linear de operações
      sem bifurcações de caminhos concorrentes.
    - Se single_chain=False: retorna a união de todas as ramificações críticas possíveis.
    """
    task_dict = {(t['job_id'], t['task_id']): t for t in tasks}

    # Predecessores no mesmo job
    job_preds = {}
    job_tasks = defaultdict(list)
    for t in tasks:
        job_tasks[t['job_id']].append(t)
    for j_id, t_list in job_tasks.items():
        t_sorted = sorted(t_list, key=lambda x: x['task_id'])
        for i in range(1, len(t_sorted)):
            u = (j_id, t_sorted[i-1]['task_id'])
            v = (j_id, t_sorted[i]['task_id'])
            job_preds[v] = u

    # Predecessores na mesma máquina
    machine_preds = {}
    mach_tasks = defaultdict(list)
    for t in tasks:
        mach_tasks[t['machine']].append(t)
    for m_id, t_list in mach_tasks.items():
        t_sorted = sorted(t_list, key=lambda x: x['start'])
        for i in range(1, len(t_sorted)):
            u = (t_sorted[i-1]['job_id'], t_sorted[i-1]['task_id'])
            v = (t_sorted[i]['job_id'], t_sorted[i]['task_id'])
            machine_preds[v] = u

    # Tarefas que determinam o makespan
    end_ops = [(t['job_id'], t['task_id']) for t in tasks if t['end'] == makespan]
    if not end_ops:
        return set(), []

    if single_chain:
        # Seleciona uma cadeia contínua única e linear a partir da última tarefa crítica
        curr = end_ops[0]
        chain = [curr]
        while True:
            curr_info = task_dict[curr]
            st = curr_info['start']
            p_j = job_preds.get(curr)
            p_m = machine_preds.get(curr)

            valid_j = (p_j is not None and task_dict[p_j]['end'] == st)
            valid_m = (p_m is not None and task_dict[p_m]['end'] == st)

            # Prioriza precedência do Job para manter continuidade do fluxo
            next_node = p_j if valid_j else (p_m if valid_m else None)

            if next_node is None:
                break
            chain.append(next_node)
            curr = next_node

        ordered_chain = list(reversed(chain))
        return set(ordered_chain), ordered_chain
    else:
        # União de todas as ramificações críticas
        critical_ops = set(end_ops)
        queue = list(end_ops)
        visited = set(end_ops)

        while queue:
            curr = queue.pop(0)
            curr_info = task_dict[curr]
            start_t = curr_info['start']

            if curr in job_preds:
                p_job = job_preds[curr]
                if task_dict[p_job]['end'] == start_t and p_job not in visited:
                    critical_ops.add(p_job)
                    visited.add(p_job)
                    queue.append(p_job)

            if curr in machine_preds:
                p_mach = machine_preds[curr]
                if task_dict[p_mach]['end'] == start_t and p_mach not in visited:
                    critical_ops.add(p_mach)
                    visited.add(p_mach)
                    queue.append(p_mach)

        return critical_ops, []


def generate_gantt(filepath, output_dir, single_chain=True, show_arrows=False):
    """Gera, salva e exibe o gráfico de Gantt com o caminho crítico destacado."""
    tasks, makespan = parse_txt(filepath)
    if not tasks:
        print(f"Nenhuma tarefa encontrada no arquivo {filepath}.")
        return

    filename = os.path.basename(filepath)
    name_without_ext = os.path.splitext(filename)[0]

    # Identificar tarefas que pertencem ao caminho crítico
    critical_ops, ordered_chain = compute_critical_path(tasks, makespan, single_chain=single_chain)

    machines = sorted(list(set(t['machine'] for t in tasks)))
    jobs = sorted(list(set(t['job_id'] for t in tasks)))

    cmap = plt.get_cmap('tab20')
    colors = {job: cmap(i % 20) for i, job in enumerate(jobs)}

    fig, ax = plt.subplots(figsize=(18, 9))

    task_centers = {}

    for task in tasks:
        m = task['machine']
        start = task['start']
        duration = task['duration']
        job = task['job_id']
        t_id = task['task_id']

        is_critical = (job, t_id) in critical_ops

        # Destaque da borda para tarefas do caminho crítico
        edge_color = '#e74c3c' if is_critical else 'black'
        line_width = 3.0 if is_critical else 0.8
        alpha_val = 1.0 if is_critical else 0.75
        z_order = 4 if is_critical else 2

        ax.barh(
            y=m,
            width=duration,
            left=start,
            height=0.8,
            color=colors[job],
            edgecolor=edge_color,
            linewidth=line_width,
            alpha=alpha_val,
            zorder=z_order
        )

        job_letter = get_job_name(job)
        task_num = t_id + 1
        label = f"{job_letter}{task_num}"

        center_x = start + (duration / 2)
        center_y = m
        text_weight = 'bold' if is_critical else 'normal'
        ax.text(center_x, center_y, label, ha='center', va='center', color='black',
                fontsize=7.5, fontweight=text_weight, clip_on=True, zorder=5)

        if is_critical:
            task_centers[(job, t_id)] = (start, start + duration, m)

    # Traçar setas/linhas conectoras entre as tarefas do caminho crítico se solicitado
    if show_arrows and ordered_chain:
        for i in range(len(ordered_chain) - 1):
            u = ordered_chain[i]
            v = ordered_chain[i + 1]
            if u in task_centers and v in task_centers:
                _, u_end, u_m = task_centers[u]
                v_start, _, v_m = task_centers[v]
                arrow = patches.FancyArrowPatch(
                    (u_end, u_m), (v_start, v_m),
                    connectionstyle="arc3,rad=0.15" if u_m != v_m else "arc3,rad=0",
                    linestyle="-",
                    linewidth=2.0,
                    color="#c0392b",
                    arrowstyle="-|>",
                    mutation_scale=12,
                    zorder=6
                )
                ax.add_patch(arrow)

    # Configurações dos Eixos
    ax.set_yticks(machines)
    ax.set_yticklabels([f"Máquina {m}" for m in machines])
    ax.set_xlabel("Unidades de Tempo")
    ax.set_ylabel("Máquinas")
    title_suffix = f"({len(critical_ops)} tarefas críticas)"
    ax.set_title(f"Gráfico de Gantt - {name_without_ext} (Makespan: {makespan}) {title_suffix}", fontsize=13, fontweight='bold')

    ax.invert_yaxis()
    ax.grid(axis='x', linestyle='--', alpha=0.5)

    handles = [
        patches.Patch(facecolor='white', edgecolor='#e74c3c', linewidth=2.8,
                      label=f'Caminho Crítico ({len(critical_ops)} ops)')
    ]
    handles.extend([patches.Patch(color=colors[j], label=f"Job {get_job_name(j)}") for j in jobs])

    ax.legend(handles=handles, title="Legenda", bbox_to_anchor=(1.01, 1), loc='upper left')

    plt.tight_layout()

    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, f"{name_without_ext}.png")
    plt.savefig(output_path, dpi=300)
    print(f"Gráfico salvo com sucesso em: {output_path}")

    plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Gera o Gráfico de Gantt com destaque do Caminho Crítico.")
    parser.add_argument("filepath", help="Caminho para o arquivo .txt gerado pelo código em C")

    default_outdir = os.path.join(os.path.dirname(__file__), '..', 'out', 'gantt')
    parser.add_argument("--outdir", default=default_outdir, help="Diretório para salvar a imagem .png")
    parser.add_argument("--all-paths", action="store_true", help="Destaca a união de todas as ramificações críticas em vez de um caminho contínuo único")
    parser.add_argument("--show-arrows", action="store_true", help="Desenha setas conectoras vermelhas entre as tarefas críticas")

    args = parser.parse_args()

    filepath = os.path.abspath(args.filepath)
    outdir = os.path.abspath(args.outdir)

    generate_gantt(filepath, outdir, single_chain=not args.all_paths, show_arrows=args.show_arrows)