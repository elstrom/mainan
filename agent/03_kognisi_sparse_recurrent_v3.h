#pragma once
#include <vector>
#include <string>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <fstream>

#ifndef __host__
#define __host__
#define __device__
#endif

// =============================================================================
// ARSITEKTUR KOGNISI V3: EVOLVABLE ABCD PLASTICITY DENGAN ELIGIBILITY TRACE
// - Setiap sinapsis membawa DNA plastisitas lokal: eta, A, B, C, D
// - Formula Update Bobot O(N):
//     Trace = clamp(gamma * Trace + (x_i * x_j), -2.0, 2.0)
//     dW = eta * Trace * (A * (x_i * x_j) + B * x_i + C * x_j + D)
// - Terproteksi dari NaN (bounded clamp), Delayed Reward Memory, & Heterogeneous
// =============================================================================

const int KOGNISI_V3_NUM_IN = 4;
const int KOGNISI_V3_NUM_HID = 8;
const int KOGNISI_V3_NUM_OUT = 2;
const int KOGNISI_V3_TOTAL_NEURONS = KOGNISI_V3_NUM_IN + KOGNISI_V3_NUM_HID + KOGNISI_V3_NUM_OUT; // 14
const int KOGNISI_V3_MAX_SYNAPSES = 128;

struct SynapseV3 {
    int from_neuron;
    int to_neuron;
    float weight;            // Bobot saat lahir
    float eligibility_trace; // Memori aktivitas lokal (temporal bridge)

    // DNA Plastisitas Epigenetik Lokal (Ikut Berevolusi)
    float eta;               // Learning rate spesifik sinapsis [-0.05, 0.05]
    float A;                 // Koefisien korelasi bersama (x_i * x_j) [-1.0, 1.0]
    float B;                 // Koefisien presinaptik (x_i) [-1.0, 1.0]
    float C;                 // Koefisien postsinaptik (x_j) [-1.0, 1.0]
    float D;                 // Koefisien drift / basal decay [-1.0, 1.0]
};

struct KognisiV3Brain {
    int num_synapses;
    SynapseV3 synapses[KOGNISI_V3_MAX_SYNAPSES];
    float neuron_act[KOGNISI_V3_TOTAL_NEURONS];
    float neuron_prev_act[KOGNISI_V3_TOTAL_NEURONS];
    float neuron_bias[KOGNISI_V3_TOTAL_NEURONS];

    __host__ __device__ static inline float clamp_val(float v, float lo, float hi) {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    __host__ __device__ static inline float act_sigmoid(float x) {
        return 1.0f / (1.0f + expf(-x));
    }

    __host__ __device__ static inline float act_tanh(float x) {
        return tanhf(x);
    }

    __host__ void init_random() {
        for (int i = 0; i < KOGNISI_V3_TOTAL_NEURONS; ++i) {
            neuron_act[i] = 0.0f;
            neuron_prev_act[i] = 0.0f;
            neuron_bias[i] = (((rand() % 200) - 100) * 0.001f);
        }

        auto make_syn = [](int from, int to, float w_scale) -> SynapseV3 {
            float w = ((rand() % 200) - 100) * 0.01f * w_scale;
            float eta = ((rand() % 200) - 100) * 0.0005f;
            float a = ((rand() % 200) - 100) * 0.01f;
            float b = ((rand() % 200) - 100) * 0.01f;
            float c = ((rand() % 200) - 100) * 0.01f;
            float d = ((rand() % 200) - 100) * 0.005f;
            return {from, to, w, 0.0f, eta, a, b, c, d};
        };

        num_synapses = 0;
        for (int in = 0; in < KOGNISI_V3_NUM_IN; ++in) {
            for (int hid = 0; hid < KOGNISI_V3_NUM_HID; ++hid) {
                synapses[num_synapses++] = make_syn(in, KOGNISI_V3_NUM_IN + hid, 1.0f);
            }
        }
        for (int h1 = 0; h1 < KOGNISI_V3_NUM_HID; ++h1) {
            for (int h2 = 0; h2 < KOGNISI_V3_NUM_HID; ++h2) {
                if (h1 != h2 && (rand() % 100 < 40)) {
                    synapses[num_synapses++] = make_syn(KOGNISI_V3_NUM_IN + h1, KOGNISI_V3_NUM_IN + h2, 0.5f);
                }
            }
        }
        for (int hid = 0; hid < KOGNISI_V3_NUM_HID; ++hid) {
            for (int out = 0; out < KOGNISI_V3_NUM_OUT; ++out) {
                synapses[num_synapses++] = make_syn(KOGNISI_V3_NUM_IN + hid, KOGNISI_V3_TOTAL_NEURONS - KOGNISI_V3_NUM_OUT + out, 1.0f);
            }
        }
    }

    __host__ __device__ void forward_step(
        float delta_predator, float delta_food, float norm_heading, float norm_energy,
        float& out_steer, float& out_speed
    ) {
        // 1. Input Sensor
        neuron_act[0] = clamp_val(delta_predator, -1.0f, 1.0f);
        neuron_act[1] = clamp_val(delta_food, -1.0f, 1.0f);
        neuron_act[2] = clamp_val(norm_heading, -1.0f, 1.0f);
        neuron_act[3] = clamp_val(norm_energy, -1.0f, 1.0f);

        float net_inputs[KOGNISI_V3_TOTAL_NEURONS];
        for (int n = 0; n < KOGNISI_V3_TOTAL_NEURONS; ++n) {
            neuron_prev_act[n] = neuron_act[n];
            net_inputs[n] = neuron_bias[n];
        }

        // 2. Propagasi Input & Recurrent ke Hidden Layer
        for (int s = 0; s < num_synapses; ++s) {
            int u = synapses[s].from_neuron;
            int v = synapses[s].to_neuron;
            if (v >= KOGNISI_V3_NUM_IN && v < KOGNISI_V3_NUM_IN + KOGNISI_V3_NUM_HID) {
                net_inputs[v] += neuron_prev_act[u] * synapses[s].weight;
            }
        }

        for (int hid = 0; hid < KOGNISI_V3_NUM_HID; ++hid) {
            int nid = KOGNISI_V3_NUM_IN + hid;
            neuron_act[nid] = act_tanh(net_inputs[nid]);
        }

        // 3. Propagasi Hidden ke Output (Tick yang sama)
        for (int s = 0; s < num_synapses; ++s) {
            int u = synapses[s].from_neuron;
            int v = synapses[s].to_neuron;
            if (v >= KOGNISI_V3_TOTAL_NEURONS - KOGNISI_V3_NUM_OUT) {
                net_inputs[v] += neuron_act[u] * synapses[s].weight;
            }
        }

        int idx_steer = KOGNISI_V3_TOTAL_NEURONS - 2;
        int idx_speed = KOGNISI_V3_TOTAL_NEURONS - 1;

        neuron_act[idx_steer] = act_tanh(net_inputs[idx_steer]);
        neuron_act[idx_speed] = act_sigmoid(net_inputs[idx_speed]);

        // 4. Update Plastisitas Lokal ABCD + Eligibility Trace O(N)
        for (int s = 0; s < num_synapses; ++s) {
            int u = synapses[s].from_neuron;
            int v = synapses[s].to_neuron;
            if (u >= 0 && u < KOGNISI_V3_TOTAL_NEURONS && v >= 0 && v < KOGNISI_V3_TOTAL_NEURONS) {
                float pre = neuron_act[u];
                float post = neuron_act[v];
                float corr = pre * post;

                // Update Trace
                synapses[s].eligibility_trace = clamp_val(0.85f * synapses[s].eligibility_trace + corr, -2.0f, 2.0f);

                // Rumus Evolusi ABCD Plasticity: dW = eta * Trace * (A*corr + B*pre + C*post + D)
                float rule = synapses[s].A * corr + synapses[s].B * pre + synapses[s].C * post + synapses[s].D;
                float dw = synapses[s].eta * synapses[s].eligibility_trace * rule;

                synapses[s].weight = clamp_val(synapses[s].weight + dw, -5.0f, 5.0f);
            }
        }

        out_steer = neuron_act[idx_steer];
        out_speed = neuron_act[idx_speed];
    }

    __host__ __device__ void reset_traces() {
        for (int s = 0; s < num_synapses; ++s) {
            synapses[s].eligibility_trace = 0.0f;
        }
    }

    __host__ void mutate(float intensity = 0.001f, float prob = 0.10f) {
        for (int s = 0; s < num_synapses; ++s) {
            if ((rand() % 100) < (prob * 100)) {
                float noise = ((rand() % 200) - 100) * intensity;
                synapses[s].weight = clamp_val(synapses[s].weight + noise, -5.0f, 5.0f);
            }
            if ((rand() % 100) < (prob * 100)) {
                synapses[s].eta = clamp_val(synapses[s].eta + (((rand() % 200) - 100) * 0.0001f), -0.05f, 0.05f);
            }
            if ((rand() % 100) < (prob * 100)) {
                synapses[s].A = clamp_val(synapses[s].A + (((rand() % 200) - 100) * 0.005f), -1.0f, 1.0f);
            }
            if ((rand() % 100) < (prob * 100)) {
                synapses[s].B = clamp_val(synapses[s].B + (((rand() % 200) - 100) * 0.005f), -1.0f, 1.0f);
            }
            if ((rand() % 100) < (prob * 100)) {
                synapses[s].C = clamp_val(synapses[s].C + (((rand() % 200) - 100) * 0.005f), -1.0f, 1.0f);
            }
            if ((rand() % 100) < (prob * 100)) {
                synapses[s].D = clamp_val(synapses[s].D + (((rand() % 200) - 100) * 0.001f), -1.0f, 1.0f);
            }
        }
        for (int i = 0; i < KOGNISI_V3_TOTAL_NEURONS; ++i) {
            if ((rand() % 100) < (prob * 100)) {
                float noise = ((rand() % 200) - 100) * intensity;
                neuron_bias[i] = clamp_val(neuron_bias[i] + noise, -1.0f, 1.0f);
            }
        }
    }

    __host__ void save_weights(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) return;
        size_t syn_count = num_synapses;
        out.write(reinterpret_cast<const char*>(&syn_count), sizeof(syn_count));
        for (int s = 0; s < num_synapses; ++s) {
            out.write(reinterpret_cast<const char*>(&synapses[s]), sizeof(SynapseV3));
        }
        for (int i = 0; i < KOGNISI_V3_TOTAL_NEURONS; ++i) {
            out.write(reinterpret_cast<const char*>(&neuron_bias[i]), sizeof(float));
        }
    }

    __host__ bool load_weights(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in.is_open()) return false;
        size_t syn_count = 0;
        in.read(reinterpret_cast<char*>(&syn_count), sizeof(syn_count));
        if (syn_count == 0 || syn_count > KOGNISI_V3_MAX_SYNAPSES) return false;
        num_synapses = static_cast<int>(syn_count);
        for (int s = 0; s < num_synapses; ++s) {
            in.read(reinterpret_cast<char*>(&synapses[s]), sizeof(SynapseV3));
            synapses[s].eligibility_trace = 0.0f; // Reset trace saat lahir
        }
        for (int i = 0; i < KOGNISI_V3_TOTAL_NEURONS; ++i) {
            in.read(reinterpret_cast<char*>(&neuron_bias[i]), sizeof(float));
        }
        return true;
    }
};
