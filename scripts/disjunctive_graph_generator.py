"""
disjunctive_graph_generator.py
Gera o grafo disjuntivo G = (N, A, E) a partir de instâncias JSSP no formato FT06 / OR-Library.
Versão com espaçamento ampliado e visualização panorâmica desobstruída.
"""

import os
import argparse
from itertools import combinations
import matplotlib.pyplot as plt
import matplotlib.patches as patches


def parse_instance(filepath):
    """Lê a instância no padrão ft06 / OR-Library."""
    with open(filepath, 'r', encoding='utf-8') as f:
        lines = [line.strip() for line in f if line.strip() and not line.startswith('#')]

    header = lines[0].split()
    num_jobs = int(header[0])
    num_machines = int(header[1])

    jobs = []
    for line in lines[1:num_jobs + 1]:
        parts = list(map(int, line.split()))
        ops = []
        for i in range(0, len(parts), 2):
            machine = parts[i]
            duration = parts[i + 1]
            ops.append({'machine': machine, 'duration': duration})
        jobs.append(ops)

    return num_jobs, num_machines, jobs


def generate_disjunctive_graph(filepath, output_dir, x_spacing=3.6, y_spacing=2.4, fig_scale=1.0, show_plot=True):
    num_jobs, num_machines, jobs = parse_instance(filepath)
    filename = os.path.basename(filepath)
    name_without_ext = os.path.splitext(filename)[0]

    # Dimensões da figura ampliadas e panorâmicas
    fig_width = max(22.0, (num_machines + 2) * x_spacing * 0.95) * fig_scale
    fig_height = max(11.0, (num_jobs + 1) * y_spacing * 0.85) * fig_scale
    fig, ax = plt.subplots(figsize=(fig_width, fig_height))

    # Paleta de cores para as máquinas
    cmap = plt.get_cmap('tab10' if num_machines <= 10 else 'tab20')
    machine_colors = {m: cmap(m % (10 if num_machines <= 10 else 20)) for m in range(num_machines)}

    # Nós e coordenadas ampliadas
    node_positions = {}
    machine_operations = {m: [] for m in range(num_machines)}

    # Posições de S e F
    start_x = 0.0
    end_x = (num_machines + 1) * x_spacing
    mid_y = ((num_jobs - 1) * y_spacing) / 2.0

    start_pos = (start_x, mid_y)
    end_pos = (end_x, mid_y)
    node_positions['S'] = start_pos
    node_positions['F'] = end_pos

    for j_idx, job in enumerate(jobs):
        y = (num_jobs - 1 - j_idx) * y_spacing
        for t_idx, op in enumerate(job):
            x = (t_idx + 1) * x_spacing
            node_id = f"J{j_idx}_T{t_idx}"
            node_positions[node_id] = (x, y)
            machine_operations[op['machine']].append((node_id, j_idx, t_idx))

    node_radius = 0.72

    # 1. Desenhar Arestas Disjuntivas E (Curvas tracejadas nas cores das máquinas)
    for m, op_list in machine_operations.items():
        color = machine_colors[m]
        for (u, j_u, t_u), (v, j_v, t_v) in combinations(op_list, 2):
            pos_u = node_positions[u]
            pos_v = node_positions[v]

            dy_jobs = abs(j_u - j_v)
            dx_tasks = abs(t_u - t_v)

            # Curvatura balanceada para não passar por cima de nós intermediários
            base_rad = 0.18 + 0.05 * dy_jobs
            if (j_u + j_v) % 2 == 0:
                rad = base_rad
            else:
                rad = -base_rad

            arrow = patches.FancyArrowPatch(
                pos_u, pos_v,
                connectionstyle=f"arc3,rad={rad}",
                linestyle="--",
                linewidth=1.2,
                color=color,
                alpha=0.42,
                arrowstyle="-",
                shrinkA=22,
                shrinkB=22,
                zorder=1
            )
            ax.add_patch(arrow)

    # 2. Desenhar Arcos Conjuntivos A (Retos, direcionados, bem visíveis)
    for j_idx, job in enumerate(jobs):
        # S -> Primeira tarefa
        first_op = f"J{j_idx}_T0"
        arrow = patches.FancyArrowPatch(
            start_pos, node_positions[first_op],
            connectionstyle="arc3,rad=0",
            linestyle="-",
            linewidth=2.0,
            color="#2c3e50",
            arrowstyle="-|>",
            mutation_scale=16,
            shrinkA=22,
            shrinkB=22,
            zorder=2
        )
        ax.add_patch(arrow)

        # Entre operações consecutivas do mesmo job
        for t_idx in range(len(job) - 1):
            curr_op = f"J{j_idx}_T{t_idx}"
            next_op = f"J{j_idx}_T{t_idx + 1}"
            arrow = patches.FancyArrowPatch(
                node_positions[curr_op], node_positions[next_op],
                connectionstyle="arc3,rad=0",
                linestyle="-",
                linewidth=2.0,
                color="#2c3e50",
                arrowstyle="-|>",
                mutation_scale=16,
                shrinkA=22,
                shrinkB=22,
                zorder=2
            )
            ax.add_patch(arrow)

        # Última tarefa -> F
        last_op = f"J{j_idx}_T{len(job) - 1}"
        arrow = patches.FancyArrowPatch(
            node_positions[last_op], end_pos,
            connectionstyle="arc3,rad=0",
            linestyle="-",
            linewidth=2.0,
            color="#2c3e50",
            arrowstyle="-|>",
            mutation_scale=16,
            shrinkA=22,
            shrinkB=22,
            zorder=2
        )
        ax.add_patch(arrow)

    # 3. Desenhar os Nós
    # Nós Fictícios (S e F)
    for node_id, label in [('S', 'INÍCIO\n(S)'), ('F', 'FIM\n(F)')]:
        pos = node_positions[node_id]
        circle = plt.Circle(pos, node_radius, facecolor='#ecf0f1', edgecolor='#2c3e50', linewidth=2.5, zorder=3)
        ax.add_patch(circle)
        ax.text(pos[0], pos[1], label, ha='center', va='center', fontsize=9.5, fontweight='bold', color='#2c3e50', zorder=4)

    # Nós de Operações Reais
    for j_idx, job in enumerate(jobs):
        for t_idx, op in enumerate(job):
            node_id = f"J{j_idx}_T{t_idx}"
            pos = node_positions[node_id]
            m = op['machine']
            d = op['duration']
            color = machine_colors[m]

            circle = plt.Circle(pos, node_radius, facecolor=color, edgecolor='#1a252f', linewidth=2.0, zorder=3, alpha=0.95)
            ax.add_patch(circle)

            text_label = f"Job {j_idx}\nOp {t_idx}\nM{m} | d={d}"
            ax.text(pos[0], pos[1], text_label, ha='center', va='center', fontsize=8.5, fontweight='bold', color='white', zorder=4)

    # Identificadores de Linha (Jobs) na lateral esquerda
    for j_idx in range(num_jobs):
        y = (num_jobs - 1 - j_idx) * y_spacing
        ax.text(-x_spacing * 0.4, y, f"Job {j_idx}", ha='right', va='center', fontsize=11, fontweight='bold', color='#34495e')

    # Limites e Layout
    ax.set_xlim(-x_spacing * 0.8, end_x + x_spacing * 0.8)
    ax.set_ylim(-y_spacing * 0.9, (num_jobs - 1) * y_spacing + y_spacing * 0.9)
    ax.set_aspect('equal')
    ax.axis('off')

    ax.set_title(f"Grafo Disjuntivo G = (N, A, E) — Instância: {name_without_ext}\n"
                 f"Dimensão: {num_jobs} Jobs × {num_machines} Máquinas | Total de Operações: {num_jobs * num_machines}",
                 fontsize=16, fontweight='bold', pad=25)

    # Legenda espaçada
    legend_elements = [
        patches.Patch(facecolor='#2c3e50', edgecolor='black', label='Arco Conjuntivo (Precedência do Job)'),
        patches.Patch(facecolor='none', edgecolor='gray', linestyle='--', label='Aresta Disjuntiva (Mesma Máquina)'),
    ]
    for m in range(num_machines):
        legend_elements.append(patches.Patch(facecolor=machine_colors[m], edgecolor='black', label=f'Máquina {m}'))

    ax.legend(handles=legend_elements, loc='upper center', bbox_to_anchor=(0.5, -0.03),
              ncol=min(8, num_machines + 2), frameon=True, fontsize=10)

    plt.tight_layout()

    os.makedirs(output_dir, exist_ok=True)
    out_path = os.path.join(output_dir, f"disjunctive_{name_without_ext}.png")
    plt.savefig(out_path, dpi=300, bbox_inches='tight')
    print(f"[OK] Grafo Disjuntivo salvo em: {out_path}")

    if show_plot:
        plt.show()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Gera o Grafo Disjuntivo de entrada com nós bem espaçados.")
    parser.add_argument("filepath", help="Caminho para o arquivo da instância (ex: instances/ft06.txt)")
    default_out = os.path.join(os.path.dirname(__file__), '..', 'out', 'graphs')
    parser.add_argument("--outdir", default=default_out, help="Diretório de saída")
    parser.add_argument("--x-spacing", type=float, default=3.6, help="Espaçamento horizontal entre tarefas (padrão: 3.6)")
    parser.add_argument("--y-spacing", type=float, default=2.4, help="Espaçamento vertical entre jobs (padrão: 2.4)")
    parser.add_argument("--fig-scale", type=float, default=1.0, help="Fator de escala da figura (padrão: 1.0)")
    parser.add_argument("--no-show", action="store_true", help="Não exibe a janela gráfica interativa")

    args = parser.parse_args()
    generate_disjunctive_graph(
        os.path.abspath(args.filepath),
        os.path.abspath(args.outdir),
        x_spacing=args.x_spacing,
        y_spacing=args.y_spacing,
        fig_scale=args.fig_scale,
        show_plot=not args.no_show
    )