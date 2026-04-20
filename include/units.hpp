#pragma once

#include "const.hpp"

#include <string>
#include <set>

namespace Units
{

class Unit
{
public:
    Unit(const std::set<Op>& ops, std::string& name) : ops_(ops), name_(name), busy_cycle_(-1) {}

    bool try_push_instr(int cycle, Op op)
    {
        if (ops_.count(op) == 0 || cycle == busy_cycle_)
        {
            return false;
        }

        busy_cycle_ = cycle;
        return true;
    }

    std::string get_name() const
    {
        return name_;
    }
private:
    const std::set<Op> ops_;
    std::string name_;
    int busy_cycle_;
};
    
}