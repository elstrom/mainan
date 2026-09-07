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
    OP_PRUNE_NEURON      // Synaptic Pruning: Ringkaskan sirkuit jika tenang & stabil
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
    float fruits_count;      // 0 - 6 buah
    float formula_resonance; // Keselarasan rumus kognisi agen
    float age_years;         // Usia pohon
    float health;            // Vitalitas pohon (0.0 - 100.0%)
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
    float age_years;         // Usia dalam Tahun (0 - 100 Tahun)
    float hunger_rate_mult;  // Pengali rasa lapar (Meningkat seiring penuaan)
    float mating_cooldown;   // Waktu tunggu sebelum bisa reproduksi kembali
    int fruits_eaten;        // Jumlah buah yang berhasil dikonsumsi
    int formulas_discovered; // Jumlah rumus pertumbuhan yang berhasil dicetuskan
    int predators_slain;     // Jumlah predator yang berhasil dikalahkan agen dengan rumus
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
    float age_years;
    float mating_cooldown;
    float formula_shield;    // Perisai kognitif musuh (0.0 - 1.0)
    float comm_signal;       // Pesan siaran koordinasi kawanan serigala predator
    float comm_received;     // Pesan koordinasi yang didengar dari predator kawan
    int prey_devoured;
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
    DnaInstruction dna_program[DNA_PROGRAM_SIZE];
};

__device__ inline double gpu_clamp(double v, double min_val, double max_val) {
    if (v < min_val) return min_val;
    if (v > max_val) return max_val;
    return v;
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

    // Cari Predator Terdekat untuk insting waspada & manuver menghindar
    float nearest_pred_dist = 10000.0f;
    for (int p = 0; p < predators_count; ++p) {
        if (predators[p].is_alive && predators[p].energy > 0.0f) {
            float pdx = predators[p].x - ag.x;
            float pdy = predators[p].y - ag.y;
            float pd = sqrtf(pdx * pdx + pdy * pdy);
            if (pd < nearest_pred_dist) {
                nearest_pred_dist = pd;
            }
        }
    }

    // Sensorik Lingkungan & Spasial Berevolusi
    float vis_factor = 0.4f + 0.6f * climate->daylight_factor; // Penglihatan tajam saat siang (1.0), redup saat malam (0.4)
    float norm_tree_dx = (nearest_tree_dx / (nearest_tree_dist + 1e-3f)) * vis_factor;
    float norm_tree_dy = (nearest_tree_dy / (nearest_tree_dist + 1e-3f)) * vis_factor;
    float norm_min_dx = (nearest_min_dx / (nearest_min_dist + 1e-3f)) * vis_factor;
    float norm_min_dy = (nearest_min_dy / (nearest_min_dist + 1e-3f)) * vis_factor;
    float norm_min_dist = (float)gpu_clamp(nearest_min_dist / 500.0f, 0.0, 1.0);
    float norm_energy = ag.energy / 100.0f;
    float norm_temp = (climate->temperature - 15.0f) / 30.0f;
    float norm_pred_dist = (float)gpu_clamp(nearest_pred_dist / 300.0f, 0.0, 1.0); // Sensor Radar Deteksi Predator
    float norm_age = ag.age_years / 100.0f;

    // Dynamic Fear: Respon alami terhadap kegelapan malam, ancaman predator dekat, dan kelaparan
    ag.fear_level = (float)gpu_clamp((1.0f - norm_energy) * 0.4f + (1.0f - norm_pred_dist) * 0.5f, 0.0, 1.0);

    // 2. Eksekusi Program DNA Virtual Machine
    int max_r = (ag.active_registers_count > 0 && ag.active_registers_count <= REGISTERS_COUNT) ? 
                ag.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;
    int active_prog = (ag.active_program_size > 0 && ag.active_program_size <= DNA_PROGRAM_SIZE) ? 
                      ag.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

    // Leaky Memory Decay
    for (int r = 0; r < max_r; ++r) {
        ag.registers[r] *= 0.95;
    }

    float move_cmd_x = 0.0f;
    float move_cmd_y = 0.0f;
    float broadcast_out = 0.0f;

    for (int ip = 0; ip < active_prog; ++ip) {
        const auto& inst = ag.dna_program[ip];
        int rd = inst.r_dest % max_r;
        int rs1 = inst.r_src1 % max_r;
        int rs2 = inst.r_src2 % max_r;

        switch (inst.op % 15) {
            case OP_NOP: break;
            case OP_LOAD_SENSOR: {
                int s = inst.r_src1 % 8;
                if (s == 0) ag.registers[rd] = norm_tree_dx;
                else if (s == 1) ag.registers[rd] = norm_tree_dy;
                else if (s == 2) ag.registers[rd] = norm_min_dx;      // Sensor Arah Sumbu X Mineral Purba
                else if (s == 3) ag.registers[rd] = norm_min_dy;      // Sensor Arah Sumbu Y Mineral Purba
                else if (s == 4) ag.registers[rd] = norm_energy;
                else if (s == 5) ag.registers[rd] = norm_temp;
                else if (s == 6) ag.registers[rd] = norm_pred_dist;   // Sensor Radar Deteksi Predator
                else ag.registers[rd] = norm_age;                     // Sensor Usia Individu
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
                ag.registers[rd] = (float)tanh(ag.registers[rs1] * norm_temp);
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
        }
    }

    // 3. DINAMIKA GERAK FISIK (Inersia, Medan Angin, dan Perintah Aksi Otak)
    float max_spd = (float)DuniaFisika::MAX_AGENT_SPEED;
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
    
    // Suhu ekstrem mempercepat kelaparan secara objektif
    float temp_diff = fabsf(climate->temperature - 20.0f);
    float climate_stress = 1.0f + temp_diff * (float)ParameterAgent::CLIMATE_HUNGER_IMPACT_MULT;

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
    ag.hunger_rate_mult = (1.0f + (ag.age_years / (float)ParameterAgent::MAX_AGE_YEARS) * 2.5f) * climate_stress * overcrowding_stress;
    if (ag.mating_cooldown > 0.0f) ag.mating_cooldown = fmaxf(0.0f, ag.mating_cooldown - (float)dt);

    // Pengaruh Atmosfer Oksigen (O2) Terhadap Efisiensi Metabolisme & Stamina
    float o2_factor = climate->oxygen_level / 21.0f; // 1.0 pada kondisi normal
    float speed = sqrtf(ag.vx * ag.vx + ag.vy * ag.vy);
    float energy_cost = ((float)ParameterAgent::METABOLISM_BASE_RATE + speed * (float)ParameterAgent::METABOLISM_MOVE_COST) * ag.hunger_rate_mult * (float)dt / fmaxf(0.5f, o2_factor);
    ag.energy = fmaxf(0.0f, ag.energy - energy_cost);

    // 5. Interaksi dengan Pohon & Konsumsi Buah (Fotosintesis Siang/Malam Organik)
    if (nearest_tree_idx >= 0 && nearest_tree_dist < (float)DuniaFisika::TREE_INTERACTION_RADIUS) {
        float photo_mult = 0.5f + (float)DuniaFisika::DAY_PHOTOSYNTHESIS_MULT * climate->daylight_factor;
        float shared_pool_reward = ((float)DuniaFisika::TREE_ENERGY_POOL_RATE * photo_mult) / (float)(1 + nearby_agents_count);
        ag.energy = fminf(100.0f, ag.energy + (float)(shared_pool_reward * dt));

        // Konsumsi Buah Alami
        if (trees[nearest_tree_idx].fruits_count >= 1.0f && ag.energy < 100.0f) {
            float old_fruits = atomicAdd(&trees[nearest_tree_idx].fruits_count, -1.0f);
            if (old_fruits >= 1.0f) {
                float nut = (float)DuniaFisika::FRUIT_NUTRITION_ENERGY;
                ag.energy = fminf(100.0f, ag.energy + nut);
                ag.fruits_eaten++;
            } else {
                atomicAdd(&trees[nearest_tree_idx].fruits_count, 1.0f);
            }
        }
    }

    // 5b. Interaksi Material Spasial Lingkungan
    if (nearest_min_idx >= 0 && nearest_min_dist < 32.0f && minerals[nearest_min_idx].mass > 0.0f) {
        atomicAdd(&minerals[nearest_min_idx].mass, - (float)DuniaFisika::MINERAL_HARDNESS_DECAY * 0.1f * (float)dt);
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

    // 6b. Reproduksi Alami Antar-Gender (Kubu A)
    float req_mating_energy = (float)ParameterAgent::MATING_MIN_ENERGY;
    if (ag.gender == 1 && ag.energy >= req_mating_energy && ag.mating_cooldown <= 0.0f && ag.age_years >= 15.0f && ag.age_years <= 75.0f) {
        for (int p = 0; p < population_size; ++p) {
            float partner_req_energy = (float)ParameterAgent::MATING_MIN_ENERGY;
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
                            // Kelamin anak ditentukan secara acak (50% Jantan, 50% Betina)
                            unsigned int rng_seed = (slot * 1973 + (int)(ag.x * 31.0f) + (int)(ag.age_years * 11.0f));
                            agents[slot].gender = (rng_seed % 2); 
                            agents[slot].sin_type = 0;
                            agents[slot].x = ag.x + (float)((slot % 5) - 2) * 4.0f;
                            agents[slot].y = ag.y + (float)((slot % 3) - 1) * 4.0f;
                            agents[slot].vx = 0.0f;
                            agents[slot].vy = 0.0f;
                            agents[slot].energy = (float)ParameterAgent::NEWBORN_INITIAL_ENERGY; // Energi awal bayi kecil, butuh langsung makan
                            agents[slot].corpse_energy = 0.0f;
                            agents[slot].age_years = 0.0f;
                            agents[slot].mating_cooldown = (float)ParameterAgent::MATING_COOLDOWN * 2.0f;
                            agents[slot].hunger_rate_mult = 1.0f;
                            agents[slot].fruits_eaten = 0;
                            agents[slot].formulas_discovered = 0;
                            agents[slot].predators_slain = 0;
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
                                unsigned int m_hash = rng_seed + ip * 199 + slot * 37;
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
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    // 7. Interaksi Duel Ekologis: Perburuan vs Pertahanan Kognitif & Senjata Purba
    for (int pred_i = 0; pred_i < predators_count; ++pred_i) {
        GpuPredatorAgent& pred = predators[pred_i];
        if (!pred.is_alive || pred.energy <= 0.0f) continue;

        float pdx = pred.x - ag.x;
        float pdy = pred.y - ag.y;
        float pdist = sqrtf(pdx * pdx + pdy * pdy);

        if (pdist < (float)DuniaFisika::PREDATOR_ATTACK_RADIUS) {
            // Interaksi Kognitif Kontinu Simetris: Kecocokan gelombang kognisi menentukan redaman kerusakan secara objektif
            float cognitive_resonance_match = gpu_clamp(1.0f - fabsf(fabsf(ag.growth_signal) - pred.formula_shield), 0.0, 1.0);
            float shield_factor = cognitive_resonance_match * (float)DuniaFisika::HERBIVORE_DEFENSE_REDUCTION;

            // Kerusakan yang diterima mangsa berbanding terbalik dengan kecocokan spektrum kognisinya
            float effective_pred_damage = (float)DuniaFisika::PREDATOR_DAMAGE_RATE * (1.0f - shield_factor) * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - effective_pred_damage);

            // Predator menyerap energi sebanding dengan penetrasi gigitan yang berhasil
            float energy_gained = effective_pred_damage * 0.8f;
            pred.energy = fminf(100.0f, pred.energy + energy_gained);

            // Serangan Balik Kognitif Herbivora: Muncul kontinu proporsional terhadap keunggulan resonansi
            if (cognitive_resonance_match > 0.5f) {
                float swarm_bonus = fminf(3.0f, 1.0f + (float)nearby_agents_count * 0.40f);
                float counter_dmg = ((float)DuniaFisika::HERBIVORE_COUNTER_BASE_DAMAGE * (cognitive_resonance_match - 0.5f) * 2.0f * swarm_bonus) * (float)dt;

                pred.energy = fmaxf(0.0f, pred.energy - counter_dmg);
                ag.formulas_discovered++;

                if (pred.energy <= 0.0f) {
                    pred.is_alive = false;
                    pred.just_died = true;
                    pred.just_killed = true;
                    pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE * 1.5f;
                    ag.predators_slain++;
                }
            }

            if (ag.energy <= 0.0f) {
                pred.prey_devoured++;
                pred.energy = fminf(100.0f, pred.energy + 25.0f); // Nutrisi instan hasil mangsa hidup
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
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE * 1.5f;
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
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE * 1.5f;
        return;
    }

    // 1. SENSORIK PREDATOR: Target Mangsa Hidup & Target Bangkai / Mayat (Scavenging)
    float nearest_prey_dist = 10000.0f;
    float nearest_prey_dx = 0.0f;
    float nearest_prey_dy = 0.0f;
    float nearest_prey_energy = 0.0f;

    float nearest_corpse_dist = 10000.0f;
    float nearest_corpse_dx = 0.0f;
    float nearest_corpse_dy = 0.0f;
    int nearest_corpse_agent_idx = -1;

    // Scan mayat dan mangsa hidup Kubu A
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
            // Deteksi mayat untuk dimakan (Scavenging)
            if (d < nearest_corpse_dist) {
                nearest_corpse_dist = d;
                nearest_corpse_dx = dx;
                nearest_corpse_dy = dy;
                nearest_corpse_agent_idx = i;
            }
        }
    }

    // Pemangsaan Mayat Kubu A oleh Karnivora Alami (Bebas tanpa restriksi artifisial)
    if (nearest_corpse_agent_idx >= 0 && nearest_corpse_dist < (float)DuniaFisika::CORPSE_SCAVENGE_RADIUS && pred.energy < 100.0f) {
        float bite = fminf(agents[nearest_corpse_agent_idx].corpse_energy, (float)DuniaFisika::CARNIVORE_CORPSE_EAT_RATE * (float)dt);
        agents[nearest_corpse_agent_idx].corpse_energy -= bite;
        pred.energy = fminf(100.0f, pred.energy + bite * 0.8f);
    }

    // Sensorik Spasial Predator Murni (Mendeteksi Posisi Mangsa Hidup & Bangkai)
    float target_dx = (nearest_prey_dist < 10000.0f) ? nearest_prey_dx : 0.0f;
    float target_dy = (nearest_prey_dist < 10000.0f) ? nearest_prey_dy : 0.0f;
    float target_dist = (nearest_prey_dist < 10000.0f) ? nearest_prey_dist : 1000.0f;

    // Jika ada bangkai lebih dekat dari mangsa hidup, arahkan sensor terdekat ke sumber energi terdekat
    if (nearest_corpse_dist < nearest_prey_dist) {
        target_dx = nearest_corpse_dx;
        target_dy = nearest_corpse_dy;
        target_dist = nearest_corpse_dist;
    }

    float vis_factor = 0.5f + 0.5f * climate->daylight_factor;
    float norm_prey_dx = (target_dx / (target_dist + 1e-3f)) * vis_factor;
    float norm_prey_dy = (target_dy / (target_dist + 1e-3f)) * vis_factor;
    float norm_prey_dist = (float)gpu_clamp(target_dist / 500.0f, 0.0, 1.0);
    float norm_energy = pred.energy / 100.0f;
    float norm_temp = (climate->temperature - 15.0f) / 30.0f;
    float norm_daylight = climate->daylight_factor;
    float norm_comm = (float)tanh(pred.comm_received); // Sensor Sinyal Komunikasi dari Kawanan Predator

    // 2. EKSEKUSI DNA VIRTUAL MACHINE PREDATOR
    int max_r = (pred.active_registers_count > 0 && pred.active_registers_count <= REGISTERS_COUNT) ? 
                pred.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;
    int active_prog = (pred.active_program_size > 0 && pred.active_program_size <= DNA_PROGRAM_SIZE) ? 
                      pred.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

    // Leaky Memory Decay
    for (int r = 0; r < max_r; ++r) {
        pred.registers[r] *= 0.95;
    }

    float move_cmd_x = 0.0f;
    float move_cmd_y = 0.0f;
    float broadcast_out = 0.0f;

    for (int ip = 0; ip < active_prog; ++ip) {
        const auto& inst = pred.dna_program[ip];
        int rd = inst.r_dest % max_r;
        int rs1 = inst.r_src1 % max_r;
        int rs2 = inst.r_src2 % max_r;

        switch (inst.op % 15) {
            case OP_NOP: break;
            case OP_LOAD_SENSOR: {
                int s = inst.r_src1 % 8;
                if (s == 0) pred.registers[rd] = norm_prey_dx;
                else if (s == 1) pred.registers[rd] = norm_prey_dy;
                else if (s == 2) pred.registers[rd] = norm_prey_dist;
                else if (s == 3) pred.registers[rd] = nearest_prey_energy / 100.0f;
                else if (s == 4) pred.registers[rd] = norm_energy;
                else if (s == 5) pred.registers[rd] = norm_temp;
                else if (s == 6) pred.registers[rd] = norm_daylight;
                else pred.registers[rd] = norm_comm;
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
                pred.registers[rd] = (float)tanh(pred.registers[rs1] * norm_temp);
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
            default: break;
        }
    }

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

    // 4. Metabolisme & Kematian Predator
    float p_speed = sqrtf(pred.vx * pred.vx + pred.vy * pred.vy);
    float o2_factor = climate->oxygen_level / 21.0f;
    float p_energy_cost = ((float)DuniaFisika::PREDATOR_METABOLISM + p_speed * (float)ParameterAgent::METABOLISM_MOVE_COST) * (float)dt / fmaxf(0.5f, o2_factor);
    pred.energy = fmaxf(0.0f, pred.energy - p_energy_cost);

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

    if (pred.energy <= 0.0f) {
        pred.is_alive = false;
        pred.just_died = true;
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE * 1.5f;
        return;
    }

    // 5. Reproduksi Seksual Predator
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

                        unsigned int rng_seed = (unsigned int)(p_idx * 31337 + slot * 7919 + (int)(pred.x * 10.0f));

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
                        predators[slot].corpse_energy = 0.0f;
                        predators[slot].age_years = 0.0f;
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

                            unsigned int m_hash = rng_seed + ip * 131 + slot * 71;
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