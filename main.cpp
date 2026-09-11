#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include "agent/dunia/parameter_dunia.h"
#include "agent/ingatan/parameter_agent.h"
#include <cuda_runtime.h>

#define DNA_PROGRAM_SIZE ParameterAgent::MAX_DNA_CAPACITY
#define REGISTERS_COUNT ParameterAgent::MAX_REGISTERS
#define ECO_MAX_TREES ParameterAgent::MAX_TREES

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
  int tree_type;           // 0: TREE_TYPE_FRUIT, 1: TREE_TYPE_OXYGEN, 2: TREE_TYPE_PIONEER
  float soil_fertility;    // Kesuburan tanah lokasi pohon (0.0 - 2.0)
  float moisture;          // Kadar air/kelembaban pohon (0.0 - 100.0%)
};

struct GpuMineralDeposit {
  float x;
  float y;
  int element_type;    // 0: Silikon/Batu, 1: Besi/Logam Konduktif, 2: Kristal Energi Resonance
  float mass;          // Kerapatan massa/jumlah material (0 - 100%)
  float hardness;      // Ketahanan material terhadap degradasi (0 - 100%)
  float conductivity;  // Medan konduksi listrik/akustik
};

struct GpuClimateState {
  float wind_x;
  float wind_y;
  float temperature;    // -10.0°C hingga +40.0°C
  float magnetic_angle; // 0 - 2*PI
  float daylight_factor;// 0.0 (Malam) hingga 1.0 (Siang)
  float season_phase;   // 0.0 - 4.0
  int current_season;   // 0: Semi, 1: Panas, 2: Gugur, 3: Dingin
  float season_timer;
  float oxygen_level;   // Atmosfer Oksigen (%) (Dasar 21.0%)
  float co2_level;      // Karbon Dioksida (%) (Dasar 0.04%)
  float h2o_level;      // Kelembaban Uap Air H2O (%) (Dasar 60.0%)
  float nitrogen_level; // Gas Nitrogen N2 (%) (Dasar 78.0%)
  float atmospheric_pressure; // Tekanan atmosfer (Atm)
  float nature_adversarial_pressure; // Tekanan Adaptif Kubu Alam AI (0.0 - 5.0)
};

struct GpuEcosystemAgent {
  int id;         // Unique Agent ID
  int generation; // Generasi Keturunan
  int gender;     // 0: Jantan (Male), 1: Betina (Female)
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
  int predators_slain;     // Jumlah predator yang berhasil dikalahkan agen
  int material_interactions;// Frekuensi berinteraksi/menambang material megalitikum
  int tools_crafted;       // Jumlah alat/senjata/struktur megalitikum yang berhasil dibuat
  float mined_material;    // Stok material mentah yang dikumpulkan
  float growth_signal;     // Output rumus kognisi yang dipancarkan
  float comm_signal;       // Pesan siaran komunikasi / feromon
  float comm_received;     // Pesan komunikasi diterima dari koloni
  float fear_level;        // Respon bahaya kelaparan / cuaca ekstrem
  float corpse_energy;     // Biomassa mayat saat mati yang bisa dimakan karnivora
  bool is_alive;           // Status Kehidupan (True: Hidup, False: Baru Mati/Mayat)
  bool just_died;          // Flag event kematian pada tick ini
  bool just_born;          // Flag event kelahiran pada tick ini

  int active_program_size;
  int active_registers_count;
  int sin_type;

  double registers[REGISTERS_COUNT];
  double prev_registers[REGISTERS_COUNT];
  double integrated_registers[REGISTERS_COUNT];
  float reservoir_weights_in[REGISTERS_COUNT];   // Bobot input sensorik ke reservoir
  float reservoir_weights_rec[REGISTERS_COUNT];  // Bobot recurrent self-loop reservoir
  float pred_sensor_prev[4];                     // Sensor t-1 untuk Predictive Loss
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
  float formula_shield;
  float comm_signal;       // Pesan koordinasi predator
  float comm_received;     // Pesan koordinasi kawan
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
  int sin_type;
  double registers[REGISTERS_COUNT];
  double prev_registers[REGISTERS_COUNT];
  double integrated_registers[REGISTERS_COUNT];
  float reservoir_weights_in[REGISTERS_COUNT];   // Bobot input sensorik ke reservoir
  float reservoir_weights_rec[REGISTERS_COUNT];  // Bobot recurrent self-loop reservoir
  float pred_sensor_prev[4];                     // Sensor t-1 untuk Predictive Loss
  float immortality_timer;                       // Sisa waktu keabadian (detik simulasi); 0 = normal
  DnaInstruction dna_program[DNA_PROGRAM_SIZE];
};

extern "C" void
launch_ecosystem_simulation(GpuEcosystemAgent *d_agents, int population_size,
                            GpuTreeEntity *d_trees, int trees_count,
                            GpuPredatorAgent *d_predators, int predators_count,
                            GpuMineralDeposit *d_minerals, int minerals_count,
                            GpuClimateState *d_climate, double dt);

GpuEcosystemAgent *g_h_agents = nullptr;
GpuEcosystemAgent *g_d_agents = nullptr;
GpuTreeEntity *g_h_trees = nullptr;
GpuTreeEntity *g_d_trees = nullptr;
GpuPredatorAgent *g_h_predators = nullptr;
GpuPredatorAgent *g_d_predators = nullptr;
GpuMineralDeposit *g_h_minerals = nullptr;
GpuMineralDeposit *g_d_minerals = nullptr;
GpuClimateState g_climate = {};
GpuClimateState *g_d_climate = nullptr;

// =========================================================================
// SISTEM ARTIFACT: Ujian Aritmatika & Keabadian 20 Hari
// =========================================================================
struct ArtifactEntity {
    float x, y;          // Posisi aktif di dunia
    bool  is_active;     // Apakah artifact ada di dunia (visible)
    int   level;         // Level kesulitan (mulai 1, naik tiap diklaim)
    int   spawn_count;   // Counter spawn untuk pola Golden Angle
    int   challenge_a;   // Operand A soal aritmatika
    int   challenge_b;   // Operand B soal aritmatika
    int   challenge_c;   // Operand C soal aritmatika (lvl tinggi)
    float answer_norm;   // Jawaban ternormalisasi ke [-1,1] (target comm_signal)
    float immortality_seconds; // Durasi keabadian yang diberikan (detik sim)
    int   status_state;  // 0: mencari, 1: gagal/kabur, 2: terjawab, 3: active-immortal
};

// Hitung posisi spawn berikutnya: Golden Angle Spiral
static void artifact_next_spawn(ArtifactEntity &art, int faction) {
    const float golden_angle = 2.39996f; // 137.508° dalam radian
    float n = (float)art.spawn_count;
    float R = 150.0f + (float)(art.spawn_count % 7) * 80.0f + (float)art.level * 15.0f;
    float faction_off = (faction == 0) ? 0.0f : 3.14159f;
    float cx = (float)DuniaFisika::WORLD_WIDTH  * 0.5f;
    float cy = (float)DuniaFisika::WORLD_HEIGHT * 0.5f;
    art.x = cx + R * cosf(n * golden_angle + faction_off);
    art.y = cy + R * sinf(n * golden_angle + faction_off);
    // Clamp ke dalam dunia
    art.x = fmaxf(50.0f, fminf((float)DuniaFisika::WORLD_WIDTH  - 50.0f, art.x));
    art.y = fmaxf(50.0f, fminf((float)DuniaFisika::WORLD_HEIGHT - 50.0f, art.y));
}

// Deterministic LCG untuk challenge generation (tidak pakai rand())
static int artifact_lcg(int &seed) {
    seed = seed * 1664525 + 1013904223;
    return (seed >> 1) & 0x7FFFFFFF;
}

// Buat soal aritmatika baru dan normalisasi jawaban ke [-1, 1]
static void artifact_gen_challenge(ArtifactEntity &art) {
    int seed = art.spawn_count * 7919 + art.level * 1973;
    int a = (artifact_lcg(seed) % 12) + 1;
    int b = (artifact_lcg(seed) % 12) + 1;
    int c = (artifact_lcg(seed) % 6)  + 1;
    art.challenge_a = a;
    art.challenge_b = b;
    art.challenge_c = c;

    int lvl = art.level;
    float raw = 0.0f;
    float scale = 1.0f;

    if (lvl == 1)       { raw = (float)(a + b);              scale = 20.0f; }  // SD: a+b
    else if (lvl == 2)  { raw = (float)(a * b);              scale = 144.0f;}  // SD 6: a*b
    else if (lvl == 3)  { raw = (float)(a*a + b);            scale = 160.0f;}  // SMP: a²+b
    else if (lvl == 4)  { raw = (float)(a*b - c);            scale = 130.0f;}  // SMA: a*b-c
    else if (lvl == 5)  { raw = (float)(a*a - b*b);          scale = 140.0f;}  // D3: a²-b²
    else if (lvl == 6)  { raw = (float)(a*a*a + b);          scale = 1740.0f;} // S1: a³+b
    else if (lvl == 7)  { raw = (float)((a+b)*(a+b));        scale = 580.0f; } // S2: (a+b)²
    else if (lvl == 8)  { raw = (float)(a/b + a%b);          scale = 25.0f;  } // S3: div+mod
    else if (lvl == 9)  { // Doktor: gcd
        int ga=a, gb=b; while(gb){ int t=gb; gb=ga%gb; ga=t; }
        raw = (float)ga; scale = 12.0f;
    } else if (lvl == 10) { // Prof: lcm
        int ga=a, gb=b, la=a, lb=b; while(gb){ int t=gb; gb=ga%gb; ga=t; }
        raw = (float)(la/ga*lb); scale = 156.0f;
    } else { // Einstein+: a^c mod (b+7)
        int mod = b + 7; int result = 1;
        for(int i=0;i<c;i++) result=(result*a)%mod;
        raw = (float)result; scale = (float)(mod - 1);
    }
    art.answer_norm = tanhf(raw / scale);
    art.immortality_seconds = (float)(DuniaFisika::ARTIFACT_IMMORTALITY_DAYS * DuniaFisika::SECONDS_PER_DAY);
}

// State global: 1 artifact per kubu (0=Herbi, 1=Karni)
ArtifactEntity g_artifacts[2] = {};

CRITICAL_SECTION g_cs;
std::atomic<bool> g_server_running(true);
std::atomic<double> g_time_speed_multiplier(DuniaFisika::TIME_ACCELERATION_FACTOR);
std::atomic<double> g_growth_bias_ratio(DuniaFisika::DEFAULT_GROWTH_BIAS);

int g_total_predator_kills = 0; // Total predator yang mati terbunuh rumus agen
int g_predator_births = DuniaFisika::INITIAL_PREDATORS;
int g_predator_deaths = 0;
int g_total_material_interactions_a = 0; // Total interaksi material Kubu A
int g_total_tools_crafted_a = 0;        // Total alat/senjata aktif Kubu A
int g_cum_tools_crafted_a = 0;          // Total kumulatif alat dibuat Kubu A
int g_total_material_interactions_b = 0; // Total interaksi material Kubu B
int g_total_tools_crafted_b = 0;        // Total alat/senjata aktif Kubu B
int g_cum_tools_crafted_b = 0;          // Total kumulatif alat dibuat Kubu B

int g_rpm_material_a = 0;               // Kecepatan interaksi material A per menit
int g_rpm_material_b = 0;               // Kecepatan interaksi material B per menit
float g_dna_diversity_index = 0.0f;     // Code Diversity & Shannon Entropy (0 - 100%)
float g_social_learning_index = 0.0f;   // Information Transfer / Feromon / Sinyal (0 - 100%)
float g_eroi_score = 1.0f;              // Energy Return on Investment
std::string g_strategy_hierarchy_level = "Tingkat 1: Refleks & Pencarian Pangan";

int g_loop_run_count = 1;               // Counter putaran infinite loop simulasi
int g_best_gen_record_a = 1;             // Rekor generasi tertinggi Herbivora antar loop
int g_best_gen_record_b = 1;             // Rekor generasi tertinggi Predator antar loop
int g_best_art_lvl_record_a = 1;         // Rekor level artifact tertinggi Herbivora
int g_best_art_lvl_record_b = 1;         // Rekor level artifact tertinggi Predator
float g_best_nature_pressure_record = 1.0f; // Rekor daya survival tertinggi (tekanan alam tertinggi yang berhasil dihadapi)

// Pencatat Milestone Evolusi Ringkas ke File .log
static void log_evolution_milestone(int loop_num, int gen_a, int art_lvl_a, int gen_b, int art_lvl_b, float nature_press, int day_num) {
  std::ofstream log_file(DuniaFisika::EVOLUTION_LOG_FILE, std::ios::app);
  if (log_file.is_open()) {
    log_file << "loop #" << loop_num 
             << " (day " << day_num << ") | Herbi: gen " << gen_a << ", art lvl " << art_lvl_a 
             << " | Karni: gen " << gen_b << ", art lvl " << art_lvl_b 
             << " | alam: " << std::fixed << std::setprecision(2) << nature_press << "x" << std::endl;
  }
}

int g_day_count = 1;
int g_total_fruits_harvested = 0;
int g_total_formulas_synthesized = 0;
int g_total_births = ParameterAgent::INITIAL_POPULATION;
int g_total_deaths = 0;
int g_total_tree_deaths = 0;
int g_total_tree_sprouts = ParameterAgent::MAX_TREES;
double g_sim_time = 0.0;
int g_alpha_agent_idx = 0;
int g_alpha_predator_idx = 0;
std::string g_best_ecosystem_formula =
    "Formula = tanh(R0·SensorTree + R4·Energy)";
std::string g_best_predator_formula =
    "Formula = tanh(R0·SensorPrey + R4·Energy)";

struct TopAgentRecord {
  int id;
  int generation;
  int gender;
  int sin_type;
  float age_years;
  int fruits_eaten;
  int formulas_discovered;
  int predators_slain;
  int material_interactions;
  int tools_crafted;
  float energy;
  std::string formula;
};

struct TopPredatorRecord {
  int id;
  int generation;
  int gender;
  int sin_type;
  float age_years;
  int prey_devoured;
  int material_interactions;
  int tools_crafted;
  float formula_shield;
  float energy;
};

std::vector<TopAgentRecord> g_living_leaderboard;
std::vector<TopPredatorRecord> g_predator_leaderboard;

std::string decompile_dna_formula(const DnaInstruction *prog, int prog_size) {
  int sensor_count = 0;
  int math_ops = 0;
  int move_ops = 0;
  int self_code_ops = 0;
  int mem_ops = 0;
  int resonate_ops = 0;

  for (int ip = 0; ip < prog_size; ++ip) {
    int op = prog[ip].op % 19;
    if (op == 1) sensor_count++;
    else if (op >= 2 && op <= 7) math_ops++;
    else if (op == 10) move_ops++;
    else if (op == 11 || op == 12) resonate_ops++;
    else if (op == 15 || op == 16) self_code_ops++;
    else if (op == 17 || op == 18) mem_ops++;
  }

  std::string profile = "";
  if (self_code_ops > 0) {
    profile = "Algoritma: Self-Rewriting Circuit (" + std::to_string(self_code_ops) + " self-mods, " + std::to_string(prog_size) + " nodes)";
  } else if (resonate_ops >= 3) {
    profile = "Algoritma: Swarm Resonance & Field Attractor (" + std::to_string(resonate_ops) + " signals)";
  } else if (move_ops >= 3) {
    profile = "Algoritma: Dynamic Spatial Navigator (Hyper-Kinetic, " + std::to_string(move_ops) + " vectors)";
  } else if (math_ops > sensor_count) {
    profile = "Algoritma: Continuous Dynamical Reservoir (" + std::to_string(math_ops) + " math transformations)";
  } else {
    profile = "Algoritma: Reactive Sensor-Driven Graph (" + std::to_string(sensor_count) + " inputs, " + std::to_string(prog_size) + " nodes)";
  }
  return profile;
}

void save_best_dna_to_file(const GpuEcosystemAgent &agent) {
  std::ofstream out(ParameterAgent::DNA_STORAGE_FILE, std::ios::binary);
  if (out.is_open()) {
    out.write(reinterpret_cast<const char *>(&agent.generation),
              sizeof(agent.generation));
    out.write(reinterpret_cast<const char *>(&agent.active_program_size),
              sizeof(agent.active_program_size));
    out.write(reinterpret_cast<const char *>(&agent.active_registers_count),
              sizeof(agent.active_registers_count));
    out.write(reinterpret_cast<const char *>(agent.dna_program),
              sizeof(DnaInstruction) * DNA_PROGRAM_SIZE);
    out.write(reinterpret_cast<const char *>(agent.epigenetic_methylation),
              sizeof(float) * DNA_PROGRAM_SIZE);
    out.close();
  }
}

bool load_best_dna_from_file(GpuEcosystemAgent &agent) {
  std::ifstream in(ParameterAgent::DNA_STORAGE_FILE, std::ios::binary);
  if (!in.is_open())
    return false;

  in.read(reinterpret_cast<char *>(&agent.generation),
          sizeof(agent.generation));
  in.read(reinterpret_cast<char *>(&agent.active_program_size),
          sizeof(agent.active_program_size));
  in.read(reinterpret_cast<char *>(&agent.active_registers_count),
          sizeof(agent.active_registers_count));
  in.read(reinterpret_cast<char *>(agent.dna_program),
          sizeof(DnaInstruction) * DNA_PROGRAM_SIZE);
  in.read(reinterpret_cast<char *>(agent.epigenetic_methylation),
          sizeof(float) * DNA_PROGRAM_SIZE);
  bool ok = in.good() || in.gcount() > 0;
  in.close();
  return ok;
}

void save_best_predator_dna_to_file(const GpuPredatorAgent &predator) {
  std::ofstream out(ParameterAgent::PREDATOR_DNA_STORAGE_FILE, std::ios::binary);
  if (out.is_open()) {
    out.write(reinterpret_cast<const char *>(&predator.generation),
              sizeof(predator.generation));
    out.write(reinterpret_cast<const char *>(&predator.active_program_size),
              sizeof(predator.active_program_size));
    out.write(reinterpret_cast<const char *>(&predator.active_registers_count),
              sizeof(predator.active_registers_count));
    out.write(reinterpret_cast<const char *>(predator.dna_program),
              sizeof(DnaInstruction) * DNA_PROGRAM_SIZE);
    out.close();
  }
}

bool load_best_predator_dna_from_file(GpuPredatorAgent &predator) {
  std::ifstream in(ParameterAgent::PREDATOR_DNA_STORAGE_FILE, std::ios::binary);
  if (!in.is_open())
    return false;

  in.read(reinterpret_cast<char *>(&predator.generation),
          sizeof(predator.generation));
  in.read(reinterpret_cast<char *>(&predator.active_program_size),
          sizeof(predator.active_program_size));
  in.read(reinterpret_cast<char *>(&predator.active_registers_count),
          sizeof(predator.active_registers_count));
  in.read(reinterpret_cast<char *>(predator.dna_program),
          sizeof(DnaInstruction) * DNA_PROGRAM_SIZE);
  bool ok = in.good() || in.gcount() > 0;
  in.close();
  return ok;
}

#pragma pack(push, 1)
struct EcosystemCheckpointHeader {
  char magic[8]; // "GPUMIND\0"
  uint32_t version;
  uint64_t timestamp;
  int32_t day_count;
  double sim_time;
  int32_t total_births;
  int32_t total_deaths;
  int32_t total_fruits_harvested;
  int32_t total_predator_kills;
  int32_t predator_births;
  int32_t predator_deaths;
  int32_t total_material_interactions_a;
  int32_t total_tools_crafted_a;
  int32_t cum_tools_crafted_a;
  int32_t total_material_interactions_b;
  int32_t total_tools_crafted_b;
  int32_t cum_tools_crafted_b;
  int32_t total_formulas_synthesized;
  int32_t total_tree_deaths;
  int32_t total_tree_sprouts;
  float dna_diversity_index;
  float social_learning_index;
  float eroi_score;
  int32_t max_population_buffer;
  int32_t max_predators_buffer;
  int32_t max_trees;
  int32_t max_minerals;
};
#pragma pack(pop)

bool save_ecosystem_checkpoint(const std::string &filepath) {
  EnterCriticalSection(&g_cs);
  if (!g_h_agents || !g_d_agents) {
    LeaveCriticalSection(&g_cs);
    return false;
  }

  // Sinkronisasi data mutakhir dari GPU VRAM ke Host RAM
  cudaMemcpy(g_h_agents, g_d_agents,
             sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER,
             cudaMemcpyDeviceToHost);
  cudaMemcpy(g_h_predators, g_d_predators,
             sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER,
             cudaMemcpyDeviceToHost);
  cudaMemcpy(g_h_trees, g_d_trees,
             sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES,
             cudaMemcpyDeviceToHost);
  cudaMemcpy(g_h_minerals, g_d_minerals,
             sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS,
             cudaMemcpyDeviceToHost);
  cudaMemcpy(&g_climate, g_d_climate, sizeof(GpuClimateState),
             cudaMemcpyDeviceToHost);

  std::ofstream out(filepath, std::ios::binary);
  if (!out.is_open()) {
    LeaveCriticalSection(&g_cs);
    return false;
  }

  EcosystemCheckpointHeader header = {};
  memcpy(header.magic, "GPUMIND\0", 8);
  header.version = 1;
  header.timestamp = static_cast<uint64_t>(time(NULL));
  header.day_count = g_day_count;
  header.sim_time = g_sim_time;
  header.total_births = g_total_births;
  header.total_deaths = g_total_deaths;
  header.total_fruits_harvested = g_total_fruits_harvested;
  header.total_predator_kills = g_total_predator_kills;
  header.predator_births = g_predator_births;
  header.predator_deaths = g_predator_deaths;
  header.total_material_interactions_a = g_total_material_interactions_a;
  header.total_tools_crafted_a = g_total_tools_crafted_a;
  header.cum_tools_crafted_a = g_cum_tools_crafted_a;
  header.total_material_interactions_b = g_total_material_interactions_b;
  header.total_tools_crafted_b = g_total_tools_crafted_b;
  header.cum_tools_crafted_b = g_cum_tools_crafted_b;
  header.total_formulas_synthesized = g_total_formulas_synthesized;
  header.total_tree_deaths = g_total_tree_deaths;
  header.total_tree_sprouts = g_total_tree_sprouts;
  header.dna_diversity_index = g_dna_diversity_index;
  header.social_learning_index = g_social_learning_index;
  header.eroi_score = g_eroi_score;
  header.max_population_buffer = ParameterAgent::MAX_POPULATION_BUFFER;
  header.max_predators_buffer = DuniaFisika::MAX_PREDATORS_BUFFER;
  header.max_trees = ParameterAgent::MAX_TREES;
  header.max_minerals = DuniaFisika::MAX_PERIODIC_DEPOSITS;

  out.write(reinterpret_cast<const char *>(&header), sizeof(header));
  out.write(reinterpret_cast<const char *>(g_h_agents),
            sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER);
  out.write(reinterpret_cast<const char *>(g_h_predators),
            sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER);
  out.write(reinterpret_cast<const char *>(g_h_trees),
            sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES);
  out.write(reinterpret_cast<const char *>(g_h_minerals),
            sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS);
  out.write(reinterpret_cast<const char *>(&g_climate), sizeof(GpuClimateState));

  out.close();
  LeaveCriticalSection(&g_cs);
  return true;
}

bool load_ecosystem_checkpoint(const std::string &filepath) {
  std::ifstream in(filepath, std::ios::binary);
  if (!in.is_open()) return false;

  EcosystemCheckpointHeader header = {};
  in.read(reinterpret_cast<char *>(&header), sizeof(header));
  if (memcmp(header.magic, "GPUMIND\0", 8) != 0 || header.version != 1) {
    in.close();
    return false;
  }
  if (header.max_population_buffer != ParameterAgent::MAX_POPULATION_BUFFER ||
      header.max_predators_buffer != DuniaFisika::MAX_PREDATORS_BUFFER ||
      header.max_trees != ParameterAgent::MAX_TREES ||
      header.max_minerals != DuniaFisika::MAX_PERIODIC_DEPOSITS) {
    in.close();
    return false;
  }

  EnterCriticalSection(&g_cs);
  g_day_count = header.day_count;
  g_sim_time = header.sim_time;
  g_total_births = header.total_births;
  g_total_deaths = header.total_deaths;
  g_total_fruits_harvested = header.total_fruits_harvested;
  g_total_predator_kills = header.total_predator_kills;
  g_predator_births = header.predator_births;
  g_predator_deaths = header.predator_deaths;
  g_total_material_interactions_a = header.total_material_interactions_a;
  g_total_tools_crafted_a = header.total_tools_crafted_a;
  g_cum_tools_crafted_a = header.cum_tools_crafted_a;
  g_total_material_interactions_b = header.total_material_interactions_b;
  g_total_tools_crafted_b = header.total_tools_crafted_b;
  g_cum_tools_crafted_b = header.cum_tools_crafted_b;
  g_total_formulas_synthesized = header.total_formulas_synthesized;
  g_total_tree_deaths = header.total_tree_deaths;
  g_total_tree_sprouts = header.total_tree_sprouts;
  g_dna_diversity_index = header.dna_diversity_index;
  g_social_learning_index = header.social_learning_index;
  g_eroi_score = header.eroi_score;

  in.read(reinterpret_cast<char *>(g_h_agents),
          sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER);
  in.read(reinterpret_cast<char *>(g_h_predators),
          sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER);
  in.read(reinterpret_cast<char *>(g_h_trees),
          sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES);
  in.read(reinterpret_cast<char *>(g_h_minerals),
          sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS);
  in.read(reinterpret_cast<char *>(&g_climate), sizeof(GpuClimateState));

  in.close();

  // Sinkronisasi data yang dipulihkan ke GPU VRAM
  cudaMemcpy(g_d_agents, g_h_agents,
             sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_predators, g_h_predators,
             sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_trees, g_h_trees,
             sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_minerals, g_h_minerals,
             sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_climate, &g_climate, sizeof(GpuClimateState),
             cudaMemcpyHostToDevice);

  LeaveCriticalSection(&g_cs);
  return true;
}

int inject_spawn_agents(int count, int faction) {
  EnterCriticalSection(&g_cs);
  int spawned = 0;
  if (faction == 0) { // Herbivora
    for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER && spawned < count; ++i) {
      if (!g_h_agents[i].is_alive) {
        g_h_agents[i].id = static_cast<int>(g_total_births) + 1;
        g_h_agents[i].generation = 1;
        g_h_agents[i].gender = rand() % 2;
        g_h_agents[i].sin_type = rand() % DuniaFisika::SINS_COUNT;
        g_h_agents[i].x = 100.0f + static_cast<float>(rand() % 800);
        g_h_agents[i].y = 100.0f + static_cast<float>(rand() % 800);
        g_h_agents[i].vx = (static_cast<float>(rand() % 200) - 100.0f) * 0.05f;
        g_h_agents[i].vy = (static_cast<float>(rand() % 200) - 100.0f) * 0.05f;
        g_h_agents[i].energy = static_cast<float>(ParameterAgent::INITIAL_ENERGY);
        g_h_agents[i].age_years = 0.0f;
        g_h_agents[i].hunger_rate_mult = 1.0f;
        g_h_agents[i].mating_cooldown = 2.0f;
        g_h_agents[i].fruits_eaten = 0;
        g_h_agents[i].formulas_discovered = 0;
        g_h_agents[i].predators_slain = 0;
        g_h_agents[i].material_interactions = 0;
        g_h_agents[i].tools_crafted = 0;
        g_h_agents[i].mined_material = 0.0f;
        g_h_agents[i].growth_signal = 0.0f;
        g_h_agents[i].fear_level = 0.0f;
        g_h_agents[i].is_alive = true;
        g_h_agents[i].just_died = false;
        g_h_agents[i].just_born = true;
        g_h_agents[i].active_program_size = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;
        g_h_agents[i].active_registers_count = ParameterAgent::MIN_DYNAMIC_REGISTERS;
        for (int r = 0; r < REGISTERS_COUNT; ++r) {
          g_h_agents[i].registers[r] = 0.0;
          g_h_agents[i].prev_registers[r] = 0.0;
          g_h_agents[i].integrated_registers[r] = 0.0;
          g_h_agents[i].reservoir_weights_in[r] = (static_cast<float>((rand() % 200) - 100) / 100.0f) * static_cast<float>(ParameterAgent::RESERVOIR_INPUT_SCALE);
          g_h_agents[i].reservoir_weights_rec[r] = (static_cast<float>((rand() % 200) - 100) / 100.0f) * static_cast<float>(ParameterAgent::RESERVOIR_SPECTRAL_RADIUS);
        }
        for (int s = 0; s < 4; ++s) g_h_agents[i].pred_sensor_prev[s] = 0.0f;
        for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
          g_h_agents[i].epigenetic_methylation[ip] = 0.0f;
          g_h_agents[i].dna_program[ip].op = static_cast<unsigned char>(rand() % 19);
          g_h_agents[i].dna_program[ip].r_dest = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
          g_h_agents[i].dna_program[ip].r_src1 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
          g_h_agents[i].dna_program[ip].r_src2 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
          g_h_agents[i].dna_program[ip].immediate_val = (static_cast<float>((rand() % 200) - 100)) * 0.05f;
        }
        g_total_births++;
        spawned++;
      }
    }
    if (spawned > 0) {
      cudaMemcpy(g_d_agents, g_h_agents,
                 sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER,
                 cudaMemcpyHostToDevice);
    }
  } else { // Predator
    for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER && spawned < count; ++p) {
      if (!g_h_predators[p].is_alive) {
        g_h_predators[p].id = static_cast<int>(g_predator_births) + 1;
        g_h_predators[p].generation = 1;
        g_h_predators[p].gender = rand() % 2;
        g_h_predators[p].sin_type = rand() % DuniaFisika::SINS_COUNT;
        g_h_predators[p].x = 100.0f + static_cast<float>(rand() % 800);
        g_h_predators[p].y = 100.0f + static_cast<float>(rand() % 800);
        g_h_predators[p].vx = (static_cast<float>(rand() % 200) - 100.0f) * 0.05f;
        g_h_predators[p].vy = (static_cast<float>(rand() % 200) - 100.0f) * 0.05f;
        g_h_predators[p].energy = static_cast<float>(DuniaFisika::PREDATOR_INITIAL_ENERGY);
        g_h_predators[p].age_years = 0.0f;
        g_h_predators[p].mating_cooldown = 2.0f;
        g_h_predators[p].formula_shield = 0.5f;
        g_h_predators[p].prey_devoured = 0;
        g_h_predators[p].material_interactions = 0;
        g_h_predators[p].tools_crafted = 0;
        g_h_predators[p].mined_material = 0.0f;
        g_h_predators[p].is_alive = true;
        g_h_predators[p].just_killed = false;
        g_h_predators[p].just_died = false;
        g_h_predators[p].just_born = true;
        g_h_predators[p].active_program_size = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;
        g_h_predators[p].active_registers_count = ParameterAgent::MIN_DYNAMIC_REGISTERS;
        for (int r = 0; r < REGISTERS_COUNT; ++r) {
          g_h_predators[p].registers[r] = 0.0;
          g_h_predators[p].prev_registers[r] = 0.0;
          g_h_predators[p].integrated_registers[r] = 0.0;
          g_h_predators[p].reservoir_weights_in[r] = (static_cast<float>((rand() % 200) - 100) / 100.0f) * static_cast<float>(ParameterAgent::RESERVOIR_INPUT_SCALE);
          g_h_predators[p].reservoir_weights_rec[r] = (static_cast<float>((rand() % 200) - 100) / 100.0f) * static_cast<float>(ParameterAgent::RESERVOIR_SPECTRAL_RADIUS);
        }
        for (int s = 0; s < 4; ++s) g_h_predators[p].pred_sensor_prev[s] = 0.0f;
        for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
          g_h_predators[p].dna_program[ip].op = static_cast<unsigned char>(rand() % 19);
          g_h_predators[p].dna_program[ip].r_dest = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
          g_h_predators[p].dna_program[ip].r_src1 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
          g_h_predators[p].dna_program[ip].r_src2 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
          g_h_predators[p].dna_program[ip].immediate_val = (static_cast<float>((rand() % 200) - 100)) * 0.05f;
        }
        g_predator_births++;
        spawned++;
      }
    }
    if (spawned > 0) {
      cudaMemcpy(g_d_predators, g_h_predators,
                 sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER,
                 cudaMemcpyHostToDevice);
    }
  }
  LeaveCriticalSection(&g_cs);
  return spawned;
}

int inject_spawn_trees(int count) {
  EnterCriticalSection(&g_cs);
  int replenished = 0;
  for (int t = 0; t < ParameterAgent::MAX_TREES && replenished < count; ++t) {
    if (g_h_trees[t].health < 60.0f || g_h_trees[t].fruits_count < 2.0f) {
      g_h_trees[t].health = 100.0f;
      g_h_trees[t].fruits_count = static_cast<float>(DuniaFisika::MAX_FRUIT_PER_TREE);
      g_h_trees[t].growth_stage = 100.0f;
      replenished++;
    }
  }
  if (replenished > 0) {
    cudaMemcpy(g_d_trees, g_h_trees,
               sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES,
               cudaMemcpyHostToDevice);
  }
  LeaveCriticalSection(&g_cs);
  return replenished;
}

int inject_spawn_minerals(int count) {
  EnterCriticalSection(&g_cs);
  int replenished = 0;
  for (int m = 0; m < DuniaFisika::MAX_PERIODIC_DEPOSITS && replenished < count; ++m) {
    if (g_h_minerals[m].mass < 60.0f) {
      g_h_minerals[m].mass = 100.0f;
      g_h_minerals[m].hardness = (g_h_minerals[m].element_type == 1) ? 90.0f : (g_h_minerals[m].element_type == 2 ? 65.0f : 50.0f);
      replenished++;
    }
  }
  if (replenished > 0) {
    cudaMemcpy(g_d_minerals, g_h_minerals,
               sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS,
               cudaMemcpyHostToDevice);
  }
  LeaveCriticalSection(&g_cs);
  return replenished;
}

void inject_disaster_event(const std::string &type) {
  EnterCriticalSection(&g_cs);
  if (type == "solar") {
    g_climate.temperature += static_cast<float>(DuniaFisika::SOLAR_STORM_HEAT_SPIKE);
  } else if (type == "blizzard") {
    g_climate.temperature += static_cast<float>(DuniaFisika::BLIZZARD_TEMP_DROP);
  } else if (type == "emp") {
    g_climate.magnetic_angle += 3.14159f;
  } else if (type == "lava") {
    g_climate.temperature += 15.0f;
    g_climate.co2_level += 2.0f;
  }
  cudaMemcpy(g_d_climate, &g_climate, sizeof(GpuClimateState), cudaMemcpyHostToDevice);
  LeaveCriticalSection(&g_cs);
}

void inject_set_physics(const std::string &param, double val1, double val2 = 0.0) {
  EnterCriticalSection(&g_cs);
  if (param == "wind") {
    g_climate.wind_x = static_cast<float>(val1);
    g_climate.wind_y = static_cast<float>(val2);
  } else if (param == "temp") {
    g_climate.temperature = static_cast<float>(val1);
  } else if (param == "nature_pressure") {
    g_climate.nature_adversarial_pressure = static_cast<float>(val1);
  }
  cudaMemcpy(g_d_climate, &g_climate, sizeof(GpuClimateState), cudaMemcpyHostToDevice);
  LeaveCriticalSection(&g_cs);
}

void reset_ecosystem_state() {
  memset(g_h_agents, 0, sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER);
  memset(g_h_trees, 0, sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES);
  memset(g_h_predators, 0, sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER);
  memset(g_h_minerals, 0, sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS);

  // Reset counters global
  g_total_predator_kills = 0;
  g_predator_births = DuniaFisika::INITIAL_PREDATORS;
  g_predator_deaths = 0;
  g_total_material_interactions_a = 0;
  g_total_tools_crafted_a = 0;
  g_cum_tools_crafted_a = 0;
  g_total_material_interactions_b = 0;
  g_total_tools_crafted_b = 0;
  g_cum_tools_crafted_b = 0;
  g_dna_diversity_index = 0.0f;
  g_social_learning_index = 0.0f;
  g_eroi_score = 1.0f;
  g_strategy_hierarchy_level = "Tingkat 1: Refleks & Pencarian Pangan";
  g_day_count = 1;
  g_total_fruits_harvested = 0;
  g_total_formulas_synthesized = 0;
  g_total_births = ParameterAgent::INITIAL_POPULATION;
  g_total_deaths = 0;
  g_total_tree_deaths = 0;
  g_total_tree_sprouts = ParameterAgent::MAX_TREES;
  g_sim_time = 0.0;
  g_living_leaderboard.clear();
  g_predator_leaderboard.clear();

  // Inisialisasi Formasi Deposit Periodik Material (Silikon, Logam Konduktif, Kristal Resonansi)
  for (int m = 0; m < DuniaFisika::MAX_PERIODIC_DEPOSITS; ++m) {
    g_h_minerals[m].x = 120.0f + (float)(rand() % 760);
    g_h_minerals[m].y = 120.0f + (float)(rand() % 760);
    g_h_minerals[m].element_type = m % 3; // 0: Batu Silikon, 1: Logam Konduktif, 2: Kristal Resonan
    g_h_minerals[m].mass = 80.0f + (float)(rand() % 20);
    g_h_minerals[m].hardness = (g_h_minerals[m].element_type == 1) ? 90.0f : (g_h_minerals[m].element_type == 2 ? 65.0f : 50.0f);
    g_h_minerals[m].conductivity = (g_h_minerals[m].element_type == 1) ? 0.95f : (g_h_minerals[m].element_type == 2 ? 0.75f : 0.1f);
  }

  GpuPredatorAgent saved_predator_ancestor = {};
  bool has_saved_predator_dna = load_best_predator_dna_from_file(saved_predator_ancestor);

  for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
    g_h_predators[p].id = p + 1;
    g_h_predators[p].generation = 1;
    g_h_predators[p].gender = rand() % 2; // Kelamin Acak (0: Jantan ♂, 1: Betina ♀)
    g_h_predators[p].x = 100.0f + (float)(rand() % 800);
    g_h_predators[p].y = 100.0f + (float)(rand() % 800);
    g_h_predators[p].vx = 0.0f;
    g_h_predators[p].vy = 0.0f;
    g_h_predators[p].energy = (p < DuniaFisika::INITIAL_PREDATORS) ? (float)DuniaFisika::PREDATOR_INITIAL_ENERGY : 0.0f;
    g_h_predators[p].lung_oxygen = (p < DuniaFisika::INITIAL_PREDATORS) ? (float)DuniaFisika::AGENT_LUNG_CAPACITY : 0.0f;
    g_h_predators[p].hydration = (p < DuniaFisika::INITIAL_PREDATORS) ? (float)DuniaFisika::AGENT_HYDRATION_INITIAL : 0.0f;
    g_h_predators[p].age_years = (p < DuniaFisika::INITIAL_PREDATORS) ? (float)(rand() % 30) : 0.0f;
    g_h_predators[p].mating_cooldown = (float)(rand() % 5);
    g_h_predators[p].formula_shield = (float)((p * 37) % 100) / 100.0f;
    g_h_predators[p].prey_devoured = 0;
    g_h_predators[p].material_interactions = 0;
    g_h_predators[p].tools_crafted = 0;
    g_h_predators[p].mined_material = 0.0f;
    g_h_predators[p].is_alive = (p < DuniaFisika::INITIAL_PREDATORS);
    g_h_predators[p].just_killed = false;
    g_h_predators[p].just_died = false;
    g_h_predators[p].just_born = false;
    g_h_predators[p].sin_type = 0;
    for (int r = 0; r < REGISTERS_COUNT; ++r) {
      g_h_predators[p].registers[r] = 0.0;
      g_h_predators[p].prev_registers[r] = 0.0;
      g_h_predators[p].integrated_registers[r] = 0.0;
      g_h_predators[p].reservoir_weights_in[r] = ((float)((rand() % 200) - 100) / 100.0f) * (float)ParameterAgent::RESERVOIR_INPUT_SCALE;
      g_h_predators[p].reservoir_weights_rec[r] = ((float)((rand() % 200) - 100) / 100.0f) * (float)ParameterAgent::RESERVOIR_SPECTRAL_RADIUS;
    }
    for (int s = 0; s < 4; ++s) g_h_predators[p].pred_sensor_prev[s] = 0.0f;

    if (has_saved_predator_dna && (p < DuniaFisika::INITIAL_PREDATORS)) {
      g_h_predators[p].active_program_size = saved_predator_ancestor.active_program_size;
      g_h_predators[p].active_registers_count = saved_predator_ancestor.active_registers_count;
      for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
        g_h_predators[p].dna_program[ip] = saved_predator_ancestor.dna_program[ip];
        if (p > 0 && ((rand() % 100) < 20)) {
          g_h_predators[p].dna_program[ip].immediate_val += ((rand() % 40) - 20) * 0.05f;
        }
      }
    } else {
      g_h_predators[p].active_program_size = 8;
      g_h_predators[p].active_registers_count = ParameterAgent::MIN_DYNAMIC_REGISTERS;
      for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
        g_h_predators[p].dna_program[ip].op = (unsigned char)(rand() % 15);
        g_h_predators[p].dna_program[ip].r_dest = (unsigned char)(rand() % ParameterAgent::MIN_DYNAMIC_REGISTERS);
        g_h_predators[p].dna_program[ip].r_src1 = (unsigned char)(rand() % ParameterAgent::MIN_DYNAMIC_REGISTERS);
        g_h_predators[p].dna_program[ip].r_src2 = (unsigned char)(rand() % ParameterAgent::MIN_DYNAMIC_REGISTERS);
        g_h_predators[p].dna_program[ip].immediate_val = (float)((rand() % 200) - 100) * 0.05f;
      }
    }
  }

  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    if (t < 32) {
      float gx = 100.0f + (float)(t % 6) * 160.0f + (float)((rand() % 40) - 20);
      float gy = 100.0f + ((float)t / 6.0f) * 160.0f + (float)((rand() % 40) - 20);
      // Evaluasi kesuburan geologi tanah di koordinat gx, gy
      float soil = (float)DuniaFisika::SOIL_FERTILITY_BASE + 
                   (float)DuniaFisika::SOIL_FERTILITY_VARIATION * sinf(gx * (float)DuniaFisika::SOIL_NOISE_SCALE_X) * cosf(gy * (float)DuniaFisika::SOIL_NOISE_SCALE_Y);
      
      g_h_trees[t].x = gx;
      g_h_trees[t].y = gy;
      g_h_trees[t].tree_type = t % DuniaFisika::TREE_SPECIES_COUNT;
      g_h_trees[t].soil_fertility = fmaxf(0.2f, soil);
      g_h_trees[t].moisture = 80.0f + (float)(rand() % 20);
      g_h_trees[t].growth_stage = 50.0f + (float)(rand() % 50);
      g_h_trees[t].fruits_count = (g_h_trees[t].tree_type == DuniaFisika::TREE_TYPE_FRUIT) ? (3.0f + (float)(rand() % 4)) : 0.0f;
      g_h_trees[t].formula_resonance = 0.0f;
      g_h_trees[t].age_years = (float)(rand() % 5);
      g_h_trees[t].health = 80.0f + (float)(rand() % 20);
    } else {
      g_h_trees[t].x = 0.0f;
      g_h_trees[t].y = 0.0f;
      g_h_trees[t].tree_type = 0;
      g_h_trees[t].soil_fertility = 0.0f;
      g_h_trees[t].moisture = 0.0f;
      g_h_trees[t].growth_stage = 0.0f;
      g_h_trees[t].fruits_count = 0.0f;
      g_h_trees[t].formula_resonance = 0.0f;
      g_h_trees[t].age_years = 0.0f;
      g_h_trees[t].health = 0.0f;
    }
  }

  GpuEcosystemAgent saved_ancestor = {};
  bool has_saved_dna = load_best_dna_from_file(saved_ancestor);

  for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
    g_h_agents[i].id = i + 1;
    g_h_agents[i].generation = 1;
    g_h_agents[i].gender = rand() % 2; // Kelamin Acak (0: Jantan, 1: Betina)
    g_h_agents[i].sin_type = 0;
    g_h_agents[i].x = 100.0f + (float)(rand() % 800);
    g_h_agents[i].y = 100.0f + (float)(rand() % 800);
    g_h_agents[i].vx = 0.0f;
    g_h_agents[i].vy = 0.0f;
    g_h_agents[i].energy = (i < ParameterAgent::INITIAL_POPULATION) ? (float)ParameterAgent::INITIAL_ENERGY : 0.0f;
    g_h_agents[i].lung_oxygen = (i < ParameterAgent::INITIAL_POPULATION) ? (float)DuniaFisika::AGENT_LUNG_CAPACITY : 0.0f;
    g_h_agents[i].hydration = (i < ParameterAgent::INITIAL_POPULATION) ? (float)DuniaFisika::AGENT_HYDRATION_INITIAL : 0.0f;
    g_h_agents[i].age_years = (i < ParameterAgent::INITIAL_POPULATION) ? (float)(rand() % 30) : 0.0f;
    g_h_agents[i].hunger_rate_mult = 1.0f;
    g_h_agents[i].mating_cooldown = (float)(rand() % 5);
    g_h_agents[i].fruits_eaten = 0;
    g_h_agents[i].formulas_discovered = 0;
    g_h_agents[i].predators_slain = 0;
    g_h_agents[i].material_interactions = 0;
    g_h_agents[i].tools_crafted = 0;
    g_h_agents[i].mined_material = 0.0f;
    g_h_agents[i].growth_signal = 0.0f;
    g_h_agents[i].fear_level = 0.0f;
    g_h_agents[i].is_alive = (i < ParameterAgent::INITIAL_POPULATION);
    g_h_agents[i].just_died = false;
    g_h_agents[i].just_born = false;

    for (int r = 0; r < REGISTERS_COUNT; ++r) {
      g_h_agents[i].registers[r] = 0.0;
      g_h_agents[i].prev_registers[r] = 0.0;
      g_h_agents[i].integrated_registers[r] = 0.0;
      g_h_agents[i].reservoir_weights_in[r] = ((float)((rand() % 200) - 100) / 100.0f) * (float)ParameterAgent::RESERVOIR_INPUT_SCALE;
      g_h_agents[i].reservoir_weights_rec[r] = ((float)((rand() % 200) - 100) / 100.0f) * (float)ParameterAgent::RESERVOIR_SPECTRAL_RADIUS;
    }
    for (int s = 0; s < 4; ++s) g_h_agents[i].pred_sensor_prev[s] = 0.0f;

    if (has_saved_dna && (i < ParameterAgent::INITIAL_POPULATION)) {
      g_h_agents[i].active_program_size = saved_ancestor.active_program_size;
      g_h_agents[i].active_registers_count = saved_ancestor.active_registers_count;
      for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
        g_h_agents[i].dna_program[ip] = saved_ancestor.dna_program[ip];
        g_h_agents[i].epigenetic_methylation[ip] = saved_ancestor.epigenetic_methylation[ip];
        if (i > 0 && ((rand() % 100) < 20)) {
          g_h_agents[i].dna_program[ip].immediate_val += ((rand() % 40) - 20) * 0.05f;
        }
      }
    } else {
      g_h_agents[i].active_program_size =
          ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE + (rand() % 6);
      g_h_agents[i].active_registers_count =
          ParameterAgent::MIN_DYNAMIC_REGISTERS;

      for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
        g_h_agents[i].epigenetic_methylation[ip] = 0.0f;
        g_h_agents[i].dna_program[ip].op = static_cast<unsigned char>(rand() % 15);
        g_h_agents[i].dna_program[ip].r_dest = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
        g_h_agents[i].dna_program[ip].r_src1 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
        g_h_agents[i].dna_program[ip].r_src2 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
        g_h_agents[i].dna_program[ip].immediate_val = ((rand() % 200) - 100) * 0.05f;
      }
    }
  }

  g_climate.wind_x = 1.5f;
  g_climate.wind_y = 0.5f;
  g_climate.temperature = 22.0f;
  g_climate.magnetic_angle = 0.785f;
  g_climate.current_season = 0;
  g_climate.season_phase = 0.0f;
  g_climate.season_timer = 0.0f;
  g_climate.oxygen_level = (float)DuniaFisika::OXYGEN_BASE_LEVEL;
  g_climate.co2_level = (float)DuniaFisika::CO2_BASE_LEVEL;
  g_climate.h2o_level = (float)DuniaFisika::H2O_HUMIDITY_BASE_LEVEL;
  g_climate.nitrogen_level = (float)DuniaFisika::NITROGEN_BASE_LEVEL;
  g_climate.atmospheric_pressure = 1.0f;

  cudaMemcpy(g_d_agents, g_h_agents,
             sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_trees, g_h_trees,
             sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_predators, g_h_predators,
             sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_minerals, g_h_minerals,
             sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_climate, &g_climate, sizeof(GpuClimateState),
             cudaMemcpyHostToDevice);

  // Inisialisasi Artifact Kubu A (Herbivora) & Kubu B (Karnivora)
  for (int f = 0; f < 2; ++f) {
    g_artifacts[f] = {};
    g_artifacts[f].level = 1;
    g_artifacts[f].spawn_count = f * 37;
    artifact_next_spawn(g_artifacts[f], f);
    artifact_gen_challenge(g_artifacts[f]);
    g_artifacts[f].is_active = true;
  }
}

void init_ecosystem_pipeline() {
  g_h_agents = (GpuEcosystemAgent *)malloc(sizeof(GpuEcosystemAgent) *
                                           ParameterAgent::MAX_POPULATION_BUFFER);
  g_h_trees = (GpuTreeEntity *)malloc(sizeof(GpuTreeEntity) *
                                      ParameterAgent::MAX_TREES);
  g_h_predators = (GpuPredatorAgent *)malloc(sizeof(GpuPredatorAgent) *
                                             DuniaFisika::MAX_PREDATORS_BUFFER);
  g_h_minerals = (GpuMineralDeposit *)malloc(sizeof(GpuMineralDeposit) *
                                             DuniaFisika::MAX_PERIODIC_DEPOSITS);

  cudaMalloc(&g_d_agents,
             sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER);
  cudaMalloc(&g_d_trees, sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES);
  cudaMalloc(&g_d_predators, sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER);
  cudaMalloc(&g_d_minerals, sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS);
  cudaMalloc(&g_d_climate, sizeof(GpuClimateState));

  reset_ecosystem_state();
}

void step_ecosystem(double dt) {
  g_sim_time += dt;

  g_climate.season_timer += (float)dt;
  if (g_climate.season_timer >= (float)DuniaFisika::SECONDS_PER_DAY) {
    g_climate.season_timer = 0.0f;
    g_day_count++;
    if (g_day_count % DuniaFisika::DAYS_PER_SEASON == 0) {
      g_climate.current_season =
          (g_climate.current_season + 1) % DuniaFisika::SEASONS_COUNT;
    }
  }

  float time_f = (float)g_sim_time;
  
  // Siklus Siang & Malam Dinamis (0.0 = Malam Paling Gelap, 1.0 = Siang Terik Puncak)
  float day_cycle_phase = fmodf(time_f, (float)DuniaFisika::DAY_NIGHT_CYCLE_SECONDS) / (float)DuniaFisika::DAY_NIGHT_CYCLE_SECONDS;
  g_climate.daylight_factor = 0.5f + 0.5f * sinf(day_cycle_phase * 6.28318f);

  g_climate.wind_x = sinf(time_f * 0.1f) * 3.5f + cosf(time_f * 0.03f) * 1.5f;
  g_climate.wind_y = cosf(time_f * 0.08f) * 2.5f;
  g_climate.magnetic_angle = fmodf(time_f * 0.05f, 6.28318f);

  // Loop Pembaruan Vitalitas & Buah Pohon (Geologi Kesuburan Tanah, Spesies, Jarak & Realisme)
  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    if (g_h_trees[t].health <= 0.0f) continue; // Slot pohon kosong/mati menunggu ditanam atau bertunas

    g_h_trees[t].age_years += (float)(dt / ParameterAgent::SECONDS_PER_YEAR);

    // Pohon mati karena usia tua alami atau kehabisan vitalitas
    if (g_h_trees[t].age_years >= (float)DuniaFisika::TREE_MAX_AGE_YEARS || g_h_trees[t].health <= 0.0f) {
      g_total_tree_deaths++;
      g_h_trees[t].health = 0.0f;
      g_h_trees[t].growth_stage = 0.0f;
      g_h_trees[t].fruits_count = 0.0f;
      continue;
    }

    // Geologi Kesuburan Tanah & Jarak ke Sungai
    float soil = g_h_trees[t].soil_fertility;
    if (soil <= 0.1f) {
      soil = (float)DuniaFisika::SOIL_FERTILITY_BASE + 
             (float)DuniaFisika::SOIL_FERTILITY_VARIATION * sinf(g_h_trees[t].x * (float)DuniaFisika::SOIL_NOISE_SCALE_X) * cosf(g_h_trees[t].y * (float)DuniaFisika::SOIL_NOISE_SCALE_Y);
      g_h_trees[t].soil_fertility = fmaxf(0.2f, soil);
    }

    // Penyerapan Air oleh Pohon (Dari Bantaran Sungai & Kelembaban Atmosfer H2O)
    float river_x = (float)DuniaFisika::RIVER_CENTER_X + (float)DuniaFisika::RIVER_MEANDER_AMP * sinf(g_h_trees[t].y * (float)DuniaFisika::RIVER_MEANDER_FREQ);
    float dist_river = fabsf(g_h_trees[t].x - river_x);
    float river_water_gain = 0.0f;
    if (dist_river <= (float)DuniaFisika::RIVER_MOISTURE_RADIUS) {
      float proximity = 1.0f - (dist_river / (float)DuniaFisika::RIVER_MOISTURE_RADIUS);
      river_water_gain = (float)DuniaFisika::TREE_WATER_ABSORB_RIVER_RATE * proximity * (float)dt;
    }
    float rain_water_gain = (float)DuniaFisika::TREE_WATER_ABSORB_RAIN_RATE * (g_climate.h2o_level / 100.0f) * (float)dt;
    g_h_trees[t].moisture = fminf((float)DuniaFisika::TREE_MOISTURE_MAX, g_h_trees[t].moisture + river_water_gain + rain_water_gain);

    // Pohon Mengonsumsi Air untuk Fotosintesis & Hidup
    float water_consumption = (float)DuniaFisika::TREE_WATER_CONSUMPTION_RATE * (0.5f + 0.5f * g_climate.daylight_factor) * (float)dt;
    g_h_trees[t].moisture = fmaxf(0.0f, g_h_trees[t].moisture - water_consumption);

    // Kekeringan: Jika moisture habis (0%), kesehatan pohon membusuk/mengering drastis
    if (g_h_trees[t].moisture <= 0.0f) {
      float drought_dmg = (float)DuniaFisika::TREE_DROUGHT_DECAY_RATE * (float)dt;
      g_h_trees[t].health = fmaxf(0.0f, g_h_trees[t].health - drought_dmg);
    }

    // Musim mempengaruhi laju pertumbuhan (Semi: 1.3x, Panas: 1.0x, Gugur: 0.6x, Dingin: 0.2x)
    float season_growth_mult = 1.0f;
    if (g_climate.current_season == 0) season_growth_mult = 1.3f; // Semi
    else if (g_climate.current_season == 1) season_growth_mult = 1.0f; // Panas
    else if (g_climate.current_season == 2) season_growth_mult = 0.6f; // Gugur
    else if (g_climate.current_season == 3) season_growth_mult = 0.2f; // Dingin

    // Efisiensi Air terhadap Pertumbuhan Pohon
    float moisture_eff = fmaxf(0.1f, g_h_trees[t].moisture / 50.0f);

    // Pertumbuhan Fotosintesis Pohon Murni Berbasis Realita (Sinar Matahari + Kesuburan Geologi + Air + Musim)
    float heat_inhibition = (g_climate.temperature > 40.0f) ? fminf(0.85f, (g_climate.temperature - 40.0f) * 0.04f) : 0.0f;
    float sunlight_boost = fmaxf(0.05f, (0.4f + (float)DuniaFisika::DAY_PHOTOSYNTHESIS_MULT * g_climate.daylight_factor) * (1.0f - heat_inhibition));
    
    // Spesies Pioneer tumbuh lebih tahan iklim ekstrem
    float species_hardiness = (g_h_trees[t].tree_type == DuniaFisika::TREE_TYPE_PIONEER) ? 1.4f : 1.0f;
    float growth_rate = (float)DuniaFisika::TREE_REALISTIC_GROWTH_BASE * sunlight_boost * soil * season_growth_mult * species_hardiness * moisture_eff;

    g_h_trees[t].growth_stage = fminf(100.0f, g_h_trees[t].growth_stage + growth_rate * (float)dt);
    if (g_h_trees[t].moisture > 0.0f) {
      g_h_trees[t].health = fminf(100.0f, g_h_trees[t].health + (3.0f * soil - heat_inhibition * 6.0f) * (float)dt);
    }
    
    // Pembuahan khusus Pohon Buah (TREE_TYPE_FRUIT) saat matang & memiliki air cukup
    if (g_h_trees[t].tree_type == DuniaFisika::TREE_TYPE_FRUIT) {
      if (g_h_trees[t].growth_stage >= 40.0f && g_h_trees[t].moisture >= 20.0f && g_h_trees[t].fruits_count < (float)DuniaFisika::MAX_FRUIT_PER_TREE) {
        g_h_trees[t].fruits_count = fminf((float)DuniaFisika::MAX_FRUIT_PER_TREE, 
                                          g_h_trees[t].fruits_count + (float)DuniaFisika::FRUIT_SPAWN_RATE * sunlight_boost * season_growth_mult * (float)dt);
      }
    } else {
      g_h_trees[t].fruits_count = 0.0f; // Pohon Oksigen & Pioneer tidak berbuah
    }
  }

  // Dinamika Atmosfer: Fotosintesis Pohon menghasilkan O2, Respirasi Pohon & Agen menghasilkan CO2
  float total_tree_o2_prod = 0.0f;
  float total_tree_co2_resp = 0.0f;
  int living_trees_count = 0;

  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    if (g_h_trees[t].growth_stage >= 15.0f && g_h_trees[t].health > 0.0f) {
      living_trees_count++;
      float maturity = g_h_trees[t].growth_stage / 100.0f;
      float o2_mult = 1.0f;
      float co2_mult = 1.0f;
      if (g_h_trees[t].tree_type == DuniaFisika::TREE_TYPE_OXYGEN) {
        o2_mult = (float)DuniaFisika::TREE_OXYGEN_SPECIES_O2_MULT;
        co2_mult = (float)DuniaFisika::TREE_OXYGEN_SPECIES_CO2_MULT;
      }

      // Siang: Fotosintesis aktif menyerap CO2 & menghasilkan O2
      total_tree_o2_prod += (float)DuniaFisika::PHOTOSYNTHESIS_O2_RATE * o2_mult * maturity * g_climate.daylight_factor * (float)dt;
      // Malam: Respirasi pohon melepaskan CO2
      total_tree_co2_resp += (float)DuniaFisika::TREE_RESPIRATION_CO2_RATE * co2_mult * maturity * (1.0f - g_climate.daylight_factor) * (float)dt;
    }
  }
  int living_pop_total = (int)(g_living_leaderboard.size() + g_predator_leaderboard.size());
  
  // Respirasi seluruh populasi hidup
  float o2_consumed = (float)DuniaFisika::RESPIRATION_O2_CONSUMPTION * (float)living_pop_total * (float)dt;
  float agent_co2_emission = (float)DuniaFisika::RESPIRATION_CO2_EMISSION * (float)living_pop_total * (float)dt;

  // Hukum Keseimbangan Alam Atmosfer O2 & CO2
  g_climate.oxygen_level = fminf(30.0f, fmaxf(5.0f, g_climate.oxygen_level + (total_tree_o2_prod - o2_consumed)));
  g_climate.co2_level = fminf(5.0f, fmaxf(0.01f, g_climate.co2_level + (total_tree_co2_resp + agent_co2_emission - total_tree_o2_prod * 0.6f)));

  // Kelembaban Uap Air (H2O) bertranspirasi dari pohon dan suhu
  float transpiration = (float)living_trees_count * 0.02f * g_climate.daylight_factor * (float)dt;
  g_climate.h2o_level = fminf(95.0f, fmaxf(20.0f, g_climate.h2o_level + transpiration - 0.01f * (float)dt));

  // Efek Rumah Kaca: CO2 & H2O menahan radiasi panas matahari
  float greenhouse_warming = (g_climate.co2_level - (float)DuniaFisika::CO2_BASE_LEVEL) * (float)DuniaFisika::GREENHOUSE_CO2_WARMING_FACTOR +
                             (g_climate.h2o_level - (float)DuniaFisika::H2O_HUMIDITY_BASE_LEVEL) * (float)DuniaFisika::GREENHOUSE_H2O_WARMING_FACTOR;

  // Full Adversarial Nature AI: Tekanan adaptif yang tidak pernah longgar
  // Alam selalu menekan minimum NATURE_AGGRESSION_MIN (tidak pernah turun ke 0)
  float pop_ratio = (float)living_pop_total / fmaxf(1.0f, (float)DuniaFisika::NATURE_PRESSURE_TARGET_POP);
  float base_target = 1.0f + (pop_ratio - 1.0f) * (float)DuniaFisika::NATURE_PRESSURE_KP * 200.0f;

  // Tekanan entropi: koloni stagnan/monokultur mendapat tekanan seleksi lebih keras
  if (g_dna_diversity_index < 70.0f) {
    base_target += (70.0f - g_dna_diversity_index) * 0.025f;
  }

  // Surge instan jika populasi meledak > 1.5x target kapasitas
  if (pop_ratio > 1.5f) {
    base_target += (pop_ratio - 1.5f) * 2.0f;
  }

  // Clamp ke rentang adversarial [MIN, MAX]
  float target_nature_pressure = fmaxf((float)ParameterAgent::NATURE_AGGRESSION_MIN,
                                        fminf((float)ParameterAgent::NATURE_AGGRESSION_MAX, base_target));

  // Smooth adjustment dengan rebound speed lebih cepat (agresif)
  float rebound = (float)ParameterAgent::NATURE_REBOUND_SPEED;
  g_climate.nature_adversarial_pressure = g_climate.nature_adversarial_pressure * (1.0f - rebound) + target_nature_pressure * rebound;

  // Evaluasi Tekanan Lingkungan Bencana Alam Dinamis (Solar Storm / Blizzard / EMP Chaos)
  float disaster_cycle = fmodf(time_f, (float)DuniaFisika::DISASTER_INTERVAL_SECONDS);
  float disaster_temp_offset = 0.0f;
  if (disaster_cycle < (float)DuniaFisika::DISASTER_DURATION_SECONDS) {
    int disaster_type = ((int)(time_f / DuniaFisika::DISASTER_INTERVAL_SECONDS)) % 3;
    if (disaster_type == 0) {
      // Badai Matahari (Solar Storm Heatwave)
      disaster_temp_offset = (float)DuniaFisika::SOLAR_STORM_HEAT_SPIKE * sinf((disaster_cycle / (float)DuniaFisika::DISASTER_DURATION_SECONDS) * 3.14159f);
    } else if (disaster_type == 1) {
      // Badai Salju Beku Ekstrem (Blizzard Deep Freeze)
      disaster_temp_offset = (float)DuniaFisika::BLIZZARD_TEMP_DROP * sinf((disaster_cycle / (float)DuniaFisika::DISASTER_DURATION_SECONDS) * 3.14159f);
    } else {
      // Badai EMP / Radiasi Medan Magnetik (Mengacaukan Angin & Kompas)
      g_climate.magnetic_angle += sinf(time_f * 5.0f) * (float)DuniaFisika::EMP_MAGNETIC_CHAOS;
      g_climate.wind_x *= 2.5f;
      g_climate.wind_y *= 2.5f;
    }
  }

  // Pengaruh Siang & Malam + Efek Rumah Kaca + Bencana Alam Terhadap Suhu Lingkungan
  float base_season_temps[4] = {20.0f, 32.0f, 16.0f, 2.0f};
  float day_night_temp_delta = (g_climate.daylight_factor - 0.5f) * 2.0f * (float)DuniaFisika::DAYLIGHT_TEMP_BOOST;
  g_climate.temperature = base_season_temps[g_climate.current_season] + day_night_temp_delta + greenhouse_warming + disaster_temp_offset + sinf(time_f * 0.2f) * 1.5f;

  // Regenerasi & Pelapukan Formasi Material Periodik (Dipengaruhi Tekanan Alam)
  float mat_scarcity = 1.0f / fmaxf(0.8f, g_climate.nature_adversarial_pressure);
  for (int m = 0; m < DuniaFisika::MAX_PERIODIC_DEPOSITS; ++m) {
    if (g_h_minerals[m].mass < 100.0f) {
      g_h_minerals[m].mass = fminf(100.0f, g_h_minerals[m].mass + (float)DuniaFisika::MATERIAL_REGEN_RATE * mat_scarcity * (float)dt);
    }
  }

  cudaMemcpy(g_d_trees, g_h_trees,
             sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_minerals, g_h_minerals,
             sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS,
             cudaMemcpyHostToDevice);
  cudaMemcpy(g_d_climate, &g_climate, sizeof(GpuClimateState),
             cudaMemcpyHostToDevice);

  launch_ecosystem_simulation(g_d_agents, ParameterAgent::MAX_POPULATION_BUFFER,
                              g_d_trees, ParameterAgent::MAX_TREES,
                              g_d_predators, DuniaFisika::MAX_PREDATORS_BUFFER,
                              g_d_minerals, DuniaFisika::MAX_PERIODIC_DEPOSITS,
                              g_d_climate, dt);

  cudaMemcpy(g_h_agents, g_d_agents,
             sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER,
             cudaMemcpyDeviceToHost);
  cudaMemcpy(g_h_trees, g_d_trees,
             sizeof(GpuTreeEntity) * ParameterAgent::MAX_TREES,
             cudaMemcpyDeviceToHost);
  cudaMemcpy(g_h_predators, g_d_predators,
             sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER,
             cudaMemcpyDeviceToHost);
  cudaMemcpy(g_h_minerals, g_d_minerals,
             sizeof(GpuMineralDeposit) * DuniaFisika::MAX_PERIODIC_DEPOSITS,
             cudaMemcpyDeviceToHost);

  // =========================================================================
  // ARTIFACT SYSTEM: Klaim, Keabadian & Population Balancer
  // =========================================================================
  {
    // Hitung populasi hidup per kubu untuk balancer
    int pop_a = 0, pop_b = 0;
    for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i)
      if (g_h_agents[i].is_alive && g_h_agents[i].energy > 0.0f) pop_a++;
    for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p)
      if (g_h_predators[p].is_alive && g_h_predators[p].energy > 0.0f) pop_b++;
    int pop_total = pop_a + pop_b;
    float thresh = (float)DuniaFisika::ARTIFACT_BALANCER_POP_THRESHOLD;

    // === Artifact Kubu A (Herbivora) ===
    ArtifactEntity &art_a = g_artifacts[0];
    bool has_immortal_a = false;
    for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
      if (g_h_agents[i].is_alive && g_h_agents[i].immortality_timer > 0.0f) {
        has_immortal_a = true;
        break;
      }
    }
    if (art_a.is_active) {
      bool near_a = false;
      bool claimed_a = false;
      float claim_r2 = (float)(DuniaFisika::ARTIFACT_CLAIM_RADIUS * DuniaFisika::ARTIFACT_CLAIM_RADIUS);
      for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
        if (!g_h_agents[i].is_alive || g_h_agents[i].energy <= 0.0f) continue;
        float dx = g_h_agents[i].x - art_a.x;
        float dy = g_h_agents[i].y - art_a.y;
        if (dx*dx + dy*dy > claim_r2) continue;
        near_a = true;
        // Cek apakah comm_signal mendekati jawaban
        float err = fabsf(g_h_agents[i].comm_signal - art_a.answer_norm);
        if (err <= (float)DuniaFisika::ARTIFACT_ANSWER_TOLERANCE) {
          // Klaim berhasil
          claimed_a = true;
          art_a.status_state = 2; // terjawab
          g_h_agents[i].immortality_timer = art_a.immortality_seconds;
          std::cout << "[ARTIFACT-A] Herbi #" << g_h_agents[i].id
                    << " klaim Lvl " << art_a.level
                    << " (jawab=" << art_a.answer_norm << " err=" << err << ")"
                    << " | Keabadian " << DuniaFisika::ARTIFACT_IMMORTALITY_DAYS << " hari\n";
          // Population balancer: jika Kubu A hampir punah saat Kubu B klaim
          if (pop_total > 0 && (float)pop_b / (float)pop_total > (1.0f - thresh)) {
            int spawned = 0;
            for (int s = 0; s < ParameterAgent::MAX_POPULATION_BUFFER && spawned < DuniaFisika::ARTIFACT_BALANCER_SPAWN_COUNT; ++s) {
              if (!g_h_agents[s].is_alive) {
                g_h_agents[s] = g_h_agents[i]; // Klon dari pemenang
                g_h_agents[s].x = art_a.x + (float)((spawned % 3) - 1) * 20.0f;
                g_h_agents[s].y = art_a.y + (float)((spawned / 3) - 1) * 20.0f;
                g_h_agents[s].energy = 60.0f;
                g_h_agents[s].immortality_timer = 0.0f;
                g_h_agents[s].just_born = true;
                spawned++;
              }
            }
            if (spawned > 0)
              std::cout << "[BALANCER-A] Spawn " << spawned << " herbivora penyeimbang\n";
          }
          // Level naik & respawn
          art_a.level++;
          art_a.spawn_count++;
          artifact_next_spawn(art_a, 0);
          artifact_gen_challenge(art_a);
          art_a.is_active = true;
          // Sync perubahan immortality kembali ke GPU
          cudaMemcpy(g_d_agents, g_h_agents,
                     sizeof(GpuEcosystemAgent) * ParameterAgent::MAX_POPULATION_BUFFER,
                     cudaMemcpyHostToDevice);
          break;
        }
      }
      if (!claimed_a) {
        if (has_immortal_a) {
          art_a.status_state = 3; // active-immortal
        } else if (near_a) {
          art_a.status_state = 1; // gagal/kabur
        } else {
          art_a.status_state = 0; // mencari
        }
      }
    }

    // === Artifact Kubu B (Karnivora) ===
    ArtifactEntity &art_b = g_artifacts[1];
    bool has_immortal_b = false;
    for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
      if (g_h_predators[p].is_alive && g_h_predators[p].immortality_timer > 0.0f) {
        has_immortal_b = true;
        break;
      }
    }
    if (art_b.is_active) {
      bool near_b = false;
      bool claimed_b = false;
      float claim_r2 = (float)(DuniaFisika::ARTIFACT_CLAIM_RADIUS * DuniaFisika::ARTIFACT_CLAIM_RADIUS);
      for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
        if (!g_h_predators[p].is_alive || g_h_predators[p].energy <= 0.0f) continue;
        float dx = g_h_predators[p].x - art_b.x;
        float dy = g_h_predators[p].y - art_b.y;
        if (dx*dx + dy*dy > claim_r2) continue;
        near_b = true;
        float err = fabsf(g_h_predators[p].comm_signal - art_b.answer_norm);
        if (err <= (float)DuniaFisika::ARTIFACT_ANSWER_TOLERANCE) {
          claimed_b = true;
          art_b.status_state = 2; // terjawab
          g_h_predators[p].immortality_timer = art_b.immortality_seconds;
          std::cout << "[ARTIFACT-B] Karni #" << g_h_predators[p].id
                    << " klaim Lvl " << art_b.level
                    << " (jawab=" << art_b.answer_norm << " err=" << err << ")"
                    << " | Keabadian " << DuniaFisika::ARTIFACT_IMMORTALITY_DAYS << " hari\n";
          // Population balancer: jika Kubu B hampir punah
          if (pop_total > 0 && (float)pop_a / (float)pop_total > (1.0f - thresh)) {
            int spawned = 0;
            for (int s = 0; s < DuniaFisika::MAX_PREDATORS_BUFFER && spawned < DuniaFisika::ARTIFACT_BALANCER_SPAWN_COUNT; ++s) {
              if (!g_h_predators[s].is_alive) {
                g_h_predators[s] = g_h_predators[p];
                g_h_predators[s].x = art_b.x + (float)((spawned % 3) - 1) * 20.0f;
                g_h_predators[s].y = art_b.y + (float)((spawned / 3) - 1) * 20.0f;
                g_h_predators[s].energy = 60.0f;
                g_h_predators[s].immortality_timer = 0.0f;
                g_h_predators[s].just_born = true;
                spawned++;
              }
            }
            if (spawned > 0)
              std::cout << "[BALANCER-B] Spawn " << spawned << " karnivora penyeimbang\n";
          }
          art_b.level++;
          art_b.spawn_count++;
          artifact_next_spawn(art_b, 1);
          artifact_gen_challenge(art_b);
          art_b.is_active = true;
          cudaMemcpy(g_d_predators, g_h_predators,
                     sizeof(GpuPredatorAgent) * DuniaFisika::MAX_PREDATORS_BUFFER,
                     cudaMemcpyHostToDevice);
          break;
        }
      }
      if (!claimed_b) {
        if (has_immortal_b) {
          art_b.status_state = 3; // active-immortal
        } else if (near_b) {
          art_b.status_state = 1; // gagal/kabur
        } else {
          art_b.status_state = 0; // mencari
        }
      }
    }
  }

  // Monitoring Kelahiran & Kematian Faksi B (Predator)
  int total_mat_b = 0;
  int total_craft_b = 0;
  std::vector<TopPredatorRecord> current_living_predators;
  for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
    if (g_h_predators[p].just_killed) {
      g_total_predator_kills++;
    }
    if (g_h_predators[p].just_died) {
      g_predator_deaths++;
      g_cum_tools_crafted_b += g_h_predators[p].tools_crafted;
    }
    if (g_h_predators[p].just_born) {
      g_predator_births++;
    }
    total_mat_b += g_h_predators[p].material_interactions;
    total_craft_b += g_h_predators[p].tools_crafted;
    if (g_h_predators[p].is_alive && g_h_predators[p].energy > 0.0f) {
      TopPredatorRecord rec;
      rec.id = g_h_predators[p].id;
      rec.generation = g_h_predators[p].generation;
      rec.gender = g_h_predators[p].gender;
      rec.sin_type = g_h_predators[p].sin_type;
      rec.age_years = g_h_predators[p].age_years;
      rec.prey_devoured = g_h_predators[p].prey_devoured;
      rec.material_interactions = g_h_predators[p].material_interactions;
      rec.tools_crafted = g_h_predators[p].tools_crafted;
      rec.formula_shield = g_h_predators[p].formula_shield;
      rec.energy = g_h_predators[p].energy;
      current_living_predators.push_back(rec);
    }
  }

  // Urutkan Papan Peringkat Predator berdasarkan Kontribusi Terbesar pada Koloni
  std::sort(current_living_predators.begin(), current_living_predators.end(),
            [](const TopPredatorRecord &a, const TopPredatorRecord &b) {
              int score_a = (a.prey_devoured * 300) + (a.tools_crafted * 150) + (a.material_interactions * 10) + (int)a.energy + (int)(a.age_years * 2);
              int score_b = (b.prey_devoured * 300) + (b.tools_crafted * 150) + (b.material_interactions * 10) + (int)b.energy + (int)(b.age_years * 2);
              return score_a > score_b;
            });

  // Monitoring Kelahiran & Kematian Realtime + Dynamic Living Leaderboard Faksi A (Herbivora)
  int total_fruits = 0;
  int total_formulas = 0;
  int total_mat_a = 0;
  int total_craft_a = 0;
  std::vector<TopAgentRecord> current_living;

  for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
    if (g_h_agents[i].just_died) {
      g_total_deaths++;
      g_cum_tools_crafted_a += g_h_agents[i].tools_crafted;
    }
    if (g_h_agents[i].just_born)
      g_total_births++;

    total_fruits += g_h_agents[i].fruits_eaten;
    total_formulas += g_h_agents[i].formulas_discovered;
    total_mat_a += g_h_agents[i].material_interactions;
    total_craft_a += g_h_agents[i].tools_crafted;

    // Hanya masukkan individu yang masih hidup ke papan peringkat
    if (g_h_agents[i].is_alive && g_h_agents[i].energy > 0.0f) {
      TopAgentRecord rec;
      rec.id = g_h_agents[i].id;
      rec.generation = g_h_agents[i].generation;
      rec.gender = g_h_agents[i].gender;
      rec.sin_type = g_h_agents[i].sin_type;
      rec.age_years = g_h_agents[i].age_years;
      rec.fruits_eaten = g_h_agents[i].fruits_eaten;
      rec.formulas_discovered = g_h_agents[i].formulas_discovered;
      rec.predators_slain = g_h_agents[i].predators_slain;
      rec.material_interactions = g_h_agents[i].material_interactions;
      rec.tools_crafted = g_h_agents[i].tools_crafted;
      rec.energy = g_h_agents[i].energy;
      rec.formula = decompile_dna_formula(g_h_agents[i].dna_program,
                                          g_h_agents[i].active_program_size);
      current_living.push_back(rec);
    }
  }

  // Urutkan Papan Peringkat Herbivora berdasarkan Kontribusi Terbesar pada Koloni
  std::sort(current_living.begin(), current_living.end(),
            [](const TopAgentRecord &a, const TopAgentRecord &b) {
              int score_a = (a.predators_slain * 300) + (a.tools_crafted * 150) +
                            (a.fruits_eaten * 25) + (a.formulas_discovered * 50) + 
                            (a.material_interactions * 10) + (int)a.energy + (int)(a.age_years * 2);
              int score_b = (b.predators_slain * 300) + (b.tools_crafted * 150) +
                            (b.fruits_eaten * 25) + (b.formulas_discovered * 50) + 
                            (b.material_interactions * 10) + (int)b.energy + (int)(b.age_years * 2);
              return score_a > score_b;
            });

  EnterCriticalSection(&g_cs);
  g_living_leaderboard = std::move(current_living);
  g_predator_leaderboard = std::move(current_living_predators);
  g_total_fruits_harvested = total_fruits;
  g_total_formulas_synthesized = total_formulas;
  g_total_material_interactions_a = total_mat_a;
  g_total_tools_crafted_a = total_craft_a;
  g_total_material_interactions_b = total_mat_b;
  g_total_tools_crafted_b = total_craft_b;

  // Perhitungan Kecepatan Interaksi Material per Menit (RPM)
  static double rpm_timer = 0.0;
  static int last_mat_a = 0;
  static int last_mat_b = 0;
  rpm_timer += dt;
  if (rpm_timer >= 1.0) {
    g_rpm_material_a = (int)((total_mat_a - last_mat_a) * (60.0 / rpm_timer));
    g_rpm_material_b = (int)((total_mat_b - last_mat_b) * (60.0 / rpm_timer));
    if (g_rpm_material_a < 0) g_rpm_material_a = 0;
    if (g_rpm_material_b < 0) g_rpm_material_b = 0;
    last_mat_a = total_mat_a;
    last_mat_b = total_mat_b;
    rpm_timer = 0.0;
  }

  // Perhitungan Kompleksitas & Entropi DNA (Code Diversity Index)
  int opcode_counts[15] = {0};
  int total_opcodes = 0;
  int communicating_count = 0;
  int living_total = (int)(g_living_leaderboard.size() + g_predator_leaderboard.size());

  for (size_t i = 0; i < g_living_leaderboard.size(); ++i) {
    int ag_id = g_living_leaderboard[i].id;
    for (int idx = 0; idx < ParameterAgent::MAX_POPULATION_BUFFER; ++idx) {
      if (g_h_agents[idx].id == ag_id) {
        if (fabsf(g_h_agents[idx].comm_received) > 0.05f || fabsf(g_h_agents[idx].comm_signal) > 0.05f) {
          communicating_count++;
        }
        int prog_sz = std::min(g_h_agents[idx].active_program_size, (int)DNA_PROGRAM_SIZE);
        for (int ip = 0; ip < prog_sz; ++ip) {
          opcode_counts[g_h_agents[idx].dna_program[ip].op % 15]++;
          total_opcodes++;
        }
        break;
      }
    }
  }
  for (size_t p = 0; p < g_predator_leaderboard.size(); ++p) {
    int pred_id = g_predator_leaderboard[p].id;
    for (int p_idx = 0; p_idx < DuniaFisika::MAX_PREDATORS_BUFFER; ++p_idx) {
      if (g_h_predators[p_idx].id == pred_id) {
        if (fabsf(g_h_predators[p_idx].comm_received) > 0.05f || fabsf(g_h_predators[p_idx].comm_signal) > 0.05f) {
          communicating_count++;
        }
        int prog_sz = std::min(g_h_predators[p_idx].active_program_size, (int)DNA_PROGRAM_SIZE);
        for (int ip = 0; ip < prog_sz; ++ip) {
          opcode_counts[g_h_predators[p_idx].dna_program[ip].op % 15]++;
          total_opcodes++;
        }
        break;
      }
    }
  }

  // Shannon Entropy DNA
  double entropy = 0.0;
  if (total_opcodes > 0) {
    for (int k = 0; k < 15; ++k) {
      if (opcode_counts[k] > 0) {
        double prob = (double)opcode_counts[k] / (double)total_opcodes;
        entropy -= prob * (log(prob) / log(2.0));
      }
    }
    g_dna_diversity_index = (float)std::min(100.0, (entropy / (log(15.0) / log(2.0))) * 100.0);
  } else {
    g_dna_diversity_index = 0.0f;
  }

  g_social_learning_index = living_total > 0 ? ((float)communicating_count / (float)living_total) * 100.0f : 0.0f;

  // Energy Return on Investment (EROI)
  float total_nutrisi_gained = (float)(g_total_fruits_harvested * DuniaFisika::FRUIT_NUTRITION_ENERGY + g_total_predator_kills * DuniaFisika::PREDATOR_ENERGY_GAIN);
  float total_metabolism_cost = fmaxf(1.0f, (float)(g_sim_time * std::max(1, living_total) * ParameterAgent::METABOLISM_BASE_RATE * 0.5f));
  g_eroi_score = fminf(20.0f, fmaxf(0.1f, total_nutrisi_gained / total_metabolism_cost));

  // Ketinggian Hierarki Strategi
  if (total_craft_a > 10 && total_craft_b > 10 && g_social_learning_index > 20.0f && g_dna_diversity_index > 40.0f) {
    g_strategy_hierarchy_level = "Tingkat 4: Meta-Intelligence & Budaya Kolektif";
  } else if (g_social_learning_index > 15.0f && (total_craft_a > 0 || total_craft_b > 0)) {
    g_strategy_hierarchy_level = "Tingkat 3: Koordinasi Swarm & Megalitikum";
  } else if (total_craft_a > 0 || total_craft_b > 0) {
    g_strategy_hierarchy_level = "Tingkat 2: Penguasaan Alat (Tool Crafting)";
  } else {
    g_strategy_hierarchy_level = "Tingkat 1: Refleks Alami & Pencarian Pangan";
  }

  if (!g_living_leaderboard.empty()) {
    g_best_ecosystem_formula = g_living_leaderboard[0].formula;
    int top_id = g_living_leaderboard[0].id;
    for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
      if (g_h_agents[i].id == top_id) {
        g_alpha_agent_idx = i;
        static int save_counter = 0;
        if (++save_counter % 150 == 0) { // Simpan berkala setiap ~5 detik
          save_best_dna_to_file(g_h_agents[i]);
        }
        break;
      }
    }
  }

  if (!g_predator_leaderboard.empty()) {
    int top_pred_id = g_predator_leaderboard[0].id;
    for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
      if (g_h_predators[p].id == top_pred_id) {
        g_alpha_predator_idx = p;
        static int pred_save_counter = 0;
        if (++pred_save_counter % 150 == 0) { // Simpan berkala setiap ~5 detik
          save_best_predator_dna_to_file(g_h_predators[p]);
        }
        break;
      }
    }
  }
  LeaveCriticalSection(&g_cs);
}

std::string g_cached_telemetry_json = "{}";
CRITICAL_SECTION g_json_cs;

std::string build_telemetry_json_internal() {
  EnterCriticalSection(&g_cs);
  float total_available_fruits = 0.0f;
  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    total_available_fruits += g_h_trees[t].fruits_count;
  }

  std::stringstream ss;
  ss << std::fixed << std::setprecision(2);
  ss << "{\n";
  ss << "  \"day\": " << g_day_count << ",\n";
  ss << "  \"season\": " << g_climate.current_season << ",\n";
  const char *season_names[] = {"Semi (Spring)", "Panas (Summer)",
                                "Gugur (Autumn)", "Dingin (Winter)"};
  ss << "  \"seasonName\": \"" << season_names[g_climate.current_season % 4]
     << "\",\n";
  ss << "  \"temperature\": " << g_climate.temperature << ",\n";
  ss << "  \"daylightFactor\": " << g_climate.daylight_factor << ",\n";
  ss << "  \"windX\": " << g_climate.wind_x << ",\n";
  ss << "  \"windY\": " << g_climate.wind_y << ",\n";
  ss << "  \"oxygenLevel\": " << g_climate.oxygen_level << ",\n";
  ss << "  \"co2Level\": " << g_climate.co2_level << ",\n";
  ss << "  \"h2oLevel\": " << g_climate.h2o_level << ",\n";
  ss << "  \"nitrogenLevel\": " << g_climate.nitrogen_level << ",\n";
  ss << "  \"atmosphericPressure\": " << g_climate.atmospheric_pressure << ",\n";
  ss << "  \"naturePressure\": " << g_climate.nature_adversarial_pressure << ",\n";
  ss << "  \"totalFruitsHarvested\": " << g_total_fruits_harvested << ",\n";
  ss << "  \"totalFruitsAvailable\": " << total_available_fruits << ",\n";
  ss << "  \"totalFormulasSynthesized\": " << g_total_formulas_synthesized << ",\n";
  ss << "  \"totalMaterialInteractionsA\": " << g_total_material_interactions_a << ",\n";
  ss << "  \"totalToolsCraftedA\": " << g_total_tools_crafted_a << ",\n";
  ss << "  \"totalMaterialInteractionsB\": " << g_total_material_interactions_b << ",\n";
  ss << "  \"totalToolsCraftedB\": " << g_total_tools_crafted_b << ",\n";
  ss << "  \"rpmMaterialA\": " << g_rpm_material_a << ",\n";
  ss << "  \"rpmMaterialB\": " << g_rpm_material_b << ",\n";
  ss << "  \"dnaDiversity\": " << g_dna_diversity_index << ",\n";
  ss << "  \"socialLearning\": " << g_social_learning_index << ",\n";
  ss << "  \"eroiScore\": " << g_eroi_score << ",\n";
  ss << "  \"strategyHierarchy\": \"" << g_strategy_hierarchy_level << "\",\n";
  ss << "  \"speedMultiplier\": " << g_time_speed_multiplier.load() << ",\n";
  ss << "  \"minerals\": [";
  for (int m = 0; m < DuniaFisika::MAX_PERIODIC_DEPOSITS; ++m) {
    ss << "{\"x\":" << g_h_minerals[m].x << ",\"y\":" << g_h_minerals[m].y
       << ",\"t\":" << g_h_minerals[m].element_type
       << ",\"m\":" << g_h_minerals[m].mass
       << ",\"h\":" << g_h_minerals[m].hardness
       << ",\"c\":" << g_h_minerals[m].conductivity << "}"
       << (m < DuniaFisika::MAX_PERIODIC_DEPOSITS - 1 ? "," : "");
  }
  ss << "],\n";
  ss << "  \"totalBirths\": " << g_total_births << ",\n";
  ss << "  \"totalDeaths\": " << g_total_deaths << ",\n";
  ss << "  \"livingPopulation\": " << g_living_leaderboard.size() << ",\n";
  ss << "  \"formula\": \"" << g_best_ecosystem_formula << "\",\n";

  // Alpha Leader Stats
  if (!g_living_leaderboard.empty()) {
    const auto &alpha = g_living_leaderboard[0];
    ss << "  \"alphaId\": " << alpha.id << ",\n";
    ss << "  \"alphaGen\": " << alpha.generation << ",\n";
    ss << "  \"alphaGender\": " << alpha.gender << ",\n";
    ss << "  \"alphaSin\": " << alpha.sin_type << ",\n";
    ss << "  \"alphaAge\": " << alpha.age_years << ",\n";
    ss << "  \"alphaEnergy\": " << alpha.energy << ",\n";
    ss << "  \"alphaFruits\": " << alpha.fruits_eaten << ",\n";
    ss << "  \"alphaFormulas\": " << alpha.formulas_discovered << ",\n";
    ss << "  \"alphaMaterial\": " << alpha.material_interactions << ",\n";
    ss << "  \"alphaCrafts\": " << alpha.tools_crafted << ",\n";
  } else {
    ss << "  \"alphaId\": 1,\n  \"alphaGen\": 1,\n  \"alphaGender\": 0,\n  \"alphaSin\": 0,\n  "
          "\"alphaAge\": 0,\n  \"alphaEnergy\": 100,\n  \"alphaFruits\": 0,\n  "
          "\"alphaFormulas\": 0,\n  \"alphaMaterial\": 0,\n  \"alphaCrafts\": 0,\n";
  }

  // Top Leaderboard (Hanya yang masih hidup)
  ss << "  \"topRankings\": [";
  int top_n = std::min(5, (int)g_living_leaderboard.size());
  for (int k = 0; k < top_n; ++k) {
    const auto &r = g_living_leaderboard[k];
    ss << "{\"id\":" << r.id << ",\"gen\":" << r.generation
       << ",\"g\":" << r.gender << ",\"sin\":" << r.sin_type << ",\"age\":" << r.age_years
       << ",\"f\":" << r.formulas_discovered << ",\"fruits\":" << r.fruits_eaten
       << ",\"pk\":" << r.predators_slain
       << ",\"mi\":" << r.material_interactions
       << ",\"tc\":" << r.tools_crafted
       << ",\"e\":" << r.energy << ",\"formula\":\"" << r.formula << "\"}"
       << (k < top_n - 1 ? "," : "");
  }
  ss << "],\n";

  // Dynamic Living Agents Coordinate Stream
  ss << "  \"agents\": [";
  bool first_agent = true;
  for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
    if (g_h_agents[i].is_alive && g_h_agents[i].energy > 0.0f) {
      if (!first_agent) ss << ",";
      ss << "{\"id\":" << g_h_agents[i].id << ",\"x\":" << g_h_agents[i].x
         << ",\"y\":" << g_h_agents[i].y << ",\"e\":" << g_h_agents[i].energy
         << ",\"a\":1"
         << ",\"g\":" << g_h_agents[i].gender
         << ",\"sin\":" << g_h_agents[i].sin_type
         << ",\"age\":" << g_h_agents[i].age_years
         << ",\"pk\":" << g_h_agents[i].predators_slain
         << ",\"mi\":" << g_h_agents[i].material_interactions
         << ",\"tc\":" << g_h_agents[i].tools_crafted
         << ",\"f\":" << g_h_agents[i].fruits_eaten << "}";
      first_agent = false;
    }
  }
  ss << "],\n";

  int living_trees_count = 0;
  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    if (g_h_trees[t].growth_stage >= 20.0f && g_h_trees[t].health > 0.0f) {
      living_trees_count++;
    }
  }

  // Trees Coordinate & Fruits Stream (Hanya pohon aktif)
  ss << "  \"livingTreesCount\": " << living_trees_count << ",\n";
  ss << "  \"totalTreeDeaths\": " << g_total_tree_deaths << ",\n";
  ss << "  \"totalTreeSprouts\": " << g_total_tree_sprouts << ",\n";
  ss << "  \"trees\": [";
  bool first_tree = true;
  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    if (g_h_trees[t].health > 0.0f) {
      if (!first_tree) ss << ",";
      ss << "{\"x\":" << g_h_trees[t].x << ",\"y\":" << g_h_trees[t].y
         << ",\"g\":" << g_h_trees[t].growth_stage
         << ",\"h\":" << g_h_trees[t].health
         << ",\"type\":" << g_h_trees[t].tree_type
         << ",\"soil\":" << g_h_trees[t].soil_fertility
         << ",\"m\":" << g_h_trees[t].moisture
         << ",\"f\":" << g_h_trees[t].fruits_count << "}";
      first_tree = false;
    }
  }
  ss << "],\n";

  // Faction B (Predator / Karnivora) Telemetry
  ss << "  \"totalPredatorKills\": " << g_total_predator_kills << ",\n";
  ss << "  \"predatorBirths\": " << g_predator_births << ",\n";
  ss << "  \"predatorDeaths\": " << g_predator_deaths << ",\n";
  ss << "  \"livingPredators\": " << g_predator_leaderboard.size() << ",\n";
  
  // Top Predator Rankings
  ss << "  \"topPredators\": [";
  int top_p = std::min(5, (int)g_predator_leaderboard.size());
  for (int k = 0; k < top_p; ++k) {
    const auto &pr = g_predator_leaderboard[k];
    ss << "{\"id\":" << pr.id << ",\"gen\":" << pr.generation
       << ",\"g\":" << pr.gender << ",\"sin\":" << pr.sin_type << ",\"age\":" << pr.age_years
       << ",\"d\":" << pr.prey_devoured
       << ",\"mi\":" << pr.material_interactions
       << ",\"tc\":" << pr.tools_crafted
       << ",\"s\":" << pr.formula_shield
       << ",\"e\":" << pr.energy << "}"
       << (k < top_p - 1 ? "," : "");
  }
  ss << "],\n";

  // Dynamic Living Predators Coordinate Stream
  ss << "  \"predators\": [";
  bool first_pred = true;
  for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
    if (g_h_predators[p].is_alive && g_h_predators[p].energy > 0.0f) {
      if (!first_pred) ss << ",";
      ss << "{\"id\":" << g_h_predators[p].id << ",\"gen\":" << g_h_predators[p].generation
         << ",\"x\":" << g_h_predators[p].x << ",\"y\":" << g_h_predators[p].y
         << ",\"e\":" << g_h_predators[p].energy << ",\"g\":" << g_h_predators[p].gender
         << ",\"sin\":" << g_h_predators[p].sin_type
         << ",\"age\":" << g_h_predators[p].age_years
         << ",\"s\":" << g_h_predators[p].formula_shield
         << ",\"mi\":" << g_h_predators[p].material_interactions
         << ",\"tc\":" << g_h_predators[p].tools_crafted
         << ",\"d\":" << g_h_predators[p].prey_devoured << "}";
      first_pred = false;
    }
  }
  ss << "],\n";
  
  // Stream Bangkai / Mayat (Corpses) yang tersedia untuk dimakan Karnivora
  ss << "  \"corpses\": [";
  bool first_corpse = true;
  for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
    if (!g_h_agents[i].is_alive && g_h_agents[i].corpse_energy > 0.5f) {
      if (!first_corpse) ss << ",";
      ss << "{\"x\":" << g_h_agents[i].x << ",\"y\":" << g_h_agents[i].y
         << ",\"ce\":" << g_h_agents[i].corpse_energy << ",\"t\":0}";
      first_corpse = false;
    }
  }
  for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
    if (!g_h_predators[p].is_alive && g_h_predators[p].corpse_energy > 0.5f) {
      if (!first_corpse) ss << ",";
      ss << "{\"x\":" << g_h_predators[p].x << ",\"y\":" << g_h_predators[p].y
         << ",\"ce\":" << g_h_predators[p].corpse_energy << ",\"t\":1}";
      first_corpse = false;
    }
  }
  ss << "],\n";

  // DNA Disassembly of Alpha
  const auto &alpha_h = g_h_agents[g_alpha_agent_idx];
  ss << "  \"activeProgramSize\": " << alpha_h.active_program_size << ",\n";
  ss << "  \"dna\": [\n";
  const char *op_names[] = {"NOP",
                            "LOAD_SENSOR",
                            "ADD",
                            "SUB",
                            "MUL",
                            "DIV",
                            "TANH",
                            "SIGMOID",
                            "INTEGRAL",
                            "DERIVATIVE",
                            "ACTION_MOVE",
                            "ACTION_FORMULA",
                            "RESONATE_CLIMATE",
                            "FORK_NEURON",
                            "PRUNE_NEURON"};
  int send_prog = std::min(12, alpha_h.active_program_size);
  for (int ip = 0; ip < send_prog; ++ip) {
    const auto &inst = alpha_h.dna_program[ip];
    ss << "    {\"op\":\"" << op_names[inst.op % 15]
       << "\",\"arg\":" << inst.immediate_val << "}"
       << (ip < send_prog - 1 ? "," : "") << "\n";
  }
  ss << "  ]\n";
  ss << "}";
  LeaveCriticalSection(&g_cs);
  return ss.str();
}

void update_telemetry_cache() {
  std::string fresh_json = build_telemetry_json_internal();
  EnterCriticalSection(&g_json_cs);
  g_cached_telemetry_json = std::move(fresh_json);
  LeaveCriticalSection(&g_json_cs);
}

std::string get_cached_telemetry_json() {
  EnterCriticalSection(&g_json_cs);
  std::string copy = g_cached_telemetry_json;
  LeaveCriticalSection(&g_json_cs);
  return copy;
}

std::vector<SOCKET> g_ws_clients;
CRITICAL_SECTION g_ws_cs;

std::string base64_encode(const unsigned char *src, size_t len) {
  static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  int val = 0, valb = -6;
  for (size_t i = 0; i < len; ++i) {
    val = (val << 8) + src[i];
    valb += 8;
    while (valb >= 0) {
      out.push_back(tbl[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6) out.push_back(tbl[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4) out.push_back('=');
  return out;
}

#include <wincrypt.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "crypt32.lib")

std::string calculate_ws_accept(const std::string &client_key) {
  std::string magic = client_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  BYTE sha1_hash[20];
  DWORD hash_len = 20;

  if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
    if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
      CryptHashData(hHash, (const BYTE *)magic.c_str(), (DWORD)magic.length(), 0);
      CryptGetHashParam(hHash, HP_HASHVAL, sha1_hash, &hash_len, 0);
      CryptDestroyHash(hHash);
    }
    CryptReleaseContext(hProv, 0);
  }
  return base64_encode(sha1_hash, 20);
}

void broadcast_ws_telemetry(const std::string &json) {
  std::vector<uint8_t> frame;
  frame.push_back(0x81); // FIN + Text frame
  size_t len = json.length();
  if (len <= 125) {
    frame.push_back((uint8_t)len);
  } else if (len <= 65535) {
    frame.push_back(126);
    frame.push_back((uint8_t)((len >> 8) & 0xFF));
    frame.push_back((uint8_t)(len & 0xFF));
  } else {
    frame.push_back(127);
    for (int i = 7; i >= 0; --i) {
      frame.push_back((uint8_t)((len >> (i * 8)) & 0xFF));
    }
  }
  frame.insert(frame.end(), json.begin(), json.end());

  EnterCriticalSection(&g_ws_cs);
  std::vector<SOCKET> active_clients;
  for (SOCKET s : g_ws_clients) {
    int res = send(s, (const char *)frame.data(), (int)frame.size(), 0);
    if (res != SOCKET_ERROR && res > 0) {
      active_clients.push_back(s);
    } else {
      closesocket(s);
    }
  }
  g_ws_clients = std::move(active_clients);
  LeaveCriticalSection(&g_ws_cs);
}

void http_server_thread() {
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    return;

  SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == INVALID_SOCKET) {
    WSACleanup();
    return;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
             sizeof(opt));

  sockaddr_in address = {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(8088);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) ==
      SOCKET_ERROR) {
    closesocket(server_fd);
    WSACleanup();
    return;
  }

  listen(server_fd, 64);

  while (g_server_running) {
    SOCKET client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == INVALID_SOCKET)
      continue;

    char buffer[2048] = {0};
    int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read > 0) {
      std::string req(buffer);
      if (req.find("Upgrade: websocket") != std::string::npos || req.find("Upgrade: WebSocket") != std::string::npos) {
        // WebSocket Upgrade Handshake
        size_t key_pos = req.find("Sec-WebSocket-Key: ");
        if (key_pos != std::string::npos) {
          size_t end_pos = req.find("\r\n", key_pos);
          std::string client_key = req.substr(key_pos + 19, end_pos - (key_pos + 19));
          std::string accept_val = calculate_ws_accept(client_key);

          std::string ws_resp = "HTTP/1.1 101 Switching Protocols\r\n"
                                "Upgrade: websocket\r\n"
                                "Connection: Upgrade\r\n"
                                "Sec-WebSocket-Accept: " + accept_val + "\r\n\r\n";
          send(client_fd, ws_resp.c_str(), (int)ws_resp.length(), 0);

          // Pasang socket ke mode non-blocking
          u_long mode = 1;
          ioctlsocket(client_fd, FIONBIO, &mode);

          EnterCriticalSection(&g_ws_cs);
          g_ws_clients.push_back(client_fd);
          LeaveCriticalSection(&g_ws_cs);
          continue; // Jangan close client_fd, pertahankan koneksi jalan tol WebSocket
        }
      }

      if (req.find("GET /telemetry") != std::string::npos) {
        std::string json = get_cached_telemetry_json();
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: " +
                           std::to_string(json.length()) + "\r\n\r\n" + json;
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET /set_speed?val=") != std::string::npos) {
        size_t pos = req.find("GET /set_speed?val=");
        if (pos != std::string::npos) {
          std::string val_str = req.substr(pos + 19);
          double new_speed = std::stod(val_str);
          if (new_speed >= 1.0 && new_speed <= 2000.0) {
            g_time_speed_multiplier = new_speed;
          }
        }
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: 15\r\n\r\n{\"status\":\"ok\"}";
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET /set_bias?val=") != std::string::npos) {
        size_t pos = req.find("GET /set_bias?val=");
        if (pos != std::string::npos) {
          std::string val_str = req.substr(pos + 18);
          double new_bias = std::stod(val_str);
          if (new_bias >= 0.0 && new_bias <= 100.0) {
            g_growth_bias_ratio = new_bias;
          }
        }
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: 15\r\n\r\n{\"status\":\"ok\"}";
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET /spawn?") != std::string::npos) {
        int count = 10;
        size_t count_pos = req.find("count=");
        if (count_pos != std::string::npos) {
          count = std::max(1, std::min(500, std::stoi(req.substr(count_pos + 6))));
        }
        int spawned = 0;
        if (req.find("type=herbi") != std::string::npos) {
          spawned = inject_spawn_agents(count, 0);
        } else if (req.find("type=karni") != std::string::npos) {
          spawned = inject_spawn_agents(count, 1);
        } else if (req.find("type=tree") != std::string::npos) {
          spawned = inject_spawn_trees(count);
        } else if (req.find("type=mineral") != std::string::npos) {
          spawned = inject_spawn_minerals(count);
        }
        std::string json = "{\"status\":\"ok\",\"spawned\":" + std::to_string(spawned) + "}";
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: " +
                           std::to_string(json.length()) + "\r\n\r\n" + json;
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET /trigger_disaster?") != std::string::npos) {
        std::string dis_type = "solar";
        size_t type_pos = req.find("type=");
        if (type_pos != std::string::npos) {
          size_t end_p = req.find_first_of(" &\r\n", type_pos + 5);
          if (end_p != std::string::npos) {
            dis_type = req.substr(type_pos + 5, end_p - (type_pos + 5));
          } else {
            dis_type = req.substr(type_pos + 5);
          }
        }
        inject_disaster_event(dis_type);
        std::string json = "{\"status\":\"ok\",\"disaster\":\"" + dis_type + "\"}";
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: " +
                           std::to_string(json.length()) + "\r\n\r\n" + json;
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET /set_physics?") != std::string::npos) {
        if (req.find("param=wind") != std::string::npos) {
          double wx = 0.0, wy = 0.0;
          size_t x_pos = req.find("x=");
          if (x_pos != std::string::npos) wx = std::stod(req.substr(x_pos + 2));
          size_t y_pos = req.find("y=");
          if (y_pos != std::string::npos) wy = std::stod(req.substr(y_pos + 2));
          inject_set_physics("wind", wx, wy);
        } else if (req.find("param=temp") != std::string::npos) {
          size_t val_pos = req.find("val=");
          if (val_pos != std::string::npos) {
            double temp = std::stod(req.substr(val_pos + 4));
            inject_set_physics("temp", temp);
          }
        } else if (req.find("param=nature_pressure") != std::string::npos) {
          size_t val_pos = req.find("val=");
          if (val_pos != std::string::npos) {
            double pres = std::stod(req.substr(val_pos + 4));
            inject_set_physics("nature_pressure", pres);
          }
        }
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: 15\r\n\r\n{\"status\":\"ok\"}";
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET /save_checkpoint") != std::string::npos) {
        bool ok = save_ecosystem_checkpoint(ParameterAgent::ECOSYSTEM_CHECKPOINT_FILE);
        std::string json = "{\"status\":\"" + std::string(ok ? "ok" : "error") + "\",\"message\":\"" + (ok ? "Checkpoint saved" : "Save failed") + "\"}";
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: " +
                           std::to_string(json.length()) + "\r\n\r\n" + json;
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET /load_checkpoint") != std::string::npos) {
        bool ok = load_ecosystem_checkpoint(ParameterAgent::ECOSYSTEM_CHECKPOINT_FILE);
        std::string json = "{\"status\":\"" + std::string(ok ? "ok" : "error") + "\",\"message\":\"" + (ok ? "Checkpoint loaded" : "Load failed") + "\"}";
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Connection: close\r\n"
                           "Content-Length: " +
                           std::to_string(json.length()) + "\r\n\r\n" + json;
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      } else if (req.find("GET / ") != std::string::npos ||
                 req.find("GET /index.html") != std::string::npos) {
        std::ifstream html_file(ParameterAgent::WEB_DASHBOARD_FILE, std::ios::binary);
        if (!html_file.is_open()) {
          html_file.open("web/index.html", std::ios::binary);
        }
        std::string content;
        if (html_file.is_open()) {
          std::stringstream ss;
          ss << html_file.rdbuf();
          content = ss.str();
        } else {
          content = "<h1>404 Not Found (web/index.html)</h1>";
        }
        std::string resp = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/html; charset=UTF-8\r\n"
                           "Connection: close\r\n"
                           "Content-Length: " +
                           std::to_string(content.length()) + "\r\n\r\n" +
                           content;
        send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
      }
    }
    shutdown(client_fd, SD_BOTH);
    closesocket(client_fd);
  }

  closesocket(server_fd);
  WSACleanup();
}

int main(int argc, char *argv[]) {
  srand(ParameterAgent::FIXED_SIMULATION_SEED);
  InitializeCriticalSection(&g_cs);
  InitializeCriticalSection(&g_json_cs);
  InitializeCriticalSection(&g_ws_cs);

  init_ecosystem_pipeline();

  bool resume_requested = false;
  std::string checkpoint_path = ParameterAgent::ECOSYSTEM_CHECKPOINT_FILE;
  for (int a = 1; a < argc; ++a) {
    std::string arg = argv[a];
    if (arg == "--resume" || arg == "-r") {
      resume_requested = true;
    } else if ((arg == "--load-state" || arg == "-l") && a + 1 < argc) {
      resume_requested = true;
      checkpoint_path = argv[++a];
    }
  }

  if (resume_requested) {
    if (load_ecosystem_checkpoint(checkpoint_path)) {
      std::cout << "[CHECKPOINT RESUME] Sukses memuat state dari " << checkpoint_path << " (Day " << g_day_count << ")" << std::endl;
    } else {
      std::cout << "[CHECKPOINT RESUME] Checkpoint tidak ditemukan/gagal, memulai simulasi baru." << std::endl;
    }
  }

  update_telemetry_cache();

  // Jalankan server telemetri background
  std::thread server_th(http_server_thread);
  server_th.detach();

  std::cout
      << "================================================================"
      << std::endl;
  std::cout << " [GPU MIND ECOSYSTEM] 100 LIVING AGENTS MULTI-PHYSICS WORLD"
            << std::endl;
  std::cout << " Web Dashboard: http://localhost:8088" << std::endl;
  std::cout
      << " Features: 100 Agents, 32 Trees, Climate, Wind, Magnetism, Fruits"
      << std::endl;
  std::cout
      << "================================================================"
      << std::endl;

  if (!DuniaFisika::HEADLESS_MODE) {
    // Buka antarmuka webapp standalone mode (Edge / Chrome / default browser)
    HINSTANCE hApp = ShellExecuteA(NULL, "open", "msedge.exe", "--app=http://localhost:8088 --window-size=1280,820", NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hApp <= 32) {
      hApp = ShellExecuteA(NULL, "open", "chrome.exe", "--app=http://localhost:8088 --window-size=1280,820", NULL, SW_SHOWNORMAL);
      if ((INT_PTR)hApp <= 32) {
        ShellExecuteA(NULL, "open", "http://localhost:8088", NULL, NULL, SW_SHOWNORMAL);
      }
    }
  } else {
    std::cout << "\n[MODE HEADLESS AKTIF] Menjalankan simulasi akselerasi penuh di latar belakang...\n" << std::endl;
  }

  auto last_log = std::chrono::steady_clock::now();
  auto last_cache_update = std::chrono::steady_clock::now();
  int last_saved_day = g_day_count;

  while (g_server_running) {
    // Deterministic Fixed Timestep Execution
    double substep_dt = ParameterAgent::FIXED_SUBSTEP_DT;
    step_ecosystem(substep_dt);

    // Auto-Save Checkpoint Periodik
    if (g_day_count % ParameterAgent::CHECKPOINT_INTERVAL_DAYS == 0 && g_day_count != last_saved_day) {
      last_saved_day = g_day_count;
      save_ecosystem_checkpoint(ParameterAgent::ECOSYSTEM_CHECKPOINT_FILE);
      std::cout << "[CHECKPOINT AUTO-SAVE] State Day " << g_day_count << " tersimpan ke " << ParameterAgent::ECOSYSTEM_CHECKPOINT_FILE << std::endl;
    }

    // Evaluasi Kepunahan Otomatis (Stop & Report)
    if (DuniaFisika::STOP_ON_EXTINCTION && g_day_count >= 2) {
      int living_a = (int)g_living_leaderboard.size();
      int living_b = (int)g_predator_leaderboard.size();
      if (living_a == 0 || living_b == 0) {
        save_ecosystem_checkpoint(ParameterAgent::ECOSYSTEM_CHECKPOINT_FILE);
        std::cout << "\n================================================================" << std::endl;
        std::cout << " [SIMULASI BERHENTI: SALAH SATU FAKSI PUNAH]" << std::endl;
        std::cout << "================================================================" << std::endl;
        std::cout << "Hari Terakhir : Day " << g_day_count << std::endl;
        std::cout << "Pemenang      : " << (living_a > 0 ? "KUBU A (HERBIVORA)" : "KUBU B (PREDATOR)") << std::endl;
        int total_all_tools_a = g_total_tools_crafted_a + g_cum_tools_crafted_a;
        int total_all_tools_b = g_total_tools_crafted_b + g_cum_tools_crafted_b;
        std::cout << "Populasi A    : " << living_a << " Semut Hidup (Lahir: " << g_total_births << ", Mati: " << g_total_deaths << ", Alat Aktif: " << g_total_tools_crafted_a << ", Total Alat Dibuat: " << total_all_tools_a << ")" << std::endl;
        std::cout << "Populasi B    : " << living_b << " Predator Hidup (Lahir: " << g_predator_births << ", Mati: " << g_predator_deaths << ", Alat Aktif: " << g_total_tools_crafted_b << ", Total Alat Dibuat: " << total_all_tools_b << ")" << std::endl;
        std::cout << "Total Panen   : " << g_total_fruits_harvested << " Buah" << std::endl;
        std::cout << "Dimangsa      : " << g_total_predator_kills << " Semut" << std::endl;
        if (!g_living_leaderboard.empty()) {
          const auto &champ_a = g_living_leaderboard[0];
          std::cout << "Terbaik Kubu A: #" << champ_a.id << " (Gen " << champ_a.generation 
                    << ", " << (champ_a.gender == 0 ? "Cewe" : "Cowo") 
                    << ", Usia " << std::fixed << std::setprecision(1) << champ_a.age_years << " Th"
                    << ", Kill " << champ_a.predators_slain 
                    << ", Alat " << champ_a.tools_crafted 
                    << ", Panen " << champ_a.fruits_eaten 
                    << ", Energi " << (int)champ_a.energy << ")" << std::endl;
          std::cout << "  Algoritma A : " << champ_a.formula << std::endl;
        }
        if (!g_predator_leaderboard.empty()) {
          const auto &champ_b = g_predator_leaderboard[0];
          std::cout << "Terbaik Kubu B: #" << champ_b.id << " (Gen " << champ_b.generation 
                    << ", " << (champ_b.gender == 0 ? "Cewe" : "Cowo") 
                    << ", Usia " << std::fixed << std::setprecision(1) << champ_b.age_years << " Th"
                    << ", Mangsa " << champ_b.prey_devoured 
                    << ", Alat " << champ_b.tools_crafted 
                    << ", Energi " << (int)champ_b.energy << ")" << std::endl;
        }
        std::cout << "----------------------------------------------------------------" << std::endl;
        std::cout << " [LAPORAN KUBU ALAM (ADVERSARIAL NATURE AI)]" << std::endl;
        std::cout << "Tekanan Adaptif : " << std::fixed << std::setprecision(2) << g_climate.nature_adversarial_pressure << "x (Target Keseimbangan: " << (int)DuniaFisika::NATURE_PRESSURE_TARGET_POP << " Pop)" << std::endl;
        std::cout << "Atmosfer & Suhu : " << std::setprecision(1) << g_climate.temperature << "°C | Musim: " 
                  << (g_climate.current_season == 0 ? "Semi" : (g_climate.current_season == 1 ? "Panas" : (g_climate.current_season == 2 ? "Gugur" : "Dingin"))) 
                  << " | O2: " << g_climate.oxygen_level << "% | CO2: " << g_climate.co2_level << "% | H2O: " << g_climate.h2o_level << "%" << std::endl;
        std::cout << "10 Bahaya Alam  : Wabah Pathogen, Toksisitas Bangkai, Erosi Sungai, Safe Haven Gua," << std::endl;
        std::cout << "                  Degradasi Karat, Siklus Diurnal Gelap, Udara Beracun, Lava Geothermal," << std::endl;
        std::cout << "                  Salinitas Air, Anomali Gravitasi Spasial." << std::endl;
        std::cout << "----------------------------------------------------------------" << std::endl;
        bool p3_req1 = (g_day_count >= 100);
        bool p3_req2 = (total_all_tools_a >= 10 && total_all_tools_b >= 10);
        bool p3_req3 = (g_dna_diversity_index >= 50.0f);
        bool p3_ready = (p3_req1 && p3_req2 && p3_req3);
        std::cout << " [STATUS KELULUSAN FASE 3 : " << (p3_ready ? "LULUS / TERCAPAI" : "BELUM MEMADAI") << "]" << std::endl;
        std::cout << "  1. Usia Ekosistem (>=100 Hari)  : " << (p3_req1 ? "[V] " : "[X] ") << "Day " << g_day_count << std::endl;
        std::cout << "  2. Perang Megalitikum Simetris  : " << (p3_req2 ? "[V] " : "[X] ") << "Alat A: " << total_all_tools_a << " vs Alat B: " << total_all_tools_b << " (Target: >=10 tiap kubu)" << std::endl;
        std::cout << "  3. Diversitas Genom (>=50.0%)   : " << (p3_req3 ? "[V] " : "[X] ") << std::fixed << std::setprecision(1) << g_dna_diversity_index << "%" << std::endl;
        std::cout << "================================================================\n" << std::endl;

        if (DuniaFisika::AUTO_RESTART_ON_EXTINCTION) {
          log_evolution_milestone(g_loop_run_count, g_best_gen_record_a, g_best_art_lvl_record_a, g_best_gen_record_b, g_best_art_lvl_record_b, g_climate.nature_adversarial_pressure, g_day_count);
          g_loop_run_count++;
          std::cout << "[AUTO-RESTART] Memulai putaran simulasi baru (Loop #" << g_loop_run_count << ")...\n" << std::endl;
          EnterCriticalSection(&g_cs);
          reset_ecosystem_state();
          last_saved_day = 1;
          last_log = std::chrono::steady_clock::now();
          LeaveCriticalSection(&g_cs);
          continue;
        } else {
          break;
        }
      }
    }

    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_cache_update).count() >= 16) {
      last_cache_update = now;
      update_telemetry_cache();
      broadcast_ws_telemetry(get_cached_telemetry_json());
    }

    if (std::chrono::duration_cast<std::chrono::seconds>(now - last_log).count() >= 3) {
      last_log = now;
      EnterCriticalSection(&g_cs);
      int pop_a = (int)g_living_leaderboard.size();
      int pop_b = (int)g_predator_leaderboard.size();
      int max_gen_a = 1;
      int max_dna_a = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;
      int max_gen_b = 1;
      int max_dna_b = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

      for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
        if (g_h_agents[i].is_alive && g_h_agents[i].energy > 0.0f) {
          if (g_h_agents[i].generation > max_gen_a) max_gen_a = g_h_agents[i].generation;
          if (g_h_agents[i].active_program_size > max_dna_a) max_dna_a = g_h_agents[i].active_program_size;
        }
      }
      for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
        if (g_h_predators[p].is_alive && g_h_predators[p].energy > 0.0f) {
          if (g_h_predators[p].generation > max_gen_b) max_gen_b = g_h_predators[p].generation;
          if (g_h_predators[p].active_program_size > max_dna_b) max_dna_b = g_h_predators[p].active_program_size;
        }
      }

      bool has_new_record = false;
      if (max_gen_a > g_best_gen_record_a) { g_best_gen_record_a = max_gen_a; has_new_record = true; }
      if (max_gen_b > g_best_gen_record_b) { g_best_gen_record_b = max_gen_b; has_new_record = true; }
      if (g_artifacts[0].level > g_best_art_lvl_record_a) { g_best_art_lvl_record_a = g_artifacts[0].level; has_new_record = true; }
      if (g_artifacts[1].level > g_best_art_lvl_record_b) { g_best_art_lvl_record_b = g_artifacts[1].level; has_new_record = true; }
      if (g_climate.nature_adversarial_pressure > g_best_nature_pressure_record + 0.25f) {
        g_best_nature_pressure_record = g_climate.nature_adversarial_pressure;
        has_new_record = true;
      }

      if (has_new_record) {
        log_evolution_milestone(g_loop_run_count, max_gen_a, g_artifacts[0].level, max_gen_b, g_artifacts[1].level, g_climate.nature_adversarial_pressure, g_day_count);
        std::cout << ">>> [MILESTONE .LOG TERSIMPAN] loop #" << g_loop_run_count 
                  << " | Herbi: gen " << max_gen_a << ", art lvl " << g_artifacts[0].level 
                  << " | Karni: gen " << max_gen_b << ", art lvl " << g_artifacts[1].level 
                  << " | alam: " << std::fixed << std::setprecision(2) << g_climate.nature_adversarial_pressure << "x <<<\n";
      }

      auto get_art_status = [](const ArtifactEntity &art) -> std::string {
        std::string st = "[mencari]";
        if (art.status_state == 1) st = "[gagal/kabur]";
        else if (art.status_state == 2) st = "[terjawab]";
        else if (art.status_state == 3) st = "[active-immortal]";
        return "Art: Lvl " + std::to_string(art.level) + " " + st;
      };

      std::string art_a_str = get_art_status(g_artifacts[0]);
      std::string art_b_str = get_art_status(g_artifacts[1]);

      std::cout << "[(day " << g_day_count << ") | Herbi: pop " << pop_a 
                << " (Gen " << max_gen_a << ", DNA " << max_dna_a << " ops, Alat " << g_total_tools_crafted_a << ", " << art_a_str << ")"
                << " vs Karni: pop " << pop_b 
                << " (Gen " << max_gen_b << ", DNA " << max_dna_b << " ops, Alat " << g_total_tools_crafted_b << ", " << art_b_str << ")"
                << " | Alam: " << std::fixed << std::setprecision(2) << g_climate.nature_adversarial_pressure << "x"
                << " | Div: " << std::fixed << std::setprecision(1) << g_dna_diversity_index << "%"
                << " | EROI: " << std::fixed << std::setprecision(2) << g_eroi_score << "x ]" << std::endl;
      LeaveCriticalSection(&g_cs);
    }

    if (!DuniaFisika::HEADLESS_MODE) {
      Sleep(16);
    }
  }

  if (g_d_agents)
    cudaFree(g_d_agents);
  if (g_d_trees)
    cudaFree(g_d_trees);
  if (g_d_predators)
    cudaFree(g_d_predators);
  if (g_d_climate)
    cudaFree(g_d_climate);
  if (g_h_agents)
    free(g_h_agents);
  if (g_h_trees)
    free(g_h_trees);
  if (g_h_predators)
    free(g_h_predators);
  DeleteCriticalSection(&g_cs);
  DeleteCriticalSection(&g_json_cs);
  DeleteCriticalSection(&g_ws_cs);
  return 0;
}
