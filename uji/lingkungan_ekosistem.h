#pragma once
#include <cuda_runtime.h>
#include "agent/ingatan/parameter_agent.h"
#include "agent/02_kognisi_vm_bytecode.h"

// =========================================================================
// DEFINISI DATA & STRUCT UJIAN EKOSISTEM
// =========================================================================

struct GpuTreeEntity {
    float x;
    float y;
    float growth_stage;      // 0.0 - 100.0%
    float fruits_count;      // 0 - 6 buah (khusus TREE_TYPE_FRUIT)
    float formula_resonance; // Keselarasan rumus kognisi agen
    float age_years;         // Usia pohon
    float health;            // Vitalitas pohon (0.0 - 100.0%)
    int tree_type;           // 0: TREE_TYPE_FRUIT, 1: TREE_TYPE_OXYGEN, 2: TREE_TYPE_PIONEER
    float soil_fertility;    // Kesuburan tanah lokasi pohon (0.0 - 2.0)
    float moisture;          // Kadar air / kelembaban pohon (0.0 - 100.0%)
};

struct GpuFloraPlant {
    float x;
    float y;
    int flora_type;          // 0: FLORA_GRASS, 1: FLORA_BERRY_BUSH, 2: FLORA_GRAIN_CROP
    float growth_stage;      // 0.0 - 100.0%
    float moisture;          // 0.0 - 100.0%
    float health;            // 0.0 - 100.0%
    float yield_amount;      // Kuantitas panen / nutrisi biomassa
};

struct GpuWildFauna {
    int id;
    int fauna_type;          // 0: FAUNA_WILD_CATTLE, 1: FAUNA_WILD_BIRD
    float x;
    float y;
    float vx;
    float vy;
    float energy;            // 0.0 - 100.0%
    float hydration;         // 0.0 - 100.0%
    float stamina;           // 0.0 - 100.0%
    float age_years;
    float corpse_energy;     // Daging saat mati diburu
    int owner_agent_id;      // 0: Liar, >0: Dijinakkan / Diternak
    int owner_faction;       // -1: Liar, 0: Kubu Omnivora, 1: Kubu Predator
    bool is_sleeping;        // True saat tidur di malam hari / istirahat
    bool is_fleeing;         // True saat sprint lari dari predator
    bool is_alive;
};

struct GpuMineralDeposit {
    float x;
    float y;
    int element_type;    // 0: Silikon/Batu, 1: Logam Konduktif, 2: Kristal Energi Resonance
    float mass;          // Kerapatan massa/jumlah material (0 - 100%)
    float hardness;      // Ketahanan material terhadap degradasi (0 - 100%)
    float conductivity;  // Medan konduksi listrik/akustik
};

struct GpuClimateState {
    float wind_x;
    float wind_y;
    float temperature;   // -10.0°C hingga +40.0°C
    float magnetic_angle;// 0 - 2*PI
    float daylight_factor; // 0.0 (Malam Gelap) hingga 1.0 (Siang Terik)
    float season_phase;  // 0.0 - 4.0
    int current_season;  // 0: Semi, 1: Panas, 2: Gugur, 3: Dingin
    float season_timer;
    float oxygen_level;   // Atmosfer Oksigen (%)
    float co2_level;      // Karbon Dioksida (%)
    float h2o_level;      // Kelembaban Uap Air H2O (%)
    float nitrogen_level; // Gas Nitrogen N2 (%)
    float atmospheric_pressure; // Tekanan Atmosfer (Atm)
    float nature_adversarial_pressure; // Tekanan Adaptif Kubu Alam AI (0.0 - 5.0)
    float rain_intensity; // Intensitas Curah Hujan (0.0 - 100.0%)
    int active_disaster_type; // -1: Normal, 0: Solar Storm, 1: Blizzard, 2: EMP / Hurricane Blast
    float disaster_severity;  // Intensitas keparahan bencana (0.0 - 1.0)
};

struct GpuEcosystemAgent {
    int id;                  // Unique Agent ID
    int generation;          // Generasi Keturunan
    int gender;              // 0: Jantan, 1: Betina
    float x;
    float y;
    float vx;
    float vy;
    float energy;            // Homeostasis Metabolisme (0.0 - 100.0%)
    float lung_oxygen;       // Tabung Oksigen Paru-paru (0.0 - 100.0%)
    float hydration;         // Tingkat Hidrasi / Kadar Air Tubuh (0.0 - 100.0%)
    float age_years;         // Usia dalam Tahun
    float hunger_rate_mult;  // Pengali rasa lapar
    float mating_cooldown;   // Waktu tunggu sebelum bisa reproduksi
    int fruits_eaten;        // Jumlah buah yang dikonsumsi
    int formulas_discovered; // Jumlah rumus pertumbuhan yang dicetuskan
    int predators_slain;     // Jumlah predator yang dikalahkan
    int material_interactions;// Frekuensi berinteraksi/menambang
    int tools_crafted;       // Jumlah alat/senjata megalitikum yang dibuat
    float mined_material;    // Stok material mentah
    float growth_signal;     // Output rumus kognisi
    float comm_signal;       // Pesan siaran komunikasi / feromon
    float comm_received;     // Pesan siaran yang didengar
    float fear_level;        // Respon ketakutan / bahaya
    float corpse_energy;     // Biomassa mayat
    bool is_alive;           // Status Hidup
    bool just_died;
    bool just_born;
    
    int active_program_size;
    int active_registers_count;
    int sin_type;
    
    double registers[REGISTERS_COUNT];
    double prev_registers[REGISTERS_COUNT];
    double integrated_registers[REGISTERS_COUNT];
    float reservoir_weights_in[REGISTERS_COUNT];   // Bobot input sensorik
    float reservoir_weights_rec[REGISTERS_COUNT];  // Bobot recurrent
    float pred_sensor_prev[4];                     // Sensor t-1 untuk Predictive Loss
    float immortality_timer;                       // Sisa waktu keabadian
    
    DnaInstruction dna_program[DNA_PROGRAM_SIZE];
    float epigenetic_methylation[DNA_PROGRAM_SIZE];
};

struct GpuPredatorAgent {
    int id;
    int generation;
    int gender;              // 0: Jantan, 1: Betina
    float x;
    float y;
    float vx;
    float vy;
    float energy;
    float lung_oxygen;
    float hydration;
    float age_years;
    float mating_cooldown;
    float formula_shield;
    float comm_signal;
    float comm_received;
    int prey_devoured;
    int material_interactions;
    int tools_crafted;
    float mined_material;
    float corpse_energy;
    bool is_alive;
    bool just_killed;
    bool just_died;
    bool just_born;
    
    int active_program_size;
    int active_registers_count;
    int sin_type;
    double registers[REGISTERS_COUNT];
    double prev_registers[REGISTERS_COUNT];
    double integrated_registers[REGISTERS_COUNT];
    float reservoir_weights_in[REGISTERS_COUNT];
    float reservoir_weights_rec[REGISTERS_COUNT];
    float pred_sensor_prev[4];
    float immortality_timer;
    DnaInstruction dna_program[DNA_PROGRAM_SIZE];
};

// =========================================================================
// DEKLARASI API PELUNCUR UJIAN EKOSISTEM
// =========================================================================
extern "C" void launch_ecosystem_simulation(
    GpuEcosystemAgent* d_agents,
    int population_size,
    GpuTreeEntity* d_trees,
    int trees_count,
    GpuPredatorAgent* d_predators,
    int predators_count,
    GpuMineralDeposit* d_minerals,
    int minerals_count,
    GpuFloraPlant* d_flora,
    int flora_count,
    GpuWildFauna* d_fauna,
    int fauna_count,
    GpuClimateState* d_climate,
    double dt
);

extern "C" void launch_ecosystem_simulation_batched(
    GpuEcosystemAgent* d_agents,
    int population_size,
    GpuTreeEntity* d_trees,
    int trees_count,
    GpuPredatorAgent* d_predators,
    int predators_count,
    GpuMineralDeposit* d_minerals,
    int minerals_count,
    GpuFloraPlant* d_flora,
    int flora_count,
    GpuWildFauna* d_fauna,
    int fauna_count,
    GpuClimateState* d_climate,
    double dt,
    int substeps_count
);
