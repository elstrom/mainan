#ifndef PARAMETER_AGENT_H
#define PARAMETER_AGENT_H

namespace ParameterAgent {
    // =========================================================================
    // PARAMETER MURNI ARSITEKTUR KOGNISI & VIRTUAL MACHINE OTAK AGEN
    // =========================================================================
    constexpr int MAX_DNA_CAPACITY = 64;            // Kapasitas Maksimum Instruksi Otak (DNA Program)
    constexpr int MAX_REGISTERS = 8;                // Kapasitas Working Memory Register (R0 - R7)
    constexpr int MIN_DYNAMIC_PROGRAM_SIZE = 8;     // Ukuran Program Awal / Minimal
    constexpr int MIN_DYNAMIC_REGISTERS = 6;        // Register Awal / Minimal
    constexpr double DNA_MUTATION_RATE = 0.25;      // Probabilitas mutasi acak pada keturunan baru (25%)
    
    // Dimensi Sensorik & State Memory
    constexpr int SENSORS_COUNT = 16;               // Kapasitas Sensorik 3 Pilar (Exteroception, Interoception, Proprioception)
    constexpr int PRED_SENSORS_COUNT = 4;           // Dimensi sensorik prediksi kausalitas waktu
    constexpr double RESERVOIR_SPECTRAL_RADIUS = 0.95;// Radius spektral bobot recurrent reservoir
    constexpr double RESERVOIR_INPUT_SCALE = 0.5;   // Skala bobot input sensor ke reservoir

    // Parameter Predictive Loss (Kausalitas Fisika Otak)
    constexpr double PREDICTIVE_LOSS_SCALE = 0.15;  // Skala koreksi error prediksi sensor ke register

    // Parameter Kuantisasi Sinyal Swarm (Bahasa Diskrit Kognisi)
    constexpr double SWARM_QUANT_THRESHOLD = 0.33;  // Ambang batas ternary quantization sinyal komunikasi

    // Parameter Hebbian Plasticity Real-Time (Sinapsis Otak)
    constexpr double HEBBIAN_LEARNING_RATE = 0.002; // Laju update bobot reservoir online saat hidup
    constexpr double HEBBIAN_DECAY = 0.0005;        // Weight decay anti-runaway Hebbian

    // Parameter Respon Takut Eksponensial/Sigmoid & Neuromodulasi Otak
    constexpr double FEAR_SIGMOID_STEEPNESS = 6.0;         // Keterjalan kurva sigmoid rasa takut
    constexpr double FEAR_SIGMOID_MIDPOINT = 0.45;         // Titik tengah pemicu lonjakan panik eksponensial
    constexpr double FEAR_NEUROMODULATION_PLASTICITY = 4.0;// Pengali lonjakan plastisitas/mutasi Hebbian saat panik
    constexpr double EPIGENETIC_TRAUMA_WEIGHT_BIAS = 0.15; // Pembobotan trauma rasa takut induk ke anak

    // Lokasi File Penyimpanan Model Otak Agen
    inline const char* DNA_STORAGE_FILE = "d:/MyProjects/mainan/agent/ingatan/best_manusia_dna.bin";
    inline const char* PREDATOR_DNA_STORAGE_FILE = "d:/MyProjects/mainan/agent/ingatan/best_predator_dna.bin";
}

#endif // PARAMETER_AGENT_H
