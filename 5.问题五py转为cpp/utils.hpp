#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include "optimizer.hpp" // Contains the definition for GlobalStrategy

void save_global_strategy_to_csv(
    const std::string& filename,
    const GlobalStrategy& strategy
);

#endif // UTILS_HPP
