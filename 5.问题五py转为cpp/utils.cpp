#include "utils.hpp"
#include "core_objects.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>

void save_global_strategy_to_csv(
    const std::string& filename,
    const GlobalStrategy& strategy)
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }

    // Set precision for floating point numbers
    file << std::fixed << std::setprecision(4);

    // Write header
    file << "Object,Target Missile,Speed (m/s),Angle (rad),Deploy Time (s),Fuse Time (s),"
         << "Deploy Pos X,Deploy Pos Y,Deploy Pos Z,"
         << "Detonate Pos X,Detonate Pos Y,Detonate Pos Z\n";

    // Create temporary UAV objects to calculate positions
    std::map<std::string, UAV> uavs;
    for(const auto& [uav_id, uav_strat] : strategy) {
        uavs.emplace(uav_id, UAV(uav_id));
    }

    // Iterate through the sorted strategy
    for (const auto& [uav_id, uav_strat] : strategy) {
        auto& uav = uavs.at(uav_id);
        uav.set_flight_strategy(uav_strat.speed, uav_strat.angle);

        // Write UAV info row
        file << "UAV " << uav_id << ","
             << "N/A" << ","
             << uav_strat.speed << ","
             << uav_strat.angle << ","
             << "N/A,N/A,N/A,N/A,N/A,N/A,N/A,N/A\n";

        // Write grenade info rows
        int grenade_idx = 1;
        for (const auto& g_strat : uav_strat.grenades) {
            Eigen::Vector3d deploy_pos = uav.get_position(g_strat.t_deploy);
            
            // This recalculates the trajectory, which is acceptable for a utility function
            Grenade grenade(deploy_pos, uav.get_velocity_vec(), g_strat.t_deploy, g_strat.t_fuse);
            Eigen::Vector3d detonate_pos = grenade.get_detonate_pos();

            file << "\"  - Grenade " << grenade_idx++ << " (from " << uav_id << ")\"" << ","
                 << g_strat.target_missile << ","
                 << "N/A,N/A," // Speed, Angle
                 << g_strat.t_deploy << ","
                 << g_strat.t_fuse << ","
                 << deploy_pos.x() << "," << deploy_pos.y() << "," << deploy_pos.z() << ","
                 << detonate_pos.x() << "," << detonate_pos.y() << "," << detonate_pos.z() << "\n";
        }
    }

    std::cout << "\nGlobal strategy successfully saved to: " << filename << std::endl;
}
