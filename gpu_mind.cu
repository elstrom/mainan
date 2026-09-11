#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>
#include "agent/dunia/parameter_dunia.h"
#include "agent/ingatan/parameter_agent.h"

#define DNA_PROGRAM_SIZE ParameterAgent::MAX_DNA_CAPACITY
#define REGISTERS_COUNT ParameterAgent::MAX_REGISTERS
#define ECO_MAX_TREES ParameterAgent::MAX_TREES

// Virtual Machine Bytecode Operasi Aljabar & Kognisi Ekologis
enum DnaOpcode {
    OP_NOP = 0,
    OP_LOAD_SENSOR,      // reg[A] = sensor[B]
    OP_ADD,              // reg[A] = reg[B] + reg[C]
    OP_SUB,              // reg[A] = reg[B] - reg[C]
    OP_MUL,              // reg[A] = reg[B] * reg[C]
    OP_DIV,              // reg[A] = reg[B] / (reg[C] + eps)
    OP_TANH,             // reg[A] = tanh(reg[B])
    OP_SIGMOID,          // reg[A] = sigmoid(reg[B])
    OP_INTEGRAL,         // reg[A] += reg[B] * dt
    OP_DERIVATIVE,       // reg[A] = (reg[B] - prev[A]) / dt
    OP_ACTION_MOVE,      // Set Velocity (vx = reg[B], vy = reg[C])
    OP_ACTION_FORMULA,   // Emit Resonant Frequency / Growth Formula = reg[B]
    OP_RESONATE_CLIMATE, // Harmonize internal state with climate/temperature
    OP_FORK_NEURON,      // Neurogenesis: Tumbuhkan sirkuit baru jika energi cukup
    OP_PRUNE_NEURON,     // Synaptic Pruning: Ringkaskan sirkuit jika tenang & stabil
    OP_WRITE_CODE,       // Self-Modifying Code: dna[reg[A]].op = reg[B], dest/src = reg[C]
    OP_MUTATE_SELF,      // Self-Metaprogramming: Acak/edit instruksi target berdasarkan sinyal internal
    OP_ALLOC_REG,        // Dynamic Working Memory Allocation: Tambah kapasitas register aktif (+1)
    OP_FREE_REG          // Dynamic Memory Free: Kurangi kapasitas register aktif (-1)
};

struct DnaInstruction {
    unsigned char op;
    unsigned char r_dest;
    unsigned char r_src1;
    unsigned char r_src2;
    float immediate_val;
};

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
};

struct GpuEcosystemAgent {
    int id;                  // Unique Agent ID
    int generation;          // Generasi Keturunan
    int gender;              // 0: Jantan (Male), 1: Betina (Female)
    float x;
    float y;
    float vx;
    float vy;
    float energy;            // Homeostasis Metabolisme (0.0 - 100.0%)
    float lung_oxygen;       // Tabung Oksigen Paru-paru (0.0 - 100.0%)
    float hydration;         // Tingkat Hidrasi / Kadar Air Tubuh (0.0 - 100.0%)
    float age_years;         // Usia dalam Tahun (0 - 100 Tahun)
    float hunger_rate_mult;  // Pengali rasa lapar (Meningkat seiring penuaan)
    float mating_cooldown;   // Waktu tunggu sebelum bisa reproduksi kembali
    int fruits_eaten;        // Jumlah buah yang berhasil dikonsumsi
    int formulas_discovered; // Jumlah rumus pertumbuhan yang berhasil dicetuskan
    int predators_slain;     // Jumlah predator yang berhasil dikalahkan agen dengan rumus
    int material_interactions;// Frekuensi berinteraksi/menambang material megalitikum
    int tools_crafted;       // Jumlah alat/senjata/struktur megalitikum yang berhasil dibuat
    float mined_material;    // Stok material mentah yang dikumpulkan
    float growth_signal;     // Output rumus kognisi yang dipancarkan
    float comm_signal;       // Pesan siaran komunikasi / feromon yang dipancarkan ke tetangga
    float comm_received;     // Pesan siaran komunikasi yang didengar dari koloni sekitar
    float fear_level;        // Respon bahaya kelaparan / cuaca ekstrem
    float corpse_energy;     // Biomassa mayat yang bisa dimakan karnivora
    bool is_alive;           // Status Kehidupan (True: Hidup, False: Baru Mati/Mayat)
    bool just_died;          // Flag event kematian pada tick ini
    bool just_born;          // Flag event kelahiran pada tick ini
    
    int active_program_size;
    int active_registers_count;
    int sin_type;            // Sifat Dosa Pokok (0: Pride, 1: Greed, 2: Lust, 3: Envy, 4: Gluttony, 5: Wrath, 6: Sloth)
    
    double registers[REGISTERS_COUNT];
    double prev_registers[REGISTERS_COUNT];
    double integrated_registers[REGISTERS_COUNT];
    float reservoir_weights_in[REGISTERS_COUNT];   // Bobot input sensorik ke reservoir hidden state
    float reservoir_weights_rec[REGISTERS_COUNT];  // Bobot recurrent self-loop reservoir
    float pred_sensor_prev[4];                     // Sensor t-1 untuk Predictive Loss [tree_dx, tree_dy, energy, pred_dist]
    float immortality_timer;                       // Sisa waktu keabadian (detik simulasi); 0 = normal
    
    DnaInstruction dna_program[DNA_PROGRAM_SIZE];
    float epigenetic_methylation[DNA_PROGRAM_SIZE];
};

struct GpuPredatorAgent {
    int id;
    int generation;
    int gender;              // 0: Jantan (♂), 1: Betina (♀)
    float x;
    float y;
    float vx;
    float vy;
    float energy;
    float lung_oxygen;       // Tabung Oksigen Paru-paru Predator (0.0 - 100.0%)
    float hydration;         // Tingkat Hidrasi Predator (0.0 - 100.0%)
    float age_years;
    float mating_cooldown;
    float formula_shield;    // Perisai kognitif musuh (0.0 - 1.0)
    float comm_signal;       // Pesan siaran koordinasi kawanan serigala predator
    float comm_received;     // Pesan koordinasi yang didengar dari predator kawan
    int prey_devoured;
    int material_interactions;// Frekuensi berinteraksi/menambang material megalitikum
    int tools_crafted;       // Jumlah alat/senjata/struktur megalitikum yang berhasil dibuat
    float mined_material;    // Stok material mentah yang dikumpulkan
    float corpse_energy;     // Biomassa mayat karnivora (bisa dikanibal)
    bool is_alive;
    bool just_killed;
    bool just_died;
    bool just_born;
    
    int active_program_size;
    int active_registers_count;
    int sin_type;            // Sifat Dosa Pokok (0: Pride, 1: Greed, 2: Lust, 3: Envy, 4: Gluttony, 5: Wrath, 6: Sloth)
    double registers[REGISTERS_COUNT];
    double prev_registers[REGISTERS_COUNT];
    double integrated_registers[REGISTERS_COUNT];
    float reservoir_weights_in[REGISTERS_COUNT];   // Bobot input sensorik ke reservoir
    float reservoir_weights_rec[REGISTERS_COUNT];  // Bobot recurrent self-loop reservoir
    float pred_sensor_prev[4];                     // Sensor t-1 untuk Predictive Loss [prey_dx, prey_dy, energy, prey_dist]
    float immortality_timer;                       // Sisa waktu keabadian (detik simulasi); 0 = normal
    DnaInstruction dna_program[DNA_PROGRAM_SIZE];
};

__device__ inline double gpu_clamp(double v, double min_val, double max_val) {
    if (v < min_val) return min_val;
    if (v > max_val) return max_val;
    return v;
}

// Determinisme Presisi: Integer Hash PRNG untuk CUDA
__device__ inline unsigned int gpu_hash(unsigned int seed) {
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

__global__ void simulate_ecosystem_step_cuda_kernel(
    GpuEcosystemAgent* agents,
    int population_size,
    GpuTreeEntity* trees,
    int trees_count,
    GpuPredatorAgent* predators,
    int predators_count,
    GpuMineralDeposit* minerals,
    int minerals_count,
    GpuClimateState* climate,
    double dt
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= population_size) return;

    GpuEcosystemAgent& ag = agents[idx];
    ag.just_died = false;
    ag.just_born = false;

    // 0. Cek Status Kehidupan
    if (!ag.is_alive) {
        // Dekomposisi Mayat Alami
        if (ag.corpse_energy > 0.0f) {
            ag.corpse_energy = fmaxf(0.0f, ag.corpse_energy - (float)DuniaFisika::CORPSE_DECAY_RATE * (float)dt);
        }
        return; // Agen mati tidak memproses komputasi atau bergerak
    }

    // Cek Ambang Kematian (Mati karena Kelaparan, Serangan Predator, atau Usia 100 Tahun)
    if (ag.energy <= (float)ParameterAgent::STARVATION_THRESHOLD || ag.age_years >= (float)ParameterAgent::MAX_AGE_YEARS) {
        ag.is_alive = false;
        ag.just_died = true;
        ag.energy = 0.0f;
        ag.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE; // Menyisakan nutrisi mayat
        return;
    }

    // Friendly Fire Antar-Agen Kubu A (Benturan / Gesekan Kecepatan Tinggi tak Sengaja)
    for (int k = 0; k < 16; ++k) {
        int peer_idx = (idx + k * 29 + 1) % population_size;
        if (peer_idx != idx && agents[peer_idx].is_alive && agents[peer_idx].energy > 0.0f) {
            float cdx = agents[peer_idx].x - ag.x;
            float cdy = agents[peer_idx].y - ag.y;
            float cdist2 = cdx * cdx + cdy * cdy;
            if (cdist2 < 4.0f * 4.0f) { // Kontak fisik sangat dekat
                float rel_v2 = (ag.vx - agents[peer_idx].vx) * (ag.vx - agents[peer_idx].vx) + 
                               (ag.vy - agents[peer_idx].vy) * (ag.vy - agents[peer_idx].vy);
                if (rel_v2 > 16.0f) { // Terjadi benturan keras
                    float ff_dmg = (float)DuniaFisika::FRIENDLY_FIRE_DAMAGE_RATE * 0.2f * (float)dt;
                    ag.energy = fmaxf(0.0f, ag.energy - ff_dmg);
                }
            }
        }
    }

    // 1. Cari Pohon Terdekat, Deposit Mineral Terdekat, & Predator Terdekat
    float nearest_tree_dist = 10000.0f;
    float nearest_tree_dx = 0.0f;
    float nearest_tree_dy = 0.0f;
    int nearest_tree_idx = -1;

    for (int t = 0; t < trees_count; ++t) {
        float dx = trees[t].x - ag.x;
        float dy = trees[t].y - ag.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < nearest_tree_dist) {
            nearest_tree_dist = d;
            nearest_tree_dx = dx;
            nearest_tree_dy = dy;
            nearest_tree_idx = t;
        }
    }

    // Cari Deposit Mineral Terdekat (Untuk penambangan alat/batu purba & perlindungan medan)
    float nearest_min_dist = 10000.0f;
    float nearest_min_dx = 0.0f;
    float nearest_min_dy = 0.0f;
    int nearest_min_idx = -1;

    for (int m = 0; m < minerals_count; ++m) {
        if (minerals[m].mass <= 0.0f) continue;
        float dx = minerals[m].x - ag.x;
        float dy = minerals[m].y - ag.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < nearest_min_dist) {
            nearest_min_dist = d;
            nearest_min_dx = dx;
            nearest_min_dy = dy;
            nearest_min_idx = m;
        }
    }

    // Cari Predator Terdekat untuk insting waspada & manuver menghindar (Pilar 1: Exteroception Presisi)
    float nearest_pred_dist = 10000.0f;
    float nearest_pred_x = 0.0f;
    float nearest_pred_y = 0.0f;
    for (int p = 0; p < predators_count; ++p) {
        if (predators[p].is_alive && predators[p].energy > 0.0f) {
            float pdx = predators[p].x - ag.x;
            float pdy = predators[p].y - ag.y;
            float pd = sqrtf(pdx * pdx + pdy * pdy);
            if (pd < nearest_pred_dist) {
                nearest_pred_dist = pd;
                nearest_pred_x = predators[p].x;
                nearest_pred_y = predators[p].y;
            }
        }
    }

    // Ability Alam: 1. Toksisitas Pembusukan Bangkai Sekitar (Corpse Toxicity)
    for (int p = 0; p < 8; ++p) {
        int c_idx = (idx + p * 13) % population_size;
        if (!agents[c_idx].is_alive && agents[c_idx].corpse_energy > 5.0f) {
            float cdx = agents[c_idx].x - ag.x;
            float cdy = agents[c_idx].y - ag.y;
            if ((cdx * cdx + cdy * cdy) < (float)(DuniaFisika::CORPSE_TOXICITY_RADIUS * DuniaFisika::CORPSE_TOXICITY_RADIUS)) {
                ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::CORPSE_TOXICITY_DAMAGE * (float)dt); // Racun tanah/air bangkai
            }
        }
    }

    // Ability Alam: 2. Drag Gravitasi / Rawa Spasial (Spatial Drag Slowdown di area rawa (x: 400..600, y: 400..600))
    bool in_drag_zone = (ag.x > 400.0f && ag.x < 600.0f && ag.y > 400.0f && ag.y < 600.0f);
    if (in_drag_zone) {
        ag.vx *= (float)DuniaFisika::SPATIAL_DRAG_SLOWDOWN;
        ag.vy *= (float)DuniaFisika::SPATIAL_DRAG_SLOWDOWN;
    }

    // Ability Alam: 3. Zona Safe Haven / Gua Perlindungan (x: 100..200, y: 100..200)
    bool in_safe_haven = (ag.x > 100.0f && ag.x < 220.0f && ag.y > 100.0f && ag.y < 220.0f);
    if (in_safe_haven && ag.energy < 100.0f) {
        ag.energy = fminf(100.0f, ag.energy + 2.0f * (float)dt); // Pemulihan aman di gua
    }

    // Hitung Jarak ke Alur Sungai Terdekat (Meandering River: x = CENTER_X + AMP * sin(y * FREQ))
    float river_x = (float)DuniaFisika::RIVER_CENTER_X + (float)DuniaFisika::RIVER_MEANDER_AMP * sinf(ag.y * (float)DuniaFisika::RIVER_MEANDER_FREQ);
    float dist_to_river = fabsf(ag.x - river_x);
    float river_dx = river_x - ag.x;

    // Minum Air di Tepi Sungai (Hidrasi Tubuh)
    if (dist_to_river <= (float)DuniaFisika::RIVER_DRINK_RADIUS) {
        ag.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, ag.hydration + (float)DuniaFisika::AGENT_DRINK_WATER_RATE * (float)dt);
    }

    // =========================================================================
    // PILAR 2: SENSOR TIGA PILAR (TERKOMPRESI 16 FLOAT) - HERBIVORA
    // =========================================================================
    // 1. EXTEROCEPTION (Dunia Luar: S0 - S9)
    float vis_factor = 0.4f + 0.6f * climate->daylight_factor; // Penglihatan tajam saat siang, redup saat malam
    float s0_tree_dx = (nearest_tree_dx / (nearest_tree_dist + 1e-3f)) * vis_factor;
    float s1_tree_dy = (nearest_tree_dy / (nearest_tree_dist + 1e-3f)) * vis_factor;
    float s2_tree_dist = 1.0f - (float)gpu_clamp(nearest_tree_dist / 500.0f, 0.0, 1.0);
    float s3_threat_dx = (nearest_pred_dist < 10000.0f) ? ((nearest_pred_x - ag.x) / (nearest_pred_dist + 1e-3f)) * vis_factor : 0.0f;
    float s4_threat_dy = (nearest_pred_dist < 10000.0f) ? ((nearest_pred_y - ag.y) / (nearest_pred_dist + 1e-3f)) * vis_factor : 0.0f;
    float s5_threat_dist = (nearest_pred_dist < 10000.0f) ? (1.0f - (float)gpu_clamp(nearest_pred_dist / 300.0f, 0.0, 1.0)) : 0.0f;
    float s6_river_dx = (river_dx / (dist_to_river + 1e-3f)) * vis_factor; // Sensor arah sungai
    float s7_river_dist = 1.0f - (float)gpu_clamp(dist_to_river / 300.0f, 0.0, 1.0); // Sensor jarak ke air
    float s8_comm_recv = (float)tanh(ag.comm_received);
    float s9_climate_ambient = (float)tanh((climate->temperature - 20.0f) * 0.05f) * climate->daylight_factor;

    // 2. INTEROCEPTION (Kondisi Tubuh / Homeostasis: Energi S10, Hidrasi S11, Rasa Haus/Panik S12)
    float s10_energy = (float)gpu_clamp(ag.energy / 100.0f, 0.0, 1.0);
    float s11_hydration = (float)gpu_clamp(ag.hydration / 100.0f, 0.0, 1.0);
    // 1. Formula Sigmoid/Eksponensial Non-Linier: Lonjakan Panik saat Krisis Energi/Haus/Predator
    float raw_stress = (1.0f - s10_energy) * 0.35f + (1.0f - s11_hydration) * 0.35f + s5_threat_dist * 0.30f;
    float fear_sig = 1.0f / (1.0f + expf(-(float)DuniaFisika::FEAR_SIGMOID_STEEPNESS * (raw_stress - (float)DuniaFisika::FEAR_SIGMOID_MIDPOINT)));
    ag.fear_level = (float)gpu_clamp(fear_sig, 0.0, 1.0);
    float s12_fear_pain = ag.fear_level;

    // 3. PROPRIOCEPTION (Kesadaran Gerak & Fisik Diri: S13 - S15)
    float max_spd = (float)DuniaFisika::MAX_AGENT_SPEED;
    float s13_vx = (float)gpu_clamp(ag.vx / (max_spd + 1e-3f), -1.0, 1.0);
    float s14_vy = (float)gpu_clamp(ag.vy / (max_spd + 1e-3f), -1.0, 1.0);
    float s15_tool_status = (ag.tools_crafted > 0) ? 1.0f : (float)gpu_clamp(ag.mined_material / (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD, 0.0, 0.9);

    float agent_sensor_inputs[ParameterAgent::SENSORS_COUNT] = {
        s0_tree_dx, s1_tree_dy, s2_tree_dist, s3_threat_dx,
        s4_threat_dy, s5_threat_dist, s6_river_dx, s7_river_dist,
        s8_comm_recv, s9_climate_ambient, s10_energy, s11_hydration,
        s12_fear_pain, s13_vx, s14_vy, s15_tool_status
    };

    // =========================================================================
    // PILAR 3: RECURRENT RESERVOIR LOOP (Spatial Depth 64 Step + Temporal T)
    // =========================================================================
    int max_r = (ag.active_registers_count > 0 && ag.active_registers_count <= REGISTERS_COUNT) ? 
                ag.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;
    int active_prog = (ag.active_program_size > 0 && ag.active_program_size <= DNA_PROGRAM_SIZE) ? 
                      ag.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

    // Reservoir state update dengan dual-channel input projection + recurrent loop
    for (int r = 0; r < max_r; ++r) {
        float in_signal = ag.reservoir_weights_in[r] * agent_sensor_inputs[r % ParameterAgent::SENSORS_COUNT] +
                          ag.reservoir_weights_in[(r + 8) % REGISTERS_COUNT] * agent_sensor_inputs[(r + 8) % ParameterAgent::SENSORS_COUNT];
        float rec_signal = ag.reservoir_weights_rec[r] * (float)ag.prev_registers[r];
        double u_val = in_signal + rec_signal;
        ag.registers[r] = (1.0 - ParameterAgent::RESERVOIR_SPECTRAL_RADIUS) * ag.registers[r] + 
                          ParameterAgent::RESERVOIR_SPECTRAL_RADIUS * tanh(u_val);
    }

    // Predictive Loss: Koreksi register berdasarkan error prediksi sensor t-1 → t
    float pred_sensors_now[ParameterAgent::PRED_SENSORS_COUNT] = { s0_tree_dx, s1_tree_dy, s10_energy, s5_threat_dist };
    for (int r = 0; r < max_r; ++r) {
        float pred_error = pred_sensors_now[r % ParameterAgent::PRED_SENSORS_COUNT] - ag.pred_sensor_prev[r % ParameterAgent::PRED_SENSORS_COUNT];
        ag.registers[r] = gpu_clamp(ag.registers[r] + ParameterAgent::PREDICTIVE_LOSS_SCALE * pred_error, -5.0, 5.0);
    }
    ag.pred_sensor_prev[0] = s0_tree_dx;
    ag.pred_sensor_prev[1] = s1_tree_dy;
    ag.pred_sensor_prev[2] = s10_energy;
    ag.pred_sensor_prev[3] = s5_threat_dist;

    float move_cmd_x = 0.0f;
    float move_cmd_y = 0.0f;
    float broadcast_out = 0.0f;

    // Eksekusi Virtual Machine DNA (Kedalaman 64 Step Spatial Ops)
    for (int ip = 0; ip < active_prog; ++ip) {
        const auto& inst = ag.dna_program[ip];
        int rd = inst.r_dest % max_r;
        int rs1 = inst.r_src1 % max_r;
        int rs2 = inst.r_src2 % max_r;

        switch (inst.op % 15) {
            case OP_NOP: break;
            case OP_LOAD_SENSOR: {
                int s = inst.r_src1 % ParameterAgent::SENSORS_COUNT;
                ag.registers[rd] = agent_sensor_inputs[s];
                break;
            }
            case OP_ADD: ag.registers[rd] = gpu_clamp(ag.registers[rs1] + ag.registers[rs2], -5.0, 5.0); break;
            case OP_SUB: ag.registers[rd] = gpu_clamp(ag.registers[rs1] - ag.registers[rs2], -5.0, 5.0); break;
            case OP_MUL: ag.registers[rd] = gpu_clamp(ag.registers[rs1] * ag.registers[rs2], -5.0, 5.0); break;
            case OP_DIV: ag.registers[rd] = gpu_clamp(ag.registers[rs1] / (fabs(ag.registers[rs2]) + 1e-3), -5.0, 5.0); break;
            case OP_TANH: ag.registers[rd] = tanh(ag.registers[rs1]); break;
            case OP_SIGMOID: ag.registers[rd] = 1.0 / (1.0 + exp(-gpu_clamp(ag.registers[rs1], -5.0, 5.0))); break;
            case OP_INTEGRAL: {
                ag.integrated_registers[rd] += ag.registers[rs1] * dt;
                ag.registers[rd] = tanh(ag.integrated_registers[rd]);
                break;
            }
            case OP_DERIVATIVE: {
                ag.registers[rd] = gpu_clamp((ag.registers[rs1] - ag.prev_registers[rd]) / (dt + 1e-4), -5.0, 5.0);
                ag.prev_registers[rd] = ag.registers[rs1];
                break;
            }
            case OP_ACTION_MOVE: {
                move_cmd_x = (float)tanh(ag.registers[rs1]);
                move_cmd_y = (float)tanh(ag.registers[rs2]);
                break;
            }
            case OP_ACTION_FORMULA: {
                broadcast_out = (float)tanh(ag.registers[rs1]); // Memancarkan frekuensi komunikasi / sinyal feromon
                ag.growth_signal = broadcast_out;
                ag.comm_signal = broadcast_out;
                break;
            }
            case OP_RESONATE_CLIMATE: {
                ag.registers[rd] = (float)tanh(ag.registers[rs1] * s9_climate_ambient);
                break;
            }
            case OP_FORK_NEURON: {
                if (ag.active_program_size < DNA_PROGRAM_SIZE && ag.energy > 80.0f) {
                    ag.active_program_size++;
                }
                break;
            }
            case OP_PRUNE_NEURON: {
                if (ag.active_program_size > ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE && ag.energy < 25.0f) {
                    ag.active_program_size--;
                }
                break;
            }
            case OP_WRITE_CODE: {
                // Self-Modifying Code: Agen menulis ulang instruksi DNA di slot target
                int target_ip = (int)fabs(ag.registers[rs1]) % DNA_PROGRAM_SIZE;
                unsigned char new_op = (unsigned char)((int)fabs(ag.registers[rs2]) % 19);
                ag.dna_program[target_ip].op = new_op;
                ag.dna_program[target_ip].r_dest = (unsigned char)((int)fabs(ag.registers[rd]) % max_r);
                break;
            }
            case OP_MUTATE_SELF: {
                // Self-Metaprogramming: Mutasi adaptif runtime terhadap instruksi sendiri
                if (ag.energy > 40.0f) {
                    int target_ip = (int)fabs(ag.registers[rs1]) % active_prog;
                    float mod_val = (float)ag.registers[rs2] * 0.1f;
                    ag.dna_program[target_ip].immediate_val += mod_val;
                    ag.energy -= 0.05f; // Biaya kognisi metaprogramming
                }
                break;
            }
            case OP_ALLOC_REG: {
                // Dynamic Working Memory Allocation: Tambah kapasitas register aktif
                if (ag.active_registers_count < REGISTERS_COUNT && ag.energy > 60.0f) {
                    ag.active_registers_count++;
                    ag.energy -= 0.1f; // Biaya memori
                }
                break;
            }
            case OP_FREE_REG: {
                // Dynamic Memory Deallocation: Pangkas register aktif saat minim sumber daya
                if (ag.active_registers_count > ParameterAgent::MIN_DYNAMIC_REGISTERS) {
                    ag.active_registers_count--;
                }
                break;
            }
        }

    }

    // 3. Neuromodulation (Lonjakan Adrenalin & Plastisitas Saat Panik)
    // Sinyal rasa takut meningkatkan learning rate Hebbian hingga (1 + 4*fear) = 5x lipat
    float plasticity_neuromod = 1.0f + ag.fear_level * (float)DuniaFisika::FEAR_NEUROMODULATION_PLASTICITY;
    for (int r = 0; r < max_r; ++r) {
        float hebb_delta = (float)(ParameterAgent::HEBBIAN_LEARNING_RATE * plasticity_neuromod * agent_sensor_inputs[r % ParameterAgent::SENSORS_COUNT] * ag.registers[r]);
        ag.reservoir_weights_in[r] += hebb_delta - (float)(ParameterAgent::HEBBIAN_DECAY * ag.reservoir_weights_in[r]);
        if (ag.reservoir_weights_in[r] > 1.0f) ag.reservoir_weights_in[r] = 1.0f;
        if (ag.reservoir_weights_in[r] < -1.0f) ag.reservoir_weights_in[r] = -1.0f;
        ag.prev_registers[r] = ag.registers[r]; // Simpan state temporal memory loop
    }

    // 2d. Swarm Signal Quantization: Kuantisasi comm_signal ke simbol diskrit {-1.0, 0.0, +1.0}
    float quant_thresh = (float)ParameterAgent::SWARM_QUANT_THRESHOLD;
    if (ag.comm_signal > quant_thresh)       ag.comm_signal = 1.0f;
    else if (ag.comm_signal < -quant_thresh) ag.comm_signal = -1.0f;
    else                                      ag.comm_signal = 0.0f;

    // 3. DINAMIKA GERAK FISIK (Inersia, Medan Angin, dan Perintah Aksi Otak)
    float wind_influence_x = climate->wind_x * (float)DuniaFisika::WIND_FORCE_MULT;
    float wind_influence_y = climate->wind_y * (float)DuniaFisika::WIND_FORCE_MULT;

    float target_vx = move_cmd_x * max_spd + wind_influence_x;
    float target_vy = move_cmd_y * max_spd + wind_influence_y;

    ag.vx = ag.vx * (float)DuniaFisika::FRICTION_COEFF + target_vx * 0.15f;
    ag.vy = ag.vy * (float)DuniaFisika::FRICTION_COEFF + target_vy * 0.15f;

    ag.x += ag.vx * (float)dt;
    ag.y += ag.vy * (float)dt;

    // Batas Tepi Dunia
    if (ag.x < 10.0f) { ag.x = 10.0f; ag.vx = -ag.vx * 0.5f; }
    if (ag.x > (float)DuniaFisika::WORLD_WIDTH - 10.0f) { ag.x = (float)DuniaFisika::WORLD_WIDTH - 10.0f; ag.vx = -ag.vx * 0.5f; }
    if (ag.y < 10.0f) { ag.y = 10.0f; ag.vy = -ag.vy * 0.5f; }
    if (ag.y > (float)DuniaFisika::WORLD_HEIGHT - 10.0f) { ag.y = (float)DuniaFisika::WORLD_HEIGHT - 10.0f; ag.vy = -ag.vy * 0.5f; }

    // 4. Penuaan & Eskalasi Rasa Lapar Terhadap Iklim / Suhu Musim
    ag.age_years += (float)(dt / ParameterAgent::SECONDS_PER_YEAR);

    // Hitung Kepadatan Populasi Tetangga (Overcrowding Stress)
    int nearby_agents_count = 0;
    for (int k = 0; k < 128; ++k) {
        int neighbor_idx = (idx + k * 17) % population_size;
        if (neighbor_idx != idx && agents[neighbor_idx].is_alive && agents[neighbor_idx].energy > 0.0f) {
            float ndx = agents[neighbor_idx].x - ag.x;
            float ndy = agents[neighbor_idx].y - ag.y;
            if ((ndx * ndx + ndy * ndy) < (float)(DuniaFisika::OVERCROWDING_RADIUS * DuniaFisika::OVERCROWDING_RADIUS)) {
                nearby_agents_count++;
            }
        }
    }

    float overcrowding_stress = 1.0f + (float)nearby_agents_count * (float)DuniaFisika::OVERCROWDING_PENALTY_MULT;
    
    // Ability Alam: 4. Wabah Pathogen pada Koloni Berkerumun Padat (>= 8 tetangga berdekatan)
    if (nearby_agents_count >= 8) {
        ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::PATHOGEN_DAMAGE_RATE * (float)dt);
    }

    float temp_diff = fabsf(climate->temperature - 20.0f);
    float heat_stress = (climate->temperature > 35.0f) ? (climate->temperature - 35.0f) * 0.08f : 0.0f;
    float co2_stress = fmaxf(0.0f, (climate->co2_level - (float)DuniaFisika::CO2_BASE_LEVEL) * 2.0f);
    float nature_stress = climate->nature_adversarial_pressure; // Kurva Tekanan Adaptif Kubu Alam AI
    float climate_stress = (1.0f + temp_diff * (float)ParameterAgent::CLIMATE_HUNGER_IMPACT_MULT + heat_stress + co2_stress) * nature_stress;
    ag.hunger_rate_mult = (1.0f + (ag.age_years / (float)ParameterAgent::MAX_AGE_YEARS) * 2.5f) * climate_stress * overcrowding_stress;
    if (ag.mating_cooldown > 0.0f) ag.mating_cooldown = fmaxf(0.0f, ag.mating_cooldown - (float)dt);

    // Ability Alam: 5. Degradasi & Karat Alat/Struktur Megalitikum (Tool Rust)
    if (ag.tools_crafted > 0) {
        ag.mined_material = fmaxf(0.0f, ag.mined_material - (float)DuniaFisika::TOOL_RUST_DECAY_RATE * (float)dt);
    }

    // Keabadian Artifact: kurangi timer, skip metabolisme & kematian lapar
    if (ag.immortality_timer > 0.0f) {
        ag.immortality_timer = fmaxf(0.0f, ag.immortality_timer - (float)dt);
        ag.energy = fminf(100.0f, ag.energy + 0.5f * (float)dt); // Pulih perlahan saat kebal
        ag.lung_oxygen = 100.0f;
    } else {
        // Sistem Tabung Oksigen & Paru-Paru (Respirasi Nyata):
        // Jika O2 atmosfer cukup (>= SAFE_MIN), paru-paru menghirup dan terisi penuh.
        // Jika O2 atmosfer tipis (< SAFE_MIN), paru-paru terkuras habis dan terjadi Hipoksia (Suffocation).
        if (climate->oxygen_level >= (float)DuniaFisika::O2_ATMOSPHERE_SAFE_MIN) {
            float o2_gain = (float)DuniaFisika::AGENT_O2_BREATHE_IN_RATE * (climate->oxygen_level / 21.0f) * (float)dt;
            ag.lung_oxygen = fminf((float)DuniaFisika::AGENT_LUNG_CAPACITY, ag.lung_oxygen + o2_gain);
        } else {
            float o2_loss = (float)DuniaFisika::AGENT_O2_CONSUMPTION_RATE * (float)dt;
            ag.lung_oxygen = fmaxf(0.0f, ag.lung_oxygen - o2_loss);
        }

        // Dinamika Dehidrasi Tubuh (Tergantung Suhu Iklim & Kecepatan Gerak)
        float speed = sqrtf(ag.vx * ag.vx + ag.vy * ag.vy);
        float heat_dehydration = (climate->temperature > 25.0f) ? (climate->temperature - 25.0f) * (float)DuniaFisika::AGENT_DEHYDRATION_HEAT_MULT : 0.0f;
        float dehyd_rate = ((float)DuniaFisika::AGENT_DEHYDRATION_BASE_RATE + heat_dehydration + speed * 0.02f) * (float)dt;
        ag.hydration = fmaxf(0.0f, ag.hydration - dehyd_rate);

        // Pengaruh Oksigen Paru-Paru & Hidrasi terhadap Metabolisme Energi
        float lung_eff = fmaxf(0.2f, ag.lung_oxygen / 100.0f);
        float hyd_eff = fmaxf(0.2f, ag.hydration / 100.0f);
        float energy_cost = ((float)ParameterAgent::METABOLISM_BASE_RATE + speed * (float)ParameterAgent::METABOLISM_MOVE_COST) * ag.hunger_rate_mult * (float)dt / (lung_eff * hyd_eff);
        ag.energy = fmaxf(0.0f, ag.energy - energy_cost);

        // Hipoksia: Jika tabung O2 paru-paru < HYPOXIA_THRESHOLD, agen mengalami sesak nafas mematikan
        if (ag.lung_oxygen < (float)DuniaFisika::HYPOXIA_THRESHOLD) {
            float suffocation_dmg = (float)DuniaFisika::HYPOXIA_SUFFOCATION_DAMAGE * (1.0f - ag.lung_oxygen / (float)DuniaFisika::HYPOXIA_THRESHOLD) * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - suffocation_dmg);
            ag.fear_level = fminf(1.0f, ag.fear_level + 0.3f * (float)dt); // Lonjakan panik akibat sesak nafas
        }

        // Dehidrasi Ekstrem: Jika hidrasi tubuh habis (<= 0), agen sekarat karena kehausan
        if (ag.hydration <= 0.0f) {
            float thirst_dmg = (float)DuniaFisika::DEHYDRATION_DAMAGE_RATE * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - thirst_dmg);
            ag.fear_level = fminf(1.0f, ag.fear_level + 0.35f * (float)dt);
        }
    }

    // Strict Health Check: Mati langsung jika kehabisan energi atau usia melebihi batas (diabaikan saat immortal)
    if ((ag.immortality_timer <= 0.0f) && (ag.energy <= (float)ParameterAgent::STARVATION_THRESHOLD || ag.age_years >= (float)ParameterAgent::MAX_AGE_YEARS)) {
        ag.is_alive = false;
        ag.just_died = true;
        ag.energy = 0.0f;
        ag.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
        return;
    }

    // 5. Interaksi dengan Pohon & Konsumsi Buah (Fotosintesis Siang/Malam Organik & Variasi Spesies)
    if (nearest_tree_idx >= 0 && nearest_tree_dist < (float)DuniaFisika::TREE_INTERACTION_RADIUS && trees[nearest_tree_idx].health > 0.0f) {
        float photo_mult = 0.5f + (float)DuniaFisika::DAY_PHOTOSYNTHESIS_MULT * climate->daylight_factor;
        float species_pool_mult = (trees[nearest_tree_idx].tree_type == DuniaFisika::TREE_TYPE_OXYGEN) ? 1.5f : 1.0f;
        float shared_pool_reward = ((float)DuniaFisika::TREE_ENERGY_POOL_RATE * photo_mult * species_pool_mult) / (float)(1 + nearby_agents_count);
        ag.energy = fminf(100.0f, ag.energy + (float)(shared_pool_reward * dt));

        // Konsumsi Buah Alami (Khusus Pohon Berbuah TREE_TYPE_FRUIT)
        if (trees[nearest_tree_idx].tree_type == DuniaFisika::TREE_TYPE_FRUIT && 
            trees[nearest_tree_idx].fruits_count >= 1.0f && ag.energy < 100.0f) {
            float old_fruits = atomicAdd(&trees[nearest_tree_idx].fruits_count, -1.0f);
            if (old_fruits >= 1.0f) {
                float nut = (float)DuniaFisika::FRUIT_NUTRITION_ENERGY;
                ag.energy = fminf(100.0f, ag.energy + nut);
                ag.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, ag.hydration + 15.0f); // Buah juga memberi hidrasi air
                ag.fruits_eaten++;
            } else {
                atomicAdd(&trees[nearest_tree_idx].fruits_count, 1.0f);
            }
        }
    }

    // 5a. Penanaman & Penyebaran Benih Pohon Organik (Geologi Tanah, Jarak Minimum, Air & Variasi Spesies)
    // Agen butuh energi (>= 60%) dan hidrasi air (>= 30%) untuk menanam bibit secara nyata
    if (ag.growth_signal > 0.4f && ag.energy > 60.0f && ag.hydration > 30.0f) {
        // Cek apakah posisi agen memenuhi jarak minimum dari pohon lain (Hukum Jarak Ekologis MIN_TREE_SPACING)
        if (nearest_tree_dist >= (float)DuniaFisika::MIN_TREE_SPACING) {
            // Hitung kesuburan geologi tanah di posisi agen
            float soil = (float)DuniaFisika::SOIL_FERTILITY_BASE + 
                         (float)DuniaFisika::SOIL_FERTILITY_VARIATION * sinf(ag.x * (float)DuniaFisika::SOIL_NOISE_SCALE_X) * cosf(ag.y * (float)DuniaFisika::SOIL_NOISE_SCALE_Y);
            
            // Hanya bisa tumbuh jika tanah memiliki kesuburan di atas ambang batas
            if (soil >= (float)DuniaFisika::TREE_SPROUT_SOIL_MIN_FERTILITY) {
                // Cari slot pohon kosong / mati untuk menumbuhkan tunas baru
                for (int t = 0; t < trees_count; ++t) {
                    if (trees[t].health <= 0.0f) {
                        float old_h = atomicExch(&trees[t].health, 50.0f);
                        if (old_h <= 0.0f) {
                            // Tentukan spesies pohon berdasarkan geologi & sinyal agen
                            unsigned int p_seed = gpu_hash((unsigned int)idx * 7919u ^ (unsigned int)t * 104729u);
                            int species = (p_seed % DuniaFisika::TREE_SPECIES_COUNT);
                            if (soil < 0.7f) species = DuniaFisika::TREE_TYPE_PIONEER; // Tanah gersang cocok untuk perintis

                            trees[t].x = ag.x;
                            trees[t].y = ag.y;
                            trees[t].growth_stage = 5.0f; // Bibit muda bertunas
                            trees[t].fruits_count = 0.0f;
                            trees[t].formula_resonance = ag.growth_signal;
                            trees[t].age_years = 0.0f;
                            trees[t].tree_type = species;
                            trees[t].soil_fertility = fmaxf(0.2f, soil);
                            trees[t].moisture = (float)DuniaFisika::TREE_MOISTURE_INITIAL; // Disiram air saat penanaman

                            ag.energy -= (float)DuniaFisika::SEED_GERMINATION_ENERGY_COST;
                            ag.hydration -= (float)DuniaFisika::SEED_GERMINATION_WATER_COST;
                            ag.formulas_discovered++;
                            break;
                        }
                    }
                }
            }
        }
    }

    // 5b. Interaksi Material Spasial Megalitikum (Penambangan Batu/Logam untuk Senjata & Benteng)
    bool has_megalith_tool = (ag.tools_crafted > 0);
    if (has_megalith_tool) {
        // Biaya metabolisme tambahan saat memikul/menggunakan peralatan megalitikum
        ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::MEGALITH_TOOL_USAGE_ENERGY_RATE * (float)dt);
    }
    if (nearest_min_idx >= 0 && nearest_min_dist < (float)DuniaFisika::MEGALITH_MINING_RADIUS && minerals[nearest_min_idx].mass > 0.0f) {
        float mined = (float)DuniaFisika::MEGALITH_MINING_RATE * 0.1f * (float)dt;
        atomicAdd(&minerals[nearest_min_idx].mass, - mined);
        ag.material_interactions++;
        ag.mined_material += mined;
        if (ag.mined_material >= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD) {
            if (ag.energy >= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST) {
                ag.mined_material -= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD;
                ag.tools_crafted++;
                ag.energy -= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST; // Biaya energi crafting alat
                has_megalith_tool = true;
            }
        }
    }

    // 6. Transmisi Gelombang Komunikasi / Feromon Antar-Agen
    if (fabsf(ag.comm_signal) > 0.1f) {
        ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::BROADCAST_ENERGY_COST * (float)dt);
        for (int k = 0; k < 64; ++k) {
            int peer_idx = (idx + k * 23) % population_size;
            if (peer_idx != idx && agents[peer_idx].is_alive) {
                float cdx = agents[peer_idx].x - ag.x;
                float cdy = agents[peer_idx].y - ag.y;
                if ((cdx * cdx + cdy * cdy) < (float)(DuniaFisika::BROADCAST_COMM_RADIUS * DuniaFisika::BROADCAST_COMM_RADIUS)) {
                    agents[peer_idx].comm_received = ag.comm_signal;
                }
            }
        }
    }

    // 6b. Reproduksi Alami Antar-Gender (Kubu A) - Ditekan Drastis Saat Suhu Ekstrem & CO2 Tinggi
    float heat_repro_penalty = (climate->temperature > 38.0f) ? (climate->temperature - 38.0f) * 1.5f : 0.0f;
    float co2_repro_penalty = (climate->co2_level > 0.5f) ? (climate->co2_level - 0.5f) * 20.0f : 0.0f;
    float req_mating_energy = (float)ParameterAgent::MATING_MIN_ENERGY + heat_repro_penalty + co2_repro_penalty;
    if (climate->temperature > 55.0f || climate->co2_level > 2.5f) {
        req_mating_energy = 999.0f; // Steril / Reproduksi lumpuh total di suhu ekstrem 58.8°C
    }

    if (ag.gender == 1 && ag.energy >= req_mating_energy && ag.mating_cooldown <= 0.0f && ag.age_years >= 15.0f && ag.age_years <= 75.0f) {
        for (int p = 0; p < population_size; ++p) {
            float partner_req_energy = req_mating_energy;
            if (p != idx && agents[p].gender == 0 && agents[p].energy >= partner_req_energy && agents[p].mating_cooldown <= 0.0f) {
                float pdx = agents[p].x - ag.x;
                float pdy = agents[p].y - ag.y;
                float pdist = sqrtf(pdx * pdx + pdy * pdy);
                if (pdist < (float)ParameterAgent::MATING_RADIUS) {
                    // Terjadi Perkawinan: Transfer & Crossover DNA ke anak
                    ag.energy -= (float)ParameterAgent::MATING_ENERGY_COST;
                    agents[p].energy -= (float)ParameterAgent::MATING_ENERGY_COST * 0.5f;
                    float cd = (float)ParameterAgent::MATING_COOLDOWN;
                    ag.mating_cooldown = cd;
                    agents[p].mating_cooldown = cd;

                    // Cari slot agen yang sudah mati untuk ditempati oleh bayi yang baru lahir
                    for (int slot = 0; slot < population_size; ++slot) {
                        if (!agents[slot].is_alive) {
                            agents[slot].is_alive = true;
                            agents[slot].just_born = true;
                            agents[slot].just_died = false;
                            agents[slot].generation = (ag.generation > agents[p].generation ? ag.generation : agents[p].generation) + 1;
                            // Determinisme Presisi: PRNG berbasis integer hash untuk reproduksi
                            unsigned int rng_seed = gpu_hash((unsigned int)slot * 1973u ^ (unsigned int)ag.id * 8191u ^ (unsigned int)agents[p].id * 2741u);
                            agents[slot].gender = (rng_seed % 2); 
                            agents[slot].sin_type = 0;
                            agents[slot].x = ag.x + (float)((slot % 5) - 2) * 4.0f;
                            agents[slot].y = ag.y + (float)((slot % 3) - 1) * 4.0f;
                            agents[slot].vx = 0.0f;
                            agents[slot].vy = 0.0f;
                            agents[slot].energy = (float)ParameterAgent::NEWBORN_INITIAL_ENERGY; // Energi awal bayi kecil, butuh langsung makan
                            agents[slot].lung_oxygen = (float)DuniaFisika::AGENT_LUNG_CAPACITY;
                            agents[slot].hydration = (float)DuniaFisika::AGENT_HYDRATION_INITIAL;
                            agents[slot].corpse_energy = 0.0f;
                            agents[slot].age_years = 0.0f;
                            agents[slot].mating_cooldown = (float)ParameterAgent::MATING_COOLDOWN * 2.0f;
                            agents[slot].hunger_rate_mult = 1.0f;
                            agents[slot].fruits_eaten = 0;
                            agents[slot].formulas_discovered = 0;
                            agents[slot].predators_slain = 0;
                            agents[slot].material_interactions = 0;
                            agents[slot].tools_crafted = 0;
                            agents[slot].mined_material = 0.0f;
                            agents[slot].active_program_size = ag.active_program_size;
                            agents[slot].active_registers_count = ag.active_registers_count;
                            agents[slot].fear_level = 0.1f;

                            // Cacat Kelahiran Biologis / Mutasi Acak Ekstrim
                            bool is_defective = ((rng_seed % 100) < (int)(DuniaFisika::DEFECTIVE_BIRTH_CHANCE * 100));
                            bool is_super_mutant = ((rng_seed % 100) < (int)(DuniaFisika::SUPER_MUTATION_CHANCE * 100));

                            if (is_defective) {
                                // Cacat biologis: metabolisme boros (lemah), energi awal sangat rendah / pincang
                                agents[slot].energy = 8.0f; 
                                agents[slot].hunger_rate_mult = 2.2f; // Cepat lapar dan rentan
                                agents[slot].active_program_size = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;
                            }

                            // Crossover Genetik DNA Induk Jantan & Betina
                            int crossover_point = ag.active_program_size / 2;
                            for (int ip = 0; ip < crossover_point; ++ip) {
                                agents[slot].dna_program[ip] = ag.dna_program[ip];
                                agents[slot].epigenetic_methylation[ip] = ag.epigenetic_methylation[ip];
                            }
                            for (int ip = crossover_point; ip < DNA_PROGRAM_SIZE; ++ip) {
                                agents[slot].dna_program[ip] = agents[p].dna_program[ip];
                                agents[slot].epigenetic_methylation[ip] = agents[p].epigenetic_methylation[ip];
                            }

                            // Mutasi Genetik Super Acak (Evolusi Opcode, Register, & Konstanta Baru / Acak Total)
                            float mutation_prob = is_super_mutant ? 0.70f : (float)ParameterAgent::DNA_MUTATION_RATE;
                            if (is_super_mutant && agents[slot].active_program_size < DNA_PROGRAM_SIZE) {
                                agents[slot].active_program_size = min((int)DNA_PROGRAM_SIZE, agents[slot].active_program_size + 1 + (int)(rng_seed % 3));
                            }
                            for (int ip = 0; ip < agents[slot].active_program_size; ++ip) {
                                unsigned int m_hash = gpu_hash(rng_seed ^ ((unsigned int)ip * 199u + (unsigned int)slot * 37u));
                                if ((m_hash % 100) < (int)(mutation_prob * 100)) {
                                    int mut_type = (m_hash / 100) % 4;
                                    if (mut_type == 0 || is_defective) {
                                        agents[slot].dna_program[ip].op = static_cast<unsigned char>((m_hash >> 3) % 15);
                                    } else if (mut_type == 1) {
                                        agents[slot].dna_program[ip].r_dest = static_cast<unsigned char>((m_hash >> 5) % REGISTERS_COUNT);
                                    } else if (mut_type == 2) {
                                        agents[slot].dna_program[ip].r_src1 = static_cast<unsigned char>((m_hash >> 7) % REGISTERS_COUNT);
                                    } else {
                                        agents[slot].dna_program[ip].immediate_val = ((float)((int)(m_hash % 400) - 200)) * 0.1f;
                                    }
                                }
                            }

                            // 2. Epigenetic Trauma Inheritance: Trauma ketakutan induk ditransfer ke anak
                            float parent_trauma = 0.5f * (ag.fear_level + agents[p].fear_level);
                            float trauma_bias = parent_trauma * (float)DuniaFisika::EPIGENETIC_TRAUMA_WEIGHT_BIAS;

                            // Inherit & mutate reservoir weights dengan Epigenetic Trauma Bias
                            for (int r = 0; r < REGISTERS_COUNT; ++r) {
                                agents[slot].registers[r] = 0.0;
                                agents[slot].prev_registers[r] = 0.0;
                                agents[slot].integrated_registers[r] = 0.0;
                                float base_win = (r % 2 == 0) ? ag.reservoir_weights_in[r] : agents[p].reservoir_weights_in[r];
                                float base_wrec = (r % 2 == 0) ? ag.reservoir_weights_rec[r] : agents[p].reservoir_weights_rec[r];

                                // Jika sensor r terkait ancaman/ketakutan (S3, S4, S5, S12), perkuat kepekaan bobot
                                if (r == 3 || r == 4 || r == 5 || r == 12 % REGISTERS_COUNT) {
                                    base_win = (base_win >= 0.0f) ? base_win + trauma_bias : base_win - trauma_bias;
                                }

                                agents[slot].reservoir_weights_in[r] = (float)gpu_clamp(base_win, -1.0, 1.0);
                                agents[slot].reservoir_weights_rec[r] = (float)gpu_clamp(base_wrec, -0.99, 0.99);

                                unsigned int w_hash = gpu_hash(rng_seed ^ ((unsigned int)r * 313u + (unsigned int)slot * 17u));
                                if ((w_hash % 100) < (int)(mutation_prob * 100)) {
                                    agents[slot].reservoir_weights_in[r] = (float)gpu_clamp(agents[slot].reservoir_weights_in[r] + ((float)((int)(w_hash % 200) - 100)) * 0.005f, -1.0, 1.0);
                                    agents[slot].reservoir_weights_rec[r] = (float)gpu_clamp(agents[slot].reservoir_weights_rec[r] + ((float)((int)((w_hash / 200) % 200) - 100)) * 0.005f, -0.99, 0.99);
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    // 7. Interaksi Duel Ekologis & Perang Megalitikum Simetris
    for (int pred_i = 0; pred_i < predators_count; ++pred_i) {
        GpuPredatorAgent& pred = predators[pred_i];
        if (!pred.is_alive || pred.energy <= 0.0f) continue;

        float pdx = pred.x - ag.x;
        float pdy = pred.y - ag.y;
        float pdist = sqrtf(pdx * pdx + pdy * pdy);

        if (pdist < (float)DuniaFisika::COMBAT_CLASH_RADIUS) {
            // Evaluasi Bonus Peralatan/Benteng Megalitikum Faksi A (Herbivora)
            float herbi_dmg_mult = 1.0f + (has_megalith_tool ? ((float)DuniaFisika::MEGALITH_WEAPON_ATTACK_BONUS / (float)DuniaFisika::HERBIVORE_BASE_ATTACK_DAMAGE) : 0.0f);
            float herbi_defense = has_megalith_tool ? (float)DuniaFisika::MEGALITH_SHIELD_DEFENSE_BONUS : 0.0f;

            // Kerusakan yang diterima Faksi A (Diabaikan saat Keabadian Artifact aktif)
            float effective_pred_dmg = (ag.immortality_timer > 0.0f) ? 0.0f :
                                       (float)DuniaFisika::PREDATOR_DAMAGE_RATE * (1.0f - herbi_defense) * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - effective_pred_dmg);

            // Predator menyerap nutrisi dari serangan yang masuk
            pred.energy = fminf(100.0f, pred.energy + effective_pred_dmg * 0.5f);

            // Serangan Fisik Balasan Herbivora (Duel Simetris)
            float herbi_strike = (float)DuniaFisika::HERBIVORE_BASE_ATTACK_DAMAGE * herbi_dmg_mult * (float)dt;
            pred.energy = fmaxf(0.0f, pred.energy - herbi_strike);

            if (pred.energy <= 0.0f) {
                pred.is_alive = false;
                pred.just_died = true;
                pred.just_killed = true;
                pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
                ag.predators_slain++;
            }

            if (ag.energy <= 0.0f) {
                pred.prey_devoured++;
                pred.energy = fminf(100.0f, pred.energy + 25.0f); // Nutrisi mangsa jatuh
                ag.is_alive = false;
                ag.just_died = true;
                ag.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
            }
        }
    }
}

__global__ void simulate_predator_step_cuda_kernel(
    GpuPredatorAgent* predators,
    int predators_count,
    GpuEcosystemAgent* agents,
    int population_size,
    GpuMineralDeposit* minerals,
    int minerals_count,
    GpuClimateState* climate,
    double dt
) {
    int p_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (p_idx >= predators_count) return;

    GpuPredatorAgent& pred = predators[p_idx];
    pred.just_killed = false;
    pred.just_died = false;
    pred.just_born = false;

    // Dekomposisi Mayat Predator yang Mati
    if (!pred.is_alive) {
        if (pred.corpse_energy > 0.0f) {
            pred.corpse_energy = fmaxf(0.0f, pred.corpse_energy - (float)DuniaFisika::CORPSE_DECAY_RATE * (float)dt);
        }
        return;
    }

    if (pred.energy <= 0.0f) {
        pred.is_alive = false;
        pred.just_died = true;
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
        return;
    }

    // Friendly Fire & Pemisahan Antar-Predator (Kubu B) - Memberikan gaya dorong pemisah agar tidak menumpuk di pojok
    for (int k = 0; k < 8; ++k) {
        int peer_p = (p_idx + k * 13 + 1) % predators_count;
        if (peer_p != p_idx && predators[peer_p].is_alive && predators[peer_p].energy > 0.0f) {
            float pcdx = predators[peer_p].x - pred.x;
            float pcdy = predators[peer_p].y - pred.y;
            float pcdist2 = pcdx * pcdx + pcdy * pcdy;
            if (pcdist2 < 8.0f * 8.0f && pcdist2 > 1e-4f) { // Kontak dekat saling dorong menjauh
                float pcdist = sqrtf(pcdist2);
                pred.vx -= (pcdx / pcdist) * 2.0f;
                pred.vy -= (pcdy / pcdist) * 2.0f;
            }
        }
    }

    // Penuaan & Batas Usia Predator
    pred.age_years += (float)(dt / ParameterAgent::SECONDS_PER_YEAR);
    if (pred.mating_cooldown > 0.0f) {
        pred.mating_cooldown = fmaxf(0.0f, pred.mating_cooldown - (float)dt);
    }

    if (pred.age_years >= (float)ParameterAgent::MAX_AGE_YEARS) {
        pred.energy = 0.0f;
        pred.is_alive = false;
        pred.just_died = true;
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
        return;
    }

    // 1. SENSORIK PREDATOR: Target Mangsa Hidup, Bangkai (Scavenging), & Formasi Megalitikum
    float nearest_prey_dist = 10000.0f;
    float nearest_prey_dx = 0.0f;
    float nearest_prey_dy = 0.0f;
    float nearest_prey_energy = 0.0f;

    float nearest_corpse_dist = 10000.0f;
    float nearest_corpse_dx = 0.0f;
    float nearest_corpse_dy = 0.0f;
    int nearest_corpse_agent_idx = -1;
    bool is_pred_corpse = false;

    // Scan mangsa hidup dan mayat Kubu A
    for (int i = 0; i < population_size; ++i) {
        float dx = agents[i].x - pred.x;
        float dy = agents[i].y - pred.y;
        float d = sqrtf(dx * dx + dy * dy);

        if (agents[i].is_alive && agents[i].energy > 0.0f) {
            if (d < nearest_prey_dist) {
                nearest_prey_dist = d;
                nearest_prey_dx = dx;
                nearest_prey_dy = dy;
                nearest_prey_energy = agents[i].energy;
            }
        } else if (!agents[i].is_alive && agents[i].corpse_energy > 0.5f) {
            if (d < nearest_corpse_dist) {
                nearest_corpse_dist = d;
                nearest_corpse_dx = dx;
                nearest_corpse_dy = dy;
                nearest_corpse_agent_idx = i;
                is_pred_corpse = false;
            }
        }
    }

    // Scan mayat sesama Predator (Karnivora memakan bangkai apapun yang ada di ekosistem)
    for (int p = 0; p < predators_count; ++p) {
        if (p != p_idx && !predators[p].is_alive && predators[p].corpse_energy > 0.5f) {
            float dx = predators[p].x - pred.x;
            float dy = predators[p].y - pred.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < nearest_corpse_dist) {
                nearest_corpse_dist = d;
                nearest_corpse_dx = dx;
                nearest_corpse_dy = dy;
                nearest_corpse_agent_idx = p;
                is_pred_corpse = true;
            }
        }
    }

    // Cari Deposit Mineral Megalitikum Terdekat untuk Predator
    float nearest_min_dist = 10000.0f;
    float nearest_min_dx = 0.0f;
    float nearest_min_dy = 0.0f;
    int nearest_min_idx = -1;
    for (int m = 0; m < minerals_count; ++m) {
        if (minerals[m].mass <= 0.0f) continue;
        float dx = minerals[m].x - pred.x;
        float dy = minerals[m].y - pred.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < nearest_min_dist) {
            nearest_min_dist = d;
            nearest_min_dx = dx;
            nearest_min_dy = dy;
            nearest_min_idx = m;
        }
    }

    // Eksploitasi Material Megalitikum oleh Karnivora
    bool pred_has_tool = (pred.tools_crafted > 0);
    if (pred_has_tool) {
        pred.energy = fmaxf(0.0f, pred.energy - (float)DuniaFisika::MEGALITH_TOOL_USAGE_ENERGY_RATE * (float)dt);
    }
    if (nearest_min_idx >= 0 && nearest_min_dist < (float)DuniaFisika::MEGALITH_MINING_RADIUS && minerals[nearest_min_idx].mass > 0.0f) {
        float mined = (float)DuniaFisika::MEGALITH_MINING_RATE * 0.1f * (float)dt;
        atomicAdd(&minerals[nearest_min_idx].mass, - mined);
        pred.material_interactions++;
        pred.mined_material += mined;
        if (pred.mined_material >= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD) {
            if (pred.energy >= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST) {
                pred.mined_material -= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD;
                pred.tools_crafted++;
                pred.energy -= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST;
            }
        }
    }

    // Pemangsaan Bangkai (Scavenging) Adil oleh Karnivora
    if (nearest_corpse_agent_idx >= 0 && nearest_corpse_dist < (float)DuniaFisika::CORPSE_SCAVENGE_RADIUS && pred.energy < 100.0f) {
        if (!is_pred_corpse) {
            float bite = fminf(agents[nearest_corpse_agent_idx].corpse_energy, (float)DuniaFisika::CARNIVORE_CORPSE_EAT_RATE * (float)dt);
            agents[nearest_corpse_agent_idx].corpse_energy -= bite;
            pred.energy = fminf(100.0f, pred.energy + bite * (float)DuniaFisika::CORPSE_CARNIVORE_RECOVERY);
        } else {
            float bite = fminf(predators[nearest_corpse_agent_idx].corpse_energy, (float)DuniaFisika::CARNIVORE_CORPSE_EAT_RATE * (float)dt);
            predators[nearest_corpse_agent_idx].corpse_energy -= bite;
            pred.energy = fminf(100.0f, pred.energy + bite * (float)DuniaFisika::CORPSE_CARNIVORE_RECOVERY);
        }
    }

    // Sensorik Spasial Predator
    float target_dx = (nearest_prey_dist < 10000.0f) ? nearest_prey_dx : 0.0f;
    float target_dy = (nearest_prey_dist < 10000.0f) ? nearest_prey_dy : 0.0f;
    float target_dist = (nearest_prey_dist < 10000.0f) ? nearest_prey_dist : 1000.0f;

    // Jika ada bangkai lebih dekat dari mangsa hidup saat lapar, prioritaskan makan bangkai
    if (nearest_corpse_dist < nearest_prey_dist && pred.energy < 70.0f) {
        target_dx = nearest_corpse_dx;
        target_dy = nearest_corpse_dy;
        target_dist = nearest_corpse_dist;
    }

    // Hitung Jarak ke Alur Sungai Terdekat Predator (Meandering River)
    float p_river_x = (float)DuniaFisika::RIVER_CENTER_X + (float)DuniaFisika::RIVER_MEANDER_AMP * sinf(pred.y * (float)DuniaFisika::RIVER_MEANDER_FREQ);
    float p_dist_to_river = fabsf(pred.x - p_river_x);
    float p_river_dx = p_river_x - pred.x;

    // Minum Air di Tepi Sungai (Predator)
    if (p_dist_to_river <= (float)DuniaFisika::RIVER_DRINK_RADIUS) {
        pred.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, pred.hydration + (float)DuniaFisika::AGENT_DRINK_WATER_RATE * (float)dt);
    }

    // =========================================================================
    // PILAR 2: SENSOR TIGA PILAR (TERKOMPRESI 16 FLOAT) - PREDATOR
    // =========================================================================
    // 1. EXTEROCEPTION (Dunia Luar Predator: S0 - S9)
    float vis_factor = 0.5f + 0.5f * climate->daylight_factor;
    float s0_prey_dx = (target_dx / (target_dist + 1e-3f)) * vis_factor;
    float s1_prey_dy = (target_dy / (target_dist + 1e-3f)) * vis_factor;
    float s2_prey_dist = 1.0f - (float)gpu_clamp(target_dist / 500.0f, 0.0, 1.0);
    float s3_corpse_dx = (nearest_corpse_dist < 10000.0f) ? (nearest_corpse_dx / (nearest_corpse_dist + 1e-3f)) * vis_factor : 0.0f;
    float s4_corpse_dy = (nearest_corpse_dist < 10000.0f) ? (nearest_corpse_dy / (nearest_corpse_dist + 1e-3f)) * vis_factor : 0.0f;
    float s5_corpse_dist = (nearest_corpse_dist < 10000.0f) ? (1.0f - (float)gpu_clamp(nearest_corpse_dist / 300.0f, 0.0, 1.0)) : 0.0f;
    float s6_river_dx = (p_river_dx / (p_dist_to_river + 1e-3f)) * vis_factor; // Sensor arah sungai
    float s7_river_dist = 1.0f - (float)gpu_clamp(p_dist_to_river / 300.0f, 0.0, 1.0); // Sensor jarak sungai
    float s8_comm_recv = (float)tanh(pred.comm_received);
    float s9_climate_ambient = (float)tanh((climate->temperature - 20.0f) * 0.05f) * climate->daylight_factor;

    // 2. INTEROCEPTION (Kondisi Tubuh Predator: S10 - S12)
    float s10_energy = (float)gpu_clamp(pred.energy / 100.0f, 0.0, 1.0);
    float s11_hydration = (float)gpu_clamp(pred.hydration / 100.0f, 0.0, 1.0);
    float raw_pred_stress = (1.0f - s10_energy) * 0.4f + (1.0f - s11_hydration) * 0.3f + (pred.formula_shield) * 0.3f;
    float pred_fear_sig = 1.0f / (1.0f + expf(-(float)DuniaFisika::FEAR_SIGMOID_STEEPNESS * (raw_pred_stress - (float)DuniaFisika::FEAR_SIGMOID_MIDPOINT)));
    float s12_frenzy_drive = (float)gpu_clamp(pred_fear_sig, 0.0, 1.0);

    // 3. PROPRIOCEPTION (Kesadaran Gerak & Senjata Predator: S13 - S15)
    float p_spd = (float)DuniaFisika::PREDATOR_SPEED;
    float s13_vx = (float)gpu_clamp(pred.vx / (p_spd + 1e-3f), -1.0, 1.0);
    float s14_vy = (float)gpu_clamp(pred.vy / (p_spd + 1e-3f), -1.0, 1.0);
    float s15_tool_status = (pred.tools_crafted > 0) ? 1.0f : (float)gpu_clamp(pred.mined_material / (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD, 0.0, 0.9);

    float pred_sensor_inputs[ParameterAgent::SENSORS_COUNT] = {
        s0_prey_dx, s1_prey_dy, s2_prey_dist, s3_corpse_dx,
        s4_corpse_dy, s5_corpse_dist, s6_river_dx, s7_river_dist,
        s8_comm_recv, s9_climate_ambient, s10_energy, s11_hydration,
        s12_frenzy_drive, s13_vx, s14_vy, s15_tool_status
    };

    // =========================================================================
    // PILAR 3: RECURRENT RESERVOIR LOOP (Spatial Depth 64 Step + Temporal T) - PREDATOR
    // =========================================================================
    int max_r = (pred.active_registers_count > 0 && pred.active_registers_count <= REGISTERS_COUNT) ? 
                pred.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;
    int active_prog = (pred.active_program_size > 0 && pred.active_program_size <= DNA_PROGRAM_SIZE) ? 
                      pred.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

    // Reservoir state update dengan dual-channel input projection + recurrent loop
    for (int r = 0; r < max_r; ++r) {
        float in_signal = pred.reservoir_weights_in[r] * pred_sensor_inputs[r % ParameterAgent::SENSORS_COUNT] +
                          pred.reservoir_weights_in[(r + 8) % REGISTERS_COUNT] * pred_sensor_inputs[(r + 8) % ParameterAgent::SENSORS_COUNT];
        float rec_signal = pred.reservoir_weights_rec[r] * (float)pred.prev_registers[r];
        double u_val = in_signal + rec_signal;
        pred.registers[r] = (1.0 - ParameterAgent::RESERVOIR_SPECTRAL_RADIUS) * pred.registers[r] + 
                            ParameterAgent::RESERVOIR_SPECTRAL_RADIUS * tanh(u_val);
    }

    // Predictive Loss (Predator): Koreksi error prediksi sensor prey t-1 → t
    float pred_sensors_now[ParameterAgent::PRED_SENSORS_COUNT] = { s0_prey_dx, s1_prey_dy, s10_energy, s2_prey_dist };
    for (int r = 0; r < max_r; ++r) {
        float pred_error = pred_sensors_now[r % ParameterAgent::PRED_SENSORS_COUNT] - pred.pred_sensor_prev[r % ParameterAgent::PRED_SENSORS_COUNT];
        pred.registers[r] = gpu_clamp(pred.registers[r] + ParameterAgent::PREDICTIVE_LOSS_SCALE * pred_error, -5.0, 5.0);
    }
    pred.pred_sensor_prev[0] = s0_prey_dx;
    pred.pred_sensor_prev[1] = s1_prey_dy;
    pred.pred_sensor_prev[2] = s10_energy;
    pred.pred_sensor_prev[3] = s2_prey_dist;

    float move_cmd_x = 0.0f;
    float move_cmd_y = 0.0f;
    float broadcast_out = 0.0f;

    // Eksekusi Virtual Machine DNA (Kedalaman 64 Step Spatial Ops - Predator)
    for (int ip = 0; ip < active_prog; ++ip) {
        const auto& inst = pred.dna_program[ip];
        int rd = inst.r_dest % max_r;
        int rs1 = inst.r_src1 % max_r;
        int rs2 = inst.r_src2 % max_r;

        switch (inst.op % 15) {
            case OP_NOP: break;
            case OP_LOAD_SENSOR: {
                int s = inst.r_src1 % ParameterAgent::SENSORS_COUNT;
                pred.registers[rd] = pred_sensor_inputs[s];
                break;
            }
            case OP_ADD: pred.registers[rd] = gpu_clamp(pred.registers[rs1] + pred.registers[rs2], -5.0, 5.0); break;
            case OP_SUB: pred.registers[rd] = gpu_clamp(pred.registers[rs1] - pred.registers[rs2], -5.0, 5.0); break;
            case OP_MUL: pred.registers[rd] = gpu_clamp(pred.registers[rs1] * pred.registers[rs2], -5.0, 5.0); break;
            case OP_DIV: pred.registers[rd] = gpu_clamp(pred.registers[rs1] / (fabs(pred.registers[rs2]) + 1e-3), -5.0, 5.0); break;
            case OP_TANH: pred.registers[rd] = tanh(pred.registers[rs1]); break;
            case OP_SIGMOID: pred.registers[rd] = 1.0 / (1.0 + exp(-gpu_clamp(pred.registers[rs1], -5.0, 5.0))); break;
            case OP_INTEGRAL: {
                pred.integrated_registers[rd] += pred.registers[rs1] * dt;
                pred.registers[rd] = tanh(pred.integrated_registers[rd]);
                break;
            }
            case OP_DERIVATIVE: {
                pred.registers[rd] = gpu_clamp((pred.registers[rs1] - pred.prev_registers[rd]) / (dt + 1e-4), -5.0, 5.0);
                pred.prev_registers[rd] = pred.registers[rs1];
                break;
            }
            case OP_ACTION_MOVE: {
                move_cmd_x = (float)tanh(pred.registers[rs1]);
                move_cmd_y = (float)tanh(pred.registers[rs2]);
                break;
            }
            case OP_ACTION_FORMULA: {
                broadcast_out = (float)tanh(pred.registers[rs1]);
                pred.formula_shield = (float)fabs(broadcast_out); // Output adaptasi perisai musuh
                pred.comm_signal = broadcast_out;
                break;
            }
            case OP_RESONATE_CLIMATE: {
                pred.registers[rd] = (float)tanh(pred.registers[rs1] * s9_climate_ambient);
                break;
            }
            case OP_FORK_NEURON: {
                if (pred.active_program_size < DNA_PROGRAM_SIZE && pred.energy > 70.0f) {
                    pred.active_program_size++;
                }
                break;
            }
            case OP_PRUNE_NEURON: {
                if (pred.active_program_size > ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE && pred.energy < 20.0f) {
                    pred.active_program_size--;
                }
                break;
            }
            case OP_WRITE_CODE: {
                // Self-Modifying Code Predator
                int target_ip = (int)fabs(pred.registers[rs1]) % DNA_PROGRAM_SIZE;
                unsigned char new_op = (unsigned char)((int)fabs(pred.registers[rs2]) % 19);
                pred.dna_program[target_ip].op = new_op;
                pred.dna_program[target_ip].r_dest = (unsigned char)((int)fabs(pred.registers[rd]) % max_r);
                break;
            }
            case OP_MUTATE_SELF: {
                // Self-Metaprogramming Predator
                if (pred.energy > 35.0f) {
                    int target_ip = (int)fabs(pred.registers[rs1]) % active_prog;
                    float mod_val = (float)pred.registers[rs2] * 0.1f;
                    pred.dna_program[target_ip].immediate_val += mod_val;
                    pred.energy -= 0.05f;
                }
                break;
            }
            case OP_ALLOC_REG: {
                // Dynamic Working Memory Allocation Predator
                if (pred.active_registers_count < REGISTERS_COUNT && pred.energy > 50.0f) {
                    pred.active_registers_count++;
                    pred.energy -= 0.1f;
                }
                break;
            }
            case OP_FREE_REG: {
                // Dynamic Memory Deallocation Predator
                if (pred.active_registers_count > ParameterAgent::MIN_DYNAMIC_REGISTERS) {
                    pred.active_registers_count--;
                }
                break;
            }
            default: break;
        }
    }

    // Hebbian Plasticity Real-Time (Predator) + Neuromodulation Lonjakan Plastisitas saat Krisis
    float pred_plasticity_neuromod = 1.0f + s12_frenzy_drive * (float)DuniaFisika::FEAR_NEUROMODULATION_PLASTICITY;
    for (int r = 0; r < max_r; ++r) {
        float hebb_delta = (float)(ParameterAgent::HEBBIAN_LEARNING_RATE * pred_plasticity_neuromod * pred_sensor_inputs[r % ParameterAgent::SENSORS_COUNT] * pred.registers[r]);
        pred.reservoir_weights_in[r] += hebb_delta - (float)(ParameterAgent::HEBBIAN_DECAY * pred.reservoir_weights_in[r]);
        if (pred.reservoir_weights_in[r] > 1.0f) pred.reservoir_weights_in[r] = 1.0f;
        if (pred.reservoir_weights_in[r] < -1.0f) pred.reservoir_weights_in[r] = -1.0f;
        pred.prev_registers[r] = pred.registers[r];
    }

    // 2d. Swarm Signal Quantization (Predator): {-1.0, 0.0, +1.0}
    float p_quant_thresh = (float)ParameterAgent::SWARM_QUANT_THRESHOLD;
    if (pred.comm_signal > p_quant_thresh)       pred.comm_signal = 1.0f;
    else if (pred.comm_signal < -p_quant_thresh) pred.comm_signal = -1.0f;
    else                                          pred.comm_signal = 0.0f;

    // 3. DINAMIKA GERAK FISIK PREDATOR (Inersia & Hukum Gerak Fisika)
    float p_speed_base = (float)DuniaFisika::PREDATOR_SPEED;

    float target_vx = move_cmd_x * p_speed_base;
    float target_vy = move_cmd_y * p_speed_base;

    pred.vx = pred.vx * (float)DuniaFisika::FRICTION_COEFF + target_vx * 0.2f;
    pred.vy = pred.vy * (float)DuniaFisika::FRICTION_COEFF + target_vy * 0.2f;

    pred.x += pred.vx * (float)dt;
    pred.y += pred.vy * (float)dt;

    // Batas Tepi Dunia (Hukum Pantulan Tumbukan Dinding Elastis)
    if (pred.x < 10.0f) { pred.x = 10.0f; pred.vx = -pred.vx * 0.5f; }
    if (pred.x > (float)DuniaFisika::WORLD_WIDTH - 10.0f) { pred.x = (float)DuniaFisika::WORLD_WIDTH - 10.0f; pred.vx = -pred.vx * 0.5f; }
    if (pred.y < 10.0f) { pred.y = 10.0f; pred.vy = -pred.vy * 0.5f; }
    if (pred.y > (float)DuniaFisika::WORLD_HEIGHT - 10.0f) { pred.y = (float)DuniaFisika::WORLD_HEIGHT - 10.0f; pred.vy = -pred.vy * 0.5f; }

    // 4. Metabolisme & Kematian Predator (Dipengaruhi Kurva Tekanan Alam AI & Paru-Paru)
    if (pred.immortality_timer > 0.0f) {
        pred.immortality_timer = fmaxf(0.0f, pred.immortality_timer - (float)dt);
        pred.energy = fminf(100.0f, pred.energy + 0.5f * (float)dt); // Pulih saat kebal
        pred.lung_oxygen = 100.0f;
    } else {
        // Tabung Oksigen Paru-Paru Predator:
        if (climate->oxygen_level >= (float)DuniaFisika::O2_ATMOSPHERE_SAFE_MIN) {
            float o2_gain = (float)DuniaFisika::AGENT_O2_BREATHE_IN_RATE * (climate->oxygen_level / 21.0f) * (float)dt;
            pred.lung_oxygen = fminf((float)DuniaFisika::AGENT_LUNG_CAPACITY, pred.lung_oxygen + o2_gain);
        } else {
            float o2_loss = (float)DuniaFisika::AGENT_O2_CONSUMPTION_RATE * (float)dt;
            pred.lung_oxygen = fmaxf(0.0f, pred.lung_oxygen - o2_loss);
        }

        // Dinamika Dehidrasi Predator (Tergantung Suhu & Gerak)
        float p_speed = sqrtf(pred.vx * pred.vx + pred.vy * pred.vy);
        float p_heat_dehydration = (climate->temperature > 25.0f) ? (climate->temperature - 25.0f) * (float)DuniaFisika::AGENT_DEHYDRATION_HEAT_MULT : 0.0f;
        float p_dehyd_rate = ((float)DuniaFisika::AGENT_DEHYDRATION_BASE_RATE + p_heat_dehydration + p_speed * 0.02f) * (float)dt;
        pred.hydration = fmaxf(0.0f, pred.hydration - p_dehyd_rate);

        float p_lung_eff = fmaxf(0.2f, pred.lung_oxygen / 100.0f);
        float p_hyd_eff = fmaxf(0.2f, pred.hydration / 100.0f);
        float p_nature_stress = climate->nature_adversarial_pressure;
        float p_energy_cost = ((float)DuniaFisika::PREDATOR_METABOLISM + p_speed * (float)ParameterAgent::METABOLISM_MOVE_COST) * p_nature_stress * (float)dt / (p_lung_eff * p_hyd_eff);
        pred.energy = fmaxf(0.0f, pred.energy - p_energy_cost);

        // Hipoksia Predator
        if (pred.lung_oxygen < (float)DuniaFisika::HYPOXIA_THRESHOLD) {
            float suffocation_dmg = (float)DuniaFisika::HYPOXIA_SUFFOCATION_DAMAGE * (1.0f - pred.lung_oxygen / (float)DuniaFisika::HYPOXIA_THRESHOLD) * (float)dt;
            pred.energy = fmaxf(0.0f, pred.energy - suffocation_dmg);
        }

        // Dehidrasi Predator
        if (pred.hydration <= 0.0f) {
            float thirst_dmg = (float)DuniaFisika::DEHYDRATION_DAMAGE_RATE * (float)dt;
            pred.energy = fmaxf(0.0f, pred.energy - thirst_dmg);
        }
    }

    // Transmisi Komunikasi Koordinasi Antar-Predator
    if (fabsf(pred.comm_signal) > 0.1f) {
        for (int k = 0; k < 64; ++k) {
            int peer_idx = (p_idx + k * 19) % predators_count;
            if (peer_idx != p_idx && predators[peer_idx].is_alive) {
                float cdx = predators[peer_idx].x - pred.x;
                float cdy = predators[peer_idx].y - pred.y;
                if ((cdx * cdx + cdy * cdy) < (float)(DuniaFisika::BROADCAST_COMM_RADIUS * DuniaFisika::BROADCAST_COMM_RADIUS * 1.5f)) {
                    predators[peer_idx].comm_received = pred.comm_signal;
                }
            }
        }
    }

    if (pred.immortality_timer <= 0.0f && pred.energy <= 0.0f) {
        pred.is_alive = false;
        pred.just_died = true;
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE * 1.5f;
        return;
    }

    // 5. Reproduksi Seksual Predator (Determinisme Presisi)
    float p_req_mate_energy = (float)DuniaFisika::PREDATOR_MATING_MIN_ENERGY;
    if (pred.gender == 1 && pred.energy >= p_req_mate_energy && pred.mating_cooldown <= 0.0f) {
        for (int m = 0; m < predators_count; ++m) {
            float p_partner_req = (float)DuniaFisika::PREDATOR_MATING_MIN_ENERGY;
            if (m == p_idx || !predators[m].is_alive || predators[m].gender != 0 || predators[m].energy < p_partner_req) continue;

            float mdx = predators[m].x - pred.x;
            float mdy = predators[m].y - pred.y;
            float mdist = sqrtf(mdx * mdx + mdy * mdy);

            if (mdist < (float)DuniaFisika::PREDATOR_MATING_RADIUS) {
                // Temukan slot predator kosong untuk melahirkan anak baru
                for (int slot = 0; slot < predators_count; ++slot) {
                    if (!predators[slot].is_alive && predators[slot].energy <= 0.0f) {
                        pred.energy -= (float)DuniaFisika::PREDATOR_MATING_COST;
                        float p_cd = (float)DuniaFisika::PREDATOR_MATING_COOLDOWN;
                        pred.mating_cooldown = p_cd;
                        predators[m].mating_cooldown = p_cd;

                        unsigned int rng_seed = gpu_hash((unsigned int)p_idx * 31337u ^ (unsigned int)slot * 7919u ^ (unsigned int)predators[m].id * 4099u);

                        predators[slot].is_alive = true;
                        predators[slot].just_born = true;
                        predators[slot].just_died = false;
                        predators[slot].generation = max(pred.generation, predators[m].generation) + 1;
                        predators[slot].gender = (rng_seed >> 4) % 2;
                        predators[slot].sin_type = 0;
                        predators[slot].x = pred.x + ((float)((int)(rng_seed % 20) - 10));
                        predators[slot].y = pred.y + ((float)((int)((rng_seed / 20) % 20) - 10));
                        predators[slot].vx = 0.0f;
                        predators[slot].vy = 0.0f;
                        predators[slot].energy = (float)ParameterAgent::NEWBORN_INITIAL_ENERGY;
                        predators[slot].lung_oxygen = (float)DuniaFisika::AGENT_LUNG_CAPACITY;
                        predators[slot].hydration = (float)DuniaFisika::AGENT_HYDRATION_INITIAL;
                        predators[slot].corpse_energy = 0.0f;
                        predators[slot].age_years = 0.0f;
                        predators[slot].prey_devoured = 0;
                        predators[slot].material_interactions = 0;
                        predators[slot].tools_crafted = 0;
                        predators[slot].mined_material = 0.0f;
                        // Cacat Kelahiran Biologis / Mutasi Acak Ekstrim Predator
                        bool is_defective_pred = ((rng_seed % 100) < (int)(DuniaFisika::DEFECTIVE_BIRTH_CHANCE * 100));
                        bool is_super_mutant_pred = ((rng_seed % 100) < (int)(DuniaFisika::SUPER_MUTATION_CHANCE * 100));

                        predators[slot].energy = is_defective_pred ? 8.0f : (float)ParameterAgent::NEWBORN_INITIAL_ENERGY;
                        predators[slot].formula_shield = is_defective_pred ? 0.05f : (float)((rng_seed % 100)) / 100.0f;

                        predators[slot].active_program_size = is_super_mutant_pred ? min((int)DNA_PROGRAM_SIZE, pred.active_program_size + 1 + (int)(rng_seed % 3)) : pred.active_program_size;
                        predators[slot].active_registers_count = pred.active_registers_count;

                        // Crossover DNA & Mutasi Genetik Keturunan Predator
                        float pred_mut_prob = is_super_mutant_pred ? 0.70f : (float)ParameterAgent::DNA_MUTATION_RATE;
                        for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                            predators[slot].dna_program[ip] = (ip % 2 == 0) ? pred.dna_program[ip] : predators[m].dna_program[ip];

                            unsigned int m_hash = gpu_hash(rng_seed ^ ((unsigned int)ip * 131u + (unsigned int)slot * 71u));
                            if ((m_hash % 100) < (int)(pred_mut_prob * 100)) {
                                int mut_type = (m_hash / 100) % 4;
                                if (mut_type == 0 || is_defective_pred) {
                                    predators[slot].dna_program[ip].op = static_cast<unsigned char>((m_hash >> 3) % 15);
                                } else if (mut_type == 1) {
                                    predators[slot].dna_program[ip].r_dest = static_cast<unsigned char>((m_hash >> 5) % REGISTERS_COUNT);
                                } else if (mut_type == 2) {
                                    predators[slot].dna_program[ip].r_src1 = static_cast<unsigned char>((m_hash >> 7) % REGISTERS_COUNT);
                                } else {
                                    predators[slot].dna_program[ip].immediate_val = ((float)((int)(m_hash % 400) - 200)) * 0.1f;
                                }
                            }
                        }

                        // 2. Epigenetic Trauma Inheritance Predator: Trauma stres/kelaparan induk ditransfer ke anak
                        float pred_parent_trauma = 0.5f * (s12_frenzy_drive + 0.5f);
                        float pred_trauma_bias = pred_parent_trauma * (float)DuniaFisika::EPIGENETIC_TRAUMA_WEIGHT_BIAS;

                        // Inherit & mutate reservoir weights (Predator) dengan Trauma Bias
                        for (int r = 0; r < REGISTERS_COUNT; ++r) {
                            predators[slot].registers[r] = 0.0;
                            predators[slot].prev_registers[r] = 0.0;
                            predators[slot].integrated_registers[r] = 0.0;
                            float base_p_win = (r % 2 == 0) ? pred.reservoir_weights_in[r] : predators[m].reservoir_weights_in[r];
                            float base_p_wrec = (r % 2 == 0) ? pred.reservoir_weights_rec[r] : predators[m].reservoir_weights_rec[r];

                            // Perkuat respon sensorik interoception / kelaparan (S10, S11, S12)
                            if (r == 0 || r == 1 || r == 10 % REGISTERS_COUNT || r == 12 % REGISTERS_COUNT) {
                                base_p_win = (base_p_win >= 0.0f) ? base_p_win + pred_trauma_bias : base_p_win - pred_trauma_bias;
                            }

                            predators[slot].reservoir_weights_in[r] = (float)gpu_clamp(base_p_win, -1.0, 1.0);
                            predators[slot].reservoir_weights_rec[r] = (float)gpu_clamp(base_p_wrec, -0.99, 0.99);

                            unsigned int pw_hash = gpu_hash(rng_seed ^ ((unsigned int)r * 277u + (unsigned int)slot * 19u));
                            if ((pw_hash % 100) < (int)(pred_mut_prob * 100)) {
                                predators[slot].reservoir_weights_in[r] = (float)gpu_clamp(predators[slot].reservoir_weights_in[r] + ((float)((int)(pw_hash % 200) - 100)) * 0.005f, -1.0, 1.0);
                                predators[slot].reservoir_weights_rec[r] = (float)gpu_clamp(predators[slot].reservoir_weights_rec[r] + ((float)((int)((pw_hash / 200) % 200) - 100)) * 0.005f, -0.99, 0.99);
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
}

extern "C" void launch_ecosystem_simulation(
    GpuEcosystemAgent* d_agents,
    int population_size,
    GpuTreeEntity* d_trees,
    int trees_count,
    GpuPredatorAgent* d_predators,
    int predators_count,
    GpuMineralDeposit* d_minerals,
    int minerals_count,
    GpuClimateState* d_climate,
    double dt
) {
    int blockSize = 128;
    int numBlocksAgents = (population_size + blockSize - 1) / blockSize;
    simulate_ecosystem_step_cuda_kernel<<<numBlocksAgents, blockSize>>>(
        d_agents, population_size, d_trees, trees_count, d_predators, predators_count, d_minerals, minerals_count, d_climate, dt
    );

    int numBlocksPred = (predators_count + blockSize - 1) / blockSize;
    simulate_predator_step_cuda_kernel<<<numBlocksPred, blockSize>>>(
        d_predators, predators_count, d_agents, population_size, d_minerals, minerals_count, d_climate, dt
    );
    cudaDeviceSynchronize();
}