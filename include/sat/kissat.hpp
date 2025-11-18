#ifndef MY_KISSAT_SOLVER_H
#define MY_KISSAT_SOLVER_H

#include "sat/cnf.hpp"
#include <string>
#include <vector>

enum class SatSolverResultType { SAT, UNSAT };

struct SatSolverResult {
    SatSolverResultType result;
    std::vector<int> numbers;
    std::vector<std::string> proof_lines;
    [[nodiscard]] std::string to_string() const;
    void print() const;
};

SatSolverResult launch_kissat(const Cnf& cnf);

#endif