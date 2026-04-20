#pragma once

#include "graph.hpp"
#include "const.hpp"
#include "units.hpp"

#include <memory>
#include <vector>
#include <list>

namespace scheduler
{

class Scheduler
{
public:
    explicit Scheduler(std::shared_ptr<graph::DataFlowGraph> dfg) : 
        dfg_(dfg), cur_cycle_(0)
    {
        dfg_->calc_early_late_time();
        for (auto& node : dfg_->nodes_)
        {
            if (node->instr.id == dfg_->end_id_)
                continue;
            if (node->instr.id == dfg_->start_id_)
            {
                node->scheduled = true;
                continue;
            }
            
            if (node->early_time == 0)
                ready_.push_back(node);
            else
                not_ready_.push_back(node);
        }

        for (int i = 0; i < Units::NUM_LSU; i++)
        {
            std::string name = "LSU" + std::to_string(i);
            units_.push_back(std::make_shared<Units::Unit>(Units::LSU_OPS, name));
        }

        for (int i = 0; i < Units::NUM_ARITH; i++)
        {
            std::string name = "ARITH" + std::to_string(i);
            units_.push_back(std::make_shared<Units::Unit>(Units::ARITH_OPS, name));
        }

        for (int i = 0; i < Units::NUM_ARITH_C; i++)
        {
            std::string name = "ARITH_C" + std::to_string(i);
            units_.push_back(std::make_shared<Units::Unit>(Units::ARITH_C_OPS, name));
        }
    }

    void schedule()
    {
        while (!dfg_->all_scheduled() && cur_cycle_ <= 15)
        {
            dfg_->recalc_early_late_time(cur_cycle_);
            process_lists();
            //dump();

            ready_.sort([]( const std::shared_ptr<graph::Node>& a,
                            const std::shared_ptr<graph::Node>& b)
                            {
                                return a->late_time < b->late_time;
                            });

            for (auto it = ready_.begin(); it != ready_.end(); )
            {
                if (may_be_executed(*it))
                    it = ready_.erase(it);
                else
                    ++it;
            }

            std::string name = "gr" + std::to_string(cur_cycle_);
            dfg_->print_dotter(name);
            cur_cycle_++;
        }
    }

    void dump() const
    {
        std::cout << "########Cycle " << cur_cycle_ << "########\n";
        std::cout << "-------------------------\nREADY\n";
        for (auto& node : ready_)
        {
            std::cout << "[" << node->instr.id << "] " << node->instr.text << std::endl;
        }

        std::cout << "-------------------------\nPARTIAL READY\n";
        for (auto& node : partial_ready_)
        {
            std::cout << "[" << node->instr.id << "] " << node->instr.text << std::endl;
        }

        std::cout << "-------------------------\nNOT READY\n";
        for (auto& node : not_ready_)
        {
            std::cout << "[" << node->instr.id << "] " << node->instr.text << std::endl;
        }

        std::cout << "######################\n";
    }

private:
    bool may_be_executed(std::shared_ptr<graph::Node> node)
    {
        for (auto& unit : units_)
        {
            if (unit->try_push_instr(cur_cycle_, node->instr.op))
            {
                printf("[%d] %s: %s {id %d}\n", cur_cycle_, unit->get_name().c_str(),
                                                node->instr.text.c_str(), node->instr.id);
                node->scheduled = true;
                return true;
            }
        }
        return false;
    }

    void process_lists()
    {
        for (auto it = partial_ready_.begin(); it != partial_ready_.end(); )
        {
            auto node = *it;
            if (node->early_time <= cur_cycle_)
            {
                it = partial_ready_.erase(it);
                ready_.push_back(node);
            }
            else
            {
                ++it;
            }
        }

        for (auto it = not_ready_.begin(); it != not_ready_.end(); )
        {
            if (is_all_parents_scheduled(*it))
            {
                auto node = *it;
                it = not_ready_.erase(it);
                if (node->early_time <= cur_cycle_)
                    ready_.push_back(node);
                else
                    partial_ready_.push_back(node);
            }
            else
            {
                ++it;
            }
        }
    }

    bool is_all_parents_scheduled(std::shared_ptr<graph::Node> node)
    {
        for (auto& edge : node->in_edges)
        {
            if (!dfg_->nodes_[dfg_->edges_[edge]->from]->scheduled)
                return false;
        }
        return true;
    }

    std::shared_ptr<graph::DataFlowGraph> dfg_;
    std::vector<std::shared_ptr<Units::Unit>> units_;

    std::list<std::shared_ptr<graph::Node>> ready_;
    std::list<std::shared_ptr<graph::Node>> partial_ready_;
    std::list<std::shared_ptr<graph::Node>> not_ready_;

    int cur_cycle_;
};

}