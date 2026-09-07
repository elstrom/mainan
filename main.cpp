#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include "mind_math.h"
#include "uji/ground_truth_moon.h"

int g_window_width = 1200;
int g_window_height = 760;
const int UI_HEIGHT = 165;

float g_zoom = 1.0f;
float g_pan_x = 0.0f;
float g_pan_y = 0.0f;
bool g_is_dragging = false;
int g_drag_start_x = 0;
int g_drag_start_y = 0;
float g_pan_start_x = 0.0f;
float g_pan_start_y = 0.0f;

#include <cuda_runtime.h>
#include "agent/ingatan/parameter_agent.h"

#define DNA_PROGRAM_SIZE ParameterAgent::MAX_DNA_CAPACITY
#define REGISTERS_COUNT ParameterAgent::MAX_REGISTERS
#define TRAJECTORY_SAMPLES ParameterAgent::TRAJECTORY_SAMPLES

struct DnaInstruction {
    unsigned char op;
    unsigned char r_dest;
    unsigned char r_src1;
    unsigned char r_src2;
    float immediate_val;
};

struct GpuRocketAgent {
    double altitude;
    double velocity;
    double peak_velocity;
    double fuel_mass;
    double end_fuel_mass;
    double distance_to_moon;
    bool landed_safely;
    bool crashed;
    bool is_launched;
    int thought_ticks;
    double best_altitude;
    double fitness;
    double fear_level;
    double crash_trauma;
    
    // Neurogenesis & Dynamic Elastic Working Memory (Meniru Kognisi Otak Manusia)
    int active_program_size;
    int active_registers_count;

    // Flight Path Samples (Data Trayektori Nyata Hasil Evolusi GPU)
    float traj_altitudes[TRAJECTORY_SAMPLES];
    float traj_velocities[TRAJECTORY_SAMPLES];
    float traj_fuels[TRAJECTORY_SAMPLES];
    float traj_thrusts[TRAJECTORY_SAMPLES];
    int traj_points_count;

    DnaInstruction dna_program[DNA_PROGRAM_SIZE];
    float epigenetic_methylation[DNA_PROGRAM_SIZE]; // Epigenetic Memory: Tabu biologis internal di tingkat gen
    float eligibility_trace[DNA_PROGRAM_SIZE];     // Temporal Credit Assignment: Jejak kausalitas waktu eksekusi gen
    double registers[REGISTERS_COUNT];
    double prev_registers[REGISTERS_COUNT];
    double integrated_registers[REGISTERS_COUNT];
};

extern "C" void launch_cuda_simulation(GpuRocketAgent* d_agents, int population_size, int max_steps, double dt);

// Dynamic Population Controller with Safe VRAM GPU Capping
int g_current_population_size = ParameterAgent::DEFAULT_POPULATION_SIZE;
std::vector<AutonomousFlightMind> g_population(ParameterAgent::MAX_POPULATION_SIZE);
std::vector<GroundTruthMoonPhysicsEngine> g_gt_engines(ParameterAgent::MAX_POPULATION_SIZE);

GpuRocketAgent* g_h_gpu_agents = nullptr;
GpuRocketAgent* g_d_gpu_agents = nullptr;

AutonomousFlightMind g_display_mind;
GroundTruthMoonPhysicsEngine g_display_gt;

std::string g_best_landing_formula = "Belum Tercapai (Sedang Mencari...)";
double g_global_record_dist = 0.0;
int g_total_global_flights = 0;
int g_total_successful_landings = 0;
int g_total_dna_rewrites = 0;
int g_best_agent_idx = 0;
int g_generation = 1;
double g_avg_thought_ticks = 0.0;
#include <unordered_set>

CRITICAL_SECTION g_cs;
std::atomic<bool> g_server_running(true);

std::unordered_set<std::string> g_blacklisted_failed_dna;

std::string decompile_dna_formula(const DnaInstruction* prog) {
    std::string reg_expr[REGISTERS_COUNT];
    const char* sensor_names[6] = {"h", "v", "d_moon", "t_life", "fear", "c"};
    for (int r = 0; r < REGISTERS_COUNT; ++r) {
        reg_expr[r] = "0";
    }

    int last_thrust_reg = -1;
    for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
        const auto& inst = prog[ip];
        int rd = inst.r_dest % REGISTERS_COUNT;
        int rs1 = inst.r_src1 % REGISTERS_COUNT;
        int rs2 = inst.r_src2 % REGISTERS_COUNT;
        unsigned char op = inst.op % 15;

        std::string s1 = (reg_expr[rs1].length() > 32) ? ("R" + std::to_string(rs1)) : reg_expr[rs1];
        std::string s2 = (reg_expr[rs2].length() > 32) ? ("R" + std::to_string(rs2)) : reg_expr[rs2];

        switch (op) {
            case 0: break; // NOP
            case 1: { // LOAD_SENSOR
                int s = rs1 % 6;
                if (s < 5) reg_expr[rd] = sensor_names[s];
                else {
                    std::stringstream imm_ss;
                    imm_ss << std::fixed << std::setprecision(1) << inst.immediate_val;
                    reg_expr[rd] = imm_ss.str();
                }
                break;
            }
            case 2: reg_expr[rd] = "(" + s1 + " + " + s2 + ")"; break;
            case 3: reg_expr[rd] = "(" + s1 + " - " + s2 + ")"; break;
            case 4: reg_expr[rd] = "(" + s1 + " * " + s2 + ")"; break;
            case 5: reg_expr[rd] = "(" + s1 + " / (" + s2 + " + ε))"; break;
            case 6: reg_expr[rd] = "tanh(" + s1 + ")"; break;
            case 7: reg_expr[rd] = "σ(" + s1 + ")"; break;
            case 8: reg_expr[rd] = "∫(" + s1 + "·dt)"; break;
            case 9: reg_expr[rd] = "d/dt(" + s1 + ")"; break;
            case 10: reg_expr[rd] = "adapt(" + s1 + ")"; break;
            case 11: last_thrust_reg = rs1; break;
            case 12: reg_expr[rd] = "predict_sim(" + s1 + ")"; break;
            case 13: reg_expr[rd] = "neurogenesis(" + s1 + " + " + s2 + ")"; break;
            case 14: reg_expr[rd] = "prune(" + s1 + ")"; break;
        }
    }

    if (last_thrust_reg >= 0 && reg_expr[last_thrust_reg] != "0") {
        return "Thrust = 0.5 + 0.5·tanh(" + reg_expr[last_thrust_reg] + ")";
    }
    return "Thrust = 0.5 + 0.5·tanh(v - fear + ∫(h·dt))";
}

void save_best_dna_binary(const GpuRocketAgent& alpha) {
    std::ofstream out(ParameterAgent::DNA_STORAGE_FILE, std::ios::binary);
    if (!out.is_open()) return;
    out.write(reinterpret_cast<const char*>(&alpha.best_altitude), sizeof(double));
    out.write(reinterpret_cast<const char*>(&alpha.active_program_size), sizeof(int));
    out.write(reinterpret_cast<const char*>(&alpha.active_registers_count), sizeof(int));
    out.write(reinterpret_cast<const char*>(alpha.dna_program), sizeof(DnaInstruction) * DNA_PROGRAM_SIZE);
    out.write(reinterpret_cast<const char*>(alpha.epigenetic_methylation), sizeof(float) * DNA_PROGRAM_SIZE);
    out.flush();
    out.close();
}

bool load_best_dna_binary(GpuRocketAgent& alpha) {
    std::ifstream in(ParameterAgent::DNA_STORAGE_FILE, std::ios::binary);
    if (!in.is_open()) return false;
    in.read(reinterpret_cast<char*>(&alpha.best_altitude), sizeof(double));
    in.read(reinterpret_cast<char*>(&alpha.active_program_size), sizeof(int));
    in.read(reinterpret_cast<char*>(&alpha.active_registers_count), sizeof(int));
    in.read(reinterpret_cast<char*>(alpha.dna_program), sizeof(DnaInstruction) * DNA_PROGRAM_SIZE);
    in.read(reinterpret_cast<char*>(alpha.epigenetic_methylation), sizeof(float) * DNA_PROGRAM_SIZE);
    in.close();
    return true;
}

void save_failed_dna_registry() {
    std::ofstream out(ParameterAgent::FAILED_REGISTRY_FILE);
    if (!out.is_open()) return;
    out << "{\n  \"blacklisted_formulas\": [\n";
    size_t count = 0;
    for (const auto& f : g_blacklisted_failed_dna) {
        std::string clean_f = "";
        for (char c : f) {
            if (c == '"') clean_f += "\\\"";
            else clean_f += c;
        }
        out << "    \"" << clean_f << "\"" << (++count < g_blacklisted_failed_dna.size() ? "," : "") << "\n";
    }
    out << "  ]\n}\n";
    out.flush();
    out.close();
}

void load_failed_dna_registry() {
    std::ifstream in(ParameterAgent::FAILED_REGISTRY_FILE);
    if (!in.is_open()) {
        save_failed_dna_registry(); // Buat file catatan langsung jika belum ada
        return;
    }
    std::string line;
    while (std::getline(in, line) && g_blacklisted_failed_dna.size() < 100) {
        size_t start = line.find("\"");
        if (start != std::string::npos) {
            size_t end = line.rfind("\"");
            if (end > start) {
                g_blacklisted_failed_dna.insert(line.substr(start + 1, end - start - 1));
            }
        }
    }
}

std::string generate_telemetry_json() {
    EnterCriticalSection(&g_cs);
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "{\n";
    ss << "  \"generation\": " << g_generation << ",\n";
    ss << "  \"populationSize\": " << g_current_population_size << ",\n";
    ss << "  \"totalFlights\": " << g_total_global_flights << ",\n";
    ss << "  \"successCount\": " << g_total_successful_landings << ",\n";
    ss << "  \"crashedCount\": " << std::max(0, g_total_global_flights - g_total_successful_landings) << ",\n";
    ss << "  \"rewritesCount\": " << g_total_dna_rewrites << ",\n";
    ss << "  \"bestAltitude\": " << (g_global_record_dist / 1000.0) << ",\n";

    if (g_h_gpu_agents) {
        const auto& alpha = g_h_gpu_agents[g_best_agent_idx];
        double display_vel = (fabs(alpha.velocity) > 0.1) ? alpha.velocity : alpha.peak_velocity;
        ss << "  \"currentBatchAltitude\": " << (alpha.best_altitude / 1000.0) << ",\n";
        ss << "  \"currentDistMoon\": " << (alpha.distance_to_moon / 1000.0) << ",\n";
        ss << "  \"velocity\": " << std::setprecision(2) << (display_vel / 1000.0) << ",\n";
        ss << "  \"fuel\": " << std::setprecision(1) << ((alpha.end_fuel_mass / 120000.0) * 100.0) << ",\n";
        ss << "  \"fearLevel\": " << std::setprecision(1) << (alpha.fear_level * 100.0) << ",\n";
        ss << "  \"crashTrauma\": " << std::setprecision(1) << (alpha.crash_trauma * 100.0) << ",\n";
        ss << "  \"thoughtTicks\": " << alpha.thought_ticks << ",\n";
        ss << "  \"avgThoughtTicks\": " << std::setprecision(0) << g_avg_thought_ticks << ",\n";
        ss << "  \"activeProgramSize\": " << alpha.active_program_size << ",\n";
        ss << "  \"activeRegisters\": " << alpha.active_registers_count << ",\n";
        ss << "  \"isLaunched\": " << (alpha.is_launched ? "true" : "false") << ",\n";
        ss << "  \"formula\": \"" << g_best_landing_formula << "\",\n";
        ss << "  \"registers\": [";
        for (int r = 0; r < REGISTERS_COUNT; ++r) {
            ss << std::setprecision(2) << alpha.registers[r] << (r < REGISTERS_COUNT - 1 ? ", " : "");
        }
        ss << "],\n";
        ss << "  \"dna\": [\n";
        const char* op_names[] = {
            "NOP", "LOAD_SENSOR", "ADD", "SUB", "MUL", "DIV",
            "TANH", "SIGMOID", "INTEGRAL", "DERIVATIVE", "MUTATE_SELF", "OUTPUT_THRUST",
            "PREDICT_PHYSICS", "FORK_NEURON", "PRUNE_NEURON"
        };
        int send_prog = std::min(16, std::max(ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE, alpha.active_program_size));
        for (int ip = 0; ip < send_prog; ++ip) {
            const auto& inst = alpha.dna_program[ip];
            ss << "    {\"op\": \"" << op_names[inst.op % 15] << "\", \"arg\": " << std::setprecision(2) << inst.immediate_val << "}" << (ip < send_prog - 1 ? "," : "") << "\n";
        }
        ss << "  ],\n";
        ss << "  \"trajectory\": [";
        int pts = std::min(TRAJECTORY_SAMPLES, std::max(0, alpha.traj_points_count));
        for (int p = 0; p < pts; ++p) {
            ss << "{\"alt\": " << std::setprecision(1) << (alpha.traj_altitudes[p] / 1000.0) 
               << ", \"vel\": " << std::setprecision(2) << (alpha.traj_velocities[p] / 1000.0)
               << ", \"fuel\": " << std::setprecision(1) << ((alpha.traj_fuels[p] / 120000.0) * 100.0)
               << ", \"thrust\": " << std::setprecision(2) << alpha.traj_thrusts[p] << "}" << (p < pts - 1 ? ", " : "");
        }
        ss << "]\n";
    } else {
        ss << "  \"currentDistMoon\": 384400.0,\n  \"velocity\": 0.0,\n  \"fuel\": 100.0,\n  \"registers\": [0,0,0,0,0,0,0,0],\n  \"dna\": [],\n  \"trajectory\": []\n";
    }
    ss << "}";
    LeaveCriticalSection(&g_cs);
    return ss.str();
}

void http_server_thread() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return;

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) { WSACleanup(); return; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8088);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        closesocket(server_fd);
        WSACleanup();
        return;
    }

    listen(server_fd, 64);

    while (g_server_running) {
        SOCKET client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == INVALID_SOCKET) continue;

        char buffer[1024] = {0};
        int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            std::string req(buffer);
            if (req.find("GET /telemetry") != std::string::npos) {
                std::string json = generate_telemetry_json();
                std::string resp = "HTTP/1.1 200 OK\r\n"
                                   "Content-Type: application/json\r\n"
                                   "Access-Control-Allow-Origin: *\r\n"
                                   "Connection: close\r\n"
                                   "Content-Length: " + std::to_string(json.length()) + "\r\n\r\n" + json;
                send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
            } else {
                std::ifstream file("d:/MyProjects/mainan/web/index.html");
                if (file) {
                    std::stringstream file_ss;
                    file_ss << file.rdbuf();
                    std::string html = file_ss.str();
                    std::string resp = "HTTP/1.1 200 OK\r\n"
                                       "Content-Type: text/html; charset=utf-8\r\n"
                                       "Connection: close\r\n"
                                       "Content-Length: " + std::to_string(html.length()) + "\r\n\r\n" + html;
                    send(client_fd, resp.c_str(), static_cast<int>(resp.length()), 0);
                }
            }
        }
        shutdown(client_fd, SD_BOTH);
        closesocket(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
}

void init_gpu_pipeline() {
    // Alokasikan buffer maksimum di GPU VRAM agar tidak pernah crash/leak saat populasi berosilasi
    g_h_gpu_agents = (GpuRocketAgent*)malloc(sizeof(GpuRocketAgent) * ParameterAgent::MAX_POPULATION_SIZE);
    cudaMalloc(&g_d_gpu_agents, sizeof(GpuRocketAgent) * ParameterAgent::MAX_POPULATION_SIZE);

    GpuRocketAgent saved_alpha;
    bool has_saved_dna = load_best_dna_binary(saved_alpha);
    if (has_saved_dna) {
        g_global_record_dist = saved_alpha.best_altitude;
        g_best_landing_formula = decompile_dna_formula(saved_alpha.dna_program);
    }

    for (int i = 0; i < ParameterAgent::MAX_POPULATION_SIZE; ++i) {
        g_h_gpu_agents[i].best_altitude = 0.0;
        g_h_gpu_agents[i].crash_trauma = 0.0;

        if (has_saved_dna && i < ParameterAgent::MAX_POPULATION_SIZE / 2) {
            // 50% populasi mewarisi DNA terbaik & epigenetik dari sesi sebelumnya
            g_h_gpu_agents[i].active_program_size = saved_alpha.active_program_size;
            g_h_gpu_agents[i].active_registers_count = saved_alpha.active_registers_count;
            for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                g_h_gpu_agents[i].dna_program[ip] = saved_alpha.dna_program[ip];
                g_h_gpu_agents[i].epigenetic_methylation[ip] = saved_alpha.epigenetic_methylation[ip];
            }
        } else {
            g_h_gpu_agents[i].active_program_size = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE + (i % 8);
            g_h_gpu_agents[i].active_registers_count = ParameterAgent::MIN_DYNAMIC_REGISTERS;

            for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                g_h_gpu_agents[i].epigenetic_methylation[ip] = 0.0f;
                g_h_gpu_agents[i].dna_program[ip].op = static_cast<unsigned char>(rand() % 15);
                g_h_gpu_agents[i].dna_program[ip].r_dest = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                g_h_gpu_agents[i].dna_program[ip].r_src1 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                g_h_gpu_agents[i].dna_program[ip].r_src2 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                g_h_gpu_agents[i].dna_program[ip].immediate_val = ((rand() % 200) - 100) * 0.05f;
            }

            // Struktur Fondasi Fungsional Dinamis (Sensor Feed -> Thrust Link)
            g_h_gpu_agents[i].dna_program[0] = {1, 0, 0, 0, 0.0f}; // Load sensor h -> R0
            g_h_gpu_agents[i].dna_program[1] = {1, 1, 1, 0, 0.0f}; // Load sensor v -> R1
            g_h_gpu_agents[i].dna_program[2] = {1, 2, 2, 0, 0.0f}; // Load sensor d -> R2
            g_h_gpu_agents[i].dna_program[3] = {1, 3, 4, 0, 0.0f}; // Load sensor fear -> R3
            g_h_gpu_agents[i].dna_program[DNA_PROGRAM_SIZE - 1] = {11, 0, static_cast<unsigned char>(rand() % 4), 0, 0.0f}; // Output Thrust
        }
    }
    cudaMemcpy(g_d_gpu_agents, g_h_gpu_agents, sizeof(GpuRocketAgent) * g_current_population_size, cudaMemcpyHostToDevice);
}

void run_mass_parallel_simulation() {
    int cur_pop = g_current_population_size;

    // 100% PARALEL GPU VM EXECUTION (DYNAMIC SELF-MODIFYING DNA PROGRAMS)
    launch_cuda_simulation(g_d_gpu_agents, cur_pop, 2000, 50.0);
    cudaMemcpy(g_h_gpu_agents, g_d_gpu_agents, sizeof(GpuRocketAgent) * cur_pop, cudaMemcpyDeviceToHost);
    
    // Update best agent and record
    int best_idx = -1;
    double current_batch_max_fitness = -1e9;

    double total_ticks = 0.0;
    int crash_count_batch = 0;

    for (int i = 0; i < cur_pop; ++i) {
        total_ticks += g_h_gpu_agents[i].thought_ticks;
        if (g_h_gpu_agents[i].crashed) crash_count_batch++;
        if (g_h_gpu_agents[i].fitness > current_batch_max_fitness) {
            current_batch_max_fitness = g_h_gpu_agents[i].fitness;
            best_idx = i;
        }
    }

    // Dynamic Population Controller:
    // Jika crash tinggi (stagnasi/bahaya tinggi), perbesar populasi untuk eksplorasi lebih luas
    // Jika performa stabil dan mendekati target, perkecil populasi untuk kecepatan dan hemat energi
    if (crash_count_batch > cur_pop * 0.7 && cur_pop < ParameterAgent::MAX_POPULATION_SIZE) {
        g_current_population_size = std::min(ParameterAgent::MAX_POPULATION_SIZE, cur_pop + 64);
    } else if (crash_count_batch < cur_pop * 0.3 && cur_pop > ParameterAgent::MIN_POPULATION_SIZE) {
        g_current_population_size = std::max(ParameterAgent::MIN_POPULATION_SIZE, cur_pop - 64);
    }

    EnterCriticalSection(&g_cs);
    g_avg_thought_ticks = total_ticks / cur_pop;
    g_total_global_flights += cur_pop;
    if (best_idx >= 0) {
        g_best_agent_idx = best_idx;
        if (g_h_gpu_agents[best_idx].landed_safely) {
            g_total_successful_landings++;
            g_h_gpu_agents[best_idx].crash_trauma = 0.0;
        } else if (g_h_gpu_agents[best_idx].crashed) {
            // Trauma meningkat saat crash, namun memiliki peluruhan alami per generasi (decay 10%)
            double prev_trauma = g_h_gpu_agents[best_idx].crash_trauma * 0.90;
            g_h_gpu_agents[best_idx].crash_trauma = std::min(1.0, prev_trauma + 0.08);
        } else {
            // Jika berhasil mengorbit/melayang tanpa crash, trauma luruh cepat
            g_h_gpu_agents[best_idx].crash_trauma *= 0.80;
        }

        if (g_h_gpu_agents[best_idx].best_altitude > g_global_record_dist) {
            g_global_record_dist = g_h_gpu_agents[best_idx].best_altitude;
            g_total_dna_rewrites++;
            g_display_mind.on_flight_completed(g_global_record_dist);
            save_best_dna_binary(g_h_gpu_agents[best_idx]);
        } else {
            // Dynamic Epigenetic Demethylation: Jika tidak ada rekor baru, kurangi methylation agar DNA plastis kembali
            for (auto& syn : g_display_mind.synapses) {
                syn.epigenetic_methylation = std::max(0.0, syn.epigenetic_methylation - 0.01);
            }
        }

        g_best_landing_formula = decompile_dna_formula(g_h_gpu_agents[best_idx].dna_program);
    }
    LeaveCriticalSection(&g_cs);

    // Multi-Niche & Novelty-Driven Island Evolution (Anti Local Optima)
    if (best_idx >= 0) {
        double inherited_trauma = g_h_gpu_agents[best_idx].crash_trauma;

        // 1. Elites Group (Preservasi performa terbaik)
        int elite_count = cur_pop / 20; // 5%
        for (int i = 0; i < elite_count; ++i) {
            g_h_gpu_agents[i].crash_trauma = inherited_trauma;
            g_h_gpu_agents[i].active_program_size = g_h_gpu_agents[best_idx].active_program_size;
            g_h_gpu_agents[i].active_registers_count = g_h_gpu_agents[best_idx].active_registers_count;
            for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                g_h_gpu_agents[i].dna_program[ip] = g_h_gpu_agents[best_idx].dna_program[ip];
                g_h_gpu_agents[i].epigenetic_methylation[ip] = g_h_gpu_agents[best_idx].epigenetic_methylation[ip];
            }
        }

        // 2. Adaptive Mutational Exploration Group (Island 1: Focused Refinement dengan Perlindungan Epigenetik)
        int refined_end = cur_pop * 6 / 10; // 60%
        for (int i = elite_count; i < refined_end; ++i) {
            g_h_gpu_agents[i].crash_trauma = inherited_trauma;
            g_h_gpu_agents[i].active_program_size = g_h_gpu_agents[best_idx].active_program_size;
            g_h_gpu_agents[i].active_registers_count = g_h_gpu_agents[best_idx].active_registers_count;
            for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                g_h_gpu_agents[i].dna_program[ip] = g_h_gpu_agents[best_idx].dna_program[ip];
                g_h_gpu_agents[i].epigenetic_methylation[ip] = g_h_gpu_agents[best_idx].epigenetic_methylation[ip];
                
                // Epigenetic Protection: Gen yang bermetilasi tinggi terkunci dari mutasi acak
                if (g_h_gpu_agents[i].epigenetic_methylation[ip] > 0.6f) continue;

                bool is_critical_block = (ip < 4 || ip >= 60);
                int mutation_rate = is_critical_block ? 1 : (int)(8 + inherited_trauma * 15.0);

                if ((rand() % 100) < mutation_rate) {
                    int mut_type = rand() % 4;
                    if (mut_type == 0) g_h_gpu_agents[i].dna_program[ip].op = static_cast<unsigned char>(rand() % 15);
                    else if (mut_type == 1) g_h_gpu_agents[i].dna_program[ip].r_dest = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                    else if (mut_type == 2) g_h_gpu_agents[i].dna_program[ip].r_src1 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                    else {
                        float new_imm = g_h_gpu_agents[i].dna_program[ip].immediate_val + ((rand() % 100) - 50) * 0.05f;
                        if (new_imm < -10.0f) new_imm = -10.0f;
                        if (new_imm > 10.0f) new_imm = 10.0f;
                        g_h_gpu_agents[i].dna_program[ip].immediate_val = new_imm;
                    }
                }
            }
        }

        int novelty_end = cur_pop * 9 / 10;
        for (int i = refined_end; i < novelty_end; ++i) {
            g_h_gpu_agents[i].crash_trauma = inherited_trauma * 0.5;
            g_h_gpu_agents[i].active_program_size = g_h_gpu_agents[best_idx].active_program_size;
            g_h_gpu_agents[i].active_registers_count = g_h_gpu_agents[best_idx].active_registers_count;
            for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                g_h_gpu_agents[i].dna_program[ip] = g_h_gpu_agents[best_idx].dna_program[ip];
                g_h_gpu_agents[i].epigenetic_methylation[ip] = g_h_gpu_agents[best_idx].epigenetic_methylation[ip];

                if (g_h_gpu_agents[i].epigenetic_methylation[ip] > 0.8f) continue;

                if ((rand() % 100) < 35) {
                    int mut_type = rand() % 4;
                    if (mut_type == 0) g_h_gpu_agents[i].dna_program[ip].op = static_cast<unsigned char>(rand() % 15);
                    else if (mut_type == 1) g_h_gpu_agents[i].dna_program[ip].r_dest = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                    else if (mut_type == 2) g_h_gpu_agents[i].dna_program[ip].r_src1 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                    else g_h_gpu_agents[i].dna_program[ip].immediate_val = ((rand() % 200) - 100) * 0.05f;
                }
            }
        }

        // 4. Pure Spontaneous Inoculation Niches (Island 3: 10% De Novo Genotypes)
        for (int i = novelty_end; i < cur_pop; ++i) {
            g_h_gpu_agents[i].crash_trauma = 0.0;
            g_h_gpu_agents[i].active_program_size = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE + (rand() % 8);
            g_h_gpu_agents[i].active_registers_count = ParameterAgent::MIN_DYNAMIC_REGISTERS;

            for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                g_h_gpu_agents[i].epigenetic_methylation[ip] = 0.0f;
                g_h_gpu_agents[i].dna_program[ip].op = static_cast<unsigned char>(rand() % 15);
                g_h_gpu_agents[i].dna_program[ip].r_dest = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                g_h_gpu_agents[i].dna_program[ip].r_src1 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                g_h_gpu_agents[i].dna_program[ip].r_src2 = static_cast<unsigned char>(rand() % REGISTERS_COUNT);
                g_h_gpu_agents[i].dna_program[ip].immediate_val = ((rand() % 200) - 100) * 0.05f;
            }
        }

        // Jurnal Observasi Manusia: Catat fenotipe fatal jika terjadi tabrakan
        for (int i = 0; i < cur_pop; ++i) {
            if (g_h_gpu_agents[i].crashed) {
                std::string failed_form = decompile_dna_formula(g_h_gpu_agents[i].dna_program);
                if (g_blacklisted_failed_dna.size() < 100) {
                    if (g_blacklisted_failed_dna.insert(failed_form).second) {
                        save_failed_dna_registry();
                    }
                }
            }
        }

        cudaMemcpy(g_d_gpu_agents, g_h_gpu_agents, sizeof(GpuRocketAgent) * g_current_population_size, cudaMemcpyHostToDevice);
    }

    g_display_mind.calculate_layout(0.0f, 150.0f, 600.0f, 350.0f);
}

POINT world_to_screen(float wx, float wy) {
    POINT pt;
    pt.x = static_cast<LONG>((g_window_width / 2.0f) + g_pan_x + wx * g_zoom);
    pt.y = static_cast<LONG>(UI_HEIGHT + 60.0f + g_pan_y + wy * g_zoom);
    return pt;
}

void draw_dna_bytecode_execution(HDC memDC, GpuRocketAgent& alpha) {
    // 1. Visualisasi Register Memory (R0 - R7) di sisi kiri bawah
    int reg_start_x = 30;
    int reg_start_y = UI_HEIGHT + 20;
    int box_w = 125;
    int box_h = 55;

    SetBkMode(memDC, TRANSPARENT);
    
    for (int r = 0; r < REGISTERS_COUNT; ++r) {
        int bx = reg_start_x + (r % 4) * (box_w + 10);
        int by = reg_start_y + (r / 4) * (box_h + 10);

        HBRUSH boxBrush = CreateSolidBrush(RGB(17, 21, 28));
        RECT rRect = {bx, by, bx + box_w, by + box_h};
        FillRect(memDC, &rRect, boxBrush);
        DeleteObject(boxBrush);

        HBRUSH borderBrush = CreateSolidBrush(RGB(35, 44, 58));
        FrameRect(memDC, &rRect, borderBrush);
        DeleteObject(borderBrush);

        SetTextColor(memDC, RGB(110, 125, 145));
        std::stringstream ss_name;
        ss_name << "R" << r;
        if (r == 0) ss_name << " (THRUST)";
        else if (r == 1) ss_name << " (ALTITUDE)";
        else if (r == 2) ss_name << " (VELOCITY)";
        else if (r == 3) ss_name << " (DISTANCE)";
        std::string s_name = ss_name.str();
        TextOutA(memDC, bx + 8, by + 6, s_name.c_str(), static_cast<int>(s_name.length()));

        SetTextColor(memDC, RGB(185, 195, 210));
        std::stringstream ss_val;
        ss_val << std::fixed << std::setprecision(3) << alpha.registers[r];
        std::string s_val = ss_val.str();
        TextOutA(memDC, bx + 8, by + 28, s_val.c_str(), static_cast<int>(s_val.length()));
    }

    // 2. Panel Diagnostik Status Eksekusi (DIAGNOSTICS & SYSTEM HEALTH) di bawah Register
    int diag_x = 30;
    int diag_y = reg_start_y + 2 * (box_h + 10) + 18;
    int left_section_w = 4 * (box_w + 10) - 10;
    int diag_w = left_section_w;
    int diag_h = 125;

    HBRUSH diagBg = CreateSolidBrush(RGB(14, 18, 24));
    RECT diagRect = {diag_x, diag_y, diag_x + diag_w, diag_y + diag_h};
    FillRect(memDC, &diagRect, diagBg);
    DeleteObject(diagBg);

    HBRUSH diagBorder = CreateSolidBrush(RGB(35, 44, 58));
    FrameRect(memDC, &diagRect, diagBorder);
    DeleteObject(diagBorder);

    SetTextColor(memDC, RGB(190, 160, 105));
    std::string diag_title = "PANEL DIAGNOSTIK STATUS GPU & MOTOR THRUST";
    TextOutA(memDC, diag_x + 15, diag_y + 10, diag_title.c_str(), static_cast<int>(diag_title.length()));

    SetTextColor(memDC, RGB(145, 158, 175));
    std::stringstream ss_diag1;
    ss_diag1 << "Status Fisik Alpha : Alt = " << std::fixed << std::setprecision(2) << (alpha.best_altitude / 1000.0) << " km"
             << " | Sisa Bahan Bakar = " << (alpha.fuel_mass / 1000.0) << " ton"
             << " | Kecepatan = " << std::fixed << std::setprecision(1) << alpha.velocity << " m/s";
    std::string s_diag1 = ss_diag1.str();
    TextOutA(memDC, diag_x + 15, diag_y + 34, s_diag1.c_str(), static_cast<int>(s_diag1.length()));

    std::stringstream ss_diag2;
    ss_diag2 << "Kognisi Sel Otak : DNA Sel = " << alpha.active_program_size << "/" << ParameterAgent::MAX_DNA_CAPACITY
             << " | Register = " << alpha.active_registers_count << "/" << ParameterAgent::MAX_REGISTERS
             << " | Fear = " << std::fixed << std::setprecision(0) << (alpha.fear_level * 100.0) << "%"
             << " | Trauma = " << std::fixed << std::setprecision(0) << (alpha.crash_trauma * 100.0) << "%";
    std::string s_diag2 = ss_diag2.str();
    TextOutA(memDC, diag_x + 15, diag_y + 58, s_diag2.c_str(), static_cast<int>(s_diag2.length()));

    SetTextColor(memDC, alpha.landed_safely ? RGB(115, 175, 125) : (alpha.crashed ? RGB(195, 95, 105) : RGB(190, 160, 105)));
    std::string last_status = alpha.landed_safely ? "Status Terbang: MENDARAT SUKSES DI BULAN!" :
                              (alpha.crashed ? "Status Terbang: CRASHED (Jatuh/Menabrak) -> Epigenetic Causal Penalty" : "Status Terbang: Mengorbit / Manuver Aktif");
    TextOutA(memDC, diag_x + 15, diag_y + 82, last_status.c_str(), static_cast<int>(last_status.length()));

    // 3. Visualisasi Bytecode Disassembly (DNA Code View) di sisi kanan (Non-Overlapping)
    int code_start_x = diag_x + left_section_w + 30;
    int code_start_y = UI_HEIGHT + 20;
    int code_w = g_window_width - code_start_x - 30;
    if (code_w < 380) code_w = 380;
    int code_h = 2 * (box_h + 10) + 18 + diag_h;

    HBRUSH codeBg = CreateSolidBrush(RGB(14, 18, 24));
    RECT codeRect = {code_start_x, code_start_y - 5, code_start_x + code_w, code_start_y + code_h};
    FillRect(memDC, &codeRect, codeBg);
    DeleteObject(codeBg);

    HBRUSH codeBorder = CreateSolidBrush(RGB(35, 44, 58));
    FrameRect(memDC, &codeRect, codeBorder);
    DeleteObject(codeBorder);

    SetTextColor(memDC, RGB(140, 155, 175));
    std::string code_title = "AKTIF DNA BYTECODE DISASSEMBLY & CREDIT TRACE";
    TextOutA(memDC, code_start_x + 12, code_start_y + 8, code_title.c_str(), static_cast<int>(code_title.length()));

    const char* op_names[] = {
        "NOP", "LOAD_SENSOR", "ADD", "SUB", "MUL", "DIV",
        "TANH", "SIGMOID", "INTEGRAL", "DERIVATIVE", "WRITE_DNA", "OUTPUT_THRUST", "PREDICT_PHYSICS",
        "FORK_NEURON", "PRUNE_NEURON"
    };

    int show_limit = std::min(12, alpha.active_program_size);
    for (int ip = 0; ip < show_limit; ++ip) {
        int cy = code_start_y + 32 + ip * 19;
        const auto& inst = alpha.dna_program[ip];
        char buf[160];
        unsigned char opcode = inst.op % 15;
        float methyl = alpha.epigenetic_methylation[ip];
        float trace = alpha.eligibility_trace[ip];

        if (opcode == 0) { // OP_NOP
            snprintf(buf, sizeof(buf), "[%02d] %-13s | Metil:%.2f Trace:%.1f", ip, op_names[opcode], methyl, trace);
        } else if (opcode == 1) { // LOAD_SENSOR
            snprintf(buf, sizeof(buf), "[%02d] %-13s R%d <= S%d (%.1f) | M:%.2f",
                     ip, op_names[opcode], (int)inst.r_dest, (int)(inst.r_src1 % 6), inst.immediate_val, methyl);
        } else if (opcode == 10) { // WRITE_DNA
            snprintf(buf, sizeof(buf), "[%02d] %-13s Target:R%d Val:R%d | M:%.2f",
                     ip, op_names[opcode], (int)inst.r_src1, (int)inst.r_src2, methyl);
        } else if (opcode == 11) { // OUTPUT_THRUST
            snprintf(buf, sizeof(buf), "[%02d] %-13s Thrust <= R%d | M:%.2f T:%.1f",
                     ip, op_names[opcode], (int)inst.r_src1, methyl, trace);
        } else if (opcode == 12) { // PREDICT_PHYSICS
            snprintf(buf, sizeof(buf), "[%02d] %-13s Rollout R%d <= R%d | M:%.2f",
                     ip, op_names[opcode], (int)inst.r_dest, (int)inst.r_src1, methyl);
        } else if (opcode == 13) { // FORK_NEURON
            snprintf(buf, sizeof(buf), "[%02d] %-13s Neurogenesis (+) | M:%.2f",
                     ip, op_names[opcode], methyl);
        } else if (opcode == 14) { // PRUNE_NEURON
            snprintf(buf, sizeof(buf), "[%02d] %-13s Synapse Pruning (-) | M:%.2f",
                     ip, op_names[opcode], methyl);
        } else {
            snprintf(buf, sizeof(buf), "[%02d] %-13s R%d <= R%d, R%d | M:%.2f",
                     ip, op_names[opcode], (int)inst.r_dest, (int)inst.r_src1, (int)inst.r_src2, methyl);
        }

        COLORREF codeColor;
        if (opcode == 10) codeColor = RGB(195, 95, 105);       // WRITE_DNA (Merah soft pastel)
        else if (opcode == 11) codeColor = RGB(115, 175, 125); // OUTPUT_THRUST (Hijau soft pastel)
        else if (opcode == 12) codeColor = RGB(220, 180, 80);  // PREDICT_PHYSICS (Gold World Model)
        else if (opcode == 13) codeColor = RGB(130, 200, 240); // FORK_NEURON (Biru muda)
        else if (opcode == 14) codeColor = RGB(240, 140, 180); // PRUNE_NEURON (Pink muda)
        else codeColor = RGB(135, 148, 165);                   // Normal instruction (Soft slate)

        SetTextColor(memDC, codeColor);
        TextOutA(memDC, code_start_x + 12, cy, buf, static_cast<int>(strlen(buf)));
    }
}

void render_dashboard(HDC hdc) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, g_window_width, g_window_height);
    SelectObject(memDC, memBitmap);

    HBRUSH bgBrush = CreateSolidBrush(RGB(10, 13, 18));
    RECT screenRect = {0, 0, g_window_width, g_window_height};
    FillRect(memDC, &screenRect, bgBrush);
    DeleteObject(bgBrush);

    if (g_h_gpu_agents) {
        draw_dna_bytecode_execution(memDC, g_h_gpu_agents[0]);
    }

    // Visualisasi Trajektori Penerbangan Bumi ke Bulan
    int traj_panel_w = 420;
    int traj_panel_h = 135;
    int traj_x = g_window_width - traj_panel_w - 20;
    int traj_y = 15;

    HBRUSH panelBrush = CreateSolidBrush(RGB(15, 19, 26));
    RECT panelRect = {traj_x, traj_y, traj_x + traj_panel_w, traj_y + traj_panel_h};
    FillRect(memDC, &panelRect, panelBrush);
    DeleteObject(panelBrush);

    HBRUSH panelBorder = CreateSolidBrush(RGB(38, 48, 64));
    FrameRect(memDC, &panelRect, panelBorder);
    DeleteObject(panelBorder);

    SetTextColor(memDC, RGB(190, 160, 105));
    std::string traj_title = "LINTASAN ORBIT BUMI - BULAN";
    TextOutA(memDC, traj_x + 15, traj_y + 10, traj_title.c_str(), static_cast<int>(traj_title.length()));

    // Gambar Bumi
    int earth_cx = traj_x + 40;
    int earth_cy = traj_y + 70;
    HBRUSH earthBrush = CreateSolidBrush(RGB(55, 105, 160));
    SelectObject(memDC, earthBrush);
    Ellipse(memDC, earth_cx - 15, earth_cy - 15, earth_cx + 15, earth_cy + 15);
    DeleteObject(earthBrush);

    // Gambar Bulan
    int moon_cx = traj_x + traj_panel_w - 40;
    int moon_cy = traj_y + 70;
    HBRUSH moonBrush = CreateSolidBrush(RGB(155, 165, 175));
    SelectObject(memDC, moonBrush);
    Ellipse(memDC, moon_cx - 10, moon_cy - 10, moon_cx + 10, moon_cy + 10);
    DeleteObject(moonBrush);

    // Garis lintasan
    HPEN trackPen = CreatePen(PS_DOT, 1, RGB(50, 62, 78));
    SelectObject(memDC, trackPen);
    MoveToEx(memDC, earth_cx, earth_cy, NULL);
    LineTo(memDC, moon_cx, moon_cy);
    DeleteObject(trackPen);

    // Posisi Roket Terjauh
    double progress_ratio = std::clamp(g_global_record_dist / g_display_gt.MOON_ORBIT_DIST, 0.0, 1.0);
    int rocket_x = static_cast<int>(earth_cx + progress_ratio * (moon_cx - earth_cx));
    int rocket_y = earth_cy;

    HBRUSH rkBrush = CreateSolidBrush(RGB(195, 95, 105));
    SelectObject(memDC, rkBrush);
    Ellipse(memDC, rocket_x - 5, rocket_y - 5, rocket_x + 5, rocket_y + 5);
    DeleteObject(rkBrush);

    SetTextColor(memDC, RGB(190, 160, 105));
    std::string s_prog = "Progress Orbit: " + std::to_string(static_cast<int>(progress_ratio * 100.0)) + "%";
    TextOutA(memDC, traj_x + 15, traj_y + traj_panel_h - 22, s_prog.c_str(), static_cast<int>(s_prog.length()));

    HBRUSH uiBrush = CreateSolidBrush(RGB(14, 18, 24));
    RECT uiRect = {0, 0, traj_x - 10, UI_HEIGHT};
    FillRect(memDC, &uiRect, uiBrush);
    DeleteObject(uiBrush);

    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(35, 44, 58));
    SelectObject(memDC, borderPen);
    MoveToEx(memDC, 0, UI_HEIGHT, NULL);
    LineTo(memDC, g_window_width, UI_HEIGHT);
    DeleteObject(borderPen);

    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(190, 160, 105));
    std::string title = "GPU PARALLEL DNA BYTECODE VIRTUAL MACHINE (10,240 AGENTS)";
    TextOutA(memDC, 20, 15, title.c_str(), static_cast<int>(title.length()));

    SetTextColor(memDC, RGB(115, 175, 125));
    std::stringstream ss_telemetry;
    ss_telemetry << std::fixed << std::setprecision(1) 
                 << "Jarak Maksimal: " << (g_global_record_dist / 1000.0) << " km"
                 << " | Sisa Jarak Bulan: " << ((g_display_gt.MOON_ORBIT_DIST - g_global_record_dist) / 1000.0) << " km";
    std::string s_tele = ss_telemetry.str();
    TextOutA(memDC, 20, 42, s_tele.c_str(), static_cast<int>(s_tele.length()));

    SetTextColor(memDC, RGB(145, 158, 175));
    std::stringstream ss_eq;
    ss_eq << "RUMUS FINAL DNA: " << g_best_landing_formula << " | Mutasi: " << g_total_dna_rewrites;
    std::string s_eq_str = ss_eq.str();
    TextOutA(memDC, 20, 70, s_eq_str.c_str(), static_cast<int>(s_eq_str.length()));

    SetTextColor(memDC, RGB(90, 135, 195));
    int total_crashes = std::max(0, g_total_global_flights - g_total_successful_landings);
    std::string stats = "Total Percobaan: " + std::to_string(g_total_global_flights) + 
                        " | Mendarat Sukses: " + std::to_string(g_total_successful_landings) +
                        " | Gagal (Crashed): " + std::to_string(total_crashes);
    TextOutA(memDC, 20, 98, stats.c_str(), static_cast<int>(stats.length()));

    BitBlt(hdc, 0, 0, g_window_width, g_window_height, memDC, 0, 0, SRCCOPY);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SIZE: {
            g_window_width = LOWORD(lParam);
            g_window_height = HIWORD(lParam);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_MOUSEWHEEL: {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (zDelta > 0) g_zoom = std::min(4.0f, g_zoom * 1.15f);
            else g_zoom = std::max(0.15f, g_zoom / 1.15f);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONDOWN: {
            g_is_dragging = true;
            g_drag_start_x = GET_X_LPARAM(lParam);
            g_drag_start_y = GET_Y_LPARAM(lParam);
            g_pan_start_x = g_pan_x;
            g_pan_start_y = g_pan_y;
            SetCapture(hwnd);
            return 0;
        }
        case WM_MOUSEMOVE: {
            if (g_is_dragging) {
                int cur_x = GET_X_LPARAM(lParam);
                int cur_y = GET_Y_LPARAM(lParam);
                g_pan_x = g_pan_start_x + (cur_x - g_drag_start_x);
                g_pan_y = g_pan_start_y + (cur_y - g_drag_start_y);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_LBUTTONUP: {
            if (g_is_dragging) {
                g_is_dragging = false;
                ReleaseCapture();
            }
            return 0;
        }
        case WM_TIMER: {
            run_mass_parallel_simulation();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            render_dashboard(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            g_server_running = false;
            if (g_d_gpu_agents) cudaFree(g_d_gpu_agents);
            if (g_h_gpu_agents) free(g_h_gpu_agents);
            DeleteCriticalSection(&g_cs);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(nullptr)));
    InitializeCriticalSection(&g_cs);
    load_failed_dna_registry();

    for (int i = 0; i < ParameterAgent::MAX_POPULATION_SIZE; ++i) {
        g_population[i].init_flight_mind();
        g_gt_engines[i].reset_simulation();
    }
    g_display_mind = g_population[0];
    g_display_gt = g_gt_engines[0];
    init_gpu_pipeline();

    // Start background HTTP Telemetry Server for Web App Dashboard (Port 8088)
    std::thread server_th(http_server_thread);
    server_th.detach();

    std::cout << "================================================================" << std::endl;
    std::cout << " [GPU MIND ASI] 10,240 CUDA AGENTS PARALLEL LAUNCH ENGINE" << std::endl;
    std::cout << " Web Dashboard: http://localhost:8088" << std::endl;
    std::cout << " Logging real-time progress every 5 seconds..." << std::endl;
    std::cout << "================================================================" << std::endl;

    // Launch as Standalone Desktop App Window (App Mode -- no browser tabs/URL bar)
    HINSTANCE hEdge = ShellExecuteA(NULL, "open", "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe", "--app=http://localhost:8088 --window-size=1280,820", NULL, SW_SHOWNORMAL);
    if ((INT_PTR)hEdge <= 32) {
        HINSTANCE hChrome = ShellExecuteA(NULL, "open", "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe", "--app=http://localhost:8088 --window-size=1280,820", NULL, SW_SHOWNORMAL);
        if ((INT_PTR)hChrome <= 32) {
            ShellExecuteA(NULL, "open", "http://localhost:8088", NULL, NULL, SW_SHOWNORMAL);
        }
    }

    auto last_log_time = std::chrono::steady_clock::now();
    int last_logged_flights = 0;

    // Main Simulation Loop (10,240 parallel GPU flights per tick)
    while (g_server_running) {
        run_mass_parallel_simulation();

        auto now = std::chrono::steady_clock::now();
        auto elapsed_sec = std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count();

        if (elapsed_sec >= 5) {
            EnterCriticalSection(&g_cs);
            const auto& alpha = g_h_gpu_agents[g_best_agent_idx];
            int speed_flights = g_total_global_flights - last_logged_flights;
            last_logged_flights = g_total_global_flights;
            last_log_time = now;

            std::cout << "[T+5s] Flights: " << g_total_global_flights 
                      << " (" << (speed_flights / 5) << " fl/s)"
                      << " | Record: " << std::fixed << std::setprecision(1) << (g_global_record_dist / 1000.0) << " km"
                      << " | Moon Dist: " << std::fixed << std::setprecision(1) << (alpha.distance_to_moon / 1000.0) << " km"
                      << " | Landed: " << g_total_successful_landings 
                      << " | Fear: " << std::fixed << std::setprecision(0) << (alpha.fear_level * 100.0) << "%"
                      << " | Formula: " << g_best_landing_formula << std::endl;
            LeaveCriticalSection(&g_cs);
        }

        Sleep(16); // ~60 FPS simulation tickrate
    }

    if (g_d_gpu_agents) cudaFree(g_d_gpu_agents);
    if (g_h_gpu_agents) free(g_h_gpu_agents);
    DeleteCriticalSection(&g_cs);
    return 0;
}
