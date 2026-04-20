#pragma once

#include "const.hpp"

#include <string>
#include <set>

namespace Units
{

class Unit
{
public:
    Unit(const std::set<Op>& ops, std::string& name) : ops_(ops), busy_to_cycle(-1), name_(name) {}

    bool try_push_instr(int cycle, Op op, int latency)
    {
        if (ops_.count(op) == 0 || cycle < busy_to_cycle)
            return false;

        busy_to_cycle = cycle + latency;
        return true;
    }

    std::string get_name() const
    {
        return name_;
    }
private:
    const std::set<Op> ops_;
    std::string name_;
    int busy_to_cycle;
};
    
}