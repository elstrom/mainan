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
  float age_years;         // Usia dalam Tahun (0 - 100 Tahun)
  float hunger_rate_mult;  // Pengali rasa lapar (Meningkat seiring penuaan)
  float mating_cooldown;   // Waktu tunggu sebelum bisa reproduksi kembali
  int fruits_eaten;        // Jumlah buah yang berhasil dikonsumsi
  int formulas_discovered; // Jumlah rumus pertumbuhan yang berhasil dicetuskan
  int predators_slain;     // Jumlah predator yang berhasil dikalahkan agen
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
  float formula_shield;
  float comm_signal;       // Pesan koordinasi predator
  float comm_received;     // Pesan koordinasi kawan
  int prey_devoured;
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

CRITICAL_SECTION g_cs;
std::atomic<bool> g_server_running(true);
std::atomic<double> g_time_speed_multiplier(DuniaFisika::TIME_ACCELERATION_FACTOR);
std::atomic<double> g_growth_bias_ratio(DuniaFisika::DEFAULT_GROWTH_BIAS);

int g_total_predator_kills = 0; // Total predator yang mati terbunuh rumus agen
int g_predator_births = DuniaFisika::INITIAL_PREDATORS;
int g_predator_deaths = 0;

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
  float formula_shield;
  float energy;
};

std::vector<TopAgentRecord> g_living_leaderboard;
std::vector<TopPredatorRecord> g_predator_leaderboard;

std::string decompile_dna_formula(const DnaInstruction *prog, int prog_size) {
  const char *sensor_names[8] = {"TreeDX", "TreeDY", "MinDX", "MinDY",
                                 "Energy", "Temp",   "PredDist", "Age"};
  std::string reg_expr[REGISTERS_COUNT];
  for (int r = 0; r < REGISTERS_COUNT; ++r)
    reg_expr[r] = "0";

  int last_out_reg = -1;
  for (int ip = 0; ip < prog_size; ++ip) {
    const auto &inst = prog[ip];
    int rd = inst.r_dest % REGISTERS_COUNT;
    int rs1 = inst.r_src1 % REGISTERS_COUNT;
    int rs2 = inst.r_src2 % REGISTERS_COUNT;

    switch (inst.op % 15) {
    case 1: { // LOAD_SENSOR
      int s = inst.r_src1 % 8;
      reg_expr[rd] = sensor_names[s];
      break;
    }
    case 2:
      reg_expr[rd] = "(" + reg_expr[rs1] + " + " + reg_expr[rs2] + ")";
      break;
    case 3:
      reg_expr[rd] = "(" + reg_expr[rs1] + " - " + reg_expr[rs2] + ")";
      break;
    case 4:
      reg_expr[rd] = "(" + reg_expr[rs1] + " * " + reg_expr[rs2] + ")";
      break;
    case 6:
      reg_expr[rd] = "tanh(" + reg_expr[rs1] + ")";
      break;
    case 7:
      reg_expr[rd] = "σ(" + reg_expr[rs1] + ")";
      break;
    case 10:
      reg_expr[rd] = "move(" + reg_expr[rs1] + ", " + reg_expr[rs2] + ")";
      break;
    case 11:
      last_out_reg = rs1;
      break;
    case 12:
      reg_expr[rd] = "resonate(" + reg_expr[rs1] + "·Climate)";
      break;
    }
  }

  if (last_out_reg >= 0 && reg_expr[last_out_reg] != "0") {
    return "Growth = tanh(" + reg_expr[last_out_reg] + ")";
  }
  return "Growth = tanh(TreeDist·MinDX + FruitQty - Temp)";
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
    g_h_predators[p].age_years = (p < DuniaFisika::INITIAL_PREDATORS) ? (float)(rand() % 30) : 0.0f;
    g_h_predators[p].mating_cooldown = (float)(rand() % 5);
    g_h_predators[p].formula_shield = (float)((p * 37) % 100) / 100.0f;
    g_h_predators[p].prey_devoured = 0;
    g_h_predators[p].is_alive = (p < DuniaFisika::INITIAL_PREDATORS);
    g_h_predators[p].just_killed = false;
    g_h_predators[p].just_died = false;
    g_h_predators[p].just_born = false;
    g_h_predators[p].sin_type = 0;
    for (int r = 0; r < REGISTERS_COUNT; ++r) {
      g_h_predators[p].registers[r] = 0.0;
      g_h_predators[p].prev_registers[r] = 0.0;
      g_h_predators[p].integrated_registers[r] = 0.0;
    }

    if (has_saved_predator_dna && (p < DuniaFisika::INITIAL_PREDATORS)) {
      g_h_predators[p].active_program_size = saved_predator_ancestor.active_program_size;
      g_h_predators[p].active_registers_count = saved_predator_ancestor.active_registers_count;
      for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
        g_h_predators[p].dna_program[ip] = saved_predator_ancestor.dna_program[ip];
        // Sedikit variasi genetik generasi awal
        if (p > 0 && ((rand() % 100) < 20)) {
          g_h_predators[p].dna_program[ip].immediate_val += ((rand() % 40) - 20) * 0.05f;
        }
      }
    } else {
      g_h_predators[p].active_program_size = 8;
      g_h_predators[p].active_registers_count = ParameterAgent::MIN_DYNAMIC_REGISTERS;
      // Inisialisasi genetik insting murni acak (Pure Emergent ASI)
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
    float gx = 100.0f + (float)(t % 6) * 160.0f + (float)((rand() % 40) - 20);
    float gy =
        100.0f + ((float)t / 6.0f) * 160.0f + (float)((rand() % 40) - 20);
    g_h_trees[t].x = gx;
    g_h_trees[t].y = gy;
    g_h_trees[t].growth_stage = 50.0f + (float)(rand() % 50);
    g_h_trees[t].fruits_count = 3.0f + (float)(rand() % 4);
    g_h_trees[t].formula_resonance = 0.0f;
    g_h_trees[t].age_years = (float)(rand() % 5);
    g_h_trees[t].health = 80.0f + (float)(rand() % 20);
  }

  GpuEcosystemAgent saved_ancestor = {};
  bool has_saved_dna = load_best_dna_from_file(saved_ancestor);

  // Inisialisasi seluruh buffer agen (populasi awal hidup, sisanya siap untuk kelahiran baru)
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
    g_h_agents[i].age_years = (i < ParameterAgent::INITIAL_POPULATION) ? (float)(rand() % 30) : 0.0f;
    g_h_agents[i].hunger_rate_mult = 1.0f;
    g_h_agents[i].mating_cooldown = (float)(rand() % 5);
    g_h_agents[i].fruits_eaten = 0;
    g_h_agents[i].formulas_discovered = 0;
    g_h_agents[i].predators_slain = 0;
    g_h_agents[i].growth_signal = 0.0f;
    g_h_agents[i].fear_level = 0.0f;
    g_h_agents[i].is_alive = (i < ParameterAgent::INITIAL_POPULATION);
    g_h_agents[i].just_died = false;
    g_h_agents[i].just_born = false;

    for (int r = 0; r < REGISTERS_COUNT; ++r) {
      g_h_agents[i].registers[r] = 0.0;
      g_h_agents[i].prev_registers[r] = 0.0;
      g_h_agents[i].integrated_registers[r] = 0.0;
    }

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

      // Inisialisasi genetik organik dan acak murni (Pure Emergent ASI)
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

  // Loop Pembaruan Vitalitas & Buah Pohon
  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    g_h_trees[t].age_years += (float)(dt / ParameterAgent::SECONDS_PER_YEAR);

    // Pohon mati karena usia tua alami
    if (g_h_trees[t].age_years >= (float)DuniaFisika::TREE_MAX_AGE_YEARS || g_h_trees[t].health <= 0.0f) {
      g_total_tree_deaths++;
      g_total_tree_sprouts++;
      g_h_trees[t].x = 100.0f + (float)(t % 6) * 160.0f + (float)((rand() % 40) - 20);
      g_h_trees[t].y = 100.0f + ((float)t / 6.0f) * 160.0f + (float)((rand() % 40) - 20);
      g_h_trees[t].growth_stage = 0.0f;
      g_h_trees[t].fruits_count = 0.0f;
      g_h_trees[t].formula_resonance = 0.0f;
      g_h_trees[t].age_years = 0.0f;
      g_h_trees[t].health = 60.0f;
    }

    // Pertumbuhan Fotosintesis Pohon Murni Berbasis Sinar Matahari Alami
    float sunlight_boost = 0.5f + (float)DuniaFisika::DAY_PHOTOSYNTHESIS_MULT * g_climate.daylight_factor;
    g_h_trees[t].growth_stage = fminf(100.0f, g_h_trees[t].growth_stage + 0.5f * sunlight_boost * (float)dt);
    g_h_trees[t].health = fminf(100.0f, g_h_trees[t].health + 5.0f * (float)dt);
    
    if (g_h_trees[t].growth_stage >= 40.0f && g_h_trees[t].fruits_count < (float)DuniaFisika::MAX_FRUIT_PER_TREE) {
      g_h_trees[t].fruits_count = fminf((float)DuniaFisika::MAX_FRUIT_PER_TREE, 
                                        g_h_trees[t].fruits_count + (float)DuniaFisika::FRUIT_SPAWN_RATE * sunlight_boost * (float)dt);
    }
  }

  // Dinamika Atmosfer: Fotosintesis Pohon menghasilkan O2, Respirasi Pohon & Agen menghasilkan CO2
  int living_trees_count = 0;
  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    if (g_h_trees[t].growth_stage >= 20.0f && g_h_trees[t].health > 0.0f) {
      living_trees_count++;
    }
  }
  int living_pop_total = (int)(g_living_leaderboard.size() + g_predator_leaderboard.size());
  
  // Fotosintesis di siang hari memproduksi O2, respirasi malam pohon memproduksi CO2
  float o2_produced = (float)DuniaFisika::PHOTOSYNTHESIS_O2_RATE * (float)living_trees_count * g_climate.daylight_factor * (float)dt;
  float tree_co2_emission = (float)DuniaFisika::TREE_RESPIRATION_CO2_RATE * (float)living_trees_count * (1.0f - g_climate.daylight_factor) * (float)dt;
  
  // Respirasi seluruh populasi hidup
  float o2_consumed = (float)DuniaFisika::RESPIRATION_O2_CONSUMPTION * (float)living_pop_total * (float)dt;
  float agent_co2_emission = (float)DuniaFisika::RESPIRATION_CO2_EMISSION * (float)living_pop_total * (float)dt;

  g_climate.oxygen_level = fminf(30.0f, fmaxf(10.0f, g_climate.oxygen_level + (o2_produced - o2_consumed)));
  g_climate.co2_level = fminf(3.0f, fmaxf(0.01f, g_climate.co2_level + (tree_co2_emission + agent_co2_emission - o2_produced * 0.5f)));

  // Kelembaban Uap Air (H2O) bertranspirasi dari pohon dan suhu
  float transpiration = (float)living_trees_count * 0.02f * g_climate.daylight_factor * (float)dt;
  g_climate.h2o_level = fminf(95.0f, fmaxf(20.0f, g_climate.h2o_level + transpiration - 0.01f * (float)dt));

  // Efek Rumah Kaca: CO2 & H2O menahan radiasi panas matahari
  float greenhouse_warming = (g_climate.co2_level - (float)DuniaFisika::CO2_BASE_LEVEL) * (float)DuniaFisika::GREENHOUSE_CO2_WARMING_FACTOR +
                             (g_climate.h2o_level - (float)DuniaFisika::H2O_HUMIDITY_BASE_LEVEL) * (float)DuniaFisika::GREENHOUSE_H2O_WARMING_FACTOR;

  // Pengaruh Siang & Malam + Efek Rumah Kaca Terhadap Suhu Lingkungan
  float base_season_temps[4] = {20.0f, 32.0f, 16.0f, 2.0f};
  float day_night_temp_delta = (g_climate.daylight_factor - 0.5f) * 2.0f * (float)DuniaFisika::DAYLIGHT_TEMP_BOOST;
  g_climate.temperature = base_season_temps[g_climate.current_season] + day_night_temp_delta + greenhouse_warming + sinf(time_f * 0.2f) * 1.5f;

  // Regenerasi & Pelapukan Formasi Material Periodik
  for (int m = 0; m < DuniaFisika::MAX_PERIODIC_DEPOSITS; ++m) {
    if (g_h_minerals[m].mass < 100.0f) {
      g_h_minerals[m].mass = fminf(100.0f, g_h_minerals[m].mass + (float)DuniaFisika::MATERIAL_REGEN_RATE * (float)dt);
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

  // Monitoring Kelahiran & Kematian Faksi B (Predator)
  std::vector<TopPredatorRecord> current_living_predators;
  for (int p = 0; p < DuniaFisika::MAX_PREDATORS_BUFFER; ++p) {
    if (g_h_predators[p].just_killed) {
      g_total_predator_kills++;
    }
    if (g_h_predators[p].just_died) {
      g_predator_deaths++;
    }
    if (g_h_predators[p].just_born) {
      g_predator_births++;
    }
    if (g_h_predators[p].is_alive && g_h_predators[p].energy > 0.0f) {
      TopPredatorRecord rec;
      rec.id = g_h_predators[p].id;
      rec.generation = g_h_predators[p].generation;
      rec.gender = g_h_predators[p].gender;
      rec.sin_type = g_h_predators[p].sin_type;
      rec.age_years = g_h_predators[p].age_years;
      rec.prey_devoured = g_h_predators[p].prey_devoured;
      rec.formula_shield = g_h_predators[p].formula_shield;
      rec.energy = g_h_predators[p].energy;
      current_living_predators.push_back(rec);
    }
  }

  // Urutkan Papan Peringkat Predator berdasarkan mangsa dibunuh dan usia
  std::sort(current_living_predators.begin(), current_living_predators.end(),
            [](const TopPredatorRecord &a, const TopPredatorRecord &b) {
              int score_a = (a.prey_devoured * 100) + (int)a.age_years;
              int score_b = (b.prey_devoured * 100) + (int)b.age_years;
              return score_a > score_b;
            });

  // Monitoring Kelahiran & Kematian Realtime + Dynamic Living Leaderboard Faksi A (Herbivora)
  int total_fruits = 0;
  int total_formulas = 0;
  std::vector<TopAgentRecord> current_living;

  for (int i = 0; i < ParameterAgent::MAX_POPULATION_BUFFER; ++i) {
    if (g_h_agents[i].just_died)
      g_total_deaths++;
    if (g_h_agents[i].just_born)
      g_total_births++;

    total_fruits += g_h_agents[i].fruits_eaten;
    total_formulas += g_h_agents[i].formulas_discovered;

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
      rec.energy = g_h_agents[i].energy;
      rec.formula = decompile_dna_formula(g_h_agents[i].dna_program,
                                          g_h_agents[i].active_program_size);
      current_living.push_back(rec);
    }
  }

  // Urutkan Papan Peringkat berdasarkan rumus dicetuskan, predator dibunuh, panen, dan usia bertahan
  std::sort(current_living.begin(), current_living.end(),
            [](const TopAgentRecord &a, const TopAgentRecord &b) {
              int score_a = (a.predators_slain * 200) + (a.formulas_discovered * 50) +
                            (a.fruits_eaten * 20) + (int)a.age_years;
              int score_b = (b.predators_slain * 200) + (b.formulas_discovered * 50) +
                            (b.fruits_eaten * 20) + (int)b.age_years;
              return score_a > score_b;
            });

  EnterCriticalSection(&g_cs);
  g_living_leaderboard = std::move(current_living);
  g_predator_leaderboard = std::move(current_living_predators);
  g_total_fruits_harvested = total_fruits;
  g_total_formulas_synthesized = total_formulas;
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
  ss << "  \"totalFruitsHarvested\": " << g_total_fruits_harvested << ",\n";
  ss << "  \"totalFruitsAvailable\": " << total_available_fruits << ",\n";
  ss << "  \"totalFormulasSynthesized\": " << g_total_formulas_synthesized
     << ",\n";
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
  } else {
    ss << "  \"alphaId\": 1,\n  \"alphaGen\": 1,\n  \"alphaGender\": 0,\n  \"alphaSin\": 0,\n  "
          "\"alphaAge\": 0,\n  \"alphaEnergy\": 100,\n  \"alphaFruits\": 0,\n  "
          "\"alphaFormulas\": 0,\n";
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

  // Trees Coordinate & Fruits Stream
  ss << "  \"livingTreesCount\": " << living_trees_count << ",\n";
  ss << "  \"totalTreeDeaths\": " << g_total_tree_deaths << ",\n";
  ss << "  \"totalTreeSprouts\": " << g_total_tree_sprouts << ",\n";
  ss << "  \"trees\": [";
  for (int t = 0; t < ParameterAgent::MAX_TREES; ++t) {
    ss << "{\"x\":" << g_h_trees[t].x << ",\"y\":" << g_h_trees[t].y
       << ",\"g\":" << g_h_trees[t].growth_stage
       << ",\"h\":" << g_h_trees[t].health
       << ",\"f\":" << g_h_trees[t].fruits_count << "}"
       << (t < ParameterAgent::MAX_TREES - 1 ? "," : "");
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
       << ",\"d\":" << pr.prey_devoured << ",\"s\":" << pr.formula_shield
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
  srand(static_cast<unsigned int>(time(nullptr)));
  InitializeCriticalSection(&g_cs);
  InitializeCriticalSection(&g_json_cs);
  InitializeCriticalSection(&g_ws_cs);

  init_ecosystem_pipeline();
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
  auto last_time = std::chrono::steady_clock::now();

  while (g_server_running) {
    auto current_time = std::chrono::steady_clock::now();
    double frame_dt = std::chrono::duration<double>(current_time - last_time).count();
    last_time = current_time;

    // Percepatan dinamis waktu nyata (dengan multi-substepping agar integrasi fisika stabil)
    double speed_mult = DuniaFisika::HEADLESS_MODE ? 500.0 : g_time_speed_multiplier.load();
    double target_sim_dt = frame_dt * speed_mult;
    double max_substep = 0.05; // 50ms per substep untuk stabilitas numerik CUDA
    int substeps = std::max(1, std::min(100, (int)std::ceil(target_sim_dt / max_substep)));
    double substep_dt = target_sim_dt / substeps;

    for (int s = 0; s < substeps; ++s) {
      step_ecosystem(substep_dt);
    }

    // Evaluasi Kepunahan Otomatis (Stop & Report)
    if (DuniaFisika::STOP_ON_EXTINCTION && g_day_count >= 2) {
      int living_a = (int)g_living_leaderboard.size();
      int living_b = (int)g_predator_leaderboard.size();
      if (living_a == 0 || living_b == 0) {
        std::cout << "\n================================================================" << std::endl;
        std::cout << " [SIMULASI BERHENTI: SALAH SATU FAKSI PUNAH]" << std::endl;
        std::cout << "================================================================" << std::endl;
        std::cout << "Hari Terakhir : Day " << g_day_count << std::endl;
        std::cout << "Pemenang      : " << (living_a > 0 ? "KUBU A (HERBIVORA)" : "KUBU B (PREDATOR)") << std::endl;
        std::cout << "Populasi A    : " << living_a << " Semut Hidup (Total Lahir: " << g_total_births << ", Mati: " << g_total_deaths << ")" << std::endl;
        std::cout << "Populasi B    : " << living_b << " Predator Hidup (Total Lahir: " << g_predator_births << ", Mati: " << g_predator_deaths << ")" << std::endl;
        std::cout << "Total Panen   : " << g_total_fruits_harvested << " Buah" << std::endl;
        std::cout << "Mangsa Dimangsa: " << g_total_predator_kills << " Semut" << std::endl;
        std::cout << "Formula Terbaik : " << g_best_ecosystem_formula << std::endl;
        std::cout << "================================================================\n" << std::endl;
        break;
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
      const auto &alpha = g_h_agents[g_alpha_agent_idx];
      const char *s_names[] = {"Semi", "Panas", "Gugur", "Dingin"};
      std::cout << "[Day " << g_day_count << " | "
                << s_names[g_climate.current_season % 4]
                << "] Temp: " << std::fixed << std::setprecision(1)
                << g_climate.temperature << " C"
                << " | Living A: " << g_living_leaderboard.size()
                << " | Living B: " << g_predator_leaderboard.size()
                << " | Harvested: " << g_total_fruits_harvested << " fruits"
                << " | Formulas: " << g_total_formulas_synthesized
                << " | Alpha Age: " << std::setprecision(1) << alpha.age_years
                << " yrs"
                << " | Formula: " << g_best_ecosystem_formula << std::endl;
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
