#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include "agent/dunia/parameter_dunia.h"
#include "agent/ingatan/parameter_agent.h"

#define DNA_PROGRAM_SIZE ParameterAgent::MAX_DNA_CAPACITY
#define REGISTERS_COUNT ParameterAgent::MAX_REGISTERS
#define TRAJECTORY_SAMPLES ParameterAgent::TRAJECTORY_SAMPLES

// Virtual Machine Bytecode Operasi Aljabar & Mutasi Struktural
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
    OP_WRITE_DNA,        // DNA Meta-Programming: Instruksi merombak instruction pointer target berdasarkan register
    OP_OUTPUT_THRUST,    // Thrust = reg[A]
    OP_PREDICT_PHYSICS,  // Phase 2 World Model: Mental Simulation internal (forward rollout prediktif 10 langkah ke depan)
    OP_FORK_NEURON,      // Phase 3 Neurogenesis: Menumbuhkan ukuran program & sirkuit baru secara on-demand
    OP_PRUNE_NEURON      // Phase 3 Synaptic Pruning: Memangkas memori & sirkuit tidak berguna agar berpikir lebih cepat
};

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

__device__ inline double gpu_clamp(double val, double min_v, double max_v) {
    if (val < min_v) return min_v;
    if (val > max_v) return max_v;
    return val;
}

__global__ void simulate_population_cuda_kernel(GpuRocketAgent* agents, int population_size, int max_steps, double dt) {
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx >= population_size) return;

    GpuRocketAgent& ag = agents[idx];
    ag.altitude = 0.0;
    ag.velocity = 0.0;
    ag.peak_velocity = 0.0;
    ag.best_altitude = 0.0;
    ag.fuel_mass = 120000.0;
    const double dry_mass = 5000.0;
    const double MOON_ORBIT_DIST = 384400000.0;
    const double G = 6.67430e-11;
    const double M_EARTH = 5.972e24;
    const double R_EARTH = 6.371e6;
    const double M_MOON = 7.342e22;
    const double R_MOON = 1.737e6;
    const double ISP = 450.0;
    const double G0 = 9.80665;
    ag.distance_to_moon = MOON_ORBIT_DIST;
    ag.landed_safely = false;
    ag.crashed = false;
    ag.is_launched = false;
    ag.thought_ticks = 0;
    ag.traj_points_count = 0;
    ag.fear_level = ag.crash_trauma; // Mulai penerbangan dengan rasa takut warisan generasi sebelumnya

    if (ag.active_program_size < ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE || ag.active_program_size > DNA_PROGRAM_SIZE) {
        ag.active_program_size = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE + (idx % 16);
    }
    if (ag.active_registers_count < ParameterAgent::MIN_DYNAMIC_REGISTERS || ag.active_registers_count > REGISTERS_COUNT) {
        ag.active_registers_count = ParameterAgent::MIN_DYNAMIC_REGISTERS;
    }

    // Reset register komputasi otak & jejak kausalitas waktu
    for (int r = 0; r < REGISTERS_COUNT; ++r) {
        ag.registers[r] = 0.0;
        ag.prev_registers[r] = 0.0;
        ag.integrated_registers[r] = 0.0;
    }
    for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
        ag.eligibility_trace[ip] = 0.0f;
    }

    double final_thrust = 0.0;

    for (int step = 0; step < max_steps; ++step) {
        double norm_h = ag.altitude / MOON_ORBIT_DIST;
        double norm_v = ag.velocity / 11200.0;
        double norm_d = ag.distance_to_moon / MOON_ORBIT_DIST;
        double fuel_ratio = ag.fuel_mass / 120000.0; // Waktu hidup kognitif murni ditentukan oleh ketersediaan energi, bukan batasan waktu buatan

        // Dinamika Fear: Ketakutan Alami Berdasarkan Bahaya Fisik Nyata Saat Terbang
        // 1. Bahaya jatuh bebas ke Bumi
        double fall_hazard = (ag.velocity < -20.0) ? gpu_clamp(-ag.velocity / 300.0, 0.0, 0.8) : 0.0;
        
        // 2. Bahaya menabrak permukaan Bulan dengan kecepatan tinggi
        double lunar_impact_hazard = (ag.distance_to_moon < 5000000.0 && ag.velocity > 50.0) ? 
                                     gpu_clamp((ag.velocity / 2000.0) * (1.0 - ag.distance_to_moon / 5000000.0), 0.0, 0.9) : 0.0;
        
        // 3. Bahaya kehabisan bahan bakar di luar orbit
        double fuel_hazard = (ag.fuel_mass < 15000.0 && ag.distance_to_moon > 10000000.0) ? 
                             (1.0 - ag.fuel_mass / 15000.0) * 0.5 : 0.0;
        
        // Resultan Rasa Takut Dinamis (Dipengaruhi trauma masa lalu yang meluruh)
        ag.fear_level = gpu_clamp(fall_hazard + lunar_impact_hazard + fuel_hazard + (ag.crash_trauma * 0.3), 0.0, 1.0);

        // Leaky Register Memory: Peluruhan alami untuk mencegah akumulasi saturasi permanen
        int max_r = (ag.active_registers_count > 0 && ag.active_registers_count <= REGISTERS_COUNT) ? 
                    ag.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;

        for (int r = 0; r < max_r; ++r) {
            ag.registers[r] *= 0.92;
        }

        int active_prog = (ag.active_program_size > 0 && ag.active_program_size <= DNA_PROGRAM_SIZE) ? 
                          ag.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

        // Peluruhan Jejak Kausalitas Waktu (Temporal Eligibility Decay)
        for (int ip = 0; ip < active_prog; ++ip) {
            ag.eligibility_trace[ip] *= ParameterAgent::TEMPORAL_TRACE_DECAY;
        }

        // --- EKSEKUSI PROGRAM DNA (VIRTUAL MACHINE INTELLECT) ---
        for (int ip = 0; ip < active_prog; ++ip) {
            ag.eligibility_trace[ip] = gpu_clamp(ag.eligibility_trace[ip] + 1.0f, 0.0, 5.0);
            const auto& inst = ag.dna_program[ip];
            int rd = inst.r_dest % max_r;
            int rs1 = inst.r_src1 % max_r;
            int rs2 = inst.r_src2 % max_r;

            switch (inst.op % 15) {
                case OP_NOP: break;
                case OP_LOAD_SENSOR: {
                    int s = inst.r_src1 % 6;
                    if (s == 0) ag.registers[rd] = norm_h;
                    else if (s == 1) ag.registers[rd] = norm_v;
                    else if (s == 2) ag.registers[rd] = norm_d;
                    else if (s == 3) ag.registers[rd] = 1.0 - fuel_ratio; // Kesadaran konsumsi energi
                    else if (s == 4) ag.registers[rd] = ag.fear_level; // Sensor Fear & Trauma Kematian
                    else ag.registers[rd] = gpu_clamp(inst.immediate_val, -5.0, 5.0);
                    break;
                }
                case OP_ADD: ag.registers[rd] = gpu_clamp(ag.registers[rs1] + ag.registers[rs2], -5.0, 5.0); break;
                case OP_SUB: ag.registers[rd] = gpu_clamp(ag.registers[rs1] - ag.registers[rs2], -5.0, 5.0); break;
                case OP_MUL: ag.registers[rd] = gpu_clamp(ag.registers[rs1] * ag.registers[rs2], -5.0, 5.0); break;
                case OP_DIV: ag.registers[rd] = gpu_clamp(ag.registers[rs1] / (fabs(ag.registers[rs2]) + 1e-3), -5.0, 5.0); break;
                case OP_TANH: ag.registers[rd] = tanh(ag.registers[rs1]); break;
                case OP_SIGMOID: ag.registers[rd] = 1.0 / (1.0 + exp(-gpu_clamp(ag.registers[rs1], -5.0, 5.0))); break;
                case OP_INTEGRAL: {
                    ag.integrated_registers[rd] += ag.registers[rs1] * dt * 0.001;
                    ag.registers[rd] = tanh(ag.integrated_registers[rd]);
                    break;
                }
                case OP_DERIVATIVE: {
                    ag.registers[rd] = gpu_clamp((ag.registers[rs1] - ag.prev_registers[rd]) / (dt + 1e-4), -5.0, 5.0);
                    ag.prev_registers[rd] = ag.registers[rs1];
                    break;
                }
                case OP_WRITE_DNA: {
                    // Self-Directed Meta-Programming:
                    // reg[rs1] menentukan target instruction pointer, reg[rs2] menentukan opcode baru & modifikasi nilai
                    int target_ip = (int)fabs(ag.registers[rs1] * 10.0) % active_prog;
                    if (target_ip >= 0 && target_ip < active_prog) {
                        unsigned char new_op = (unsigned char)((int)fabs(ag.registers[rs2] * 10.0) % 15);
                        ag.dna_program[target_ip].op = new_op;
                        ag.dna_program[target_ip].immediate_val = (float)gpu_clamp(ag.registers[rs2], -5.0, 5.0);
                    }
                    break;
                }
                case OP_PREDICT_PHYSICS: {
                    // Mental Simulation (Internal Causal World Model Rollout):
                    // Prediksi trajektori 10 langkah ke depan jika menerapkan gaya reg[rs1]
                    double hyp_thrust = gpu_clamp(0.5 + 0.5 * tanh(ag.registers[rs1]), 0.0, 1.0);
                    double hyp_alt = ag.altitude;
                    double hyp_vel = ag.velocity;
                    double hyp_fuel = ag.fuel_mass;
                    double hyp_dt = dt * 2.0;

                    for (int m = 0; m < 5; ++m) {
                        double r_e = R_EARTH + hyp_alt;
                        double r_m = (MOON_ORBIT_DIST - hyp_alt) + R_MOON;
                        double g_net = -(G * M_EARTH) / (r_e * r_e) + (G * M_MOON) / (r_m * r_m);
                        double cur_mass = dry_mass + hyp_fuel;
                        double t_force = (hyp_fuel > 0.0) ? (hyp_thrust * 3600000.0) : 0.0;
                        double a_net = (t_force / cur_mass) + g_net;
                        hyp_vel += a_net * hyp_dt;
                        hyp_alt += hyp_vel * hyp_dt;
                        hyp_fuel = fmax(0.0, hyp_fuel - (t_force / (ISP * G0)) * hyp_dt);
                    }
                    // Simpan evaluasi prediksi (1.0 jika mendekat aman, -1.0 jika jatuh/crash) ke reg[rd]
                    double pred_dist_moon = MOON_ORBIT_DIST - hyp_alt;
                    double safety_eval = (pred_dist_moon < ag.distance_to_moon) ? 1.0 : -0.5;
                    if (hyp_alt < 0.0 || (pred_dist_moon <= 0.0 && fabs(hyp_vel) > 2.5)) safety_eval = -1.0;
                    ag.registers[rd] = gpu_clamp(safety_eval, -1.0, 1.0);
                    break;
                }
                case OP_FORK_NEURON: {
                    // Neurogenesis: Tumbuhkan sirkuit otak dan perluas memori kerja jika menghadapi stres kognitif
                    if (ag.active_program_size < DNA_PROGRAM_SIZE && ag.fear_level > 0.3) {
                        int new_slot = ag.active_program_size++;
                        ag.dna_program[new_slot].op = (unsigned char)OP_ADD;
                        ag.dna_program[new_slot].r_dest = (unsigned char)(rs1 % max_r);
                        ag.dna_program[new_slot].r_src1 = (unsigned char)(rs2 % max_r);
                        ag.dna_program[new_slot].r_src2 = (unsigned char)(rd % max_r);
                        ag.dna_program[new_slot].immediate_val = 0.0f;
                    }
                    if (ag.active_registers_count < REGISTERS_COUNT && ag.fear_level > 0.5) {
                        ag.active_registers_count++;
                    }
                    break;
                }
                case OP_PRUNE_NEURON: {
                    // Synaptic Pruning: Pangkas sirkuit jika kondisi tenang dan stabil untuk efisiensi komputasi
                    if (ag.active_program_size > ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE && ag.fear_level < 0.1) {
                        ag.active_program_size--;
                    }
                    break;
                }
                case OP_OUTPUT_THRUST: {
                    final_thrust = gpu_clamp(0.5 + 0.5 * tanh(ag.registers[rs1]), 0.0, 1.0);
                    break;
                }
            }
        }

        // Fallback Closed-Loop: Jika DNA tidak mengeksekusi OP_OUTPUT_THRUST secara eksplisit, gunakan kombinasi register R0..R3
        if (final_thrust <= 0.001) {
            final_thrust = gpu_clamp(0.5 + 0.5 * tanh(ag.registers[0] + ag.registers[1] - ag.registers[3]), 0.1, 1.0);
        }

        // SIKLUS BERPIKIR KOGNITIF OTOMATIS (AUTONOMOUS COGNITIVE CYCLES)
        ag.thought_ticks++;
        ag.is_launched = true;

        // HUKUM FISIKA ASTRONAUTIKA NYATA (REALISTIC MULTI-BODY FLIGHT DYNAMICS)
        // 1. Posisi Radial Relatif dari Pusat Massa Bumi & Bulan
        double r_earth = R_EARTH + ag.altitude;
        double r_moon = ag.distance_to_moon + R_MOON;
        
        // 2. Gravitasi Universal Newton (Inverse Square Law)
        double g_earth = (G * M_EARTH) / (r_earth * r_earth);
        double g_moon = (G * M_MOON) / (r_moon * r_moon);
        double net_gravity = -g_earth + g_moon; // Gaya resultan

        // 3. Mesin Roket & Persamaan Tsiolkovsky (Kapasitas Gaya Dorong Saturn V / SLS)
        double thrust_force = 0.0;
        double max_thrust = 3600000.0; // 3.6 MN (TWR ~ 2.94 terhadap gravitasi Bumi)

        if (ag.fuel_mass > 0.0) {
            double throttle = gpu_clamp(final_thrust, 0.0, 1.0);
            thrust_force = throttle * max_thrust;
            
            // Laju konsumsi bahan bakar: dm/dt = F / (I_sp * g0)
            double mass_flow = thrust_force / (ISP * G0);
            double fuel_burned = mass_flow * dt;
            if (fuel_burned > ag.fuel_mass) {
                fuel_burned = ag.fuel_mass;
                thrust_force = (fuel_burned / dt) * ISP * G0;
            }
            ag.fuel_mass -= fuel_burned;
        }

        // 4. Aerodinamika Atmosfer Bumi (Exponential Atmospheric Density Decay)
        double drag_force = 0.0;
        if (ag.altitude < 100000.0) { // Garis Karman (100 km)
            double air_density = 1.225 * exp(-ag.altitude / 8500.0);
            double cd = 0.3; // Koefisien hambat aerodinamis roket
            double area = 10.0; // Luas penampang melintang (m^2)
            drag_force = 0.5 * air_density * ag.velocity * fabs(ag.velocity) * cd * area;
            if (ag.velocity < 0.0) drag_force = -drag_force;
        }

        // 5. Hukum II Newton & Integrasi Kinematika Simlektik (Semi-Implicit Euler)
        double current_total_mass = dry_mass + ag.fuel_mass;
        double net_acceleration = ((thrust_force - drag_force) / current_total_mass) + net_gravity;

        ag.velocity += net_acceleration * dt;
        ag.altitude += ag.velocity * dt;

        if (ag.altitude < 0.0) {
            ag.altitude = 0.0;
            if (ag.velocity < 0.0) ag.velocity = 0.0; // Di landasan peluncuran
        }

        if (fabs(ag.velocity) > fabs(ag.peak_velocity)) {
            ag.peak_velocity = ag.velocity;
        }
        if (ag.altitude > ag.best_altitude) {
            ag.best_altitude = ag.altitude;
        }
        ag.distance_to_moon = MOON_ORBIT_DIST - ag.altitude;

        // Sample flight trajectory for real-time visualization (Setiap ~60 steps)
        if (step % (max_steps / TRAJECTORY_SAMPLES) == 0 && ag.traj_points_count < TRAJECTORY_SAMPLES) {
            ag.traj_altitudes[ag.traj_points_count] = (float)ag.altitude;
            ag.traj_velocities[ag.traj_points_count] = (float)ag.velocity;
            ag.traj_fuels[ag.traj_points_count] = (float)ag.fuel_mass;
            ag.traj_thrusts[ag.traj_points_count] = (float)(thrust_force / max_thrust);
            ag.traj_points_count++;
        }

        if (ag.distance_to_moon <= 0.0) {
            ag.distance_to_moon = 0.0;
            if (ag.traj_points_count < TRAJECTORY_SAMPLES) {
                ag.traj_altitudes[ag.traj_points_count] = (float)MOON_ORBIT_DIST;
                ag.traj_velocities[ag.traj_points_count] = (float)ag.velocity;
                ag.traj_fuels[ag.traj_points_count] = (float)ag.fuel_mass;
                ag.traj_thrusts[ag.traj_points_count] = (float)(thrust_force / max_thrust);
                ag.traj_points_count++;
            }
            if (fabs(ag.velocity) <= 2.5) ag.landed_safely = true;
            else ag.crashed = true;
            break;
        }

        if (ag.altitude <= 0.0 && step > 100 && ag.fuel_mass < 50000.0) {
            ag.crashed = true;
            break;
        }
    }

    ag.end_fuel_mass = ag.fuel_mass;

    // Evaluasi Murni Survival & Homeostasis (Fisika Alami & Prinsip Energi Bebas):
    if (ag.crashed) {
        // HUKUM MUTLAK KEHANCURAN: Kegagalan Survival (Tabrakan / Kehancuran Struktur Fisik)
        double impact_energy = (ag.velocity * ag.velocity) * 0.5;
        ag.fitness = -100000.0 - impact_energy;

        // Temporal Credit Assignment (Kausalitas Waktu Alami):
        // Gen yang aktif sesaat sebelum kehancuran menerima penalti metilasi tinggi.
        int active_prog = (ag.active_program_size > 0 && ag.active_program_size <= DNA_PROGRAM_SIZE) ? 
                          ag.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;
        for (int ip = 0; ip < active_prog; ++ip) {
            float causal_penalty = ag.eligibility_trace[ip] * ParameterAgent::METHYLATION_PENALTY_RATE;
            ag.epigenetic_methylation[ip] = (float)gpu_clamp(ag.epigenetic_methylation[ip] + causal_penalty, 0.0, 1.0);
        }
    } else if (ag.landed_safely) {
        // SURVIVAL SUKSES: Mendarat utuh dengan kecepatan aman di Bulan + Homeostasis Energi
        double fuel_homeostasis = (ag.fuel_mass / 120000.0) * 10000.0; // Sisa cadangan energi hidup
        ag.fitness = 100000.0 + fuel_homeostasis;
    } else {
        // IN-FLIGHT SURVIVAL (Keberlangsungan Hidup & Integritas Fisik):
        // Diukur murni dari seberapa dekat ke target Bulan tanpa kehancuran dan sisa cadangan energi
        double moon_proximity = (1.0 - (ag.distance_to_moon / MOON_ORBIT_DIST)) * 10000.0;
        double energy_reserve = (ag.fuel_mass / 120000.0) * 2000.0;

        // Penalti Kompleksitas Sirkuit (Efisiensi Komputasi Otak):
        double complexity_penalty = (ag.active_program_size * ParameterAgent::PENALTY_PER_INSTRUCTION) +
                                    (ag.active_registers_count * ParameterAgent::PENALTY_PER_REGISTER);

        ag.fitness = moon_proximity + energy_reserve - complexity_penalty;
    }
}

extern "C" void launch_cuda_simulation(GpuRocketAgent* d_agents, int population_size, int max_steps, double dt) {
    int blockSize = 256;
    int numBlocks = (population_size + blockSize - 1) / blockSize;
    simulate_population_cuda_kernel<<<numBlocks, blockSize>>>(d_agents, population_size, max_steps, dt);
    cudaDeviceSynchronize();
}