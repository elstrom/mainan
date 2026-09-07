#pragma once

namespace DuniaFisika {
    // Dimensi Dunia 2D Ekosistem (Meter)
    constexpr double WORLD_WIDTH = 1000.0;
    constexpr double WORLD_HEIGHT = 1000.0;

    // Konstanta Fisika & Medan
    constexpr double FRICTION_COEFF = 0.88;          // Redaman inersia gerak agen
    constexpr double MAX_AGENT_SPEED = 12.0;         // Kecepatan gerak maksimum (m/s)
    constexpr double MAGNETIC_FIELD_BASE = 1.0;      // Intensitas medan magnet dasar
    constexpr double WIND_FORCE_MULT = 0.2;          // Pengaruh dorongan medan angin terhadap gerak agen

    // Konfigurasi Mode Eksekusi & Headless Benchmark
    constexpr bool HEADLESS_MODE = true;            // True: Matikan UI Web/Browser untuk percepatan maksimal (~1000x)
    constexpr bool STOP_ON_EXTINCTION = true;        // True: Berhenti otomatis dan laporkan hasil saat salah satu kubu punah
    constexpr double TIME_ACCELERATION_FACTOR = 2.0; // 2x Waktu Nyata
    constexpr double SECONDS_PER_DAY = 60.0;         // 60 detik simulasi = 1 hari ekosistem
    constexpr int DAYS_PER_SEASON = 15;              // 15 hari = 1 musim
    constexpr int SEASONS_COUNT = 4;                 // 0: Semi, 1: Panas, 2: Gugur, 3: Dingin

    // Parameter Pohon & Buah (Sistem Siklus Hidup & Ketergantungan Rumus Kognisi)
    constexpr int MAX_TREES = 32;                    // Jumlah pohon dalam ekosistem
    constexpr double TREE_INTERACTION_RADIUS = 50.0; // Radius interaksi kognisi agen ke pohon (m)
    constexpr double MAX_FRUIT_PER_TREE = 6.0;       // Kapasitas buah maksimal per pohon
    constexpr double FRUIT_NUTRITION_ENERGY = 15.0;  // Energi per buah saat dikonsumsi (%)
    constexpr double TREE_ENERGY_POOL_RATE = 5.0;    // Pool energi fotosintesis pohon per detik (dibagi rata ke semua agen di sekitar pohon)
    constexpr double TREE_MAX_AGE_YEARS = 8.0;       // Usia maksimal pohon (mati tiap beberapa tahun)
    constexpr double TREE_HEALTH_DECAY = 0.5;        // Penurunan kesehatan/vitalitas per detik jika tanpa rumus
    constexpr double TREE_RESONANCE_BOOST = 0.8;     // Laju pertumbuhan saat rumus kognisi agen selaras
    constexpr double FRUIT_SPAWN_RATE = 0.45;        // Kecepatan pembuahan pohon saat resonan
    constexpr double TREE_RESONANCE_THRESHOLD = 0.25;// Batas minimal resonansi rumus agar pohon bisa berbuah dan hidup
    constexpr double OVERCROWDING_RADIUS = 20.0;     // Radius kepadatan populasi (koloni padat memicu kompetisi & stres)
    constexpr double OVERCROWDING_PENALTY_MULT = 0.05;// Peningkatan metabolisme per agen tetangga di area padat

    // Parameter Predator Agen Musuh / Faksi B (Apex Cognitive Hunter)
    constexpr int INITIAL_PREDATORS = 200;           // Populasi awal Faksi B (Predator/Karnivora)
    constexpr int MAX_PREDATORS_BUFFER = 1000;      // Kapasitas buffer memori GPU Faksi B (optimal & cepat)
    constexpr double PREDATOR_SPEED = 11.5;          // Kecepatan gerak predator setara & kompetitif dengan mangsa (12.0)
    constexpr double PREDATOR_ATTACK_RADIUS = 16.0;  // Radius jangkauan serangan predator ke mangsa
    constexpr double PREDATOR_DAMAGE_RATE = 20.0;    // Kerusakan energi per detik pada mangsa
    constexpr double PREDATOR_ENERGY_GAIN = 18.0;    // Energi predator bertambah saat berhasil melukai/memangsa
    constexpr double PREDATOR_INITIAL_ENERGY = 100.0; // Energi awal predator
    constexpr double PREDATOR_METABOLISM = 0.12;    // Konsumsi energi dasar predator per detik (setara & seimbang dengan herbivora)
    constexpr double PREDATOR_MATING_RADIUS = 28.0; // Jarak perkawinan Faksi B (♂ + ♀)
    constexpr double PREDATOR_MATING_MIN_ENERGY = 40.0; // Ambang batas reproduksi karnivora (adaptasi seimbang)
    constexpr double PREDATOR_MATING_COST = 15.0;   // Energi melahirkan anak predator baru
    constexpr double PREDATOR_MATING_COOLDOWN = 5.0; // Jeda waktu kawin predator (detik)
    constexpr double HERBIVORE_DEFENSE_REDUCTION = 0.90; // Reduksi kerusakan hingga 90% saat rumus kognisi selaras/berhasil menangkis
    constexpr double HERBIVORE_COUNTER_BASE_DAMAGE = 18.0; // Damage serangan balik dasar herbivora saat kognisi unggul
    constexpr double PRIMITIVE_TOOL_DAMAGE_BONUS = 25.0;  // Bonus damage tombak/alat batu purba terhadap predator
    constexpr double PRIMITIVE_SHIELD_DEFENSE_BONUS = 0.50; // Bonus ketahanan perisai batu/alat purba
    constexpr double MINERAL_SANCTUARY_RADIUS = 45.0;     // Radius medan pelindung/sanctuary di sekitar mineral kristal/logam
    constexpr double MINERAL_FIELD_SLOW_FACTOR = 0.45;    // Predator melambat 55% saat masuk ke zona medan mineral resonance

    // Parameter Siklus Siang & Malam (Diurnal Cycle & Multi-Fisika)
    constexpr double DAY_NIGHT_CYCLE_SECONDS = 60.0; // 60 detik = 1 siklus penuh (30s Siang, 30s Malam)
    constexpr double DAYLIGHT_TEMP_BOOST = 8.0;     // Suhu naik saat siang terik (+8°C)
    constexpr double NIGHT_TEMP_DROP = -8.0;        // Suhu turun saat malam dingin (-8°C)
    constexpr double NIGHT_VISIBILITY_FACTOR = 0.45;// Jarak pandang sensorik menyusut di malam hari
    constexpr double DAY_PHOTOSYNTHESIS_MULT = 1.6; // Pohon memproduksi energi 1.6x lebih aktif di siang hari

    // Parameter Komunikasi Akustik / Feromon Gelombang Antar-Agen
    constexpr double BROADCAST_COMM_RADIUS = 40.0;  // Radius transmisi sinyal komunikasi antar-agen
    constexpr double BROADCAST_ENERGY_COST = 0.05;  // Biaya energi kecil saat memancarkan pesan akustik

    // Parameter Bias Intervensi Keseimbangan Koloni (50 = Simetris 1.0x : 1.0x)
    constexpr double DEFAULT_GROWTH_BIAS = 50.0;

    // Parameter Mutasi Ekstrim & Peluang Kelahiran Cacat / Super Acak
    constexpr double SUPER_MUTATION_CHANCE = 0.35;    // 35% peluang mutasi acak radikal pada bayi baru lahir
    constexpr double DEFECTIVE_BIRTH_CHANCE = 0.12;   // 12% kemungkinan lahir dengan cacat biologis (metabolisme boros / energi lemah / program otak acak)

    // Parameter Material Periodik Dunia (Mineral, Logam Konduktif, Kristal Energi)
    constexpr int MAX_PERIODIC_DEPOSITS = 16;         // Jumlah formasi deposit material di dunia
    constexpr double MINERAL_HARDNESS_DECAY = 0.4;    // Kecepatan hancur/degradasi material saat dieksploitasi
    constexpr double MATERIAL_REGEN_RATE = 0.15;      // Pembentukan kembali kristal mineral secara periodik

    // Parameter Atmosfer, Gas Rumah Kaca & Unsur Kimia Ekosistem (O2, CO2, H2O, N2)
    constexpr double OXYGEN_BASE_LEVEL = 21.0;        // Kadar Oksigen dasar atmosfer (21.0%)
    constexpr double CO2_BASE_LEVEL = 0.04;           // Kadar Karbon Dioksida dasar (0.04%)
    constexpr double H2O_HUMIDITY_BASE_LEVEL = 60.0;  // Kelembaban Uap Air dasar (60.0%)
    constexpr double NITROGEN_BASE_LEVEL = 78.0;      // Kadar Nitrogen pembawa dasar (78.0%)
    constexpr double PHOTOSYNTHESIS_O2_RATE = 0.08;   // Produksi O2 oleh fotosintesis pohon aktif di siang hari
    constexpr double TREE_RESPIRATION_CO2_RATE = 0.03;// Emisi CO2 oleh pohon saat respirasi malam hari
    constexpr double RESPIRATION_O2_CONSUMPTION = 0.005; // Konsumsi O2 per agen hidup per detik
    constexpr double RESPIRATION_CO2_EMISSION = 0.004;   // Emisi CO2 per respirasi agen hidup
    constexpr double GREENHOUSE_CO2_WARMING_FACTOR = 3.0; // Peningkatan suhu akibat efek rumah kaca gas CO2 (seimbang & stabil)
    constexpr double GREENHOUSE_H2O_WARMING_FACTOR = 0.01; // Pengaruh retensi panas oleh kelembaban uap air H2O

    // Parameter Friendly Fire & Kanibalisme/Dekomposisi Mayat (Corpse Scavenging)
    constexpr double FRIENDLY_FIRE_DAMAGE_RATE = 8.0;   // Kerusakan akibat gesekan/benturan tak sengaja sesama faksi
    constexpr double CORPSE_MAX_ENERGY_RESERVE = 30.0;  // Cadangan nutrisi bio-organik pada mayat
    constexpr double CORPSE_DECAY_RATE = 2.0;          // Kecepatan dekomposisi mayat per detik
    constexpr double CARNIVORE_CORPSE_EAT_RATE = 15.0;  // Laju pemangsaan mayat oleh faksi karnivora
    constexpr double CORPSE_SCAVENGE_RADIUS = 22.0;     // Jarak deteksi mayat oleh karnivora

    // Parameter Seven Deadly Sins (Tujuh Dosa Pokok Biologis-Kognitif)
    enum DeadlySinType {
        SIN_PRIDE = 0,     // Superbia: Kesombongan (Soliter, Shield/Formula Maks, Tolak Berbagi)
        SIN_GREED = 1,     // Avaritia: Ketamakan (Hoarding Buah / Over-killing Mangsa)
        SIN_LUST = 2,      // Luxuria: Hawa Nafsu (Kawin Cepat, Ambang Rendah, Risiko Mutasi Tinggi)
        SIN_ENVY = 3,      // Invidia: Kedengkian (Gangguan Sinyal Alpha & Rebutan Target)
        SIN_GLUTTONY = 4,  // Gula: Kerakusan (Makan 2x Laju, Energi Besar, Massa Berat)
        SIN_WRATH = 5,     // Ira: Kemurkaan (Frenzy Hunt + Counter-attack Kognitif Tinggi)
        SIN_SLOTH = 6      // Acedia: Kemalasan (Hemat Energi, Gerak Lambat, Mengintai Pasif)
    };
    constexpr int SINS_COUNT = 7;
    constexpr double SIN_WRATH_SPEED_BOOST = 1.30;       // Peningkatan kecepatan +30% saat mode Wrath Frenzy
    constexpr double SIN_GLUTTONY_EAT_MULT = 2.0;        // Laju makan 2x lipat pada sin Gluttony
    constexpr double SIN_LUST_COOLDOWN_MULT = 0.5;       // Waktu jeda kawin 50% lebih singkat pada sin Lust
    constexpr double SIN_SLOTH_METABOLISM_SAVING = 0.6;  // Konsumsi metabolisme 40% lebih hemat pada sin Sloth
    constexpr double SIN_PRIDE_RESONANCE_BONUS = 1.25;   // Bonus +25% kekuatan perisai/rumus pada sin Pride
}