#ifndef PARAMETER_AGENT_H
#define PARAMETER_AGENT_H

namespace ParameterAgent {
    // Parameter Ekosistem Populasi Dinamis (Tanpa Cap 100)
    constexpr int INITIAL_POPULATION = 200;         // Populasi awal saat dunia dimulai
    constexpr int MAX_POPULATION_BUFFER = 5000;     // Kapasitas buffer memori GPU untuk ekspansi populasi alami (optimal & cepat)
    constexpr int MAX_TREES = 1000;                 // Kapasitas buffer pohon dinamis (bisa diperbanyak tak terbatas)
    
    // Parameter Kognisi Dinamis & Virtual Machine
    constexpr int MAX_DNA_CAPACITY = 64;            // Kapasitas Maksimum Instruksi Otak
    constexpr int MAX_REGISTERS = 8;                // Kapasitas Working Memory Register (R0 - R7)
    constexpr int MIN_DYNAMIC_PROGRAM_SIZE = 8;     // Ukuran Program Awal / Minimal
    constexpr int MIN_DYNAMIC_REGISTERS = 6;        // Register Awal / Minimal
    constexpr double DNA_MUTATION_RATE = 0.25;      // Probabilitas mutasi acak pada keturunan baru (25%)

    // Parameter Metabolisme, Usia, Gender & Reproduksi
    constexpr double MAX_AGE_YEARS = 100.0;         // Batas Usia Maksimum 100 Tahun
    constexpr double SECONDS_PER_YEAR = 12.0;       // 12 Detik Realtime = 1 Tahun Ekosistem
    constexpr double MATING_RADIUS = 25.0;          // Jarak Minimum untuk Bertemu Pasangan (M & F)
    constexpr double MATING_MIN_ENERGY = 50.0;      // Ambang batas energi untuk melahirkan anak (50%)
    constexpr double MATING_ENERGY_COST = 20.0;     // Energi terkuras saat melahirkan (-20%)
    constexpr double NEWBORN_INITIAL_ENERGY = 35.0; // Energi awal bayi lahir (35%)
    constexpr double MATING_COOLDOWN = 6.0;         // Jeda Waktu Antar Reproduksi (detik)
    constexpr double INITIAL_ENERGY = 100.0;        // Energi Awal Generasi Pertama (%)
    constexpr double METABOLISM_BASE_RATE = 0.12;   // Konsumsi energi dasar per detik (stabil & seimbang)
    constexpr double METABOLISM_MOVE_COST = 0.05;   // Biaya energi per gerak
    constexpr double CLIMATE_HUNGER_IMPACT_MULT = 0.015; // Pengaruh stres suhu terhadap kelaparan
    constexpr double STARVATION_THRESHOLD = 0.0;    // Batas kelaparan

    // Parameter Determinisme, Sensorik Tiga Pilar & Reservoir Recurrent Loop
    constexpr unsigned int FIXED_SIMULATION_SEED = 42;  // Seed deterministik tetap untuk eksperimen konsisten
    constexpr double FIXED_SUBSTEP_DT = 0.033;          // Timestep tetap per substep (~30 Hz)
    constexpr int SENSORS_COUNT = 16;                   // Kapasitas Sensorik 3 Pilar (Exteroception, Interoception, Proprioception)
    constexpr int PRED_SENSORS_COUNT = 4;               // Dimensi sensorik prediksi kausalitas waktu
    constexpr double RESERVOIR_SPECTRAL_RADIUS = 0.95;  // Radius spektral bobot recurrent reservoir
    constexpr double RESERVOIR_INPUT_SCALE = 0.5;       // Skala bobot input sensor ke reservoir

    // Parameter Predictive Loss (Kausalitas Fisika)
    constexpr double PREDICTIVE_LOSS_SCALE = 0.15;      // Skala koreksi error prediksi sensor ke register

    // Parameter Kuantisasi Sinyal Swarm (Embrio Bahasa Diskrit)
    constexpr double SWARM_QUANT_THRESHOLD = 0.33;      // Ambang batas ternary quantization sinyal komunikasi

    // Parameter Hebbian Plasticity Real-Time
    constexpr double HEBBIAN_LEARNING_RATE = 0.002;     // Laju update bobot reservoir online saat hidup
    constexpr double HEBBIAN_DECAY = 0.0005;            // Weight decay anti-runaway Hebbian

    // Parameter Full Adversarial Nature AI
    constexpr double NATURE_AGGRESSION_MAX = 5.0;       // Batas atas tekanan adaptif alam (5x)
    constexpr double NATURE_AGGRESSION_MIN = 0.5;       // Batas bawah tekanan — alam tidak pernah longgar
    constexpr double NATURE_REBOUND_SPEED = 0.02;       // Kecepatan alam pulih kembali menekan

    // Parameter Checkpointing & State Persistence
    constexpr int CHECKPOINT_INTERVAL_DAYS = 5;         // Simpan checkpoint otomatis setiap 5 hari
    inline const char* ECOSYSTEM_CHECKPOINT_FILE = "d:/MyProjects/mainan/agent/ingatan/ecosystem_checkpoint.bin";

    // Lokasi File Penyimpanan Permanen Otak Agen & Dashboard
    inline const char* DNA_STORAGE_FILE = "d:/MyProjects/mainan/agent/ingatan/best_herbivora_dna.bin";
    inline const char* PREDATOR_DNA_STORAGE_FILE = "d:/MyProjects/mainan/agent/ingatan/best_predator_dna.bin";
    inline const char* WEB_DASHBOARD_FILE = "d:/MyProjects/mainan/web/index.html";
}

#endif // PARAMETER_AGENT_H

