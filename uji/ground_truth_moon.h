#pragma once
#include <cmath>
#include <algorithm>

struct RocketState {
    double altitude;
    double velocity;
    double fuel_mass;
    double dry_mass;
    double distance_to_moon;
    bool landed_safely;
    bool crashed;
};

class GroundTruthMoonPhysicsEngine {
public:
    static constexpr double G = 6.67430e-11;
    static constexpr double M_EARTH = 5.972e24;
    static constexpr double R_EARTH = 6.371e6;
    static constexpr double M_MOON = 7.342e22;
    static constexpr double R_MOON = 1.737e6;
    static constexpr double MOON_ORBIT_DIST = 384400000.0;
    static constexpr double ISP = 450.0;
    static constexpr double G0 = 9.80665;

    RocketState rocket;
    double current_time_step;

    GroundTruthMoonPhysicsEngine() {
        reset_simulation();
    }

    void reset_simulation() {
        rocket.altitude = 0.0;
        rocket.velocity = 0.0;
        rocket.dry_mass = 5000.0;
        rocket.fuel_mass = 120000.0;
        rocket.distance_to_moon = MOON_ORBIT_DIST;
        rocket.landed_safely = false;
        rocket.crashed = false;
        current_time_step = 0.0;
    }

    double step_physics(double thrust_ratio, double dt = 1.0) {
        if (rocket.landed_safely || rocket.crashed) return 0.0;

        current_time_step += dt;
        double r_earth = R_EARTH + rocket.altitude;
        double r_moon = rocket.distance_to_moon + R_MOON;

        double g_earth = (G * M_EARTH) / (r_earth * r_earth);
        double g_moon = (G * M_MOON) / (r_moon * r_moon);
        double net_gravity = -g_earth + g_moon;

        double thrust_force = 0.0;
        double max_thrust = 980000.0;

        if (rocket.fuel_mass > 0.0) {
            thrust_force = std::clamp(thrust_ratio, 0.0, 1.0) * max_thrust;
            double mass_flow = thrust_force / (ISP * G0);
            double fuel_burned = mass_flow * dt;
            if (fuel_burned > rocket.fuel_mass) {
                fuel_burned = rocket.fuel_mass;
                thrust_force = (fuel_burned / dt) * ISP * G0;
            }
            rocket.fuel_mass -= fuel_burned;
        }

        double air_density = 1.225 * std::exp(-rocket.altitude / 8500.0);
        double drag_force = 0.5 * air_density * rocket.velocity * std::abs(rocket.velocity) * 4.5 * 0.2;
        if (rocket.velocity < 0) drag_force = -drag_force;

        double total_mass = rocket.dry_mass + rocket.fuel_mass;
        double acceleration = (thrust_force - drag_force) / total_mass + net_gravity;

        rocket.velocity += acceleration * dt;
        rocket.altitude += rocket.velocity * dt;
        rocket.distance_to_moon = MOON_ORBIT_DIST - rocket.altitude;

        if (rocket.distance_to_moon <= 0.0) {
            rocket.distance_to_moon = 0.0;
            if (std::abs(rocket.velocity) <= 2.5) {
                rocket.landed_safely = true;
            } else {
                rocket.crashed = true;
            }
        }

        if (rocket.altitude < 0.0 && current_time_step > 5.0) {
            rocket.altitude = 0.0;
            rocket.crashed = true;
        }

        return (rocket.altitude / MOON_ORBIT_DIST);
    }
};
