#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <math.h>
#include "agent/dunia/parameter_dunia.h"
#include "agent/ingatan/parameter_agent.h"
#include "agent/02_kognisi_vm_bytecode.h"
#include "lingkungan_ekosistem.h"

#define ECO_MAX_TREES DuniaFisika::MAX_TREES

__global__ void simulate_ecosystem_step_cuda_kernel(
    GpuEcosystemAgent* agents,
    int population_size,
    GpuTreeEntity* trees,
    int trees_count,
    GpuPredatorAgent* predators,
    int predators_count,
    GpuMineralDeposit* minerals,
    int minerals_count,
    GpuFloraPlant* flora,
    int flora_count,
    GpuWildFauna* fauna,
    int fauna_count,
    GpuClimateState* climate,
    double dt
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= population_size) return;

    GpuEcosystemAgent& ag = agents[idx];
    ag.just_died = false;
    ag.just_born = false;

    // 0. Cek Status Kehidupan
    if (!ag.is_alive) {
        // Dekomposisi Mayat Alami
        if (ag.corpse_energy > 0.0f) {
            ag.corpse_energy = fmaxf(0.0f, ag.corpse_energy - (float)DuniaFisika::CORPSE_DECAY_RATE * (float)dt);
        }
        return; // Agen mati tidak memproses komputasi atau bergerak
    }

    // Cek Ambang Kematian Awal (Mati jika Energi Habis / Kelaparan)
    if (ag.energy <= (float)DuniaFisika::STARVATION_THRESHOLD) {
        ag.is_alive = false;
        ag.just_died = true;
        ag.energy = 0.0f;
        ag.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE; // Menyisakan nutrisi mayat
        return;
    }

    // Friendly Fire Antar-Agen Kubu A (Benturan / Gesekan Kecepatan Tinggi tak Sengaja)
    for (int k = 0; k < 16; ++k) {
        int peer_idx = (idx + k * 29 + 1) % population_size;
        if (peer_idx != idx && agents[peer_idx].is_alive && agents[peer_idx].energy > 0.0f) {
            float cdx = agents[peer_idx].x - ag.x;
            float cdy = agents[peer_idx].y - ag.y;
            float cdist2 = cdx * cdx + cdy * cdy;
            if (cdist2 < 4.0f * 4.0f) { // Kontak fisik sangat dekat
                float rel_v2 = (ag.vx - agents[peer_idx].vx) * (ag.vx - agents[peer_idx].vx) + 
                               (ag.vy - agents[peer_idx].vy) * (ag.vy - agents[peer_idx].vy);
                if (rel_v2 > 16.0f) { // Terjadi benturan keras
                    float ff_dmg = (float)DuniaFisika::FRIENDLY_FIRE_DAMAGE_RATE * 0.2f * (float)dt;
                    ag.energy = fmaxf(0.0f, ag.energy - ff_dmg);
                }
            }
        }
    }

    // 1. Cari Pohon Terdekat, Deposit Mineral Terdekat, & Predator Terdekat
    float nearest_tree_dist = 10000.0f;
    float nearest_tree_dx = 0.0f;
    float nearest_tree_dy = 0.0f;
    int nearest_tree_idx = -1;

    for (int t = 0; t < trees_count; ++t) {
        float dx = trees[t].x - ag.x;
        float dy = trees[t].y - ag.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < nearest_tree_dist) {
            nearest_tree_dist = d;
            nearest_tree_dx = dx;
            nearest_tree_dy = dy;
            nearest_tree_idx = t;
        }
    }

    // Cari Deposit Mineral Terdekat (Untuk penambangan alat/batu purba & perlindungan medan)
    float nearest_min_dist = 10000.0f;
    float nearest_min_dx = 0.0f;
    float nearest_min_dy = 0.0f;
    int nearest_min_idx = -1;

    for (int m = 0; m < minerals_count; ++m) {
        if (minerals[m].mass <= 0.0f) continue;
        float dx = minerals[m].x - ag.x;
        float dy = minerals[m].y - ag.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < nearest_min_dist) {
            nearest_min_dist = d;
            nearest_min_dx = dx;
            nearest_min_dy = dy;
            nearest_min_idx = m;
        }
    }

    // Cari Predator Terdekat untuk insting waspada & manuver menghindar (Pilar 1: Exteroception Presisi)
    float nearest_pred_dist = 10000.0f;
    float nearest_pred_x = 0.0f;
    float nearest_pred_y = 0.0f;
    for (int p = 0; p < predators_count; ++p) {
        if (predators[p].is_alive && predators[p].energy > 0.0f) {
            float pdx = predators[p].x - ag.x;
            float pdy = predators[p].y - ag.y;
            float pd = sqrtf(pdx * pdx + pdy * pdy);
            if (pd < nearest_pred_dist) {
                nearest_pred_dist = pd;
                nearest_pred_x = predators[p].x;
                nearest_pred_y = predators[p].y;
            }
        }
    }

    // Ability Alam: 1. Toksisitas Pembusukan Bangkai Sekitar (Corpse Toxicity)
    for (int p = 0; p < 8; ++p) {
        int c_idx = (idx + p * 13) % population_size;
        if (!agents[c_idx].is_alive && agents[c_idx].corpse_energy > 5.0f) {
            float cdx = agents[c_idx].x - ag.x;
            float cdy = agents[c_idx].y - ag.y;
            if ((cdx * cdx + cdy * cdy) < (float)(DuniaFisika::CORPSE_TOXICITY_RADIUS * DuniaFisika::CORPSE_TOXICITY_RADIUS)) {
                ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::CORPSE_TOXICITY_DAMAGE * (float)dt); // Racun tanah/air bangkai
            }
        }
    }

    // Ability Alam: 2. Drag Gravitasi / Rawa Spasial (Spatial Drag Slowdown di area rawa (x: 400..600, y: 400..600))
    bool in_drag_zone = (ag.x > 400.0f && ag.x < 600.0f && ag.y > 400.0f && ag.y < 600.0f);
    if (in_drag_zone) {
        ag.vx *= (float)DuniaFisika::SPATIAL_DRAG_SLOWDOWN;
        ag.vy *= (float)DuniaFisika::SPATIAL_DRAG_SLOWDOWN;
    }

    // Ability Alam: 3. Zona Safe Haven / Gua Perlindungan (x: 100..200, y: 100..200)
    bool in_safe_haven = (ag.x > 100.0f && ag.x < 220.0f && ag.y > 100.0f && ag.y < 220.0f);
    if (in_safe_haven && ag.energy < 100.0f) {
        ag.energy = fminf(100.0f, ag.energy + 2.0f * (float)dt); // Pemulihan aman di gua
    }

    // Hitung Jarak ke Alur Sungai Terdekat (Meandering River: x = CENTER_X + AMP * sin(y * FREQ))
    float river_x = (float)DuniaFisika::RIVER_CENTER_X + (float)DuniaFisika::RIVER_MEANDER_AMP * sinf(ag.y * (float)DuniaFisika::RIVER_MEANDER_FREQ);
    float dist_to_river = fabsf(ag.x - river_x);
    float river_dx = river_x - ag.x;
    float river_dy = 0.0f;

    // Hitung Jarak ke Danau Terdekat (Lake Ecosystem)
    float lake_dx = (float)DuniaFisika::LAKE_CENTER_X - ag.x;
    float lake_dy = (float)DuniaFisika::LAKE_CENTER_Y - ag.y;
    float dist_lake_center = sqrtf(lake_dx * lake_dx + lake_dy * lake_dy);
    float dist_to_lake = fmaxf(0.0f, dist_lake_center - (float)DuniaFisika::LAKE_RADIUS);

    // Tentukan sumber air terdekat (Sungai vs Danau)
    float nearest_water_dist = dist_to_river;
    float nearest_water_dx = river_dx;
    float nearest_water_dy = river_dy;
    if (dist_to_lake < nearest_water_dist) {
        nearest_water_dist = dist_to_lake;
        nearest_water_dx = (dist_lake_center > 1e-3f) ? (lake_dx / dist_lake_center) * dist_to_lake : 0.0f;
        nearest_water_dy = (dist_lake_center > 1e-3f) ? (lake_dy / dist_lake_center) * dist_to_lake : 0.0f;
    }

    // Minum Air di Tepi Sungai atau Tepi Danau
    if (dist_to_river <= (float)DuniaFisika::RIVER_DRINK_RADIUS || dist_to_lake <= (float)DuniaFisika::LAKE_DRINK_RADIUS) {
        ag.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, ag.hydration + (float)DuniaFisika::AGENT_DRINK_WATER_RATE * (float)dt);
    }

    // Penyerapan Hidrasi Langsung dari Curah Hujan (Rainfall Hydration)
    if (climate->rain_intensity > 0.0f) {
        float rain_hydrate = (float)DuniaFisika::RAIN_HYDRATION_GAIN_RATE * (climate->rain_intensity / (float)DuniaFisika::RAIN_INTENSITY_MAX) * (float)dt;
        ag.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, ag.hydration + rain_hydrate);
    }

    // =========================================================================
    // PILAR 2: SENSOR TIGA PILAR (TERKOMPRESI 16 FLOAT) - HERBIVORA
    // =========================================================================
    // 1. EXTEROCEPTION (Dunia Luar: S0 - S9)
    float vis_factor = 0.4f + 0.6f * climate->daylight_factor; // Penglihatan tajam saat siang, redup saat malam
    float s0_tree_dx = (nearest_tree_dx / (nearest_tree_dist + 1e-3f)) * vis_factor;
    float s1_tree_dy = (nearest_tree_dy / (nearest_tree_dist + 1e-3f)) * vis_factor;
    float s2_tree_dist = 1.0f - (float)gpu_clamp(nearest_tree_dist / 500.0f, 0.0, 1.0);
    float s3_threat_dx = (nearest_pred_dist < 10000.0f) ? ((nearest_pred_x - ag.x) / (nearest_pred_dist + 1e-3f)) * vis_factor : 0.0f;
    float s4_threat_dy = (nearest_pred_dist < 10000.0f) ? ((nearest_pred_y - ag.y) / (nearest_pred_dist + 1e-3f)) * vis_factor : 0.0f;
    float s5_threat_dist = (nearest_pred_dist < 10000.0f) ? (1.0f - (float)gpu_clamp(nearest_pred_dist / 300.0f, 0.0, 1.0)) : 0.0f;
    float s6_river_dx = (nearest_water_dx / (nearest_water_dist + 1e-3f)) * vis_factor; // Sensor arah sumber air (sungai/danau)
    float s7_river_dist = 1.0f - (float)gpu_clamp(nearest_water_dist / 300.0f, 0.0, 1.0); // Sensor jarak ke air
    float s8_comm_recv = (float)tanh(ag.comm_received);
    float s9_climate_ambient = (float)tanh((climate->temperature - 20.0f) * 0.05f) * climate->daylight_factor;

    // 2. INTEROCEPTION (Kondisi Tubuh / Homeostasis: Energi S10, Hidrasi S11, Rasa Haus/Panik S12)
    float s10_energy = (float)gpu_clamp(ag.energy / 100.0f, 0.0, 1.0);
    float s11_hydration = (float)gpu_clamp(ag.hydration / 100.0f, 0.0, 1.0);
    // 1. Formula Sigmoid/Eksponensial Non-Linier: Lonjakan Panik saat Krisis Energi/Haus/Predator
    float raw_stress = (1.0f - s10_energy) * 0.35f + (1.0f - s11_hydration) * 0.35f + s5_threat_dist * 0.30f;
    float fear_sig = 1.0f / (1.0f + expf(-(float)ParameterAgent::FEAR_SIGMOID_STEEPNESS * (raw_stress - (float)ParameterAgent::FEAR_SIGMOID_MIDPOINT)));
    ag.fear_level = (float)gpu_clamp(fear_sig, 0.0, 1.0);
    float s12_fear_pain = ag.fear_level;

    // 3. PROPRIOCEPTION (Kesadaran Gerak & Fisik Diri: S13 - S15)
    float max_spd = (float)DuniaFisika::MAX_AGENT_SPEED;
    float s13_vx = (float)gpu_clamp(ag.vx / (max_spd + 1e-3f), -1.0, 1.0);
    float s14_vy = (float)gpu_clamp(ag.vy / (max_spd + 1e-3f), -1.0, 1.0);
    float s15_tool_status = (ag.tools_crafted > 0) ? 1.0f : (float)gpu_clamp(ag.mined_material / (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD, 0.0, 0.9);

    float agent_sensor_inputs[ParameterAgent::SENSORS_COUNT] = {
        s0_tree_dx, s1_tree_dy, s2_tree_dist, s3_threat_dx,
        s4_threat_dy, s5_threat_dist, s6_river_dx, s7_river_dist,
        s8_comm_recv, s9_climate_ambient, s10_energy, s11_hydration,
        s12_fear_pain, s13_vx, s14_vy, s15_tool_status
    };

    // =========================================================================
    // PILAR 3: RECURRENT RESERVOIR LOOP (Spatial Depth 64 Step + Temporal T)
    // =========================================================================
    int max_r = (ag.active_registers_count > 0 && ag.active_registers_count <= REGISTERS_COUNT) ? 
                ag.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;
    float move_cmd_x = 0.0f;
    float move_cmd_y = 0.0f;
    float broadcast_out = 0.0f;

    // Eksekusi Virtual Machine Kognisi Otak Agen (Modular)
    execute_agent_brain_vm(ag, agent_sensor_inputs, s9_climate_ambient, dt, move_cmd_x, move_cmd_y, broadcast_out);
    ag.growth_signal = broadcast_out;

    // 2d. Swarm Signal Quantization: Kuantisasi comm_signal ke simbol diskrit {-1.0, 0.0, +1.0}
    float quant_thresh = (float)ParameterAgent::SWARM_QUANT_THRESHOLD;
    if (ag.comm_signal > quant_thresh)       ag.comm_signal = 1.0f;
    else if (ag.comm_signal < -quant_thresh) ag.comm_signal = -1.0f;
    else                                      ag.comm_signal = 0.0f;

    // 3. DINAMIKA GERAK FISIK (Inersia, Medan Angin, Perintah Aksi Otak & Dorongan Kinetik Bencana)
    float wind_influence_x = climate->wind_x * (float)DuniaFisika::WIND_FORCE_MULT;
    float wind_influence_y = climate->wind_y * (float)DuniaFisika::WIND_FORCE_MULT;

    // Dampak Kinetik Badai EMP / Hurikan Badai
    if (climate->active_disaster_type == 2 && !in_safe_haven) {
        float knockback_x = sinf(ag.x * 0.01f + climate->magnetic_angle) * (float)DuniaFisika::EMP_SHOCK_KNOCKBACK * climate->disaster_severity;
        float knockback_y = cosf(ag.y * 0.01f + climate->magnetic_angle) * (float)DuniaFisika::EMP_SHOCK_KNOCKBACK * climate->disaster_severity;
        wind_influence_x += knockback_x;
        wind_influence_y += knockback_y;
    }

    float speed_mult = 1.0f;
    // Perlambatan Fisik Badai Salju Beku
    if (climate->active_disaster_type == 1 && !in_safe_haven) {
        speed_mult = fmaxf(0.2f, 1.0f - (1.0f - (float)DuniaFisika::BLIZZARD_SLOWDOWN) * climate->disaster_severity);
    }

    float target_vx = (move_cmd_x * max_spd + wind_influence_x) * speed_mult;
    float target_vy = (move_cmd_y * max_spd + wind_influence_y) * speed_mult;

    ag.vx = ag.vx * (float)DuniaFisika::FRICTION_COEFF + target_vx * 0.15f;
    ag.vy = ag.vy * (float)DuniaFisika::FRICTION_COEFF + target_vy * 0.15f;

    ag.x += ag.vx * (float)dt;
    ag.y += ag.vy * (float)dt;

    // Batas Tepi Dunia
    if (ag.x < 10.0f) { ag.x = 10.0f; ag.vx = -ag.vx * 0.5f; }
    if (ag.x > (float)DuniaFisika::WORLD_WIDTH - 10.0f) { ag.x = (float)DuniaFisika::WORLD_WIDTH - 10.0f; ag.vx = -ag.vx * 0.5f; }
    if (ag.y < 10.0f) { ag.y = 10.0f; ag.vy = -ag.vy * 0.5f; }
    if (ag.y > (float)DuniaFisika::WORLD_HEIGHT - 10.0f) { ag.y = (float)DuniaFisika::WORLD_HEIGHT - 10.0f; ag.vy = -ag.vy * 0.5f; }

    // 4. Penuaan & Eskalasi Rasa Lapar Terhadap Iklim / Suhu Musim
    ag.age_years += (float)(dt / DuniaFisika::SECONDS_PER_YEAR);

    // Hitung Kepadatan Populasi Tetangga (Overcrowding Stress)
    int nearby_agents_count = 0;
    for (int k = 0; k < 128; ++k) {
        int neighbor_idx = (idx + k * 17) % population_size;
        if (neighbor_idx != idx && agents[neighbor_idx].is_alive && agents[neighbor_idx].energy > 0.0f) {
            float ndx = agents[neighbor_idx].x - ag.x;
            float ndy = agents[neighbor_idx].y - ag.y;
            if ((ndx * ndx + ndy * ndy) < (float)(DuniaFisika::OVERCROWDING_RADIUS * DuniaFisika::OVERCROWDING_RADIUS)) {
                nearby_agents_count++;
            }
        }
    }

    float overcrowding_stress = 1.0f + (float)nearby_agents_count * (float)DuniaFisika::OVERCROWDING_PENALTY_MULT;
    
    // Ability Alam: 4. Wabah Pathogen pada Koloni Berkerumun Padat (>= 8 tetangga berdekatan)
    if (nearby_agents_count >= 8) {
        ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::PATHOGEN_DAMAGE_RATE * (float)dt);
    }

    float temp_diff = fabsf(climate->temperature - 20.0f);
    float heat_stress = (climate->temperature > 35.0f) ? (climate->temperature - 35.0f) * 0.08f : 0.0f;
    float co2_stress = fmaxf(0.0f, (climate->co2_level - (float)DuniaFisika::CO2_BASE_LEVEL) * 2.0f);
    float nature_stress = climate->nature_adversarial_pressure; // Kurva Tekanan Adaptif Kubu Alam AI
    float climate_stress = (1.0f + temp_diff * (float)DuniaFisika::CLIMATE_HUNGER_IMPACT_MULT + heat_stress + co2_stress) * nature_stress;
    
    // Penuaan Eksponensial Biologis Manusia (Makin Menua Makin Renta di Atas 40 Tahun)
    float age_senescence_mult = 1.0f;
    if (ag.age_years > (float)DuniaFisika::AGING_SENESCENCE_START_AGE) {
        float aging_delta = ag.age_years - (float)DuniaFisika::AGING_SENESCENCE_START_AGE;
        age_senescence_mult = expf((float)DuniaFisika::AGING_EXPONENTIAL_FACTOR * aging_delta);
    }
    ag.hunger_rate_mult = age_senescence_mult * climate_stress * overcrowding_stress;
    if (ag.mating_cooldown > 0.0f) ag.mating_cooldown = fmaxf(0.0f, ag.mating_cooldown - (float)dt);

    // Ability Alam: 5. Degradasi & Karat Alat/Struktur Megalitikum (Tool Rust)
    if (ag.tools_crafted > 0) {
        ag.mined_material = fmaxf(0.0f, ag.mined_material - (float)DuniaFisika::TOOL_RUST_DECAY_RATE * (float)dt);
    }

    // Dampak Bencana Alam Fisik Langsung (Direct Disaster Physical Damage) jika di luar Safe Haven:
    if (!in_safe_haven && climate->active_disaster_type >= 0 && ag.immortality_timer <= 0.0f) {
        if (climate->active_disaster_type == 0) {
            // Badai Matahari: Panas Radiasi Termal Membakar
            float scorch_dmg = (float)DuniaFisika::SOLAR_STORM_SCORCH_DAMAGE * climate->disaster_severity * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - scorch_dmg);
            ag.fear_level = fminf(1.0f, ag.fear_level + 0.5f * (float)dt);
        } else if (climate->active_disaster_type == 1) {
            // Badai Salju Ekstrem: Hipotermia Pembekuan Tubuh
            float frost_dmg = (float)DuniaFisika::BLIZZARD_FROST_DAMAGE * climate->disaster_severity * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - frost_dmg);
            ag.fear_level = fminf(1.0f, ag.fear_level + 0.4f * (float)dt);
        }
    }

    // Bahaya Banjir Luapan Air Danau / Sungai saat Hujan Sangat Lebat
    if (climate->rain_intensity > 75.0f && !in_safe_haven && ag.immortality_timer <= 0.0f) {
        float flood_river_r = (float)DuniaFisika::RIVER_DRINK_RADIUS * (float)DuniaFisika::FLOOD_EXPANSION_MULT;
        float flood_lake_r = (float)DuniaFisika::LAKE_DRINK_RADIUS * (float)DuniaFisika::FLOOD_EXPANSION_MULT;
        if (dist_to_river < flood_river_r || dist_to_lake < flood_lake_r) {
            float sweep_dmg = (float)DuniaFisika::FLOOD_SWEEP_DAMAGE * (climate->rain_intensity / 100.0f) * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - sweep_dmg);
            ag.fear_level = fminf(1.0f, ag.fear_level + 0.6f * (float)dt);
        }
    }

    // Keabadian Artifact: kurangi timer, skip metabolisme & kematian lapar
    if (ag.immortality_timer > 0.0f) {
        ag.immortality_timer = fmaxf(0.0f, ag.immortality_timer - (float)dt);
        ag.energy = fminf(100.0f, ag.energy + 0.5f * (float)dt); // Pulih perlahan saat kebal
        ag.lung_oxygen = 100.0f;
    } else {
        // Sistem Tabung Oksigen & Paru-Paru (Respirasi Nyata):
        if (climate->oxygen_level >= (float)DuniaFisika::O2_ATMOSPHERE_SAFE_MIN) {
            float o2_gain = (float)DuniaFisika::AGENT_O2_BREATHE_IN_RATE * (climate->oxygen_level / 21.0f) * (float)dt;
            ag.lung_oxygen = fminf((float)DuniaFisika::AGENT_LUNG_CAPACITY, ag.lung_oxygen + o2_gain);
        } else {
            float o2_loss = (float)DuniaFisika::AGENT_O2_CONSUMPTION_RATE * (float)dt;
            ag.lung_oxygen = fmaxf(0.0f, ag.lung_oxygen - o2_loss);
        }

        // Dinamika Dehidrasi Tubuh (Tergantung Suhu Iklim & Kecepatan Gerak)
        float speed = sqrtf(ag.vx * ag.vx + ag.vy * ag.vy);
        float heat_dehydration = (climate->temperature > 25.0f) ? (climate->temperature - 25.0f) * (float)DuniaFisika::AGENT_DEHYDRATION_HEAT_MULT : 0.0f;
        float dehyd_rate = ((float)DuniaFisika::AGENT_DEHYDRATION_BASE_RATE + heat_dehydration + speed * 0.02f) * (float)dt;
        ag.hydration = fmaxf(0.0f, ag.hydration - dehyd_rate);

        // Pengaruh Oksigen Paru-Paru & Hidrasi terhadap Metabolisme Energi
        float lung_eff = fmaxf(0.2f, ag.lung_oxygen / 100.0f);
        float hyd_eff = fmaxf(0.2f, ag.hydration / 100.0f);
        float energy_cost = ((float)DuniaFisika::METABOLISM_BASE_RATE + speed * (float)DuniaFisika::METABOLISM_MOVE_COST) * ag.hunger_rate_mult * (float)dt / (lung_eff * hyd_eff);
        ag.energy = fmaxf(0.0f, ag.energy - energy_cost);

        // Hipoksia
        if (ag.lung_oxygen < (float)DuniaFisika::HYPOXIA_THRESHOLD) {
            float suffocation_dmg = (float)DuniaFisika::HYPOXIA_SUFFOCATION_DAMAGE * (1.0f - ag.lung_oxygen / (float)DuniaFisika::HYPOXIA_THRESHOLD) * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - suffocation_dmg);
            ag.fear_level = fminf(1.0f, ag.fear_level + 0.3f * (float)dt);
        }

        // Kerentaan Biologis Alami Penuaan Organ:
        if (ag.age_years > (float)DuniaFisika::AGING_SENESCENCE_START_AGE) {
            float aging_span = ag.age_years - (float)DuniaFisika::AGING_SENESCENCE_START_AGE;
            float frailty_dmg = (float)DuniaFisika::AGING_FRAILTY_DAMAGE_BASE * expf((float)DuniaFisika::AGING_EXPONENTIAL_FACTOR * aging_span) * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - frailty_dmg);
        }

        // Dehidrasi Ekstrem
        if (ag.hydration <= 0.0f) {
            float thirst_dmg = (float)DuniaFisika::DEHYDRATION_DAMAGE_RATE * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - thirst_dmg);
            ag.fear_level = fminf(1.0f, ag.fear_level + 0.35f * (float)dt);
        }
    }

    // Health Check Biologis
    if ((ag.immortality_timer <= 0.0f) && (ag.energy <= (float)DuniaFisika::STARVATION_THRESHOLD)) {
        ag.is_alive = false;
        ag.just_died = true;
        ag.energy = 0.0f;
        ag.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
        return;
    }

    // 5. Interaksi dengan Pohon & Konsumsi Buah (Fotosintesis Siang/Malam Organik & Variasi Spesies)
    if (nearest_tree_idx >= 0 && nearest_tree_dist < (float)DuniaFisika::TREE_INTERACTION_RADIUS && trees[nearest_tree_idx].health > 0.0f) {
        float photo_mult = 0.5f + (float)DuniaFisika::DAY_PHOTOSYNTHESIS_MULT * climate->daylight_factor;
        float species_pool_mult = (trees[nearest_tree_idx].tree_type == DuniaFisika::TREE_TYPE_OXYGEN) ? 1.5f : 1.0f;
        float shared_pool_reward = ((float)DuniaFisika::TREE_ENERGY_POOL_RATE * photo_mult * species_pool_mult) / (float)(1 + nearby_agents_count);
        ag.energy = fminf(100.0f, ag.energy + (float)(shared_pool_reward * dt));

        // Konsumsi Buah Alami (Khusus Pohon Berbuah TREE_TYPE_FRUIT)
        if (trees[nearest_tree_idx].tree_type == DuniaFisika::TREE_TYPE_FRUIT && 
            trees[nearest_tree_idx].fruits_count >= 1.0f && ag.energy < 100.0f) {
            float old_fruits = atomicAdd(&trees[nearest_tree_idx].fruits_count, -1.0f);
            if (old_fruits >= 1.0f) {
                float nut = (float)DuniaFisika::FRUIT_NUTRITION_ENERGY;
                ag.energy = fminf(100.0f, ag.energy + nut);
                ag.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, ag.hydration + 15.0f); // Buah juga memberi hidrasi air
                ag.fruits_eaten++;
            } else {
                atomicAdd(&trees[nearest_tree_idx].fruits_count, 1.0f);
            }
        }
    }

    // 5-Omni. Omnivora: Foraging Flora (Rumput/Semak/Gandum), Scavenging Daging Bangkai, & Domestikasi Ternak
    // (a) Makan Rumput/Berry/Gandum di Sekitar
    for (int fl = 0; fl < 8; ++fl) {
        int f_idx = (idx + fl * 23) % flora_count;
        if (flora[f_idx].health > 20.0f && flora[f_idx].growth_stage >= 40.0f && ag.energy < 95.0f) {
            float fdx = flora[f_idx].x - ag.x;
            float fdy = flora[f_idx].y - ag.y;
            if ((fdx * fdx + fdy * fdy) < (float)(DuniaFisika::FAUNA_GRAZING_RADIUS * DuniaFisika::FAUNA_GRAZING_RADIUS)) {
                ag.energy = fminf(100.0f, ag.energy + flora[f_idx].yield_amount * 0.4f);
                flora[f_idx].growth_stage = fmaxf(10.0f, flora[f_idx].growth_stage - 25.0f);
                break;
            }
        }
    }

    // (b) Omnivora / Manusia Makan Daging Mayat/Bangkai (Scavenging)
    for (int c = 0; c < 8; ++c) {
        int c_ag_idx = (idx + c * 17) % population_size;
        if (!agents[c_ag_idx].is_alive && agents[c_ag_idx].corpse_energy > 0.5f && ag.energy < 95.0f) {
            float cdx = agents[c_ag_idx].x - ag.x;
            float cdy = agents[c_ag_idx].y - ag.y;
            if ((cdx * cdx + cdy * cdy) < (float)(DuniaFisika::CORPSE_SCAVENGE_RADIUS * DuniaFisika::CORPSE_SCAVENGE_RADIUS)) {
                float bite = fminf(agents[c_ag_idx].corpse_energy, (float)DuniaFisika::PREDATOR_CORPSE_EAT_RATE * (float)dt);
                agents[c_ag_idx].corpse_energy -= bite;
                ag.energy = fminf(100.0f, ag.energy + bite * (float)DuniaFisika::MANUSIA_CORPSE_MEAT_RECOVERY);
                break;
            }
        }
    }

    // (c) Penangkapan, Penjinakan (Domestikasi) & Budidaya Ternak (Ternak Peliharaan)
    for (int fn = 0; fn < 8; ++fn) {
        int f_idx = (idx + fn * 19) % fauna_count;
        if (fauna[f_idx].is_alive) {
            float fdx = fauna[f_idx].x - ag.x;
            float fdy = fauna[f_idx].y - ag.y;
            float fdist2 = fdx * fdx + fdy * fdy;

            // Jika hewan masih liar dan dekat, tangkap/jinakkan jika agen memiliki energi & rumus kognisi
            if (fauna[f_idx].owner_agent_id == 0 && fdist2 < (float)(DuniaFisika::FAUNA_CAPTURE_RADIUS * DuniaFisika::FAUNA_CAPTURE_RADIUS)) {
                if (ag.energy > 50.0f && ag.growth_signal > 0.2f) {
                    fauna[f_idx].owner_agent_id = ag.id;
                    fauna[f_idx].owner_faction = 0; // Faksi Omnivora
                    ag.energy -= (float)DuniaFisika::FAUNA_CAPTURE_ENERGY_COST;
                }
            }

            // Jika ternak milik agen ini (dipelihara), dapatkan hasil produksi susu/telur/biomassa
            if (fauna[f_idx].owner_agent_id == ag.id && fdist2 < (float)(DuniaFisika::FAUNA_DOMESTIC_FOLLOW_RADIUS * DuniaFisika::FAUNA_DOMESTIC_FOLLOW_RADIUS)) {
                if (fauna[f_idx].energy > 40.0f) {
                    ag.energy = fminf(100.0f, ag.energy + (float)DuniaFisika::FAUNA_DOMESTIC_YIELD_RATE * (float)dt);
                }
            }
        }
    }

    // 5a. Penanaman & Penyebaran Benih Pohon Organik (Geologi Tanah, Jarak Minimum, Air & Variasi Spesies)
    // Agen butuh energi (>= 60%) dan hidrasi air (>= 30%) untuk menanam bibit secara nyata
    if (ag.growth_signal > 0.4f && ag.energy > 60.0f && ag.hydration > 30.0f) {
        // Cek apakah posisi agen memenuhi jarak minimum dari pohon lain (Hukum Jarak Ekologis MIN_TREE_SPACING)
        if (nearest_tree_dist >= (float)DuniaFisika::MIN_TREE_SPACING) {
            // Hitung kesuburan geologi tanah di posisi agen
            float soil = (float)DuniaFisika::SOIL_FERTILITY_BASE + 
                         (float)DuniaFisika::SOIL_FERTILITY_VARIATION * sinf(ag.x * (float)DuniaFisika::SOIL_NOISE_SCALE_X) * cosf(ag.y * (float)DuniaFisika::SOIL_NOISE_SCALE_Y);
            
            // Hanya bisa tumbuh jika tanah memiliki kesuburan di atas ambang batas
            if (soil >= (float)DuniaFisika::TREE_SPROUT_SOIL_MIN_FERTILITY) {
                // Cari slot pohon kosong / mati untuk menumbuhkan tunas baru
                for (int t = 0; t < trees_count; ++t) {
                    if (trees[t].health <= 0.0f) {
                        float old_h = atomicExch(&trees[t].health, 50.0f);
                        if (old_h <= 0.0f) {
                            // Tentukan spesies pohon berdasarkan geologi & sinyal agen
                            unsigned int p_seed = gpu_hash((unsigned int)idx * 7919u ^ (unsigned int)t * 104729u);
                            int species = (p_seed % DuniaFisika::TREE_SPECIES_COUNT);
                            if (soil < 0.7f) species = DuniaFisika::TREE_TYPE_PIONEER; // Tanah gersang cocok untuk perintis

                            trees[t].x = ag.x;
                            trees[t].y = ag.y;
                            trees[t].growth_stage = 5.0f; // Bibit muda bertunas
                            trees[t].fruits_count = 0.0f;
                            trees[t].formula_resonance = ag.growth_signal;
                            trees[t].age_years = 0.0f;
                            trees[t].tree_type = species;
                            trees[t].soil_fertility = fmaxf(0.2f, soil);
                            trees[t].moisture = (float)DuniaFisika::TREE_MOISTURE_INITIAL; // Disiram air saat penanaman

                            ag.energy -= (float)DuniaFisika::SEED_GERMINATION_ENERGY_COST;
                            ag.hydration -= (float)DuniaFisika::SEED_GERMINATION_WATER_COST;
                            ag.formulas_discovered++;
                            break;
                        }
                    }
                }
            }
        }
    }

    // 5b. Interaksi Material Spasial Megalitikum (Penambangan Batu/Logam & Rekayasa Sifat Fisik Materi)
    bool has_megalith_tool = (ag.tools_crafted > 0);
    if (has_megalith_tool) {
        // Biaya metabolisme tambahan saat memikul/menggunakan peralatan megalitikum
        ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::MEGALITH_TOOL_USAGE_ENERGY_RATE * (float)dt);
    }
    if (nearest_min_idx >= 0 && nearest_min_dist < (float)DuniaFisika::MEGALITH_MINING_RADIUS && minerals[nearest_min_idx].mass > 0.0f) {
        float mined = (float)DuniaFisika::MEGALITH_MINING_RATE * 0.1f * (float)dt;
        atomicAdd(&minerals[nearest_min_idx].mass, - mined);
        ag.material_interactions++;
        ag.mined_material += mined;

        // Efek Fisika Lingkungan Spasial (Affordance Murni):
        // 1. Logam Alkali Tanah / Silikat (Ca, Mg, Si): Bertindak sebagai Shelter/Isolator Termal di sekitarnya
        if (minerals[nearest_min_idx].hardness < 70.0f) {
            float temp_diff = fabsf(climate->temperature - 20.0f);
            if (temp_diff > 10.0f) {
                // Reduksi stres suhu lingkungan karena perlindungan formasi batuan/gua
                ag.energy = fminf(100.0f, ag.energy + temp_diff * (float)DuniaFisika::SHELTER_THERMAL_INSULATION * 0.01f * (float)dt);
            }
        }

        if (ag.mined_material >= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD) {
            if (ag.energy >= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST) {
                ag.mined_material -= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD;
                ag.tools_crafted++;
                ag.energy -= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST; // Biaya energi crafting alat
                has_megalith_tool = true;
            }
        }
    }

    // 6. Transmisi Gelombang Komunikasi / Feromon Antar-Agen (Diperkuat Konduktivitas Material)
    if (fabsf(ag.comm_signal) > 0.1f) {
        ag.energy = fmaxf(0.0f, ag.energy - (float)DuniaFisika::BROADCAST_ENERGY_COST * (float)dt);
        float comm_rad = (float)DuniaFisika::BROADCAST_COMM_RADIUS;
        if (nearest_min_idx >= 0 && nearest_min_dist < (float)DuniaFisika::MEGALITH_MINING_RADIUS && minerals[nearest_min_idx].conductivity > 0.7f) {
            comm_rad *= (float)DuniaFisika::CONDUCTOR_COMM_AMPLIFY; // Penguatan gelombang feromon oleh deposit konduktif
        }
        for (int k = 0; k < 64; ++k) {
            int peer_idx = (idx + k * 23) % population_size;
            if (peer_idx != idx && agents[peer_idx].is_alive) {
                float cdx = agents[peer_idx].x - ag.x;
                float cdy = agents[peer_idx].y - ag.y;
                if ((cdx * cdx + cdy * cdy) < (comm_rad * comm_rad)) {
                    agents[peer_idx].comm_received = ag.comm_signal;
                }
            }
        }
    }

    // 6b. Reproduksi Alami Antar-Gender (Kubu A) - Ditekan Drastis Saat Suhu Ekstrem & CO2 Tinggi
    float heat_repro_penalty = (climate->temperature > 38.0f) ? (climate->temperature - 38.0f) * 1.5f : 0.0f;
    float co2_repro_penalty = (climate->co2_level > 0.5f) ? (climate->co2_level - 0.5f) * 20.0f : 0.0f;
    float req_mating_energy = (float)DuniaFisika::MATING_MIN_ENERGY + heat_repro_penalty + co2_repro_penalty;
    if (climate->temperature > 55.0f || climate->co2_level > 2.5f) {
        req_mating_energy = 999.0f; // Steril / Reproduksi lumpuh total di suhu ekstrem 58.8°C
    }

    if (ag.gender == 1 && ag.energy >= req_mating_energy && ag.mating_cooldown <= 0.0f && ag.age_years >= 15.0f && ag.age_years <= 75.0f) {
        for (int p = 0; p < population_size; ++p) {
            float partner_req_energy = req_mating_energy;
            if (p != idx && agents[p].gender == 0 && agents[p].energy >= partner_req_energy && agents[p].mating_cooldown <= 0.0f) {
                float pdx = agents[p].x - ag.x;
                float pdy = agents[p].y - ag.y;
                float pdist = sqrtf(pdx * pdx + pdy * pdy);
                if (pdist < (float)DuniaFisika::MATING_RADIUS) {
                    // Terjadi Perkawinan: Transfer & Crossover DNA ke anak
                    ag.energy -= (float)DuniaFisika::MATING_ENERGY_COST;
                    agents[p].energy -= (float)DuniaFisika::MATING_ENERGY_COST * 0.5f;
                    float cd = (float)DuniaFisika::MATING_COOLDOWN;
                    ag.mating_cooldown = cd;
                    agents[p].mating_cooldown = cd;

                    // Cari slot agen yang sudah mati untuk ditempati oleh bayi yang baru lahir
                    for (int slot = 0; slot < population_size; ++slot) {
                        if (!agents[slot].is_alive) {
                            agents[slot].is_alive = true;
                            agents[slot].just_born = true;
                            agents[slot].just_died = false;
                            agents[slot].generation = (ag.generation > agents[p].generation ? ag.generation : agents[p].generation) + 1;
                            // Determinisme Presisi: PRNG berbasis integer hash untuk reproduksi
                            unsigned int rng_seed = gpu_hash((unsigned int)slot * 1973u ^ (unsigned int)ag.id * 8191u ^ (unsigned int)agents[p].id * 2741u);
                            agents[slot].gender = (rng_seed % 2); 
                            agents[slot].sin_type = 0;
                            agents[slot].x = ag.x + (float)((slot % 5) - 2) * 4.0f;
                            agents[slot].y = ag.y + (float)((slot % 3) - 1) * 4.0f;
                            agents[slot].vx = 0.0f;
                            agents[slot].vy = 0.0f;
                            agents[slot].energy = (float)DuniaFisika::NEWBORN_INITIAL_ENERGY; // Energi awal bayi kecil, butuh langsung makan
                            agents[slot].lung_oxygen = (float)DuniaFisika::AGENT_LUNG_CAPACITY;
                            agents[slot].hydration = (float)DuniaFisika::AGENT_HYDRATION_INITIAL;
                            agents[slot].corpse_energy = 0.0f;
                            agents[slot].age_years = 0.0f;
                            agents[slot].mating_cooldown = (float)DuniaFisika::MATING_COOLDOWN * 2.0f;
                            agents[slot].hunger_rate_mult = 1.0f;
                            agents[slot].fruits_eaten = 0;
                            agents[slot].formulas_discovered = 0;
                            agents[slot].predators_slain = 0;
                            agents[slot].material_interactions = 0;
                            agents[slot].tools_crafted = 0;
                            agents[slot].mined_material = 0.0f;
                            agents[slot].active_program_size = ag.active_program_size;
                            agents[slot].active_registers_count = ag.active_registers_count;
                            agents[slot].fear_level = 0.1f;

                            // Cacat Kelahiran Biologis / Mutasi Acak Ekstrim
                            bool is_defective = ((rng_seed % 100) < (int)(DuniaFisika::DEFECTIVE_BIRTH_CHANCE * 100));
                            bool is_super_mutant = ((rng_seed % 100) < (int)(DuniaFisika::SUPER_MUTATION_CHANCE * 100));

                            if (is_defective) {
                                // Cacat biologis: metabolisme boros (lemah), energi awal sangat rendah / pincang
                                agents[slot].energy = 8.0f; 
                                agents[slot].hunger_rate_mult = 2.2f; // Cepat lapar dan rentan
                                agents[slot].active_program_size = ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;
                            }

                            // Crossover Genetik DNA Induk Jantan & Betina
                            int crossover_point = ag.active_program_size / 2;
                            for (int ip = 0; ip < crossover_point; ++ip) {
                                agents[slot].dna_program[ip] = ag.dna_program[ip];
                                agents[slot].epigenetic_methylation[ip] = ag.epigenetic_methylation[ip];
                            }
                            for (int ip = crossover_point; ip < DNA_PROGRAM_SIZE; ++ip) {
                                agents[slot].dna_program[ip] = agents[p].dna_program[ip];
                                agents[slot].epigenetic_methylation[ip] = agents[p].epigenetic_methylation[ip];
                            }

                            // Mutasi Genetik Super Acak (Evolusi Opcode, Register, & Konstanta Baru / Acak Total)
                            float mutation_prob = is_super_mutant ? 0.70f : (float)ParameterAgent::DNA_MUTATION_RATE;
                            if (is_super_mutant && agents[slot].active_program_size < DNA_PROGRAM_SIZE) {
                                agents[slot].active_program_size = min((int)DNA_PROGRAM_SIZE, agents[slot].active_program_size + 1 + (int)(rng_seed % 3));
                            }
                            for (int ip = 0; ip < agents[slot].active_program_size; ++ip) {
                                unsigned int m_hash = gpu_hash(rng_seed ^ ((unsigned int)ip * 199u + (unsigned int)slot * 37u));
                                if ((m_hash % 100) < (int)(mutation_prob * 100)) {
                                    int mut_type = (m_hash / 100) % 4;
                                    if (mut_type == 0 || is_defective) {
                                        agents[slot].dna_program[ip].op = static_cast<unsigned char>((m_hash >> 3) % 15);
                                    } else if (mut_type == 1) {
                                        agents[slot].dna_program[ip].r_dest = static_cast<unsigned char>((m_hash >> 5) % REGISTERS_COUNT);
                                    } else if (mut_type == 2) {
                                        agents[slot].dna_program[ip].r_src1 = static_cast<unsigned char>((m_hash >> 7) % REGISTERS_COUNT);
                                    } else {
                                        agents[slot].dna_program[ip].immediate_val = ((float)((int)(m_hash % 400) - 200)) * 0.1f;
                                    }
                                }
                            }

                            // 2. Epigenetic Trauma Inheritance: Trauma ketakutan induk ditransfer ke anak
                            float parent_trauma = 0.5f * (ag.fear_level + agents[p].fear_level);
                            float trauma_bias = parent_trauma * (float)ParameterAgent::EPIGENETIC_TRAUMA_WEIGHT_BIAS;

                            // Inherit & mutate reservoir weights dengan Epigenetic Trauma Bias
                            for (int r = 0; r < REGISTERS_COUNT; ++r) {
                                agents[slot].registers[r] = 0.0;
                                agents[slot].prev_registers[r] = 0.0;
                                agents[slot].integrated_registers[r] = 0.0;
                                float base_win = (r % 2 == 0) ? ag.reservoir_weights_in[r] : agents[p].reservoir_weights_in[r];
                                float base_wrec = (r % 2 == 0) ? ag.reservoir_weights_rec[r] : agents[p].reservoir_weights_rec[r];

                                // Jika sensor r terkait ancaman/ketakutan (S3, S4, S5, S12), perkuat kepekaan bobot
                                if (r == 3 || r == 4 || r == 5 || r == 12 % REGISTERS_COUNT) {
                                    base_win = (base_win >= 0.0f) ? base_win + trauma_bias : base_win - trauma_bias;
                                }

                                agents[slot].reservoir_weights_in[r] = (float)gpu_clamp(base_win, -1.0, 1.0);
                                agents[slot].reservoir_weights_rec[r] = (float)gpu_clamp(base_wrec, -0.99, 0.99);

                                unsigned int w_hash = gpu_hash(rng_seed ^ ((unsigned int)r * 313u + (unsigned int)slot * 17u));
                                if ((w_hash % 100) < (int)(mutation_prob * 100)) {
                                    agents[slot].reservoir_weights_in[r] = (float)gpu_clamp(agents[slot].reservoir_weights_in[r] + ((float)((int)(w_hash % 200) - 100)) * 0.005f, -1.0, 1.0);
                                    agents[slot].reservoir_weights_rec[r] = (float)gpu_clamp(agents[slot].reservoir_weights_rec[r] + ((float)((int)((w_hash / 200) % 200) - 100)) * 0.005f, -0.99, 0.99);
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    // 7. Interaksi Duel Ekologis & Perang Megalitikum Simetris
    for (int pred_i = 0; pred_i < predators_count; ++pred_i) {
        GpuPredatorAgent& pred = predators[pred_i];
        if (!pred.is_alive || pred.energy <= 0.0f) continue;

        float pdx = pred.x - ag.x;
        float pdy = pred.y - ag.y;
        float pdist = sqrtf(pdx * pdx + pdy * pdy);

        if (pdist < (float)DuniaFisika::COMBAT_CLASH_RADIUS) {
            // Evaluasi Bonus Peralatan/Benteng Megalitikum Faksi A (Manusia)
            float manusia_dmg_mult = 1.0f + (has_megalith_tool ? ((float)DuniaFisika::MEGALITH_WEAPON_ATTACK_BONUS / (float)DuniaFisika::MANUSIA_BASE_ATTACK_DAMAGE) : 0.0f);
            float manusia_defense = has_megalith_tool ? (float)DuniaFisika::MEGALITH_SHIELD_DEFENSE_BONUS : 0.0f;

            // Kerusakan yang diterima Faksi A (Diabaikan saat Keabadian Artifact aktif)
            float effective_pred_dmg = (ag.immortality_timer > 0.0f) ? 0.0f :
                                       (float)DuniaFisika::PREDATOR_DAMAGE_RATE * (1.0f - manusia_defense) * (float)dt;
            ag.energy = fmaxf(0.0f, ag.energy - effective_pred_dmg);

            // Predator menyerap nutrisi dari serangan yang masuk
            pred.energy = fminf(100.0f, pred.energy + effective_pred_dmg * 0.5f);

            // Serangan Fisik Balasan Manusia (Duel Simetris)
            float manusia_strike = (float)DuniaFisika::MANUSIA_BASE_ATTACK_DAMAGE * manusia_dmg_mult * (float)dt;
            pred.energy = fmaxf(0.0f, pred.energy - manusia_strike);

            if (pred.energy <= 0.0f) {
                pred.is_alive = false;
                pred.just_died = true;
                pred.just_killed = true;
                pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
                ag.predators_slain++;
            }

            if (ag.energy <= 0.0f) {
                pred.prey_devoured++;
                pred.energy = fminf(100.0f, pred.energy + 25.0f); // Nutrisi mangsa jatuh
                ag.is_alive = false;
                ag.just_died = true;
                ag.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
            }
        }
    }
}

__global__ void simulate_predator_step_cuda_kernel(
    GpuPredatorAgent* predators,
    int predators_count,
    GpuEcosystemAgent* agents,
    int population_size,
    GpuMineralDeposit* minerals,
    int minerals_count,
    GpuWildFauna* fauna,
    int fauna_count,
    GpuClimateState* climate,
    double dt
) {
    int p_idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (p_idx >= predators_count) return;

    GpuPredatorAgent& pred = predators[p_idx];
    pred.just_killed = false;
    pred.just_died = false;
    pred.just_born = false;

    // Dekomposisi Mayat Predator yang Mati
    if (!pred.is_alive) {
        if (pred.corpse_energy > 0.0f) {
            pred.corpse_energy = fmaxf(0.0f, pred.corpse_energy - (float)DuniaFisika::CORPSE_DECAY_RATE * (float)dt);
        }
        return;
    }

    if (pred.energy <= 0.0f) {
        pred.is_alive = false;
        pred.just_died = true;
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE;
        return;
    }

    // Friendly Fire & Pemisahan Antar-Predator (Kubu B) - Memberikan gaya dorong pemisah agar tidak menumpuk di pojok
    for (int k = 0; k < 8; ++k) {
        int peer_p = (p_idx + k * 13 + 1) % predators_count;
        if (peer_p != p_idx && predators[peer_p].is_alive && predators[peer_p].energy > 0.0f) {
            float pcdx = predators[peer_p].x - pred.x;
            float pcdy = predators[peer_p].y - pred.y;
            float pcdist2 = pcdx * pcdx + pcdy * pcdy;
            if (pcdist2 < 8.0f * 8.0f && pcdist2 > 1e-4f) { // Kontak dekat saling dorong menjauh
                float pcdist = sqrtf(pcdist2);
                pred.vx -= (pcdx / pcdist) * 2.0f;
                pred.vy -= (pcdy / pcdist) * 2.0f;
            }
        }
    }

    // Penuaan Alami Predator (Senescence)
    pred.age_years += (float)(dt / DuniaFisika::SECONDS_PER_YEAR);
    if (pred.mating_cooldown > 0.0f) {
        pred.mating_cooldown = fmaxf(0.0f, pred.mating_cooldown - (float)dt);
    }

    // 1. SENSORIK PREDATOR: Target Mangsa Hidup, Bangkai (Scavenging), & Formasi Megalitikum
    float nearest_prey_dist = 10000.0f;
    float nearest_prey_dx = 0.0f;
    float nearest_prey_dy = 0.0f;
    float nearest_prey_energy = 0.0f;

    float nearest_corpse_dist = 10000.0f;
    float nearest_corpse_dx = 0.0f;
    float nearest_corpse_dy = 0.0f;
    int nearest_corpse_agent_idx = -1;
    bool is_pred_corpse = false;

    // Scan mangsa hidup dan mayat Kubu A
    for (int i = 0; i < population_size; ++i) {
        float dx = agents[i].x - pred.x;
        float dy = agents[i].y - pred.y;
        float d = sqrtf(dx * dx + dy * dy);

        if (agents[i].is_alive && agents[i].energy > 0.0f) {
            if (d < nearest_prey_dist) {
                nearest_prey_dist = d;
                nearest_prey_dx = dx;
                nearest_prey_dy = dy;
                nearest_prey_energy = agents[i].energy;
            }
        } else if (!agents[i].is_alive && agents[i].corpse_energy > 0.5f) {
            if (d < nearest_corpse_dist) {
                nearest_corpse_dist = d;
                nearest_corpse_dx = dx;
                nearest_corpse_dy = dy;
                nearest_corpse_agent_idx = i;
                is_pred_corpse = false;
            }
        }
    }

    // Scan mayat sesama Predator (Karnivora memakan bangkai apapun yang ada di ekosistem)
    for (int p = 0; p < predators_count; ++p) {
        if (p != p_idx && !predators[p].is_alive && predators[p].corpse_energy > 0.5f) {
            float dx = predators[p].x - pred.x;
            float dy = predators[p].y - pred.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < nearest_corpse_dist) {
                nearest_corpse_dist = d;
                nearest_corpse_dx = dx;
                nearest_corpse_dy = dy;
                nearest_corpse_agent_idx = p;
                is_pred_corpse = true;
            }
        }
    }

    // Cari Deposit Mineral Megalitikum Terdekat untuk Predator
    float nearest_min_dist = 10000.0f;
    float nearest_min_dx = 0.0f;
    float nearest_min_dy = 0.0f;
    int nearest_min_idx = -1;
    for (int m = 0; m < minerals_count; ++m) {
        if (minerals[m].mass <= 0.0f) continue;
        float dx = minerals[m].x - pred.x;
        float dy = minerals[m].y - pred.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d < nearest_min_dist) {
            nearest_min_dist = d;
            nearest_min_dx = dx;
            nearest_min_dy = dy;
            nearest_min_idx = m;
        }
    }

    // Eksploitasi Material Megalitikum oleh Karnivora
    bool pred_has_tool = (pred.tools_crafted > 0);
    if (pred_has_tool) {
        pred.energy = fmaxf(0.0f, pred.energy - (float)DuniaFisika::MEGALITH_TOOL_USAGE_ENERGY_RATE * (float)dt);
    }
    if (nearest_min_idx >= 0 && nearest_min_dist < (float)DuniaFisika::MEGALITH_MINING_RADIUS && minerals[nearest_min_idx].mass > 0.0f) {
        float mined = (float)DuniaFisika::MEGALITH_MINING_RATE * 0.1f * (float)dt;
        atomicAdd(&minerals[nearest_min_idx].mass, - mined);
        pred.material_interactions++;
        pred.mined_material += mined;
        if (pred.mined_material >= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD) {
            if (pred.energy >= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST) {
                pred.mined_material -= (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD;
                pred.tools_crafted++;
                pred.energy -= (float)DuniaFisika::MEGALITH_CRAFT_ENERGY_COST;
            }
        }
    }

    // Pemangsaan Bangkai (Scavenging) Adil oleh Predator
    if (nearest_corpse_agent_idx >= 0 && nearest_corpse_dist < (float)DuniaFisika::CORPSE_SCAVENGE_RADIUS && pred.energy < 100.0f) {
        if (!is_pred_corpse) {
            float bite = fminf(agents[nearest_corpse_agent_idx].corpse_energy, (float)DuniaFisika::PREDATOR_CORPSE_EAT_RATE * (float)dt);
            agents[nearest_corpse_agent_idx].corpse_energy -= bite;
            pred.energy = fminf(100.0f, pred.energy + bite * (float)DuniaFisika::CORPSE_PREDATOR_RECOVERY);
        } else {
            float bite = fminf(predators[nearest_corpse_agent_idx].corpse_energy, (float)DuniaFisika::PREDATOR_CORPSE_EAT_RATE * (float)dt);
            predators[nearest_corpse_agent_idx].corpse_energy -= bite;
            pred.energy = fminf(100.0f, pred.energy + bite * (float)DuniaFisika::CORPSE_PREDATOR_RECOVERY);
        }
    }

    // Pemangsaan Hewan Liar / Ternak oleh Karnivora (Nutrisi lebih kecil dari memangsa kubu agen)
    for (int fn = 0; fn < 8; ++fn) {
        int f_idx = (p_idx + fn * 29) % fauna_count;
        if (fauna[f_idx].is_alive && pred.energy < 100.0f) {
            float fdx = fauna[f_idx].x - pred.x;
            float fdy = fauna[f_idx].y - pred.y;
            float fdist2 = fdx * fdx + fdy * fdy;
            if (fdist2 < (float)(DuniaFisika::PREDATOR_ATTACK_RADIUS * DuniaFisika::PREDATOR_ATTACK_RADIUS)) {
                // Memangsa fauna liar
                fauna[f_idx].energy -= (float)DuniaFisika::PREDATOR_DAMAGE_RATE * (float)dt;
                pred.energy = fminf(100.0f, pred.energy + (float)DuniaFisika::PREDATOR_HUNT_FAUNA_GAIN * 0.2f * (float)dt);
                if (fauna[f_idx].energy <= 0.0f) {
                    fauna[f_idx].is_alive = false;
                    fauna[f_idx].corpse_energy = (float)DuniaFisika::FAUNA_CORPSE_MEAT_VALUE;
                    pred.energy = fminf(100.0f, pred.energy + (float)DuniaFisika::PREDATOR_HUNT_FAUNA_GAIN);
                }
                break;
            }
        }
    }

    // Sensorik Spasial Predator
    float target_dx = (nearest_prey_dist < 10000.0f) ? nearest_prey_dx : 0.0f;
    float target_dy = (nearest_prey_dist < 10000.0f) ? nearest_prey_dy : 0.0f;
    float target_dist = (nearest_prey_dist < 10000.0f) ? nearest_prey_dist : 1000.0f;

    // Jika ada bangkai lebih dekat dari mangsa hidup saat lapar, prioritaskan makan bangkai
    if (nearest_corpse_dist < nearest_prey_dist && pred.energy < 70.0f) {
        target_dx = nearest_corpse_dx;
        target_dy = nearest_corpse_dy;
        target_dist = nearest_corpse_dist;
    }

    // Hitung Jarak ke Alur Sungai Terdekat Predator (Meandering River)
    float p_river_x = (float)DuniaFisika::RIVER_CENTER_X + (float)DuniaFisika::RIVER_MEANDER_AMP * sinf(pred.y * (float)DuniaFisika::RIVER_MEANDER_FREQ);
    float p_dist_to_river = fabsf(pred.x - p_river_x);
    float p_river_dx = p_river_x - pred.x;
    float p_river_dy = 0.0f;

    // Hitung Jarak ke Danau Terdekat Predator (Lake Ecosystem)
    float p_lake_dx = (float)DuniaFisika::LAKE_CENTER_X - pred.x;
    float p_lake_dy = (float)DuniaFisika::LAKE_CENTER_Y - pred.y;
    float p_dist_lake_center = sqrtf(p_lake_dx * p_lake_dx + p_lake_dy * p_lake_dy);
    float p_dist_to_lake = fmaxf(0.0f, p_dist_lake_center - (float)DuniaFisika::LAKE_RADIUS);

    // Tentukan sumber air terdekat bagi Predator
    float p_nearest_water_dist = p_dist_to_river;
    float p_nearest_water_dx = p_river_dx;
    float p_nearest_water_dy = p_river_dy;
    if (p_dist_to_lake < p_nearest_water_dist) {
        p_nearest_water_dist = p_dist_to_lake;
        p_nearest_water_dx = (p_dist_lake_center > 1e-3f) ? (p_lake_dx / p_dist_lake_center) * p_dist_to_lake : 0.0f;
        p_nearest_water_dy = (p_dist_lake_center > 1e-3f) ? (p_lake_dy / p_dist_lake_center) * p_dist_to_lake : 0.0f;
    }

    // Minum Air di Tepi Sungai atau Tepi Danau (Predator)
    if (p_dist_to_river <= (float)DuniaFisika::RIVER_DRINK_RADIUS || p_dist_to_lake <= (float)DuniaFisika::LAKE_DRINK_RADIUS) {
        pred.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, pred.hydration + (float)DuniaFisika::AGENT_DRINK_WATER_RATE * (float)dt);
    }

    // Penyerapan Hidrasi Langsung dari Curah Hujan bagi Predator
    if (climate->rain_intensity > 0.0f) {
        float p_rain_hydrate = (float)DuniaFisika::RAIN_HYDRATION_GAIN_RATE * (climate->rain_intensity / (float)DuniaFisika::RAIN_INTENSITY_MAX) * (float)dt;
        pred.hydration = fminf((float)DuniaFisika::AGENT_HYDRATION_MAX, pred.hydration + p_rain_hydrate);
    }

    // =========================================================================
    // PILAR 2: SENSOR TIGA PILAR (TERKOMPRESI 16 FLOAT) - PREDATOR
    // =========================================================================
    // 1. EXTEROCEPTION (Dunia Luar Predator: S0 - S9)
    float vis_factor = 0.5f + 0.5f * climate->daylight_factor;
    float s0_prey_dx = (target_dx / (target_dist + 1e-3f)) * vis_factor;
    float s1_prey_dy = (target_dy / (target_dist + 1e-3f)) * vis_factor;
    float s2_prey_dist = 1.0f - (float)gpu_clamp(target_dist / 500.0f, 0.0, 1.0);
    float s3_corpse_dx = (nearest_corpse_dist < 10000.0f) ? (nearest_corpse_dx / (nearest_corpse_dist + 1e-3f)) * vis_factor : 0.0f;
    float s4_corpse_dy = (nearest_corpse_dist < 10000.0f) ? (nearest_corpse_dy / (nearest_corpse_dist + 1e-3f)) * vis_factor : 0.0f;
    float s5_corpse_dist = (nearest_corpse_dist < 10000.0f) ? (1.0f - (float)gpu_clamp(nearest_corpse_dist / 300.0f, 0.0, 1.0)) : 0.0f;
    float s6_river_dx = (p_nearest_water_dx / (p_nearest_water_dist + 1e-3f)) * vis_factor; // Sensor arah sumber air (sungai/danau)
    float s7_river_dist = 1.0f - (float)gpu_clamp(p_nearest_water_dist / 300.0f, 0.0, 1.0); // Sensor jarak sumber air
    float s8_comm_recv = (float)tanh(pred.comm_received);
    float s9_climate_ambient = (float)tanh((climate->temperature - 20.0f) * 0.05f) * climate->daylight_factor;

    // 2. INTEROCEPTION (Kondisi Tubuh Predator: S10 - S12)
    float s10_energy = (float)gpu_clamp(pred.energy / 100.0f, 0.0, 1.0);
    float s11_hydration = (float)gpu_clamp(pred.hydration / 100.0f, 0.0, 1.0);
    float raw_pred_stress = (1.0f - s10_energy) * 0.4f + (1.0f - s11_hydration) * 0.3f + (pred.formula_shield) * 0.3f;
    float pred_fear_sig = 1.0f / (1.0f + expf(-(float)ParameterAgent::FEAR_SIGMOID_STEEPNESS * (raw_pred_stress - (float)ParameterAgent::FEAR_SIGMOID_MIDPOINT)));
    float s12_frenzy_drive = (float)gpu_clamp(pred_fear_sig, 0.0, 1.0);

    // 3. PROPRIOCEPTION (Kesadaran Gerak & Senjata Predator: S13 - S15)
    float p_spd = (float)DuniaFisika::PREDATOR_SPEED;
    float s13_vx = (float)gpu_clamp(pred.vx / (p_spd + 1e-3f), -1.0, 1.0);
    float s14_vy = (float)gpu_clamp(pred.vy / (p_spd + 1e-3f), -1.0, 1.0);
    float s15_tool_status = (pred.tools_crafted > 0) ? 1.0f : (float)gpu_clamp(pred.mined_material / (float)DuniaFisika::MEGALITH_CRAFT_THRESHOLD, 0.0, 0.9);

    float pred_sensor_inputs[ParameterAgent::SENSORS_COUNT] = {
        s0_prey_dx, s1_prey_dy, s2_prey_dist, s3_corpse_dx,
        s4_corpse_dy, s5_corpse_dist, s6_river_dx, s7_river_dist,
        s8_comm_recv, s9_climate_ambient, s10_energy, s11_hydration,
        s12_frenzy_drive, s13_vx, s14_vy, s15_tool_status
    };

    // =========================================================================
    // PILAR 3: RECURRENT RESERVOIR LOOP (Spatial Depth 64 Step + Temporal T) - PREDATOR
    // =========================================================================
    int max_r = (pred.active_registers_count > 0 && pred.active_registers_count <= REGISTERS_COUNT) ? 
                pred.active_registers_count : ParameterAgent::MIN_DYNAMIC_REGISTERS;
    int active_prog = (pred.active_program_size > 0 && pred.active_program_size <= DNA_PROGRAM_SIZE) ? 
                      pred.active_program_size : ParameterAgent::MIN_DYNAMIC_PROGRAM_SIZE;

    // Reservoir state update dengan dual-channel input projection + recurrent loop
    float move_cmd_x = 0.0f;
    float move_cmd_y = 0.0f;
    float broadcast_out = 0.0f;

    // Eksekusi Virtual Machine Kognisi Otak Predator (Modular)
    execute_agent_brain_vm(pred, pred_sensor_inputs, s9_climate_ambient, dt, move_cmd_x, move_cmd_y, broadcast_out);
    pred.formula_shield = (float)fabs(broadcast_out);

    // 2d. Swarm Signal Quantization (Predator): {-1.0, 0.0, +1.0}
    float p_quant_thresh = (float)ParameterAgent::SWARM_QUANT_THRESHOLD;
    if (pred.comm_signal > p_quant_thresh)       pred.comm_signal = 1.0f;
    else if (pred.comm_signal < -p_quant_thresh) pred.comm_signal = -1.0f;
    else                                          pred.comm_signal = 0.0f;

    // 3. DINAMIKA GERAK FISIK PREDATOR (Inersia & Hukum Gerak Fisika & Bencana Alam)
    float p_speed_base = (float)DuniaFisika::PREDATOR_SPEED;
    bool p_in_safe_haven = (pred.x > 100.0f && pred.x < 220.0f && pred.y > 100.0f && pred.y < 220.0f);

    float p_wind_x = climate->wind_x * (float)DuniaFisika::WIND_FORCE_MULT;
    float p_wind_y = climate->wind_y * (float)DuniaFisika::WIND_FORCE_MULT;

    // Knockback Badai EMP
    if (climate->active_disaster_type == 2 && !p_in_safe_haven) {
        p_wind_x += sinf(pred.x * 0.01f + climate->magnetic_angle) * (float)DuniaFisika::EMP_SHOCK_KNOCKBACK * climate->disaster_severity;
        p_wind_y += cosf(pred.y * 0.01f + climate->magnetic_angle) * (float)DuniaFisika::EMP_SHOCK_KNOCKBACK * climate->disaster_severity;
    }

    float p_speed_mult = 1.0f;
    // Perlambatan Badai Salju
    if (climate->active_disaster_type == 1 && !p_in_safe_haven) {
        p_speed_mult = fmaxf(0.25f, 1.0f - (1.0f - (float)DuniaFisika::BLIZZARD_SLOWDOWN) * climate->disaster_severity);
    }

    float target_vx = (move_cmd_x * p_speed_base + p_wind_x) * p_speed_mult;
    float target_vy = (move_cmd_y * p_speed_base + p_wind_y) * p_speed_mult;

    pred.vx = pred.vx * (float)DuniaFisika::FRICTION_COEFF + target_vx * 0.2f;
    pred.vy = pred.vy * (float)DuniaFisika::FRICTION_COEFF + target_vy * 0.2f;

    pred.x += pred.vx * (float)dt;
    pred.y += pred.vy * (float)dt;

    // Batas Tepi Dunia (Hukum Pantulan Tumbukan Dinding Elastis)
    if (pred.x < 10.0f) { pred.x = 10.0f; pred.vx = -pred.vx * 0.5f; }
    if (pred.x > (float)DuniaFisika::WORLD_WIDTH - 10.0f) { pred.x = (float)DuniaFisika::WORLD_WIDTH - 10.0f; pred.vx = -pred.vx * 0.5f; }
    if (pred.y < 10.0f) { pred.y = 10.0f; pred.vy = -pred.vy * 0.5f; }
    if (pred.y > (float)DuniaFisika::WORLD_HEIGHT - 10.0f) { pred.y = (float)DuniaFisika::WORLD_HEIGHT - 10.0f; pred.vy = -pred.vy * 0.5f; }

    // 4. Metabolisme & Kematian Predator (Dipengaruhi Kurva Tekanan Alam AI & Paru-Paru)
    if (pred.immortality_timer > 0.0f) {
        pred.immortality_timer = fmaxf(0.0f, pred.immortality_timer - (float)dt);
        pred.energy = fminf(100.0f, pred.energy + 0.5f * (float)dt); // Pulih saat kebal
        pred.lung_oxygen = 100.0f;
    } else {
        // Kerusakan Fisik Bencana Alam Langsung pada Predator di luar Shelter:
        if (!p_in_safe_haven && climate->active_disaster_type >= 0) {
            if (climate->active_disaster_type == 0) {
                // Badai Matahari: Terbakar radiasi
                pred.energy = fmaxf(0.0f, pred.energy - (float)DuniaFisika::SOLAR_STORM_SCORCH_DAMAGE * 0.8f * climate->disaster_severity * (float)dt);
            } else if (climate->active_disaster_type == 1) {
                // Badai Salju: Hipotermia
                pred.energy = fmaxf(0.0f, pred.energy - (float)DuniaFisika::BLIZZARD_FROST_DAMAGE * 0.8f * climate->disaster_severity * (float)dt);
            }
        }

        // Bahaya Banjir Bandang bagi Predator
        if (climate->rain_intensity > 75.0f && !p_in_safe_haven) {
            float flood_river_r = (float)DuniaFisika::RIVER_DRINK_RADIUS * (float)DuniaFisika::FLOOD_EXPANSION_MULT;
            float flood_lake_r = (float)DuniaFisika::LAKE_DRINK_RADIUS * (float)DuniaFisika::FLOOD_EXPANSION_MULT;
            if (p_dist_to_river < flood_river_r || p_dist_to_lake < flood_lake_r) {
                pred.energy = fmaxf(0.0f, pred.energy - (float)DuniaFisika::FLOOD_SWEEP_DAMAGE * (climate->rain_intensity / 100.0f) * (float)dt);
            }
        }

        // Tabung Oksigen Paru-Paru Predator:
        if (climate->oxygen_level >= (float)DuniaFisika::O2_ATMOSPHERE_SAFE_MIN) {
            float o2_gain = (float)DuniaFisika::AGENT_O2_BREATHE_IN_RATE * (climate->oxygen_level / 21.0f) * (float)dt;
            pred.lung_oxygen = fminf((float)DuniaFisika::AGENT_LUNG_CAPACITY, pred.lung_oxygen + o2_gain);
        } else {
            float o2_loss = (float)DuniaFisika::AGENT_O2_CONSUMPTION_RATE * (float)dt;
            pred.lung_oxygen = fmaxf(0.0f, pred.lung_oxygen - o2_loss);
        }

        // Dinamika Dehidrasi Predator (Tergantung Suhu & Gerak)
        float p_speed = sqrtf(pred.vx * pred.vx + pred.vy * pred.vy);
        float p_heat_dehydration = (climate->temperature > 25.0f) ? (climate->temperature - 25.0f) * (float)DuniaFisika::AGENT_DEHYDRATION_HEAT_MULT : 0.0f;
        float p_dehyd_rate = ((float)DuniaFisika::AGENT_DEHYDRATION_BASE_RATE + p_heat_dehydration + p_speed * 0.02f) * (float)dt;
        pred.hydration = fmaxf(0.0f, pred.hydration - p_dehyd_rate);

        float p_lung_eff = fmaxf(0.2f, pred.lung_oxygen / 100.0f);
        float p_hyd_eff = fmaxf(0.2f, pred.hydration / 100.0f);
        float p_nature_stress = climate->nature_adversarial_pressure;
        
        // Penuaan Eksponensial Biologis Predator (Makin Menua Makin Renta di Atas 40 Tahun)
        float p_age_senescence_mult = 1.0f;
        if (pred.age_years > (float)DuniaFisika::AGING_SENESCENCE_START_AGE) {
            float p_aging_delta = pred.age_years - (float)DuniaFisika::AGING_SENESCENCE_START_AGE;
            p_age_senescence_mult = expf((float)DuniaFisika::AGING_EXPONENTIAL_FACTOR * p_aging_delta);
        }

        float p_energy_cost = ((float)DuniaFisika::PREDATOR_METABOLISM + p_speed * (float)DuniaFisika::METABOLISM_MOVE_COST) * p_nature_stress * p_age_senescence_mult * (float)dt / (p_lung_eff * p_hyd_eff);
        pred.energy = fmaxf(0.0f, pred.energy - p_energy_cost);

        // Kerentaan Biologis Alami Organ Predator (Senescence Frailty Damage):
        if (pred.age_years > (float)DuniaFisika::AGING_SENESCENCE_START_AGE) {
            float p_aging_span = pred.age_years - (float)DuniaFisika::AGING_SENESCENCE_START_AGE;
            float p_frailty_dmg = (float)DuniaFisika::AGING_FRAILTY_DAMAGE_BASE * expf((float)DuniaFisika::AGING_EXPONENTIAL_FACTOR * p_aging_span) * (float)dt;
            pred.energy = fmaxf(0.0f, pred.energy - p_frailty_dmg);
        }

        // Hipoksia Predator
        if (pred.lung_oxygen < (float)DuniaFisika::HYPOXIA_THRESHOLD) {
            float suffocation_dmg = (float)DuniaFisika::HYPOXIA_SUFFOCATION_DAMAGE * (1.0f - pred.lung_oxygen / (float)DuniaFisika::HYPOXIA_THRESHOLD) * (float)dt;
            pred.energy = fmaxf(0.0f, pred.energy - suffocation_dmg);
        }

        // Dehidrasi Predator
        if (pred.hydration <= 0.0f) {
            float thirst_dmg = (float)DuniaFisika::DEHYDRATION_DAMAGE_RATE * (float)dt;
            pred.energy = fmaxf(0.0f, pred.energy - thirst_dmg);
        }
    }

    // Transmisi Komunikasi Koordinasi Antar-Predator
    if (fabsf(pred.comm_signal) > 0.1f) {
        for (int k = 0; k < 64; ++k) {
            int peer_idx = (p_idx + k * 19) % predators_count;
            if (peer_idx != p_idx && predators[peer_idx].is_alive) {
                float cdx = predators[peer_idx].x - pred.x;
                float cdy = predators[peer_idx].y - pred.y;
                if ((cdx * cdx + cdy * cdy) < (float)(DuniaFisika::BROADCAST_COMM_RADIUS * DuniaFisika::BROADCAST_COMM_RADIUS * 1.5f)) {
                    predators[peer_idx].comm_received = pred.comm_signal;
                }
            }
        }
    }

    if (pred.immortality_timer <= 0.0f && pred.energy <= 0.0f) {
        pred.is_alive = false;
        pred.just_died = true;
        pred.corpse_energy = (float)DuniaFisika::CORPSE_MAX_ENERGY_RESERVE * 1.5f;
        return;
    }

    // 5. Reproduksi Seksual Predator (Determinisme Presisi)
    float p_req_mate_energy = (float)DuniaFisika::PREDATOR_MATING_MIN_ENERGY;
    if (pred.gender == 1 && pred.energy >= p_req_mate_energy && pred.mating_cooldown <= 0.0f) {
        for (int m = 0; m < predators_count; ++m) {
            float p_partner_req = (float)DuniaFisika::PREDATOR_MATING_MIN_ENERGY;
            if (m == p_idx || !predators[m].is_alive || predators[m].gender != 0 || predators[m].energy < p_partner_req) continue;

            float mdx = predators[m].x - pred.x;
            float mdy = predators[m].y - pred.y;
            float mdist = sqrtf(mdx * mdx + mdy * mdy);

            if (mdist < (float)DuniaFisika::PREDATOR_MATING_RADIUS) {
                // Temukan slot predator kosong untuk melahirkan anak baru
                for (int slot = 0; slot < predators_count; ++slot) {
                    if (!predators[slot].is_alive && predators[slot].energy <= 0.0f) {
                        pred.energy -= (float)DuniaFisika::PREDATOR_MATING_COST;
                        float p_cd = (float)DuniaFisika::PREDATOR_MATING_COOLDOWN;
                        pred.mating_cooldown = p_cd;
                        predators[m].mating_cooldown = p_cd;

                        unsigned int rng_seed = gpu_hash((unsigned int)p_idx * 31337u ^ (unsigned int)slot * 7919u ^ (unsigned int)predators[m].id * 4099u);

                        predators[slot].is_alive = true;
                        predators[slot].just_born = true;
                        predators[slot].just_died = false;
                        predators[slot].generation = max(pred.generation, predators[m].generation) + 1;
                        predators[slot].gender = (rng_seed >> 4) % 2;
                        predators[slot].sin_type = 0;
                        predators[slot].x = pred.x + ((float)((int)(rng_seed % 20) - 10));
                        predators[slot].y = pred.y + ((float)((int)((rng_seed / 20) % 20) - 10));
                        predators[slot].vx = 0.0f;
                        predators[slot].vy = 0.0f;
                        predators[slot].energy = (float)DuniaFisika::NEWBORN_INITIAL_ENERGY;
                        predators[slot].lung_oxygen = (float)DuniaFisika::AGENT_LUNG_CAPACITY;
                        predators[slot].hydration = (float)DuniaFisika::AGENT_HYDRATION_INITIAL;
                        predators[slot].corpse_energy = 0.0f;
                        predators[slot].age_years = 0.0f;
                        predators[slot].prey_devoured = 0;
                        predators[slot].material_interactions = 0;
                        predators[slot].tools_crafted = 0;
                        predators[slot].mined_material = 0.0f;
                        // Cacat Kelahiran Biologis / Mutasi Acak Ekstrim Predator
                        bool is_defective_pred = ((rng_seed % 100) < (int)(DuniaFisika::DEFECTIVE_BIRTH_CHANCE * 100));
                        bool is_super_mutant_pred = ((rng_seed % 100) < (int)(DuniaFisika::SUPER_MUTATION_CHANCE * 100));

                        predators[slot].energy = is_defective_pred ? 8.0f : (float)DuniaFisika::NEWBORN_INITIAL_ENERGY;
                        predators[slot].formula_shield = is_defective_pred ? 0.05f : (float)((rng_seed % 100)) / 100.0f;

                        predators[slot].active_program_size = is_super_mutant_pred ? min((int)DNA_PROGRAM_SIZE, pred.active_program_size + 1 + (int)(rng_seed % 3)) : pred.active_program_size;
                        predators[slot].active_registers_count = pred.active_registers_count;

                        // Crossover DNA & Mutasi Genetik Keturunan Predator
                        float pred_mut_prob = is_super_mutant_pred ? 0.70f : (float)ParameterAgent::DNA_MUTATION_RATE;
                        for (int ip = 0; ip < DNA_PROGRAM_SIZE; ++ip) {
                            predators[slot].dna_program[ip] = (ip % 2 == 0) ? pred.dna_program[ip] : predators[m].dna_program[ip];

                            unsigned int m_hash = gpu_hash(rng_seed ^ ((unsigned int)ip * 131u + (unsigned int)slot * 71u));
                            if ((m_hash % 100) < (int)(pred_mut_prob * 100)) {
                                int mut_type = (m_hash / 100) % 4;
                                if (mut_type == 0 || is_defective_pred) {
                                    predators[slot].dna_program[ip].op = static_cast<unsigned char>((m_hash >> 3) % 15);
                                } else if (mut_type == 1) {
                                    predators[slot].dna_program[ip].r_dest = static_cast<unsigned char>((m_hash >> 5) % REGISTERS_COUNT);
                                } else if (mut_type == 2) {
                                    predators[slot].dna_program[ip].r_src1 = static_cast<unsigned char>((m_hash >> 7) % REGISTERS_COUNT);
                                } else {
                                    predators[slot].dna_program[ip].immediate_val = ((float)((int)(m_hash % 400) - 200)) * 0.1f;
                                }
                            }
                        }

                        // 2. Epigenetic Trauma Inheritance Predator: Trauma stres/kelaparan induk ditransfer ke anak
                        float pred_parent_trauma = 0.5f * (s12_frenzy_drive + 0.5f);
                        float pred_trauma_bias = pred_parent_trauma * (float)ParameterAgent::EPIGENETIC_TRAUMA_WEIGHT_BIAS;

                        // Inherit & mutate reservoir weights (Predator) dengan Trauma Bias
                        for (int r = 0; r < REGISTERS_COUNT; ++r) {
                            predators[slot].registers[r] = 0.0;
                            predators[slot].prev_registers[r] = 0.0;
                            predators[slot].integrated_registers[r] = 0.0;
                            float base_p_win = (r % 2 == 0) ? pred.reservoir_weights_in[r] : predators[m].reservoir_weights_in[r];
                            float base_p_wrec = (r % 2 == 0) ? pred.reservoir_weights_rec[r] : predators[m].reservoir_weights_rec[r];

                            // Perkuat respon sensorik interoception / kelaparan (S10, S11, S12)
                            if (r == 0 || r == 1 || r == 10 % REGISTERS_COUNT || r == 12 % REGISTERS_COUNT) {
                                base_p_win = (base_p_win >= 0.0f) ? base_p_win + pred_trauma_bias : base_p_win - pred_trauma_bias;
                            }

                            predators[slot].reservoir_weights_in[r] = (float)gpu_clamp(base_p_win, -1.0, 1.0);
                            predators[slot].reservoir_weights_rec[r] = (float)gpu_clamp(base_p_wrec, -0.99, 0.99);

                            unsigned int pw_hash = gpu_hash(rng_seed ^ ((unsigned int)r * 277u + (unsigned int)slot * 19u));
                            if ((pw_hash % 100) < (int)(pred_mut_prob * 100)) {
                                predators[slot].reservoir_weights_in[r] = (float)gpu_clamp(predators[slot].reservoir_weights_in[r] + ((float)((int)(pw_hash % 200) - 100)) * 0.005f, -1.0, 1.0);
                                predators[slot].reservoir_weights_rec[r] = (float)gpu_clamp(predators[slot].reservoir_weights_rec[r] + ((float)((int)((pw_hash / 200) % 200) - 100)) * 0.005f, -0.99, 0.99);
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
}

// =========================================================================
// KERNEL SIMULASI FLORA: Padang Rumput, Semak Berry & Tanaman Gandum Pangan
// =========================================================================
__global__ void simulate_flora_step_cuda_kernel(
    GpuFloraPlant* flora,
    int flora_count,
    GpuClimateState* climate,
    double dt
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= flora_count) return;

    GpuFloraPlant& plant = flora[idx];
    if (plant.health <= 0.0f) {
        // Peluang bertunas kembali secara alami dari tanah lembab / spora
        unsigned int r_seed = gpu_hash((unsigned int)idx * 1337u ^ (unsigned int)(plant.x * 17.0f));
        if ((r_seed % 1000) < 5) {
            plant.health = 50.0f;
            plant.growth_stage = 0.0f;
            plant.moisture = 50.0f;
            plant.yield_amount = 0.0f;
        }
        return;
    }

    // Penyerapan air dari sungai, danau & kelembaban udara H2O / curah hujan
    float river_x = (float)DuniaFisika::RIVER_CENTER_X + (float)DuniaFisika::RIVER_MEANDER_AMP * sinf(plant.y * (float)DuniaFisika::RIVER_MEANDER_FREQ);
    float dist_river = fabsf(plant.x - river_x);
    float water_gain = (dist_river < (float)DuniaFisika::RIVER_MOISTURE_RADIUS) ? 1.5f * (float)dt : 0.0f;

    // Danau
    float flk_dx = plant.x - (float)DuniaFisika::LAKE_CENTER_X;
    float flk_dy = plant.y - (float)DuniaFisika::LAKE_CENTER_Y;
    float flk_dist_center = sqrtf(flk_dx * flk_dx + flk_dy * flk_dy);
    float flk_dist = fmaxf(0.0f, flk_dist_center - (float)DuniaFisika::LAKE_RADIUS);
    if (flk_dist < (float)DuniaFisika::LAKE_MOISTURE_RADIUS) {
        float lk_prox = 1.0f - (flk_dist / (float)DuniaFisika::LAKE_MOISTURE_RADIUS);
        water_gain += 2.0f * lk_prox * (float)dt;
    }

    // Curah hujan & Kelembaban
    float rain_flora_mult = 1.0f + (climate->rain_intensity / (float)DuniaFisika::RAIN_INTENSITY_MAX) * (float)DuniaFisika::RAIN_SOIL_MOISTURE_MULT;
    water_gain += (climate->h2o_level / 100.0f) * 0.8f * rain_flora_mult * (float)dt;
    plant.moisture = fminf(100.0f, plant.moisture + water_gain - 0.2f * (float)dt);

    // Pertumbuhan flora bertahap sesuai sinar matahari & air
    float sunlight = fmaxf(0.1f, (0.4f + (float)DuniaFisika::DAY_PHOTOSYNTHESIS_MULT * climate->daylight_factor));
    float moist_mult = fmaxf(0.1f, plant.moisture / 50.0f);
    float growth_rate = (float)DuniaFisika::FLORA_GROWTH_RATE * sunlight * moist_mult;

    plant.growth_stage = fminf(100.0f, plant.growth_stage + growth_rate * (float)dt);
    if (plant.growth_stage >= 60.0f) {
        plant.yield_amount = (plant.flora_type == DuniaFisika::FLORA_GRAIN_CROP) ? 
                             (float)DuniaFisika::FLORA_HARVEST_YIELD : (float)DuniaFisika::FLORA_NUTRITION_VALUE;
    }

    // Kekeringan flora & Kerusakan Akibat Bencana Alam
    if (plant.moisture <= 0.0f) {
        plant.health = fmaxf(0.0f, plant.health - 5.0f * (float)dt);
    } else {
        plant.health = fminf(100.0f, plant.health + 2.0f * (float)dt);
    }

    // Dampak Bencana Ekstrem pada Flora:
    if (climate->active_disaster_type == 0) {
        // Badai Matahari: Membakar tanaman di ladang terbuka
        plant.health = fmaxf(0.0f, plant.health - 6.0f * climate->disaster_severity * (float)dt);
    } else if (climate->active_disaster_type == 1) {
        // Badai Salju: Membekukan pucuk tanaman
        plant.growth_stage = fmaxf(0.0f, plant.growth_stage - 3.0f * climate->disaster_severity * (float)dt);
    }
}

// =========================================================================
// KERNEL SIMULASI FAUNA LIAR: Hewan Ternak Liar & Burung (Realita Penuh)
// =========================================================================
__global__ void simulate_wild_fauna_step_cuda_kernel(
    GpuWildFauna* fauna,
    int fauna_count,
    GpuFloraPlant* flora,
    int flora_count,
    GpuTreeEntity* trees,
    int trees_count,
    GpuPredatorAgent* predators,
    int predators_count,
    GpuEcosystemAgent* agents,
    int population_size,
    GpuClimateState* climate,
    double dt
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= fauna_count) return;

    GpuWildFauna& animal = fauna[idx];
    if (!animal.is_alive) return;

    // Penuaan & metabolisme
    animal.age_years += (float)(dt / DuniaFisika::SECONDS_PER_YEAR);
    if (animal.age_years >= 25.0f || animal.energy <= 0.0f || animal.hydration <= 0.0f) {
        animal.is_alive = false;
        animal.corpse_energy = (float)DuniaFisika::FAUNA_CORPSE_MEAT_VALUE;
        return;
    }

    // Deteksi Predator Terdekat (Insting Bertahan & Kabur)
    float nearest_pred_dist = 10000.0f;
    float flee_dx = 0.0f;
    float flee_dy = 0.0f;
    for (int p = 0; p < predators_count; ++p) {
        if (predators[p].is_alive && predators[p].energy > 0.0f) {
            float pdx = animal.x - predators[p].x;
            float pdy = animal.y - predators[p].y;
            float pd = sqrtf(pdx * pdx + pdy * pdy);
            if (pd < nearest_pred_dist) {
                nearest_pred_dist = pd;
                flee_dx = pdx;
                flee_dy = pdy;
            }
        }
    }

    animal.is_fleeing = (nearest_pred_dist < (float)DuniaFisika::FAUNA_FLEE_RADIUS);

    // Siklus Tidur & Istirahat: Malam hari (daylight < 0.25) & saat lelah jika tidak sedang terancam
    bool is_night = (climate->daylight_factor < 0.25f);
    animal.is_sleeping = (!animal.is_fleeing && (is_night || (animal.stamina < 15.0f && !animal.is_fleeing)));

    if (animal.is_sleeping) {
        animal.vx *= 0.5f;
        animal.vy *= 0.5f;
        animal.stamina = fminf(100.0f, animal.stamina + (float)DuniaFisika::FAUNA_SLEEP_RECOVERY_RATE * (float)dt);
        animal.energy = fmaxf(0.0f, animal.energy - (float)DuniaFisika::FAUNA_METABOLISM_BASE * 0.4f * (float)dt);
    } else if (animal.is_fleeing && animal.stamina > 5.0f) {
        // SPRINT LARI CEPAT KABUR DARI PREDATOR
        float spd = (animal.fauna_type == DuniaFisika::FAUNA_WILD_BIRD) ? 
                    (float)DuniaFisika::FAUNA_BIRD_SPEED_FLY : (float)DuniaFisika::FAUNA_CATTLE_SPEED_FLEE;
        float inv_d = 1.0f / (nearest_pred_dist + 1e-3f);
        animal.vx = flee_dx * inv_d * spd;
        animal.vy = flee_dy * inv_d * spd;
        animal.stamina = fmaxf(0.0f, animal.stamina - 12.0f * (float)dt); // Stamina terkuras saat lari
        animal.energy = fmaxf(0.0f, animal.energy - (float)DuniaFisika::FAUNA_METABOLISM_BASE * 2.5f * (float)dt);
        animal.hydration = fmaxf(0.0f, animal.hydration - 0.5f * (float)dt);
    } else {
        // AKTIVITAS ALAMI ATAU JINAK / TERNAK
        float walk_spd = (animal.fauna_type == DuniaFisika::FAUNA_WILD_BIRD) ? 8.0f : (float)DuniaFisika::FAUNA_CATTLE_SPEED_WALK;
        
        // Cek Majikan (Domestikasi): Jika jinak, ikuti agen pemilik
        bool following_owner = false;
        if (animal.owner_agent_id > 0 && animal.owner_faction == 0) {
            int owner_idx = (animal.owner_agent_id - 1) % population_size;
            if (agents[owner_idx].id == animal.owner_agent_id && agents[owner_idx].is_alive) {
                float odx = agents[owner_idx].x - animal.x;
                float ody = agents[owner_idx].y - animal.y;
                float odist = sqrtf(odx * odx + ody * ody);
                if (odist > 15.0f && odist < (float)DuniaFisika::FAUNA_DOMESTIC_FOLLOW_RADIUS) {
                    animal.vx = (odx / (odist + 1e-3f)) * walk_spd;
                    animal.vy = (ody / (odist + 1e-3f)) * walk_spd;
                    following_owner = true;
                }
            } else {
                animal.owner_agent_id = 0; // Pemilik mati -> Kembali liar
                animal.owner_faction = -1;
            }
        }

        if (!following_owner) {
            // Cek Sumber Air Terdekat (Sungai vs Danau) untuk Minum saat Haus
            float river_x = (float)DuniaFisika::RIVER_CENTER_X + (float)DuniaFisika::RIVER_MEANDER_AMP * sinf(animal.y * (float)DuniaFisika::RIVER_MEANDER_FREQ);
            float dist_river = fabsf(animal.x - river_x);

            float f_lk_dx = (float)DuniaFisika::LAKE_CENTER_X - animal.x;
            float f_lk_dy = (float)DuniaFisika::LAKE_CENTER_Y - animal.y;
            float f_lk_center = sqrtf(f_lk_dx * f_lk_dx + f_lk_dy * f_lk_dy);
            float dist_lake = fmaxf(0.0f, f_lk_center - (float)DuniaFisika::LAKE_RADIUS);

            if (dist_river < (float)DuniaFisika::RIVER_DRINK_RADIUS || dist_lake < (float)DuniaFisika::LAKE_DRINK_RADIUS) {
                animal.hydration = fminf(100.0f, animal.hydration + (float)DuniaFisika::AGENT_DRINK_WATER_RATE * (float)dt);
            } else if (animal.hydration < 40.0f) {
                // Bergerak mendekat ke sumber air terdekat
                if (dist_lake < dist_river) {
                    animal.vx = (f_lk_center > 1e-3f) ? (f_lk_dx / f_lk_center) * walk_spd : 0.0f;
                    animal.vy = (f_lk_center > 1e-3f) ? (f_lk_dy / f_lk_center) * walk_spd : 0.0f;
                } else {
                    animal.vx = (river_x > animal.x ? walk_spd : -walk_spd);
                }
            } else {
                // Wander acak merumput
                unsigned int h = gpu_hash((unsigned int)idx * 1973u ^ (unsigned int)(animal.age_years * 10.0f));
                animal.vx = (float)((int)(h % 3) - 1) * walk_spd * 0.5f;
                animal.vy = (float)((int)((h / 3) % 3) - 1) * walk_spd * 0.5f;
            }

            // Hidrasi langsung dari curah hujan
            if (climate->rain_intensity > 0.0f) {
                animal.hydration = fminf(100.0f, animal.hydration + (float)DuniaFisika::RAIN_HYDRATION_GAIN_RATE * 0.5f * (climate->rain_intensity / (float)DuniaFisika::RAIN_INTENSITY_MAX) * (float)dt);
            }
        }

        // Makan Rumput / Flora di sekitar
        for (int fl = 0; fl < 16; ++fl) {
            int f_idx = (idx + fl * 31) % flora_count;
            if (flora[f_idx].health > 20.0f && flora[f_idx].growth_stage >= 40.0f && animal.energy < 90.0f) {
                float fdx = flora[f_idx].x - animal.x;
                float fdy = flora[f_idx].y - animal.y;
                if ((fdx * fdx + fdy * fdy) < (float)(DuniaFisika::FAUNA_GRAZING_RADIUS * DuniaFisika::FAUNA_GRAZING_RADIUS)) {
                    animal.energy = fminf(100.0f, animal.energy + flora[f_idx].yield_amount * 0.5f);
                    flora[f_idx].growth_stage = fmaxf(10.0f, flora[f_idx].growth_stage - 30.0f);
                    break;
                }
            }
        }

        // Budidaya & Reproduksi Ternak Jinak (Perkembangbiakan saat kenyang)
        if (animal.owner_agent_id > 0 && animal.energy > 75.0f && animal.age_years >= 2.0f) {
            unsigned int b_hash = gpu_hash((unsigned int)idx * 997u ^ (unsigned int)(animal.age_years * 100.0f));
            if ((b_hash % 1000) < (int)(DuniaFisika::FAUNA_DOMESTIC_BREED_CHANCE * 1000.0f * dt)) {
                for (int slot = 0; slot < fauna_count; ++slot) {
                    if (!fauna[slot].is_alive) {
                        fauna[slot].is_alive = true;
                        fauna[slot].fauna_type = animal.fauna_type;
                        fauna[slot].x = animal.x + (float)((slot % 3) - 1) * 6.0f;
                        fauna[slot].y = animal.y + (float)((slot / 3) - 1) * 6.0f;
                        fauna[slot].vx = 0.0f;
                        fauna[slot].vy = 0.0f;
                        fauna[slot].energy = 80.0f;
                        fauna[slot].hydration = 100.0f;
                        fauna[slot].stamina = 100.0f;
                        fauna[slot].age_years = 0.0f;
                        fauna[slot].corpse_energy = 0.0f;
                        fauna[slot].owner_agent_id = animal.owner_agent_id;
                        fauna[slot].owner_faction = animal.owner_faction;
                        fauna[slot].is_sleeping = false;
                        fauna[slot].is_fleeing = false;
                        animal.energy -= 20.0f;
                        break;
                    }
                }
            }
        }

        animal.stamina = fminf(100.0f, animal.stamina + 2.0f * (float)dt);
        animal.energy = fmaxf(0.0f, animal.energy - (float)DuniaFisika::FAUNA_METABOLISM_BASE * (float)dt);
        animal.hydration = fmaxf(0.0f, animal.hydration - 0.15f * (float)dt);
    }

    // Fisika Bencana Alam pada Fauna (Dorongan Angin Badai & Kerusakan Termal/Beku)
    bool fa_in_safe_haven = (animal.x > 100.0f && animal.x < 220.0f && animal.y > 100.0f && animal.y < 220.0f);
    if (!fa_in_safe_haven && climate->active_disaster_type >= 0) {
        if (climate->active_disaster_type == 2) {
            // Badai EMP / Hurikan: Terhempas angin kencang
            animal.vx += sinf(animal.x * 0.01f + climate->magnetic_angle) * (float)DuniaFisika::EMP_SHOCK_KNOCKBACK * 0.8f * climate->disaster_severity;
            animal.vy += cosf(animal.y * 0.01f + climate->magnetic_angle) * (float)DuniaFisika::EMP_SHOCK_KNOCKBACK * 0.8f * climate->disaster_severity;
        } else if (climate->active_disaster_type == 0) {
            // Badai Matahari: Dehidrasi kilat & kerusakan panas
            animal.energy = fmaxf(0.0f, animal.energy - (float)DuniaFisika::SOLAR_STORM_SCORCH_DAMAGE * 0.6f * climate->disaster_severity * (float)dt);
            animal.hydration = fmaxf(0.0f, animal.hydration - 1.0f * climate->disaster_severity * (float)dt);
        } else if (climate->active_disaster_type == 1) {
            // Badai Salju: Hipotermia & perlambatan
            animal.energy = fmaxf(0.0f, animal.energy - (float)DuniaFisika::BLIZZARD_FROST_DAMAGE * 0.6f * climate->disaster_severity * (float)dt);
            animal.vx *= (float)DuniaFisika::BLIZZARD_SLOWDOWN;
            animal.vy *= (float)DuniaFisika::BLIZZARD_SLOWDOWN;
        }
    }

    // Fisika Gerak
    animal.x += animal.vx * (float)dt;
    animal.y += animal.vy * (float)dt;

    // Batas Tepi Dunia
    if (animal.x < 15.0f) { animal.x = 15.0f; animal.vx = -animal.vx * 0.5f; }
    if (animal.x > (float)DuniaFisika::WORLD_WIDTH - 15.0f) { animal.x = (float)DuniaFisika::WORLD_WIDTH - 15.0f; animal.vx = -animal.vx * 0.5f; }
    if (animal.y < 15.0f) { animal.y = 15.0f; animal.vy = -animal.vy * 0.5f; }
    if (animal.y > (float)DuniaFisika::WORLD_HEIGHT - 15.0f) { animal.y = (float)DuniaFisika::WORLD_HEIGHT - 15.0f; animal.vy = -animal.vy * 0.5f; }
}

extern "C" void launch_ecosystem_simulation(
    GpuEcosystemAgent* d_agents,
    int population_size,
    GpuTreeEntity* d_trees,
    int trees_count,
    GpuPredatorAgent* d_predators,
    int predators_count,
    GpuMineralDeposit* d_minerals,
    int minerals_count,
    GpuFloraPlant* d_flora,
    int flora_count,
    GpuWildFauna* d_fauna,
    int fauna_count,
    GpuClimateState* d_climate,
    double dt
) {
    int blockSize = 128;
    int numBlocksAgents = (population_size + blockSize - 1) / blockSize;
    simulate_ecosystem_step_cuda_kernel<<<numBlocksAgents, blockSize>>>(
        d_agents, population_size, d_trees, trees_count, d_predators, predators_count, d_minerals, minerals_count, d_flora, flora_count, d_fauna, fauna_count, d_climate, dt
    );

    int numBlocksPred = (predators_count + blockSize - 1) / blockSize;
    simulate_predator_step_cuda_kernel<<<numBlocksPred, blockSize>>>(
        d_predators, predators_count, d_agents, population_size, d_minerals, minerals_count, d_fauna, fauna_count, d_climate, dt
    );

    int numBlocksFlora = (flora_count + blockSize - 1) / blockSize;
    simulate_flora_step_cuda_kernel<<<numBlocksFlora, blockSize>>>(
        d_flora, flora_count, d_climate, dt
    );

    int numBlocksFauna = (fauna_count + blockSize - 1) / blockSize;
    simulate_wild_fauna_step_cuda_kernel<<<numBlocksFauna, blockSize>>>(
        d_fauna, fauna_count, d_flora, flora_count, d_trees, trees_count, d_predators, predators_count, d_agents, population_size, d_climate, dt
    );

    cudaDeviceSynchronize();
}

extern "C" void launch_ecosystem_simulation_batched(
    GpuEcosystemAgent* d_agents,
    int population_size,
    GpuTreeEntity* d_trees,
    int trees_count,
    GpuPredatorAgent* d_predators,
    int predators_count,
    GpuMineralDeposit* d_minerals,
    int minerals_count,
    GpuFloraPlant* d_flora,
    int flora_count,
    GpuWildFauna* d_fauna,
    int fauna_count,
    GpuClimateState* d_climate,
    double dt,
    int substeps_count
) {
    int blockSize = 128;
    int numBlocksAgents = (population_size + blockSize - 1) / blockSize;
    int numBlocksPred = (predators_count + blockSize - 1) / blockSize;
    int numBlocksFlora = (flora_count + blockSize - 1) / blockSize;
    int numBlocksFauna = (fauna_count + blockSize - 1) / blockSize;

    for (int step = 0; step < substeps_count; ++step) {
        simulate_ecosystem_step_cuda_kernel<<<numBlocksAgents, blockSize>>>(
            d_agents, population_size, d_trees, trees_count, d_predators, predators_count, d_minerals, minerals_count, d_flora, flora_count, d_fauna, fauna_count, d_climate, dt
        );

        simulate_predator_step_cuda_kernel<<<numBlocksPred, blockSize>>>(
            d_predators, predators_count, d_agents, population_size, d_minerals, minerals_count, d_fauna, fauna_count, d_climate, dt
        );

        simulate_flora_step_cuda_kernel<<<numBlocksFlora, blockSize>>>(
            d_flora, flora_count, d_climate, dt
        );

        simulate_wild_fauna_step_cuda_kernel<<<numBlocksFauna, blockSize>>>(
            d_fauna, fauna_count, d_flora, flora_count, d_trees, trees_count, d_predators, predators_count, d_agents, population_size, d_climate, dt
        );
    }
    // Asynchronous queue — biarkan GPU stream terisi tanpa CPU blocking stall
}