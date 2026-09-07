#pragma once

namespace ParameterAgent {
    // Parameter Kognisi Dinamis & Neuroplastisitas (Dynamic VM & Elastic Working Memory)
    constexpr int MAX_DNA_CAPACITY = 128;           // Kapasitas Maksimum Instruksi Otak
    constexpr int MAX_REGISTERS = 16;               // Kapasitas Maksimum Working Memory Register (R0 - R15)
    constexpr int MIN_DYNAMIC_PROGRAM_SIZE = 8;     // Ukuran Program Awal / Minimal
    constexpr int MIN_DYNAMIC_REGISTERS = 6;        // Register Awal / Minimal
    constexpr int TRAJECTORY_SAMPLES = 32;          // Jumlah Sample Lintasan Trayektori

    // Parameter Populasi Dinamis & Perlindungan VRAM GPU
    constexpr int MIN_POPULATION_SIZE = 64;         // Populasi Minimum
    constexpr int MAX_POPULATION_SIZE = 4096;       // Kapasitas Aman VRAM GPU Maksimum (Cap VRAM)
    constexpr int DEFAULT_POPULATION_SIZE = 512;    // Ukuran Populasi Awal Default

    // Parameter Efisiensi Kognitif & Penalti Kompleksitas
    constexpr double PENALTY_PER_INSTRUCTION = 5.0; // Penalti per baris instruksi untuk mencegah bloatware genetik
    constexpr double PENALTY_PER_REGISTER = 10.0;   // Penalti per alokasi register yang tidak perlu

    // Parameter Kausalitas Waktu & Temporal Credit Assignment (Alami Tanpa Injeksi)
    constexpr float TEMPORAL_TRACE_DECAY = 0.95f;   // Peluruhan jejak kausalitas waktu per tick kognisi
    constexpr float METHYLATION_PENALTY_RATE = 0.25f; // Bobot penalti epigenetik pada gen yang aktif saat peristiwa fatal

    // Lokasi File Penyimpanan Permanen Otak Agen
    inline const char* DNA_STORAGE_FILE = "d:/MyProjects/mainan/agent/ingatan/best_evolved_dna.bin";
    inline const char* FAILED_REGISTRY_FILE = "d:/MyProjects/mainan/agent/catatan/failed_dna_registry.json";
}

