#pragma once

namespace DuniaFisika {
    // Parameter Konstan Astrofisika Universal
    constexpr double G = 6.67430e-11;               // Gravitasi Universal (N·m²/kg²)
    constexpr double M_EARTH = 5.972e24;            // Massa Bumi (kg)
    constexpr double R_EARTH = 6.371e6;             // Jari-jari Bumi (meter)
    constexpr double M_MOON = 7.342e22;             // Massa Bulan (kg)
    constexpr double R_MOON = 1.737e6;              // Jari-jari Bulan (meter)
    constexpr double MOON_ORBIT_DIST = 384400000.0; // Jarak Orbit Bumi-ke-Bulan (meter)
    
    // Parameter Propulsi & Atmosfer
    constexpr double ISP = 450.0;                   // Specific Impulse (detik)
    constexpr double G0 = 9.80665;                  // Gravitasi Standar Bumi (m/s²)
    constexpr double ROCKET_DRY_MASS = 5000.0;      // Massa Kosong Roket (kg)
    constexpr double ROCKET_FUEL_MASS = 120000.0;   // Kapasitas Bahan Bakar Maks (kg)
    constexpr double MAX_THRUST = 3600000.0;        // Gaya Dorong Mesin Maksimal (Newton)
    constexpr double AIR_DENSITY_SEA = 1.225;       // Densitas Udara Permukaan Laut (kg/m³)
    constexpr double SCALE_HEIGHT = 8500.0;         // Skala Tinggi Atmosfer Bumi (meter)
    constexpr double DRAG_COEFFICIENT = 0.3;        // Koefisien Hambat Cd
    constexpr double CROSS_SECTION_AREA = 10.0;     // Luas Penampang Roket A (m²)
}

