#pragma once
#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>
#include <algorithm>

// =============================================================================
// ARSITEKTUR NEUROMORPHIC EPIGENETIC DNA DENGAN KESADARAN MORTALITAS (FEAR OF DEATH)
// Kesadaran batas waktu hidup -> Hormon Stres/Panik -> Epigenetic Imprint ke DNA
// =============================================================================

struct Synapse {
    int from_neuron;
    int to_neuron;
    double weight;
    double eligibility_trace;
    double plasticity_rate;
    double epigenetic_methylation; // [0.0 - 1.0] Mengunci kestabilan sinapsis di DNA
};

struct Neuron {
    int id;
    std::string label;
    double activation;
    double prev_activation;
    double bias;
    float x, y;
};

// Genotipe DNA yang diwariskan lintas generasi
struct EpigeneticDNA {
    std::vector<double> base_weights;
    std::vector<double> plasticity_genes;
    std::vector<double> methylation_tags;
    double survival_drive;
    int generation_count;
};

class AutonomousFlightMind {
public:
    static const int NUM_INPUTS = 4;   // Sensor: h (alt), v (vel), d (moon dist), t_lifespan (sisa waktu hidup)
    static const int NUM_HIDDEN = 7;   // Hidden neurons (termasuk 1 Neuromodulator/Stress Center)
    static const int NUM_OUTPUTS = 1;  // Motor: Thrust roket
    static const int TOTAL_NEURONS = NUM_INPUTS + NUM_HIDDEN + NUM_OUTPUTS;

    std::vector<Neuron> neurons;
    std::vector<Synapse> synapses;
    EpigeneticDNA dna;

    double best_fitness;
    double current_fitness;
    double mortality_stress; // Tingkat kecemasan mendekati ajal / crash [0.0 - 1.0]
    int total_flights;
    int successful_moon_landings;
    int crashes;
    int structural_mutations;
    std::string flight_log;

    static inline const char* MEMORY_FILE = "d:/MyProjects/mainan/agent/ingatan/neuromorphic_epigenetic_dna.bin";

    AutonomousFlightMind() {
        best_fitness = -1.0;
        current_fitness = 0.0;
        mortality_stress = 0.0;
        total_flights = 0;
        successful_moon_landings = 0;
        crashes = 0;
        structural_mutations = 0;
        dna.generation_count = 1;
        dna.survival_drive = 1.0;
        init_flight_mind();
    }

    void build_network_topology() {
        neurons.resize(TOTAL_NEURONS);
        synapses.clear();

        // Label neuron input (dengan kesadaran batas waktu hidup 'Lifespan')
        neurons[0] = {0, "h", 0.0, 0.0, 0.0, 0.0f, 0.0f};
        neurons[1] = {1, "v", 0.0, 0.0, 0.0, 0.0f, 0.0f};
        neurons[2] = {2, "d", 0.0, 0.0, 0.0, 0.0f, 0.0f};
        neurons[3] = {3, "Life", 0.0, 0.0, 0.0, 0.0f, 0.0f}; // Kesadaran batas waktu

        // Hidden neurons + Amygdala/Fear Center (N7)
        for (int i = 0; i < NUM_HIDDEN - 1; ++i) {
            neurons[NUM_INPUTS + i] = {NUM_INPUTS + i, "N" + std::to_string(i + 1), 0.0, 0.0, ((rand() % 200) - 100) * 0.005, 0.0f, 0.0f};
        }
        neurons[NUM_INPUTS + NUM_HIDDEN - 1] = {NUM_INPUTS + NUM_HIDDEN - 1, "Fear", 0.0, 0.0, 0.1, 0.0f, 0.0f};

        // Output neuron (Thrust)
        neurons[TOTAL_NEURONS - 1] = {TOTAL_NEURONS - 1, "Thrust", 0.0, 0.0, 0.0, 0.0f, 0.0f};

        // Hubungkan Input -> Hidden
        for (int in = 0; in < NUM_INPUTS; ++in) {
            for (int hid = 0; hid < NUM_HIDDEN; ++hid) {
                double init_w = ((rand() % 200) - 100) * 0.01;
                double lr = 0.003 + (rand() % 10) * 0.001;
                synapses.push_back({in, NUM_INPUTS + hid, init_w, 0.0, lr, 0.0});
            }
        }

        // Hubungkan Hidden -> Output
        for (int hid = 0; hid < NUM_HIDDEN; ++hid) {
            double init_w = ((rand() % 200) - 100) * 0.01;
            double lr = 0.003 + (rand() % 10) * 0.001;
            synapses.push_back({NUM_INPUTS + hid, TOTAL_NEURONS - 1, init_w, 0.0, lr, 0.0});
        }

        // Lateral Recurrent Connections
        for (int h1 = 0; h1 < NUM_HIDDEN; ++h1) {
            for (int h2 = 0; h2 < NUM_HIDDEN; ++h2) {
                if (h1 != h2 && (rand() % 2 == 0)) {
                    double init_w = ((rand() % 100) - 50) * 0.005;
                    synapses.push_back({NUM_INPUTS + h1, NUM_INPUTS + h2, init_w, 0.0, 0.001, 0.0});
                }
            }
        }
    }

    void save_permanent_memory() {
        std::ofstream out(MEMORY_FILE, std::ios::binary);
        if (!out.is_open()) return;

        out.write(reinterpret_cast<const char*>(&best_fitness), sizeof(best_fitness));
        out.write(reinterpret_cast<const char*>(&total_flights), sizeof(total_flights));
        out.write(reinterpret_cast<const char*>(&successful_moon_landings), sizeof(successful_moon_landings));
        out.write(reinterpret_cast<const char*>(&crashes), sizeof(crashes));
        out.write(reinterpret_cast<const char*>(&structural_mutations), sizeof(structural_mutations));
        out.write(reinterpret_cast<const char*>(&dna.generation_count), sizeof(dna.generation_count));
        out.write(reinterpret_cast<const char*>(&dna.survival_drive), sizeof(dna.survival_drive));

        size_t num_synapses = synapses.size();
        out.write(reinterpret_cast<const char*>(&num_synapses), sizeof(num_synapses));
        for (const auto& syn : synapses) {
            out.write(reinterpret_cast<const char*>(&syn), sizeof(Synapse));
        }
        out.close();
    }

    bool load_permanent_memory() {
        std::ifstream in(MEMORY_FILE, std::ios::binary);
        if (!in.is_open()) return false;

        in.read(reinterpret_cast<char*>(&best_fitness), sizeof(best_fitness));
        in.read(reinterpret_cast<char*>(&total_flights), sizeof(total_flights));
        in.read(reinterpret_cast<char*>(&successful_moon_landings), sizeof(successful_moon_landings));
        in.read(reinterpret_cast<char*>(&crashes), sizeof(crashes));
        in.read(reinterpret_cast<char*>(&structural_mutations), sizeof(structural_mutations));
        in.read(reinterpret_cast<char*>(&dna.generation_count), sizeof(dna.generation_count));
        in.read(reinterpret_cast<char*>(&dna.survival_drive), sizeof(dna.survival_drive));

        size_t num_synapses = 0;
        in.read(reinterpret_cast<char*>(&num_synapses), sizeof(num_synapses));
        if (num_synapses == 0) return false;

        synapses.resize(num_synapses);
        for (size_t i = 0; i < num_synapses; ++i) {
            in.read(reinterpret_cast<char*>(&synapses[i]), sizeof(Synapse));
        }
        in.close();

        // Terapkan DNA epigenetik ke kondisi lahir individu
        dna.base_weights.resize(synapses.size());
        dna.plasticity_genes.resize(synapses.size());
        dna.methylation_tags.resize(synapses.size());
        for (size_t i = 0; i < synapses.size(); ++i) {
            dna.base_weights[i] = synapses[i].weight;
            dna.plasticity_genes[i] = synapses[i].plasticity_rate;
            dna.methylation_tags[i] = synapses[i].epigenetic_methylation;
        }

        flight_log = "[DNA MEMORY INHERITED] Gen-" + std::to_string(dna.generation_count) + 
                     " Rekor: " + std::to_string(static_cast<int>(best_fitness / 1000.0)) + " km";
        return true;
    }

    void init_flight_mind() {
        build_network_topology();
        load_permanent_memory();
    }

    // Dynamic Online Neuromorphic Forward Pass & Cortisol/Stress Epigenetics
    double get_thrust(double h, double v, double d, double lifespan_remaining_ratio = 1.0) {
        // Sensor kesadaran batas waktu hidup (0.0 = sekarat/habis waktu, 1.0 = muda)
        neurons[0].activation = h;
        neurons[1].activation = v;
        neurons[2].activation = d;
        neurons[3].activation = 1.0 - lifespan_remaining_ratio; // Kesadaran urgensi waktu

        // Tingkat stres mortalitas melonjak jika waktu menipis atau roket melambat di gravitasi
        mortality_stress = std::clamp((1.0 - lifespan_remaining_ratio) * 1.5 + (v < 0 ? 0.3 : 0.0), 0.0, 1.0);

        // Reset buffer net inputs
        std::vector<double> net_inputs(TOTAL_NEURONS, 0.0);
        for (int i = 0; i < TOTAL_NEURONS; ++i) {
            neurons[i].prev_activation = neurons[i].activation;
            net_inputs[i] = neurons[i].bias;
        }

        // Propagasi sinyal sinapsis
        for (const auto& syn : synapses) {
            net_inputs[syn.to_neuron] += neurons[syn.from_neuron].prev_activation * syn.weight;
        }

        // Aktivasi Fear Center
        neurons[NUM_INPUTS + NUM_HIDDEN - 1].activation = std::tanh(net_inputs[NUM_INPUTS + NUM_HIDDEN - 1] + mortality_stress * 2.0);

        // Update aktivasi hidden interneurons
        for (int hid = 0; hid < NUM_HIDDEN - 1; ++hid) {
            int idx = NUM_INPUTS + hid;
            neurons[idx].activation = std::tanh(net_inputs[idx]);
        }
        neurons[TOTAL_NEURONS - 1].activation = 1.0 / (1.0 + std::exp(-net_inputs[TOTAL_NEURONS - 1]));

        // Local Hebbian Synaptic Plasticity dimodulasi hormon stres (Neuroplastic Surge saat terdesak)
        double stress_surge = 1.0 + mortality_stress * 1.8;
        for (auto& syn : synapses) {
            double pre = neurons[syn.from_neuron].activation;
            double post = neurons[syn.to_neuron].activation;
            syn.eligibility_trace = 0.95 * syn.eligibility_trace + (pre * post);
            
            // Epigenetic Resistance: Sinapsis yang termetilasi kuat lebih tahan terhadap modifikasi acak
            double effective_lr = syn.plasticity_rate * stress_surge * (1.0 - syn.epigenetic_methylation * 0.7);
            syn.weight += effective_lr * (syn.eligibility_trace - 0.01 * syn.weight);
            syn.weight = std::clamp(syn.weight, -10.0, 10.0);
        }

        return neurons[TOTAL_NEURONS - 1].activation;
    }

    std::string get_current_formula_string() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "Gen-" << dna.generation_count << " | Fear/Stress: " << (mortality_stress * 100.0) << "% | Thrust: " << neurons.back().activation;
        return ss.str();
    }

    // Pewarisan Epigenetik DNA ke Generasi Berikutnya
    void inherit_and_birth_next_generation(double flight_distance_reached) {
        total_flights++;
        dna.generation_count++;

        if (flight_distance_reached > best_fitness) {
            best_fitness = flight_distance_reached;
            // Rekor baru -> Kunci (Metilasi) jalur sinapsis unggul ke DNA agar diwariskan permanen
            for (auto& syn : synapses) {
                syn.epigenetic_methylation = std::clamp(syn.epigenetic_methylation + 0.15, 0.0, 0.95);
            }
            save_permanent_memory();
            flight_log = "[EPIGENETIC SEALED] Rekor Gen-" + std::to_string(dna.generation_count) + 
                         ": " + std::to_string(static_cast<int>(best_fitness / 1000.0)) + " km";
        } else {
            // Adaptasi mandiri: Rekombinasi genetik dengan bobot dasar DNA yang dipengaruhi stres hidup sebelumnya
            for (auto& syn : synapses) {
                if ((rand() % 100) < 20) {
                    double mutation_intensity = (1.0 - syn.epigenetic_methylation) * 0.05;
                    syn.weight += ((rand() % 200) - 100) * mutation_intensity;
                    syn.plasticity_rate = std::clamp(syn.plasticity_rate + ((rand() % 100) - 50) * 0.0001, 0.0005, 0.05);
                }
            }
        }
    }

    void on_flight_completed(double flight_distance_reached) {
        inherit_and_birth_next_generation(flight_distance_reached);
    }

    void calculate_layout(float center_x, float center_y, float width, float height) {
        float in_x = center_x - width * 0.40f;
        for (int i = 0; i < NUM_INPUTS; ++i) {
            neurons[i].x = in_x;
            neurons[i].y = center_y - height * 0.40f + i * (height * 0.80f / (NUM_INPUTS - 1));
        }

        float hid_x = center_x;
        for (int i = 0; i < NUM_HIDDEN; ++i) {
            neurons[NUM_INPUTS + i].x = hid_x;
            neurons[NUM_INPUTS + i].y = center_y - height * 0.45f + i * (height * 0.90f / (NUM_HIDDEN - 1));
        }

        neurons[TOTAL_NEURONS - 1].x = center_x + width * 0.40f;
        neurons[TOTAL_NEURONS - 1].y = center_y;
    }
};
