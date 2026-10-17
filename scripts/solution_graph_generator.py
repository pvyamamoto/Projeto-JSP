"""
solution_graph_generator.py
Gera o grafo direcionado da solução D_S = (N, A U S) com destaque do Caminho Crítico.
- Apenas as conexões do Caminho Crítico são desenhadas em formato de 'arco' curvo e vermelho.
- O sequenciamento de operações não críticas nas máquinas é desenhado em linha tracejada.
- Setas utilizam patchA/patchB para parar precisamente na borda externa dos nós sem invadi-los.
"""

import os
import re
import argparse
from collections import defaultdict
import matplotlib.pyplot as plt
import matplotlib.patches as patches


def parse_solution(filepath):
    """Extrai os dados de agendamento das tarefas e o makespan."""
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

            t_match = task_pattern.search(line)
            if t_match:
                tasks.append({
                    'task_id': int(t_match.group(1)),
                    'machine': int(t_match.group(2)),
                    'duration': int(t_match.group(3)),
                    'job_id': int(t_match.group(4)),
                    'start': int(t_match.group(5)),
                    'end': int(t_match.group(6))
                })

    if makespan is None and tasks:
        makespan = max(t['end'] for t in tasks)

    return tasks, makespan


def compute_critical_path(tasks, makespan):
    """Identifica nós e arestas pertencentes ao Caminho Crítico (Makespan)."""
    task_dict = {f"J{t['job_id']}_T{t['task_id']}": t for t in tasks}

    critical_nodes = set()
    critical_edges = set()

    end_ops = [f"J{t['job_id']}_T{t['task_id']}" for t in tasks if t['end'] == makespan]
    for op in end_ops:
        critical_nodes.add(op)
        critical_edges.add((op, 'F'))

    queue = list(end_ops)
    visited = set(end_ops)

    job_preds = {}
    machine_preds = {}

    # Predecessor no mesmo job
    job_tasks = defaultdict(list)
    for t in tasks:
        job_tasks[t['job_id']].append(t)
    for j_id, t_list in job_tasks.items():
        t_sorted = sorted(t_list, key=lambda x: x['task_id'])
        for i in range(1, len(t_sorted)):
            u = f"J{j_id}_T{t_sorted[i-1]['task_id']}"
            v = f"J{j_id}_T{t_sorted[i]['task_id']}"
            job_preds[v] = u

    # Predecessor na mesma máquina
    mach_tasks = defaultdict(list)
    for t in tasks:
        mach_tasks[t['machine']].append(t)
    for m_id, t_list in mach_tasks.items():
        t_sorted = sorted(t_list, key=lambda x: x['start'])
        for i in range(1, len(t_sorted)):
            u = f"J{t_sorted[i-1]['job_id']}_T{t_sorted[i-1]['task_id']}"
            v = f"J{t_sorted[i]['job_id']}_T{t_sorted[i]['task_id']}"
            machine_preds[v] = u

    while queue:
        curr = queue.pop(0)
        curr_info = task_dict[curr]
        start_t = curr_info['start']

        # Predecessor no Job
        if curr in job_preds:
            p_job = job_preds[curr]
            p_info = task_dict[p_job]
            if p_info['end'] == start_t:
                critical_edges.add((p_job, curr))
                critical_nodes.add(p_job)
                if p_job not in visited:
                    visited.add(p_job)
                    queue.append(p_job)

        # Predecessor na Máquina
        if curr in machine_preds:
            p_mach = machine_preds[curr]
            p_info = task_dict[p_mach]
            if p_info['end'] == start_t:
                critical_edges.add((p_mach, curr))
                critical_nodes.add(p_mach)
                if p_mach not in visited:
                    visited.add(p_mach)
                    queue.append(p_mach)

        if curr_info['task_id'] == 0 and start_t == 0:
            critical_edges.add(('S', curr))

    return critical_nodes, critical_edges


def generate_solution_graph(filepath, output_dir, x_spacing=4.6, y_spacing=2.6, fig_scale=1.0, show_plot=True):
    tasks, makespan = parse_solution(filepath)
    if not tasks:
        print(f"[ERRO] Nenhuma tarefa encontrada no arquivo {filepath}.")
        return

    filename = os.path.basename(filepath)
    name_without_ext = os.path.splitext(filename)[0]

    num_jobs = max(t['job_id'] for t in tasks) + 1
    num_machines = max(t['machine'] for t in tasks) + 1
    num_tasks_per_job = max(t['task_id'] for t in tasks) + 1

    # Ajuste dinâmico de largura para evitar imagens estreitas quando há muitos jobs
    eff_x_spacing = max(x_spacing, (num_jobs * y_spacing * 0.9) / (num_tasks_per_job + 2))
    fig_width = max(26.0, (num_tasks_per_job + 2) * eff_x_spacing * 0.9) * fig_scale
    fig_height = max(12.0, (num_jobs + 1) * y_spacing * 0.85) * fig_scale
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))

    cmap = plt.get_cmap('tab10' if num_machines <= 10 else 'tab20')
    machine_colors = {m: cmap(m % (10 if num_machines <= 10 else 20)) for m in range(num_machines)}

    # Posições dos nós
    node_positions = {}
    start_x = 0.0
    end_x = (num_tasks_per_job + 1) * eff_x_spacing
    mid_y = ((num_jobs - 1) * y_spacing) / 2.0

    start_pos = (start_x, mid_y)
    end_pos = (end_x, mid_y)
    node_positions['S'] = start_pos
    node_positions['F'] = end_pos

    for t in tasks:
        node_id = f"J{t['job_id']}_T{t['task_id']}"
        x = (t['task_id'] + 1) * eff_x_spacing
        y = (num_jobs - 1 - t['job_id']) * y_spacing
        node_positions[node_id] = (x, y)

    node_radius = 0.76

    # Identificar Caminho Crítico
    critical_nodes, critical_edges = compute_critical_path(tasks, makespan)

    # Criar patches dos nós primeiro (essenciais para calcular o recorte exato das setas)
    node_patches = {}

    # Nós Dummy S e F
    for node_id, label in [('S', 'INÍCIO\n(S)'), ('F', f'FIM\n(F)\nMk={makespan}')]:
        pos = node_positions[node_id]
        circle = plt.Circle(pos, node_radius, facecolor='#ecf0f1', edgecolor='#e74c3c' if node_id == 'F' else '#2c3e50',
                            linewidth=2.8, zorder=6)
        ax.add_patch(circle)
        node_patches[node_id] = circle
        ax.text(pos[0], pos[1], label, ha='center', va='center', fontsize=9.5, fontweight='bold', color='#2c3e50', zorder=7)

    # Nós das Tarefas
    for t in tasks:
        node_id = f"J{t['job_id']}_T{t['task_id']}"
        pos = node_positions[node_id]
        m = t['machine']
        d = t['duration']
        st = t['start']
        et = t['end']
        color = machine_colors[m]

        is_crit = node_id in critical_nodes
        edge_c = '#e74c3c' if is_crit else '#1a252f'
        edge_w = 3.2 if is_crit else 1.8

        circle = plt.Circle(pos, node_radius, facecolor=color, edgecolor=edge_c, linewidth=edge_w, zorder=6, alpha=0.96)
        ax.add_patch(circle)
        node_patches[node_id] = circle

        text_label = f"Job {t['job_id']} Op {t['task_id']}\nM{m} (d={d})\n[{st} - {et}]"
        ax.text(pos[0], pos[1], text_label, ha='center', va='center', fontsize=8.0, fontweight='bold', color='white', zorder=7)

    # 1. Desenhar Sequenciamento das Máquinas
    # Regra:
    # - Se for do Caminho Crítico: Arco sólido curvado ('arco') e vermelho com destaque
    # - Se for Não-Crítico: Linha tracejada suave
    mach_tasks = defaultdict(list)
    for t in tasks:
        mach_tasks[t['machine']].append(t)

    for m_id, t_list in mach_tasks.items():
        t_sorted = sorted(t_list, key=lambda x: x['start'])
        color = machine_colors[m_id]
        for i in range(len(t_sorted) - 1):
            u_id = f"J{t_sorted[i]['job_id']}_T{t_sorted[i]['task_id']}"
            v_id = f"J{t_sorted[i+1]['job_id']}_T{t_sorted[i+1]['task_id']}"
            pos_u = node_positions[u_id]
            pos_v = node_positions[v_id]

            is_crit = (u_id, v_id) in critical_edges

            if is_crit:
                # Caminho Crítico: em forma de 'arco' curvo, vermelho e grosso
                j_diff = abs(t_sorted[i]['job_id'] - t_sorted[i+1]['job_id'])
                rad_base = 0.22 + 0.05 * j_diff
                rad = rad_base if (t_sorted[i]['job_id'] + t_sorted[i+1]['job_id']) % 2 == 0 else -rad_base

                arrow = patches.FancyArrowPatch(
                    pos_u, pos_v,
                    patchA=node_patches[u_id],
                    patchB=node_patches[v_id],
                    shrinkA=0,
                    shrinkB=3,
                    connectionstyle=f"arc3,rad={rad}",
                    linestyle="-",
                    linewidth=3.2,
                    color='#e74c3c',
                    arrowstyle="-|>",
                    mutation_scale=16,
                    zorder=5,
                    alpha=1.0
                )
            else:
                # Não Crítico: Conexão em linha tracejada discreta
                arrow = patches.FancyArrowPatch(
                    pos_u, pos_v,
                    patchA=node_patches[u_id],
                    patchB=node_patches[v_id],
                    shrinkA=0,
                    shrinkB=3,
                    connectionstyle="arc3,rad=0",
                    linestyle="--",
                    linewidth=1.1,
                    color=color,
                    arrowstyle="->",
                    mutation_scale=12,
                    zorder=2,
                    alpha=0.45
                )
            ax.add_patch(arrow)

    # 2. Desenhar Arcos Conjuntivos A (Precedência dos Jobs)
    job_tasks = defaultdict(list)
    for t in tasks:
        job_tasks[t['job_id']].append(t)

    for j_id, t_list in job_tasks.items():
        t_sorted = sorted(t_list, key=lambda x: x['task_id'])

        # S -> Primeira tarefa
        first_id = f"J{j_id}_T0"
        is_crit = ('S', first_id) in critical_edges
        arrow = patches.FancyArrowPatch(
            start_pos, node_positions[first_id],
            patchA=node_patches['S'],
            patchB=node_patches[first_id],
            shrinkA=0,
            shrinkB=3,
            connectionstyle="arc3,rad=0",
            linestyle="-",
            linewidth=3.2 if is_crit else 1.5,
            color='#e74c3c' if is_crit else '#7f8c8d',
            arrowstyle="-|>",
            mutation_scale=15 if is_crit else 11,
            zorder=5 if is_crit else 3
        )
        ax.add_patch(arrow)

        # Entre tarefas consecutivas do mesmo job
        for i in range(len(t_sorted) - 1):
            u_id = f"J{j_id}_T{t_sorted[i]['task_id']}"
            v_id = f"J{j_id}_T{t_sorted[i+1]['task_id']}"
            is_crit = (u_id, v_id) in critical_edges
            arrow = patches.FancyArrowPatch(
                node_positions[u_id], node_positions[v_id],
                patchA=node_patches[u_id],
                patchB=node_patches[v_id],
                shrinkA=0,
                shrinkB=3,
                connectionstyle="arc3,rad=0",
                linestyle="-",
                linewidth=3.2 if is_crit else 1.5,
                color='#e74c3c' if is_crit else '#7f8c8d',
                arrowstyle="-|>",
                mutation_scale=15 if is_crit else 11,
                zorder=5 if is_crit else 3
            )
            ax.add_patch(arrow)

        # Última tarefa -> F
        last_id = f"J{j_id}_T{t_sorted[-1]['task_id']}"
        is_crit = (last_id, 'F') in critical_edges
        arrow = patches.FancyArrowPatch(
            node_positions[last_id], end_pos,
            patchA=node_patches[last_id],
            patchB=node_patches['F'],
            shrinkA=0,
            shrinkB=3,
            connectionstyle="arc3,rad=0",
            linestyle="-",
            linewidth=3.2 if is_crit else 1.5,
            color='#e74c3c' if is_crit else '#7f8c8d',
            arrowstyle="-|>",
            mutation_scale=15 if is_crit else 11,
            zorder=5 if is_crit else 3
        )
        ax.add_patch(arrow)

    # Identificadores de Jobs
    for j_id in range(num_jobs):
        y = (num_jobs - 1 - j_id) * y_spacing
        ax.text(-eff_x_spacing * 0.35, y, f"Job {j_id}", ha='right', va='center', fontsize=11, fontweight='bold', color='#34495e')

    # Limites e Layout
    ax.set_xlim(-eff_x_spacing * 0.75, end_x + eff_x_spacing * 0.75)
    ax.set_ylim(-y_spacing * 0.9, (num_jobs - 1) * y_spacing + y_spacing * 0.9)
    ax.set_aspect('equal')
    ax.axis('off')

    ax.set_title(f"Grafo Direcionado da Solução $D_S = (N, A \\cup S)$ — {name_without_ext}\n"
                 f"Makespan: {makespan} | Jobs: {num_jobs} | Máquinas: {num_machines} | Caminho Crítico em Arco Vermelho",
                 fontsize=16, fontweight='bold', pad=25)

    # Legenda Atualizada
    legend_elements = [
        patches.Patch(facecolor='#7f8c8d', edgecolor='black', label='Arco Conjuntivo (Precedência do Job)'),
        patches.Patch(facecolor='none', edgecolor='#e74c3c', linewidth=3.0, label='Caminho Crítico (Arco Vermelho)'),
        patches.Patch(facecolor='none', edgecolor='gray', linestyle='--', label='Sequência da Máquina Não-Crítica (Tracejado)'),
    ]
    for m in range(num_machines):
        legend_elements.append(patches.Patch(facecolor=machine_colors[m], edgecolor='black', label=f'Máquina {m}'))

    ax.legend(handles=legend_elements, loc='upper center', bbox_to_anchor=(0.5, -0.03),
              ncol=min(6, num_machines + 3), frameon=True, fontsize=10)

    plt.tight_layout()

    # Salvar Imagem
    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, f"solution_{name_without_ext}.png")
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    print(f"[OK] Grafo da Solução salvo com sucesso em: {out_path}")

    if show_plot:
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Gera o Grafo Direcionado da Solução com nós bem espaçados.")
    parser.add_argument("filepath", help="Caminho para o arquivo .txt de saída das máquinas")
    default_out = os.path.join(os.path.dirname(__file__), '..', 'out', 'graphs')
    parser.add_argument("--outdir", default=default_out, help="Diretório de saída")
    parser.add_argument("--x-spacing", type=float, default=4.6, help="Espaçamento horizontal entre tarefas (padrão: 4.6)")
    parser.add_argument("--y-spacing", type=float, default=2.6, help="Espaçamento vertical entre jobs (padrão: 2.6)")
    parser.add_argument("--fig-scale", type=float, default=1.0, help="Fator de escala da figura (padrão: 1.0)")
    parser.add_argument("--no-show", action="store_true", help="Não exibe a janela gráfica interativa")

    args = parser.parse_args()
    generate_solution_graph(
        os.path.abspath(args.filepath),
        os.path.abspath(args.outdir),
        x_spacing=args.x_spacing,
        y_spacing=args.y_spacing,
        fig_scale=args.fig_scale,
        show_plot=not args.no_show
    )