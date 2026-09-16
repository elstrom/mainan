#pragma once

namespace DuniaFisika {
// =========================================================================
// PARAMETER LINGKUNGAN, FISIKA & BIOLOGI UJIAN EKOSISTEM
// =========================================================================

// Dimensi Dunia 2D Ekosistem (Meter) — Skala 50 km x 50 km
constexpr double WORLD_WIDTH = 50000.0;
constexpr double WORLD_HEIGHT = 50000.0;

// Populasi & Buffer GPU
constexpr int INITIAL_POPULATION = 1000;
constexpr int MAX_POPULATION_BUFFER = 5000;

// Konstanta Fisika & Medan
constexpr double FRICTION_COEFF = 0.88;
constexpr double MAX_AGENT_SPEED = 40.0;
constexpr double MAGNETIC_FIELD_BASE = 1.0;
constexpr double WIND_FORCE_MULT = 0.2;

// Konfigurasi Mode Eksekusi & Headless Benchmark
constexpr bool HEADLESS_MODE = true;
constexpr int HEADLESS_ENV_UPLOAD_INTERVAL = 30;
constexpr bool STOP_ON_EXTINCTION = true;
constexpr bool AUTO_RESTART_ON_EXTINCTION = true;
inline const char *EVOLUTION_LOG_FILE = "d:/MyProjects/mainan/agent/catatan/evolution_milestones.log";
constexpr double TIME_ACCELERATION_FACTOR = 2.0;
constexpr double SECONDS_PER_DAY = 60.0;
constexpr int DAYS_PER_SEASON = 15;
constexpr int SEASONS_COUNT = 4;

// Parameter Metabolisme, Usia, Gender & Reproduksi Manusia
constexpr double SECONDS_PER_YEAR = 7200.0;
constexpr double SPAWN_INITIAL_AGE_MIN = 8.0;
constexpr double SPAWN_INITIAL_AGE_MAX = 18.0;
constexpr double AGING_SENESCENCE_START_AGE = 45.0;
constexpr double AGING_LIFE_EXPECTANCY = 80.0;
constexpr double AGING_EXPONENTIAL_FACTOR = 0.05;
constexpr double AGING_FRAILTY_DAMAGE_BASE = 0.01;
constexpr double MATING_RADIUS = 25.0;
constexpr double MATING_MIN_ENERGY = 40.0;
constexpr double MATING_ENERGY_COST = 8.0;
constexpr double NEWBORN_INITIAL_ENERGY = 40.0;
constexpr double MATING_COOLDOWN = 6.0;
constexpr double INITIAL_ENERGY = 100.0;
constexpr double METABOLISM_BASE_RATE = 0.02;
constexpr double METABOLISM_MOVE_COST = 0.008;
constexpr double CLIMATE_HUNGER_IMPACT_MULT = 0.005;
constexpr double STARVATION_THRESHOLD = 0.0;

// Parameter Dehidrasi & Hidrasi
constexpr double AGENT_HYDRATION_MAX = 100.0;
constexpr double AGENT_HYDRATION_INITIAL = 100.0;
constexpr double AGENT_DEHYDRATION_BASE_RATE = 0.01;
constexpr double AGENT_DEHYDRATION_HEAT_MULT = 0.005;
constexpr double AGENT_DRINK_WATER_RATE = 40.0;
constexpr double DEHYDRATION_THIRST_THRESHOLD = 25.0;
constexpr double DEHYDRATION_DAMAGE_RATE = 1.0;

// Parameter Paru-Paru, Tabung Oksigen & Hipoksia
constexpr double AGENT_LUNG_CAPACITY = 100.0;
constexpr double AGENT_O2_BREATHE_IN_RATE = 20.0;
constexpr double AGENT_O2_CONSUMPTION_RATE = 2.0;
constexpr double HYPOXIA_THRESHOLD = 20.0;
constexpr double HYPOXIA_SUFFOCATION_DAMAGE = 12.0;
constexpr double O2_ATMOSPHERE_SAFE_MIN = 14.0;

// Parameter Predator (Faksi B)
constexpr int INITIAL_PREDATORS = 200;
constexpr int MAX_PREDATORS_BUFFER = 5000;
constexpr double PREDATOR_SPEED = 42.0;
constexpr double PREDATOR_ATTACK_RADIUS = 16.0;
constexpr double PREDATOR_DAMAGE_RATE = 25.0;
constexpr double PREDATOR_ENERGY_GAIN = 25.0;
constexpr double PREDATOR_INITIAL_ENERGY = 100.0;
constexpr double PREDATOR_METABOLISM = 0.02;
constexpr double PREDATOR_MATING_RADIUS = 15.0;
constexpr double PREDATOR_MATING_MIN_ENERGY = 35.0;
constexpr double PREDATOR_MATING_COST = 10.0;
constexpr double PREDATOR_MATING_COOLDOWN = 10.0;

// Parameter Domestikasi & Peternakan
constexpr double FAUNA_CAPTURE_RADIUS = 18.0;
constexpr double FAUNA_CAPTURE_ENERGY_COST = 3.0;
constexpr double FAUNA_DOMESTIC_FOLLOW_RADIUS = 60.0;
constexpr double FAUNA_DOMESTIC_BREED_CHANCE = 0.08;
constexpr double FAUNA_DOMESTIC_YIELD_RATE = 0.25;
constexpr double MANUSIA_HUNT_FAUNA_GAIN = 30.0;
constexpr double PREDATOR_HUNT_FAUNA_GAIN = 12.0;
constexpr double MANUSIA_CORPSE_MEAT_RECOVERY = 0.70;

// Sistem Pertarungan & Era Megalitikum
constexpr double MEGALITH_MINING_RADIUS = 35.0;
constexpr double MEGALITH_MINING_RATE = 2.0;
constexpr double MEGALITH_CRAFT_THRESHOLD = 5.0;
constexpr double MEGALITH_CRAFT_ENERGY_COST = 4.0;
constexpr double MEGALITH_TOOL_USAGE_ENERGY_RATE = 0.015;
constexpr double MEGALITH_WEAPON_ATTACK_BONUS = 15.0;
constexpr double MEGALITH_SHIELD_DEFENSE_BONUS = 0.40;
constexpr double MANUSIA_BASE_ATTACK_DAMAGE = 20.0;
constexpr double COMBAT_CLASH_RADIUS = 16.0;
constexpr double CORPSE_PREDATOR_RECOVERY = 0.85;
constexpr double FRIENDLY_FIRE_DAMAGE_RATE = 8.0;
constexpr double PREDATOR_CORPSE_EAT_RATE = 15.0;
constexpr double CORPSE_SCAVENGE_RADIUS = 22.0;
constexpr double SHELTER_THERMAL_INSULATION = 0.60;
constexpr double CONDUCTOR_COMM_AMPLIFY = 1.80;

// Parameter Geologi & Tanah
constexpr double SOIL_FERTILITY_BASE = 1.0;
constexpr double SOIL_FERTILITY_VARIATION = 0.6;
constexpr double SOIL_NOISE_SCALE_X = 0.00016;
constexpr double SOIL_NOISE_SCALE_Y = 0.00016;

// Parameter Sungai & Danau
constexpr double RIVER_CENTER_X = 25000.0;
constexpr double RIVER_MEANDER_AMP = 7000.0;
constexpr double RIVER_MEANDER_FREQ = 0.00014;
constexpr double RIVER_WIDTH = 250.0;
constexpr double RIVER_DRINK_RADIUS = 100.0;
constexpr double RIVER_MOISTURE_RADIUS = 600.0;

constexpr double LAKE_CENTER_X = 15000.0;
constexpr double LAKE_CENTER_Y = 25000.0;
constexpr double LAKE_RADIUS = 1800.0;
constexpr double LAKE_DRINK_RADIUS = 150.0;
constexpr double LAKE_MOISTURE_RADIUS = 1200.0;

// Parameter Presipitasi / Hujan
constexpr double RAIN_HUMIDITY_THRESHOLD = 70.0;
constexpr double RAIN_INTENSITY_MAX = 100.0;
constexpr double RAIN_HYDRATION_GAIN_RATE = 15.0;
constexpr double RAIN_SOIL_MOISTURE_MULT = 1.5;
constexpr double RAIN_EVAPORATION_RATE = 0.08;
constexpr double RAIN_CYCLE_FREQUENCY = 0.015;

// Parameter Pohon
constexpr double TREE_MOISTURE_MAX = 100.0;
constexpr double TREE_MOISTURE_INITIAL = 70.0;
constexpr double TREE_WATER_CONSUMPTION_RATE = 0.30;
constexpr double TREE_WATER_ABSORB_RIVER_RATE = 1.2;
constexpr double TREE_WATER_ABSORB_RAIN_RATE = 0.8;
constexpr double TREE_DROUGHT_DECAY_RATE = 4.0;
constexpr double SEED_GERMINATION_ENERGY_COST = 5.0;
constexpr double SEED_GERMINATION_WATER_COST = 3.0;

enum PlantFloraType {
  FLORA_GRASS = 0,
  FLORA_BERRY_BUSH = 1,
  FLORA_GRAIN_CROP = 2
};
constexpr int MAX_FLORA_PLANTS = 1500;
constexpr int INITIAL_FLORA_COUNT = 800;
constexpr double FLORA_GROWTH_RATE = 0.40;
constexpr double FLORA_NUTRITION_VALUE = 15.0;
constexpr double FLORA_HARVEST_YIELD = 35.0;
constexpr double FLORA_RESEED_RADIUS = 50.0;

enum WildFaunaType {
  FAUNA_WILD_CATTLE = 0,
  FAUNA_WILD_BIRD = 1
};
constexpr int MAX_WILD_FAUNA = 2000;
constexpr int INITIAL_WILD_FAUNA = 500;
constexpr double FAUNA_CATTLE_SPEED_WALK = 4.0;
constexpr double FAUNA_CATTLE_SPEED_FLEE = 14.0;
constexpr double FAUNA_BIRD_SPEED_FLY = 18.0;
constexpr double FAUNA_FLEE_RADIUS = 40.0;
constexpr double FAUNA_GRAZING_RADIUS = 15.0;
constexpr double FAUNA_METABOLISM_BASE = 0.08;
constexpr double FAUNA_SLEEP_RECOVERY_RATE = 4.0;
constexpr double FAUNA_CORPSE_MEAT_VALUE = 40.0;

enum TreeSpeciesType {
  TREE_TYPE_FRUIT = 0,
  TREE_TYPE_OXYGEN = 1,
  TREE_TYPE_PIONEER = 2
};
constexpr int TREE_SPECIES_COUNT = 3;
constexpr double MIN_TREE_SPACING = 30.0;
constexpr double TREE_OXYGEN_SPECIES_O2_MULT = 2.5;
constexpr double TREE_OXYGEN_SPECIES_CO2_MULT = 2.2;
constexpr double TREE_REALISTIC_GROWTH_BASE = 0.35;
constexpr double TREE_SPROUT_SOIL_MIN_FERTILITY = 0.4;

constexpr int MAX_TREES = 1000;
constexpr int INITIAL_TREES_COUNT = 800;
constexpr double TREE_INTERACTION_RADIUS = 120.0;
constexpr double MAX_FRUIT_PER_TREE = 8.0;
constexpr double FRUIT_NUTRITION_ENERGY = 25.0;
constexpr double TREE_ENERGY_POOL_RATE = 10.0;
constexpr double TREE_MAX_AGE_YEARS = 8.0;
constexpr double TREE_HEALTH_DECAY = 0.5;
constexpr double TREE_RESONANCE_BOOST = 0.8;
constexpr double FRUIT_SPAWN_RATE = 0.45;
constexpr double TREE_RESONANCE_THRESHOLD = 0.25;
constexpr double OVERCROWDING_RADIUS = 20.0;
constexpr double OVERCROWDING_PENALTY_MULT = 0.05;

// Parameter Bencana Alam
constexpr double DISASTER_INTERVAL_SECONDS = 240.0;
constexpr double DISASTER_DURATION_SECONDS = 12.0;
constexpr double SOLAR_STORM_HEAT_SPIKE = 22.0;
constexpr double SOLAR_STORM_SCORCH_DAMAGE = 15.0;
constexpr double BLIZZARD_TEMP_DROP = -25.0;
constexpr double BLIZZARD_FROST_DAMAGE = 12.0;
constexpr double BLIZZARD_SLOWDOWN = 0.35;
constexpr double EMP_MAGNETIC_CHAOS = 8.0;
constexpr double EMP_SHOCK_KNOCKBACK = 25.0;
constexpr double FLOOD_EXPANSION_MULT = 1.8;
constexpr double FLOOD_SWEEP_DAMAGE = 20.0;

// Parameter Adversarial Nature AI
constexpr double NATURE_PRESSURE_TARGET_POP = 1000.0;
constexpr double NATURE_PRESSURE_KP = 0.005;
constexpr double NATURE_ENTROPY_DECAY_RATE = 0.05;
constexpr double NATURE_RESOURCE_SCARCITY_MULT = 1.8;
constexpr double RESERVOIR_LEAK_RATE = 0.15;
constexpr double RESERVOIR_CHAOS_NONLINEARITY = 1.4;

constexpr double PATHOGEN_INFECTION_RADIUS = 25.0;
constexpr double PATHOGEN_DAMAGE_RATE = 1.0;
constexpr double CORPSE_TOXICITY_RADIUS = 30.0;
constexpr double CORPSE_TOXICITY_DAMAGE = 8.0;
constexpr double EROSION_TERRAIN_SPEED = 0.5;
constexpr double SAFE_HAVEN_RADIUS = 60.0;
constexpr int SAFE_HAVEN_MAX_CAPACITY = 20;
constexpr double TOOL_RUST_DECAY_RATE = 0.02;
constexpr double GEOTHERMAL_LAVA_RADIUS = 40.0;
constexpr double GEOTHERMAL_LAVA_DAMAGE = 35.0;
constexpr double WATER_SALINITY_DRAIN = 4.0;
constexpr double SPATIAL_DRAG_SLOWDOWN = 0.50;

// Parameter Siklus Siang & Malam
constexpr double DAY_NIGHT_CYCLE_SECONDS = 60.0;
constexpr double DAYLIGHT_TEMP_BOOST = 8.0;
constexpr double NIGHT_TEMP_DROP = -8.0;
constexpr double NIGHT_VISIBILITY_FACTOR = 0.45;
constexpr double DAY_PHOTOSYNTHESIS_MULT = 1.6;

// Parameter Komunikasi
constexpr double BROADCAST_COMM_RADIUS = 40.0;
constexpr double BROADCAST_ENERGY_COST = 0.05;

// Parameter Mutasi & Kelahiran
constexpr double SUPER_MUTATION_CHANCE = 0.08;
constexpr double DEFECTIVE_BIRTH_CHANCE = 0.03;

enum PeriodicTableGroup {
  ELEM_ALKALI_METALS = 0,
  ELEM_ALKALINE_EARTH = 1,
  ELEM_TRANSITION_METALS = 2,
  ELEM_POST_TRANSITION = 3,
  ELEM_METALLOIDS = 4,
  ELEM_REACTIVE_NONMETALS = 5,
  ELEM_HALOGENS = 6,
  ELEM_NOBLE_GASES = 7,
  ELEM_PRECIOUS_METALS = 8,
  ELEM_RADIOACTIVE_ACTINIDES = 9
};
constexpr int PERIODIC_GROUPS_COUNT = 10;
constexpr int MAX_PERIODIC_DEPOSITS = 256;
constexpr double MINERAL_HARDNESS_DECAY = 0.4;
constexpr double MATERIAL_REGEN_RATE = 0.15;

// Parameter Atmosfer
constexpr double OXYGEN_BASE_LEVEL = 21.0;
constexpr double CO2_BASE_LEVEL = 0.04;
constexpr double H2O_HUMIDITY_BASE_LEVEL = 60.0;
constexpr double NITROGEN_BASE_LEVEL = 78.0;
constexpr double PHOTOSYNTHESIS_O2_RATE = 0.08;
constexpr double TREE_RESPIRATION_CO2_RATE = 0.03;
constexpr double RESPIRATION_O2_CONSUMPTION = 0.005;
constexpr double RESPIRATION_CO2_EMISSION = 0.004;
constexpr double GREENHOUSE_CO2_WARMING_FACTOR = 3.0;
constexpr double GREENHOUSE_H2O_WARMING_FACTOR = 0.01;

// Parameter Bias Intervensi Keseimbangan Koloni
constexpr double DEFAULT_GROWTH_BIAS = 50.0;

// Parameter Sistem Artifact (Ujian Aritmatika & Keabadian)
constexpr double ARTIFACT_CLAIM_RADIUS = 35.0;
constexpr double ARTIFACT_IMMORTALITY_DAYS = 20.0;
constexpr double ARTIFACT_ANSWER_TOLERANCE = 0.12;
constexpr double ARTIFACT_BALANCER_POP_THRESHOLD = 0.05;
constexpr int ARTIFACT_BALANCER_SPAWN_COUNT = 5;

constexpr double NATURE_AGGRESSION_MAX = 5.0;
constexpr double NATURE_AGGRESSION_MIN = 0.5;
constexpr double NATURE_REBOUND_SPEED = 0.02;

// Parameter Mayat / Bangkai
constexpr double CORPSE_MAX_ENERGY_RESERVE = 30.0;
constexpr double CORPSE_DECAY_RATE = 2.0;

// Parameter Simulasi Deterministik
constexpr unsigned int FIXED_SIMULATION_SEED = 42;
constexpr double FIXED_SUBSTEP_DT = 0.033;
constexpr int GPU_SUBSTEPS_BATCH = 128;
constexpr int CHECKPOINT_INTERVAL_DAYS = 5;
inline const char* ECOSYSTEM_CHECKPOINT_FILE = "d:/MyProjects/mainan/agent/ingatan/ecosystem_checkpoint.bin";
inline const char* WEB_DASHBOARD_FILE = "d:/MyProjects/mainan/web/index.html";
} // namespace DuniaFisika