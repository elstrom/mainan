#pragma once
#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>
#include "dunia/parameter_dunia.h"
#include "ingatan/parameter_agent.h"

#define DNA_PROGRAM_SIZE ParameterAgent::MAX_DNA_CAPACITY
#define REGISTERS_COUNT ParameterAgent::MAX_REGISTERS

// Virtual Machine Bytecode Operasi Aljabar & Kognisi
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

// =========================================================================
// EKSEKUTOR VIRTUAL MACHINE KOGNISI DNA (MODULAR AGENT BRAIN)
// =========================================================================
template <typename AgentType>
__device__ inline void execute_agent_brain_vm(
    AgentType& ag,
    const float* sensor_inputs,
    float ambient_signal,
    double dt,
    float& out_move_x,
    float& out_move_y,
    float& out_broadcast
) {
    int max_r = (ag.active_registers_count > 0 && ag.active_registers_count <= REGISTERS_COUNT) ? 
                ag.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;
    int active_prog = (ag.active_program_size > 0 && ag.active_program_size <= DNA_PROGRAM_SIZE) ? 
                      ag.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

    // 1. Reservoir State Update dengan Recurrent Loop
    for (int r = 0; r < max_r; ++r) {
        float in_signal = ag.reservoir_weights_in[r] * sensor_inputs[r % ParameterAgent::SENSORS_COUNT] +
                          ag.reservoir_weights_in[(r + 8) % REGISTERS_COUNT] * sensor_inputs[(r + 8) % ParameterAgent::SENSORS_COUNT];
        float rec_signal = ag.reservoir_weights_rec[r] * (float)ag.prev_registers[r];
        double u_val = in_signal + rec_signal;
        ag.registers[r] = (1.0 - ParameterAgent::RESERVOIR_SPECTRAL_RADIUS) * ag.registers[r] + 
                          ParameterAgent::RESERVOIR_SPECTRAL_RADIUS * tanh(u_val);
    }

    // 2. Predictive Loss: Koreksi register khusus sensor/prediksi berdasarkan error prediksi sensor t-1 → t
    float pred_sensors_now[ParameterAgent::PRED_SENSORS_COUNT] = { sensor_inputs[0], sensor_inputs[1], sensor_inputs[10], sensor_inputs[5] };
    for (int r = 0; r < ParameterAgent::PRED_SENSORS_COUNT && r < max_r; ++r) {
        float pred_error = pred_sensors_now[r] - ag.pred_sensor_prev[r];
        ag.registers[r] = gpu_clamp(ag.registers[r] + ParameterAgent::PREDICTIVE_LOSS_SCALE * pred_error, -5.0, 5.0);
    }
    ag.pred_sensor_prev[0] = sensor_inputs[0];
    ag.pred_sensor_prev[1] = sensor_inputs[1];
    ag.pred_sensor_prev[2] = sensor_inputs[10];
    ag.pred_sensor_prev[3] = sensor_inputs[5];

    out_move_x = 0.0f;
    out_move_y = 0.0f;
    out_broadcast = 0.0f;

    // 3. Eksekusi Virtual Machine Bytecode DNA
    for (int ip = 0; ip < active_prog; ++ip) {
        const auto& inst = ag.dna_program[ip];
        int rd = inst.r_dest % max_r;
        int rs1 = inst.r_src1 % max_r;
        int rs2 = inst.r_src2 % max_r;

        switch (inst.op % 19) {
            case OP_NOP: break;
            case OP_LOAD_SENSOR: {
                int s = inst.r_src1 % ParameterAgent::SENSORS_COUNT;
                ag.registers[rd] = sensor_inputs[s];
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
                out_move_x += (float)tanh(ag.registers[rs1]);
                out_move_y += (float)tanh(ag.registers[rs2]);
                break;
            }
            case OP_ACTION_FORMULA: {
                out_broadcast = (float)tanh(ag.registers[rs1]);
                ag.comm_signal = out_broadcast;
                break;
            }
            case OP_RESONATE_CLIMATE: {
                ag.registers[rd] = (float)tanh(ag.registers[rs1] * ambient_signal);
                break;
            }
            case OP_FORK_NEURON: {
                if (ag.active_program_size < DNA_PROGRAM_SIZE && ag.energy > 75.0f) {
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
                int target_ip = (int)fabs(ag.registers[rs1]) % DNA_PROGRAM_SIZE;
                unsigned char new_op = (unsigned char)((int)fabs(ag.registers[rs2]) % 19);
                ag.dna_program[target_ip].op = new_op;
                ag.dna_program[target_ip].r_dest = (unsigned char)((int)fabs(ag.registers[rd]) % max_r);
                break;
            }
            case OP_MUTATE_SELF: {
                if (ag.energy > 40.0f) {
                    int target_ip = (int)fabs(ag.registers[rs1]) % active_prog;
                    float mod_val = (float)ag.registers[rs2] * 0.1f;
                    ag.dna_program[target_ip].immediate_val += mod_val;
                    ag.energy -= 0.05f;
                }
                break;
            }
            case OP_ALLOC_REG: {
                if (ag.active_registers_count < REGISTERS_COUNT && ag.energy > 60.0f) {
                    ag.active_registers_count++;
                    ag.energy -= 0.1f;
                }
                break;
            }
            case OP_FREE_REG: {
                if (ag.active_registers_count > ParameterAgent::MIN_DYNAMIC_REGISTERS) {
                    ag.active_registers_count--;
                }
                break;
            }
        }
    }

    out_move_x = (float)gpu_clamp(out_move_x, -1.0, 1.0);
    out_move_y = (float)gpu_clamp(out_move_y, -1.0, 1.0);

    // 4. Neuromodulasi & Hebbian Plasticity Online
    float plasticity_neuromod = 1.0f + (float)ParameterAgent::FEAR_NEUROMODULATION_PLASTICITY * sensor_inputs[12];
    for (int r = 0; r < max_r; ++r) {
        float hebb_delta = (float)(ParameterAgent::HEBBIAN_LEARNING_RATE * plasticity_neuromod * sensor_inputs[r % ParameterAgent::SENSORS_COUNT] * ag.registers[r]);
        ag.reservoir_weights_in[r] += hebb_delta - (float)(ParameterAgent::HEBBIAN_DECAY * ag.reservoir_weights_in[r]);
        if (ag.reservoir_weights_in[r] > 1.0f) ag.reservoir_weights_in[r] = 1.0f;
        if (ag.reservoir_weights_in[r] < -1.0f) ag.reservoir_weights_in[r] = -1.0f;
        ag.prev_registers[r] = ag.registers[r];
    }
}
