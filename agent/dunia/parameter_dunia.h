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
    constexpr bool STOP_ON_EXTINCTION = true;        // True: Tangani kepunahan saat salah satu kubu punah
    constexpr bool AUTO_RESTART_ON_EXTINCTION = true;// True: Infinite loop (otomatis restart simulasi baru setelah lapor kepunahan)
    inline const char* EVOLUTION_LOG_FILE = "d:/MyProjects/mainan/agent/ingatan/evolution_milestones.log"; // File log milestone rekor generasi
    constexpr double TIME_ACCELERATION_FACTOR = 2.0; // 2x Waktu Nyata
    constexpr double SECONDS_PER_DAY = 60.0;         // 60 detik simulasi = 1 hari ekosistem
    constexpr int DAYS_PER_SEASON = 15;              // 15 hari = 1 musim
    constexpr int SEASONS_COUNT = 4;                 // 0: Semi, 1: Panas, 2: Gugur, 3: Dingin

    // Parameter Geologi & Pola Kesuburan Tanah (Multi-Frekuensi Kontur Tanah)
    constexpr double SOIL_FERTILITY_BASE = 1.0;       // Kesuburan tanah dasar
    constexpr double SOIL_FERTILITY_VARIATION = 0.6;  // Variasi sinusoidal geologi (zona subur vs gersang)
    constexpr double SOIL_NOISE_SCALE_X = 0.008;      // Skala spasial kontur tanah X
    constexpr double SOIL_NOISE_SCALE_Y = 0.008;      // Skala spasial kontur tanah Y

    // Parameter Hidrologi & Geografi Sungai Dinamis (Meandering River System)
    constexpr double RIVER_CENTER_X = 500.0;          // Pusat jalur sungai X
    constexpr double RIVER_MEANDER_AMP = 140.0;       // Amplitudo kelokan meander sungai
    constexpr double RIVER_MEANDER_FREQ = 0.007;      // Frekuensi kelokan sungai
    constexpr double RIVER_WIDTH = 45.0;              // Lebar aliran sungai utama (m)
    constexpr double RIVER_DRINK_RADIUS = 30.0;       // Radius interaksi tepi sungai untuk minum air
    constexpr double RIVER_MOISTURE_RADIUS = 120.0;   // Radius kelembaban tanah di sekitar bantaran sungai

    // Parameter Dehidrasi, Hidrasi & Metabolisme Air Agen (Kubu A & Kubu B)
    constexpr double AGENT_HYDRATION_MAX = 100.0;     // Kapasitas hidrasi tubuh maksimum (100%)
    constexpr double AGENT_HYDRATION_INITIAL = 100.0; // Tingkat hidrasi awal saat lahir
    constexpr double AGENT_DEHYDRATION_BASE_RATE = 0.25; // Pengurangan hidrasi dasar per detik
    constexpr double AGENT_DEHYDRATION_HEAT_MULT = 0.04; // Percepatan dehidrasi saat suhu panas > 25°C
    constexpr double AGENT_DRINK_WATER_RATE = 35.0;   // Laju minum/mengisi air di tepi sungai (% per detik)
    constexpr double DEHYDRATION_THIRST_THRESHOLD = 25.0; // Ambang rasa haus memicu stres
    constexpr double DEHYDRATION_DAMAGE_RATE = 20.0;  // Kerusakan energi fatal saat dehidrasi mencapai 0%

    // Parameter Realisme Pohon: Kebutuhan Air, Tahap Tanam & Penyerapan
    constexpr double TREE_MOISTURE_MAX = 100.0;       // Cadangan air pohon (100%)
    constexpr double TREE_MOISTURE_INITIAL = 70.0;    // Cadangan air bibit awal
    constexpr double TREE_WATER_CONSUMPTION_RATE = 0.30; // Konsumsi air per detik untuk fotosintesis
    constexpr double TREE_WATER_ABSORB_RIVER_RATE = 1.2; // Penyerapan air dari bantaran sungai per detik
    constexpr double TREE_WATER_ABSORB_RAIN_RATE = 0.8;  // Penyerapan air dari kelembaban atmosfer/hujan
    constexpr double TREE_DROUGHT_DECAY_RATE = 4.0;   // Kerusakan kesehatan pohon saat mengalami kekeringan (Moisture <= 0)
    constexpr double SEED_GERMINATION_ENERGY_COST = 20.0; // Biaya energi agen saat menanam benih (15% energi + 10% hidrasi)
    constexpr double SEED_GERMINATION_WATER_COST = 12.0;  // Air yang disiramkan agen saat menanam benih

    // Parameter Variasi Spesies Pohon & Hukum Jarak Ekologis
    enum TreeSpeciesType {
        TREE_TYPE_FRUIT = 0,    // Pohon Buah (Fruiting Tree): Menghasilkan buah bernutrisi, fotosintesis sedang
        TREE_TYPE_OXYGEN = 1,   // Pohon Paru-paru Hijau (Oxygen Dense): Tidak berbuah, produksi O2 masif & serap CO2 besar
        TREE_TYPE_PIONEER = 2   // Pohon Perintis / Kayu Keras: Tumbuh di tanah gersang, tahan kekeringan & cuaca ekstrem
    };
    constexpr int TREE_SPECIES_COUNT = 3;
    constexpr double MIN_TREE_SPACING = 30.0;         // Jarak spasial minimum antar-pohon (mencegah tumpang-tindih)
    constexpr double TREE_OXYGEN_SPECIES_O2_MULT = 2.5; // Pengali produksi O2 untuk pohon Oxygen Dense
    constexpr double TREE_OXYGEN_SPECIES_CO2_MULT = 2.2; // Pengali serapan CO2 untuk pohon Oxygen Dense
    constexpr double TREE_REALISTIC_GROWTH_BASE = 0.35; // Kecepatan pertumbuhan dasar bertahap
    constexpr double TREE_SPROUT_SOIL_MIN_FERTILITY = 0.4; // Ambang kesuburan minimum agar benih bisa berkecambah

    // Parameter Paru-Paru, Tabung Oksigen & Hipoksia Kubu Agen
    constexpr double AGENT_LUNG_CAPACITY = 100.0;     // Kapasitas maksimum cadangan O2 di paru-paru (100%)
    constexpr double AGENT_O2_BREATHE_IN_RATE = 15.0; // Laju penyerapan O2 atmosfer ke paru-paru saat O2 atmosfer cukup
    constexpr double AGENT_O2_CONSUMPTION_RATE = 8.0; // Laju konsumsi O2 paru-paru per detik saat metabolisme aktif
    constexpr double HYPOXIA_THRESHOLD = 20.0;        // Ambang batas hipoksia (sesak nafas jika cadangan paru-paru < 20%)
    constexpr double HYPOXIA_SUFFOCATION_DAMAGE = 18.0; // Kerusakan energi per detik saat paru-paru kehabisan oksigen
    constexpr double O2_ATMOSPHERE_SAFE_MIN = 14.0;   // Batas minimum O2 atmosfer (% volume) agar paru-paru bisa menghirup normal

    // Parameter Pohon & Buah (Sistem Siklus Hidup & Ketergantungan Rumus Kognisi)
    constexpr int MAX_TREES = 1000;                  // Kapasitas buffer pohon dinamis (bisa tumbuh tak terbatas)
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
    constexpr int INITIAL_PREDATORS = 50;           // Populasi awal Faksi B (Predator/Karnivora) seimbang dengan Herbivora (200)
    constexpr int MAX_PREDATORS_BUFFER = 5000;      // Kapasitas buffer memori GPU Faksi B (optimal & cepat)
    constexpr double PREDATOR_SPEED = 12.0;          // Kecepatan gerak predator seimbang dengan herbivora (12.0)
    constexpr double PREDATOR_ATTACK_RADIUS = 16.0;  // Radius jangkauan serangan predator ke mangsa
    constexpr double PREDATOR_DAMAGE_RATE = 25.0;    // Kerusakan serangan dasar per detik
    constexpr double PREDATOR_ENERGY_GAIN = 20.0;    // Energi predator bertambah saat berhasil melukai/memangsa
    constexpr double PREDATOR_INITIAL_ENERGY = 100.0; // Energi awal predator
    constexpr double PREDATOR_METABOLISM = 0.12;    // Konsumsi energi dasar predator per detik (setara & seimbang dengan herbivora)
    constexpr double PREDATOR_MATING_RADIUS = 28.0; // Jarak perkawinan Faksi B (♂ + ♀)
    constexpr double PREDATOR_MATING_MIN_ENERGY = 40.0; // Ambang batas reproduksi karnivora (adaptasi seimbang)
    constexpr double PREDATOR_MATING_COST = 15.0;   // Energi melahirkan anak predator baru
    constexpr double PREDATOR_MATING_COOLDOWN = 5.0; // Jeda waktu kawin predator (detik)
    
    // Sistem Pertarungan Adil & Era Megalitikum (Material Batu, Logam, & Struktur Pertahanan)
    constexpr double MEGALITH_MINING_RADIUS = 35.0;       // Radius interaksi penambangan deposit megalitikum
    constexpr double MEGALITH_MINING_RATE = 2.0;         // Laju penambangan material per detik
    constexpr double MEGALITH_CRAFT_THRESHOLD = 5.0;     // Ambang akumulasi material untuk menghasilkan 1 alat/struktur megalitikum
    constexpr double MEGALITH_CRAFT_ENERGY_COST = 12.0;  // Biaya energi saat berhasil membuat alat megalitikum baru
    constexpr double MEGALITH_TOOL_USAGE_ENERGY_RATE = 0.08;// Biaya konsumsi energi saat membawa/menggunakan alat tempur
    constexpr double MEGALITH_WEAPON_ATTACK_BONUS = 15.0;// Bonus damage saat menguasai deposit batu/logam
    constexpr double MEGALITH_SHIELD_DEFENSE_BONUS = 0.40;// Reduksi damage pertahanan saat di sekitar deposit batu/struktur
    constexpr double HERBIVORE_BASE_ATTACK_DAMAGE = 20.0;// Serangan fisik dasar herbivora saat duel jarak dekat
    constexpr double COMBAT_CLASH_RADIUS = 16.0;         // Radius benturan duel antar faksi
    constexpr double CORPSE_CARNIVORE_RECOVERY = 0.85;   // Efisiensi nutrisi karnivora saat mengonsumsi bangkai (semua jenis bangkai)

    // Parameter Bencana Alam & Tekanan Lingkungan Dinamis (Disasters & Cosmic Stress)
    constexpr double DISASTER_INTERVAL_SECONDS = 240.0;  // Bencana terjadi setiap 240 detik (4 hari simulasi) agar peradaban punya waktu berkembang
    constexpr double DISASTER_DURATION_SECONDS = 12.0;   // Durasi bencana berlangsung 12 detik
    constexpr double SOLAR_STORM_HEAT_SPIKE = 22.0;      // Badai Matahari: lonjakan panas +22°C
    constexpr double BLIZZARD_TEMP_DROP = -25.0;         // Badai Salju Ekstrem: suhu anjlok -25°C
    constexpr double EMP_MAGNETIC_CHAOS = 8.0;           // Badai Magnetik/EMP: mengacaukan sensor & transmisi komunikasi

    // Parameter Kubu Alam AI (Adversarial System & Adaptive Pressure Curve)
    constexpr double NATURE_PRESSURE_TARGET_POP = 1000.0; // Kapasitas daya tampung ideal ekosistem diperluas ke 1000
    constexpr double NATURE_PRESSURE_KP = 0.005;         // Responsivitas kurva tekanan adaptif
    constexpr double NATURE_ENTROPY_DECAY_RATE = 0.05;   // Laju erosi entropi terhadap koloni yang stagnan
    constexpr double NATURE_RESOURCE_SCARCITY_MULT = 1.8;// Skalasi kelangkaan pangan/material saat populasi over-limit
    constexpr double RESERVOIR_LEAK_RATE = 0.15;         // Laju kebocoran / continuous decay Dynamic Reservoir
    constexpr double RESERVOIR_CHAOS_NONLINEARITY = 1.4; // Tingkat non-linearitas tanggap dynamical graph

    // 10 Fitur Ekosistem & Ability Alam Adversarial Baru
    constexpr double PATHOGEN_INFECTION_RADIUS = 25.0;   // Jarak penularan wabah pathogen pada koloni berkerumun
    constexpr double PATHOGEN_DAMAGE_RATE = 12.0;        // Kerusakan energi per detik pada individu terinfeksi
    constexpr double CORPSE_TOXICITY_RADIUS = 30.0;      // Radius racun pembusukan mayat jika tidak diolah
    constexpr double CORPSE_TOXICITY_DAMAGE = 8.0;       // Racun tanah/air akibat mayat membusuk
    constexpr double EROSION_TERRAIN_SPEED = 0.5;        // Laju pergeseran jurang/alur erosi dinamis
    constexpr double SAFE_HAVEN_RADIUS = 60.0;           // Radius Gua / Zona Perlindungan dari bencana
    constexpr int SAFE_HAVEN_MAX_CAPACITY = 20;          // Kapasitas maksimum agen per Gua/Safe Haven
    constexpr double TOOL_RUST_DECAY_RATE = 0.02;        // Laju degradasi/karat pada alat dan struktur megalitikum
    constexpr double GEOTHERMAL_LAVA_RADIUS = 40.0;      // Radius bahaya semburan lava / abu vulkanik
    constexpr double GEOTHERMAL_LAVA_DAMAGE = 35.0;      // Kerusakan panas mematikan abu/lava vulkanik
    constexpr double WATER_SALINITY_DRAIN = 4.0;         // Dehidrasi akibat fluktuasi salinitas air tercemar
    constexpr double SPATIAL_DRAG_SLOWDOWN = 0.50;       // Reduksi kecepatan 50% di zona gravitasi/drag spasial padat

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
    constexpr double SUPER_MUTATION_CHANCE = 0.08;    // 8% peluang mutasi acak radikal (menjaga retensi kepintaran garis keturunan)
    constexpr double DEFECTIVE_BIRTH_CHANCE = 0.03;   // 3% kemungkinan cacat (tidak memotong populasi cerdas secara agresif)

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

    // Parameter Sistem Artifact (Ujian Aritmatika & Keabadian)
    constexpr double ARTIFACT_CLAIM_RADIUS = 35.0;          // Radius klaim artifact oleh agen (m)
    constexpr double ARTIFACT_IMMORTALITY_DAYS = 20.0;      // Durasi keabadian setelah klaim (hari simulasi)
    constexpr double ARTIFACT_ANSWER_TOLERANCE = 0.12;      // Toleransi kecocokan comm_signal vs jawaban [-1,1]
    constexpr double ARTIFACT_BALANCER_POP_THRESHOLD = 0.05;// Threshold punah: < 5% total pop → spawn balancer
    constexpr int    ARTIFACT_BALANCER_SPAWN_COUNT = 5;     // Jumlah agen balancer yang di-spawn saat hampir punah

    // Parameter Respon Takut Eksponensial/Sigmoid, Epigenetik Trauma & Neuromodulasi
    constexpr double FEAR_SIGMOID_STEEPNESS = 6.0;         // Keterjalan kurva sigmoid rasa takut
    constexpr double FEAR_SIGMOID_MIDPOINT = 0.45;         // Titik tengah pemicu lonjakan panik eksponensial
    constexpr double FEAR_NEUROMODULATION_PLASTICITY = 4.0;// Pengali lonjakan plastisitas/mutasi Hebbian saat panik tinggi
    constexpr double EPIGENETIC_TRAUMA_WEIGHT_BIAS = 0.15; // Pembobotan trauma rasa takut induk yang diwariskan ke anak
}