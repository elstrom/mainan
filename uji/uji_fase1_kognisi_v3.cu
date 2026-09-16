#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <string>
#include <fstream>
#include <cuda_runtime.h>
#include <curand_kernel.h>

#include "../agent/03_kognisi_sparse_recurrent_v3.h"

// =============================================================================
// FASE 1: UJI MANDIRI (SANITY CHECK - TANPA PREDATOR, TANPA REBUTAN MAKANAN)
// - Harness Uji Fisika & Benchmark GPU murni
// - Model Kognisi Terpusat di agent/03_kognisi_sparse_recurrent_v3.h
// =============================================================================

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA Error at " << __FILE__ << ":" << __LINE__ << " -> " << cudaGetErrorString(err) << "\n"; \
            exit(1); \
        } \
    } while (0)

#define NUM_AGENTS 1000
#define WORLD_SIZE 200.0f
#define EAT_RADIUS 2.5f
const std::string BEST_BRAIN_FILE = "d:/MyProjects/mainan/agent/ingatan/best_agent_v3.bin";

struct GpuAgentData {
    float x;
    float y;
    float heading;
    float energy;
    float speed;
    
    // Target Makanan Mandiri (Privat per Agen)
    float food_x;
    float food_y;
    float prev_dist_food;
    
    int food_eaten;
    int ticks_alive;
    bool is_alive;

    // Otak Kognisi V3 (Model Terpusat)
    KognisiV3Brain brain;
};

// =============================================================================
// CUDA KERNEL: SIMULASI 1000 AGEN UJI MANDIRI (PARALEL TANPA INTERFERENSI)
// =============================================================================
__global__ void simulate_agents_kernel(
    GpuAgentData* agents,
    curandState* rand_states,
    int* d_alive_count
) {
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx >= NUM_AGENTS) return;

    GpuAgentData& ag = agents[idx];
    if (!ag.is_alive) return;

    ag.ticks_alive++;
    curandState local_rand = rand_states[idx];

    // 1. Sensor Fisika Dunia
    float dx = ag.food_x - ag.x;
    float dy = ag.food_y - ag.y;
    float curr_dist = sqrtf(dx * dx + dy * dy);

    float target_angle = atan2f(dy, dx);
    float angle_diff = target_angle - ag.heading;
    while (angle_diff > 3.14159265f) angle_diff -= 2.0f * 3.14159265f;
    while (angle_diff < -3.14159265f) angle_diff += 2.0f * 3.14159265f;
    float norm_heading = angle_diff / 3.14159265f;

    float delta_predator = 0.0f; // Bebas predator
    float delta_food = KognisiV3Brain::clamp_val((ag.prev_dist_food - curr_dist) / 5.0f, -1.0f, 1.0f);
    float norm_energy = KognisiV3Brain::clamp_val((ag.energy / 50.0f) - 1.0f, -1.0f, 1.0f);

    // 2. Eksekusi Model Kognisi V3
    float out_steer = 0.0f;
    float out_speed = 0.0f;
    ag.brain.forward_step(delta_predator, delta_food, norm_heading, norm_energy, out_steer, out_speed);

    // 3. Fisika & Gerak Agen
    ag.heading += out_steer * 0.35f;
    while (ag.heading > 3.14159265f) ag.heading -= 2.0f * 3.14159265f;
    while (ag.heading < -3.14159265f) ag.heading += 2.0f * 3.14159265f;

    ag.speed = out_speed * 1.5f;
    ag.x += cosf(ag.heading) * ag.speed;
    ag.y += sinf(ag.heading) * ag.speed;

    ag.x = KognisiV3Brain::clamp_val(ag.x, 0.0f, WORLD_SIZE);
    ag.y = KognisiV3Brain::clamp_val(ag.y, 0.0f, WORLD_SIZE);

    float move_cost = 0.05f + (ag.speed * 0.08f);
    ag.energy -= move_cost;

    // 4. Deteksi Makanan & Konsumsi Energi
    float new_dx = ag.food_x - ag.x;
    float new_dy = ag.food_y - ag.y;
    float new_dist = sqrtf(new_dx * new_dx + new_dy * new_dy);

    if (new_dist < EAT_RADIUS) {
        ag.energy = fminf(100.0f, ag.energy + 30.0f);
        ag.food_eaten++;

        // Reset trace saat makan
        ag.brain.reset_traces();

        // Spawn target baru khusus untuk agen ini
        ag.food_x = 10.0f + curand_uniform(&local_rand) * (WORLD_SIZE - 20.0f);
        ag.food_y = 10.0f + curand_uniform(&local_rand) * (WORLD_SIZE - 20.0f);

        float ndx = ag.food_x - ag.x;
        float ndy = ag.food_y - ag.y;
        ag.prev_dist_food = sqrtf(ndx * ndx + ndy * ndy);
    } else {
        ag.prev_dist_food = new_dist;
    }

    if (ag.energy <= 0.0f) {
        ag.is_alive = false;
        atomicSub(d_alive_count, 1);
    }

    rand_states[idx] = local_rand;
}

__global__ void init_curand_kernel(curandState* states, unsigned long seed) {
    int idx = blockDim.x * blockIdx.x + threadIdx.x;
    if (idx < NUM_AGENTS) {
        curand_init(seed, idx, 0, &states[idx]);
    }
}

void init_agent_state(GpuAgentData& ag) {
    ag.heading = 0.0f;
    ag.energy = 60.0f;
    ag.speed = 0.0f;
    ag.prev_dist_food = 999.0f;
    ag.food_eaten = 0;
    ag.ticks_alive = 0;
    ag.is_alive = true;
    ag.brain.init_random();
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    std::cout << "=================================================================\n";
    std::cout << "  CUDA GPU KERNEL - UJI MANDIRI FASE 1 (1000 AGENTS INDEPENDENT)\n";
    std::cout << "  (Evolvable ABCD Plasticity + Eligibility Trace O(N))\n";
    std::cout << "=================================================================\n";

    std::vector<GpuAgentData> h_agents(NUM_AGENTS);
    std::vector<GpuAgentData> h_agents_birth(NUM_AGENTS); // Backup Genotipe Saat Lahir
    GpuAgentData* d_agents = nullptr;
    curandState* d_rand_states = nullptr;
    int* d_alive_count = nullptr;

    CUDA_CHECK(cudaMalloc(&d_agents, NUM_AGENTS * sizeof(GpuAgentData)));
    CUDA_CHECK(cudaMalloc(&d_rand_states, NUM_AGENTS * sizeof(curandState)));
    CUDA_CHECK(cudaMalloc(&d_alive_count, sizeof(int)));

    int threads = 256;
    int blocks = (NUM_AGENTS + threads - 1) / threads;
    init_curand_kernel<<<blocks, threads>>>(d_rand_states, 1337);
    CUDA_CHECK(cudaDeviceSynchronize());

    int global_best_food = 0;
    long long global_best_ticks = 999999999LL;
    KognisiV3Brain champion_brain;
    bool has_champion = false;

    // Muat ingatan juara jika file ada
    if (champion_brain.load_weights(BEST_BRAIN_FILE)) {
        has_champion = true;
        std::cout << "[INFO] Memori otak juara leluhur berhasil dimuat dari disk!\n";
    } else {
        std::cout << "[INFO] Belum ada file ingatan juara. Memulai dari generasi awal (Acak).\n";
    }

    const long long EVAL_TARGET_TICKS = 200000;
    
    int gen = 0;
    bool all_agents_perfect = false;

    // Loop Open-Ended: Berjalan terus hingga seluruh 1000 agen bertahan hidup bersama sampai target
    while (!all_agents_perfect) {
        gen++;
        std::cout << "[Gen " << std::setw(4) << gen << "] ";

        for (int i = 0; i < NUM_AGENTS; ++i) {
            init_agent_state(h_agents[i]);
            h_agents[i].x = 10.0f + (rand() % static_cast<int>(WORLD_SIZE - 20.0f));
            h_agents[i].y = 10.0f + (rand() % static_cast<int>(WORLD_SIZE - 20.0f));
            h_agents[i].heading = ((rand() % 628) - 314) * 0.01f;

            // Target makanan privat unik per agen
            h_agents[i].food_x = 10.0f + (rand() % static_cast<int>(WORLD_SIZE - 20.0f));
            h_agents[i].food_y = 10.0f + (rand() % static_cast<int>(WORLD_SIZE - 20.0f));
            float dx = h_agents[i].food_x - h_agents[i].x;
            float dy = h_agents[i].food_y - h_agents[i].y;
            h_agents[i].prev_dist_food = sqrtf(dx * dx + dy * dy);

            if (has_champion) {
                h_agents[i].brain = champion_brain;
                h_agents[i].brain.reset_traces();

                // Elitisme Absolut: Agen 0 murni 100% klon juara tanpa mutasi
                if (i != 0) {
                    h_agents[i].brain.mutate(0.001f, 0.10f);
                }
            }

            // Backup Genotipe Awal Saat Lahir (Tick 0)
            h_agents_birth[i] = h_agents[i];
        }

        int h_alive = NUM_AGENTS;
        CUDA_CHECK(cudaMemcpy(d_agents, h_agents.data(), NUM_AGENTS * sizeof(GpuAgentData), cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_alive_count, &h_alive, sizeof(int), cudaMemcpyHostToDevice));

        long long world_tick = 0;
        while (h_alive > 0 && world_tick < EVAL_TARGET_TICKS) {
            world_tick++;
            simulate_agents_kernel<<<blocks, threads>>>(d_agents, d_rand_states, d_alive_count);

            if (world_tick % 50000 == 0 || world_tick == EVAL_TARGET_TICKS) {
                CUDA_CHECK(cudaMemcpy(&h_alive, d_alive_count, sizeof(int), cudaMemcpyDeviceToHost));
            }
        }

        CUDA_CHECK(cudaMemcpy(&h_alive, d_alive_count, sizeof(int), cudaMemcpyDeviceToHost));
        CUDA_CHECK(cudaMemcpy(h_agents.data(), d_agents, NUM_AGENTS * sizeof(GpuAgentData), cudaMemcpyDeviceToHost));

        int gen_best_idx = 0;
        int gen_best_food = -1;
        long long gen_best_ticks = -1;
        long long total_food = 0;

        for (int i = 0; i < NUM_AGENTS; ++i) {
            total_food += h_agents[i].food_eaten;

            // Prioritas 1: Bertahan hidup (is_alive) | Prioritas 2: Makanan terbanyak | Prioritas 3: Ticks terlama
            bool is_better = false;
            if (gen_best_food == -1) {
                is_better = true;
            } else if (h_agents[i].is_alive && !h_agents[gen_best_idx].is_alive) {
                is_better = true;
            } else if (h_agents[i].is_alive == h_agents[gen_best_idx].is_alive) {
                if (h_agents[i].food_eaten > gen_best_food) {
                    is_better = true;
                } else if (h_agents[i].food_eaten == gen_best_food && h_agents[i].ticks_alive > gen_best_ticks) {
                    is_better = true;
                }
            }

            if (is_better) {
                gen_best_food = h_agents[i].food_eaten;
                gen_best_ticks = h_agents[i].ticks_alive;
                gen_best_idx = i;
            }
        }

        // Print Ringkas per Generasi
        std::cout << "Selesai -> Bertahan: " << std::setw(4) << h_alive << "/1000 | Total Makan: " 
                  << std::setw(6) << total_food << " | Juara (ID " << gen_best_idx << ") Makan: " 
                  << gen_best_food << " buah (" << gen_best_ticks << " ticks)\n";

        // Evolutionary Pressure: Perbarui juara jika bertahan lebih lama atau makan lebih banyak
        if (gen_best_food >= 1 && (gen_best_food > global_best_food || (gen_best_food == global_best_food && gen_best_ticks > global_best_ticks))) {
            global_best_food = gen_best_food;
            global_best_ticks = gen_best_ticks;
            champion_brain = h_agents_birth[gen_best_idx].brain;
            has_champion = true;
            champion_brain.save_weights(BEST_BRAIN_FILE);
        }

        // KONDISI SELESAI: 1000 Agen berhasil bertahan 100% hingga akhir evaluasi
        if (h_alive == NUM_AGENTS) {
            all_agents_perfect = true;
            std::cout << "\n=================================================================\n";
            std::cout << ">>> SELAMAT! 1000/1000 AGEN BERHASIL BERTAHAN HIDUP 100% SAMPAI AKHIR! <<<\n";
            std::cout << " - Total Generasi yang Ditempuh : " << gen << " kali loop\n";
            std::cout << " - Rekor Tertinggi Makanan       : " << global_best_food << " buah\n";
            std::cout << " - Model Juara Tersimpan di     : " << BEST_BRAIN_FILE << "\n";
            std::cout << "=================================================================\n";
        }
    }

    cudaFree(d_agents);
    cudaFree(d_rand_states);
    cudaFree(d_alive_count);

    return 0;
}
