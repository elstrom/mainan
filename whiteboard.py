import os
import matplotlib.pyplot as plt

def generate_chalkboard_equations():
    output_filename = "whiteboard.png"
    
    # 1. Canvas Papan Tulis Gelap Matematikawan
    fig = plt.figure(figsize=(19.2, 10.8), dpi=100)
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_facecolor('#13171f')
    ax.axis('off')

    # Garis Pembatas Kapur
    ax.plot([0.03, 0.97, 0.97, 0.03, 0.03], [0.03, 0.03, 0.97, 0.97, 0.03], 
            color='#2c3545', lw=1.5, linestyle='--')
    ax.plot([0.50, 0.50], [0.18, 0.88], color='#2c3545', lw=1, linestyle=':')

    # Palet Kapur
    chalk_white  = '#f8fafc'
    chalk_cyan   = '#38bdf8'
    chalk_yellow = '#fde047'
    chalk_green  = '#4ade80'
    chalk_orange = '#fb923c'
    chalk_dim    = '#94a3b8'

    # --- JUDUL PAPAN TULIS ---
    ax.text(0.05, 0.92, r"$\Omega = \langle \mathcal{S}, \mathcal{T}, \mathcal{E}, \Psi \rangle$  --- Dynamical AST & Cognitive Morphogenesis", 
            fontsize=21, color=chalk_yellow)

    # --- KOLOM KIRI: STRUKTUR FORMAL BIT & POHON (AST) ---
    # I. RUANG BIT & OPERATOR PRIMITIF
    ax.text(0.05, 0.84, "I. Primitive Discrete Space & Relational Kernels", fontsize=15, color=chalk_cyan, fontweight='bold')
    ax.text(0.07, 0.78, r"$\mathcal{S}_t = \{ s_1, s_2, \dots, s_n \} \in \{0, 1\}^n$", fontsize=15, color=chalk_white)
    ax.text(0.07, 0.71, r"$\phi_{NAND}(a, b) = 1 - ab$", fontsize=15, color=chalk_white)
    ax.text(0.07, 0.64, r"$\phi_{DIFF}(a, b) = a \oplus b = (a + b) \% 2$", fontsize=15, color=chalk_white)

    # II. FORMULASI GRAPH / POHON AST
    ax.text(0.05, 0.54, "II. Dynamic Abstract Syntax Tree (AST) Formulation", fontsize=15, color=chalk_cyan, fontweight='bold')
    ax.text(0.07, 0.48, r"$\mathcal{T}_t = (\mathcal{V}_t, \mathcal{E}_t), \quad \mathcal{V}_t = \mathcal{V}_{leaf} \cup \mathcal{V}_{op}$", fontsize=15, color=chalk_white)
    ax.text(0.07, 0.40, r"$Eval(v) = \Phi_v(Eval(L_v), Eval(R_v)), \quad \forall v \in \mathcal{V}_{op}$", fontsize=15, color=chalk_white)
    ax.text(0.07, 0.31, r"$\hat{y}_t = Eval(Root(\mathcal{T}_t))$", fontsize=16, color=chalk_green)

    # --- KOLOM KANAN: DINAMIKA PERTUMBUHAN & KOMPRESI SIMBOLIK ---
    # III. PERTUMBUHAN DIDORONG ERROR (PREDICTION ERROR DYNAMICS)
    ax.text(0.53, 0.84, "III. Prediction Error Driven Graph Growth", fontsize=15, color=chalk_cyan, fontweight='bold')
    ax.text(0.55, 0.78, r"$\varepsilon_t = | y_{real}(t) - \hat{y}_t | \in \{0, 1\}$", fontsize=15, color=chalk_white)
    ax.text(0.55, 0.69, r"$\mathcal{T}_{t+1} = \mathcal{T}_t \cup \{ v_{new} \} \quad \mathrm{if} \quad \varepsilon_t = 1 \quad (Branching)$", fontsize=15, color=chalk_white)
    ax.text(0.55, 0.60, r"$\mathcal{C}(v_{t+1}) = \gamma \mathcal{C}(v_t) + (1 - 2\varepsilon_t)$", fontsize=15, color=chalk_white)

    # IV. KOMPRESI MINIMUM DESCRIPTION LENGTH & SIMBOL MAKRO
    ax.text(0.53, 0.50, "IV. Abstraction & Minimum Description Length (MDL)", fontsize=15, color=chalk_cyan, fontweight='bold')
    ax.text(0.55, 0.42, r"$\mathcal{L}(\mathcal{T}) = \arg\min_{\mathcal{T}} [ \mathcal{K}(\mathcal{T}) + \sum_{\tau=1}^t \varepsilon_\tau ]$", fontsize=16, color=chalk_orange)
    ax.text(0.55, 0.34, r"$\Psi : SubGraph(\mathcal{T}) \longrightarrow \mathbf{\Sigma}_{macro} \quad (Condensation)$", fontsize=15, color=chalk_white)

    # --- FOOTER / TEOREMA UTAMA ---
    ax.text(0.05, 0.20, "Fundamental Theorem of Autonomous Cognitive Emergence:", fontsize=15, color=chalk_yellow, fontweight='bold')
    ax.text(0.07, 0.12, r"$\lim_{t \to \infty} \mathcal{F}(\mathcal{T}_t, \mathcal{S}_t) = \inf \quad \Longleftrightarrow \quad \mathbb{E}[\varepsilon_t] \to 0 \quad \mathrm{and} \quad \dim(\mathbf{\Sigma}_{macro}) > 0$", fontsize=16, color=chalk_white)
    ax.text(0.07, 0.05, r"$\mathcal{S} \quad \longrightarrow \quad \mathcal{T} \quad \longrightarrow \quad \Psi \quad \longrightarrow \quad \mathcal{A} \quad (Autonomous \ Mind)$", fontsize=14, color=chalk_dim)

    # Simpan & Otomatis Replace whiteboard.png
    plt.savefig(output_filename, dpi=100, facecolor=fig.get_facecolor(), edgecolor='none', bbox_inches='tight')
    plt.close()
    print(f"[OK] Papan tulis rumus matematika murni berhasil di-render: {os.path.abspath(output_filename)}")

if __name__ == "__main__":
    generate_chalkboard_equations()
