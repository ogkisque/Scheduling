#pragma once

#include "const.hpp"
#include "dotter.hpp"

#include <string>
#include <optional>
#include <vector>
#include <cassert>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <limits>
#include <memory>

int get_latency(Op op, Form form)
{
    switch (op)
    {
        case Op::START: return Latency::START;
        case Op::LD:    return Latency::LD;
        case Op::ST:    return Latency::ST;
        case Op::ADD:
        case Op::SUB:
            return (form == Form::RD_RS_RS) ? Latency::ADD_SUB_REG : Latency::ADD_SUB_IMM;
        case Op::SHL:
        case Op::SHR:   return Latency::SHL_SHR;
        case Op::MOV:   return Latency::MOV;
        case Op::MUL:   
            return (form == Form::RD_RS_RS) ? Latency::MUL_REG : Latency::MUL_IMM;
        case Op::DIV:
            return (form == Form::RD_RS_RS) ? Latency::DIV_REG : Latency::DIV_IMM;
        case Op::END:   return Latency::END;
        default:        assert(0);
    }
}

namespace graph
{

struct Instruction
{
    int id = -1;
    int lineNo = -1;
    Op op = Op::START;
    Form form = Form::NONE;
    std::string text;

    std::optional<std::string> dst_reg;
    std::vector<std::string> src_regs;

    bool is_load = false;
    bool is_store = false;
    int latency = 0;
};

struct Edge
{
    int id = -1;
    int from = -1;
    int to = -1;
    int latency = 0;
};

struct Node
{
    Instruction instr;

    std::vector<int> in_edges;
    std::vector<int> out_edges;

    int early_time;
    int late_time;
    bool scheduled = false;
};

class DataFlowGraph
{
public:
    explicit DataFlowGraph(std::vector<Instruction> instructions)
    {
        nodes_.reserve(instructions.size());
        for (int i = 0; i < instructions.size(); i++)
        {
            nodes_.push_back(std::make_shared<Node>());
        }
        
        for (size_t i = 0; i < instructions.size(); ++i)
        {
            nodes_[i]->instr = std::move(instructions[i]);

            if (nodes_[i]->instr.op == Op::START)
            {
                start_id_ = i;
            }
            if (nodes_[i]->instr.op == Op::END)
            {
                end_id_ = i;
            }
        }

        assert(start_id_ >= 0);
        assert(end_id_ >= 0);

        build();
    }

    void print(std::ostream& os) const
    {
        os << "Data Flow Graph\n";
        os << "========================\n";

        for (const auto& node : nodes_)
        {
            const int id = node->instr.id;

            os << "[" << id << "] "
               << node->instr.text
               << "    (lat=" << node->instr.latency
               << ")\n";

            os << "  in: ";
            if (node->in_edges.empty()) {
                os << "-";
            }
            else
            {
                for (size_t i = 0; i < node->in_edges.size(); ++i)
                {
                    if (i) os << ", ";
                    auto e = edges_[node->in_edges[i]];
                    os << "[" << e->from << " -> " << e->to;
                    os << "]";
                }
            }
            os << "\n";

            os << "  out: ";
            if (node->out_edges.empty())
            {
                os << "-";
            }
            else
            {
                for (size_t i = 0; i < node->out_edges.size(); ++i)
                {
                    if (i) os << ", ";
                    auto e = edges_[node->out_edges[i]];
                    os << "[" << e->from << " -> " << e->to;
                    os << "]";
                }
            }
            os << "\n\n";
        }
    }

    void print_dotter(std::string& graph_name) const
    {
        dotter::Dotter dotter;
        static dotter::NodeStyle sched_style(dotter::NodeStyle::SHAPES::BOX, dotter::NodeStyle::STYLES::ROUNDED,
                                             dotter::COLORS::BLACK, dotter::COLORS::RED, dotter::COLORS::BLACK);

        for (auto& node : nodes_)
        {
            std::string name = "[" + std::to_string(node->instr.id) + "]\n" +
                               node->instr.text + "\n" + std::to_string(node->early_time)
                                                + "\n" + std::to_string(node->late_time);
            if (node->scheduled)
                dotter.AddNode(name, node->instr.id, sched_style);
            else
                dotter.AddNode(name, node->instr.id);
        }

        for (auto& edge : edges_)
        {
            dotter.AddLink(std::to_string(edge->latency), edge->from, edge->to);
        }

        std::string dot_name = graph_name + ".dot";
        std::string png_name = graph_name + ".png";
        dotter.PrintDotText(dot_name);
        dotter.Render(dot_name, png_name);
    }

    void calc_early_late_time()
    {
        for (int i = 0; i < nodes_.size(); i++)
        {
            int early = 0;
            for (int in_edge : nodes_[i]->in_edges)
            {
                int lat = edges_[in_edge]->latency;
                int prev_early = nodes_[edges_[in_edge]->from]->early_time;
                early = std::max(early, lat + prev_early);
            }
            nodes_[i]->early_time = early;
        }

        nodes_[nodes_.size() - 1]->late_time = nodes_[nodes_.size() - 1]->early_time;
        for (int i = nodes_.size() - 2; i > 0; i--)
        {
            int late = std::numeric_limits<int>::max();
            for (int out_edge : nodes_[i]->out_edges)
            {
                int lat = edges_[out_edge]->latency;
                int prev_late = nodes_[edges_[out_edge]->to]->late_time;
                late = std::min(late, prev_late - lat);
            }
            nodes_[i]->late_time = late;
        }
    }

    void recalc_early_late_time(int cur_cycle)
    {
        for (int i = 0; i < nodes_.size(); i++)
        {
            if (!nodes_[i]->scheduled)
            {
                
                int early = cur_cycle;
                for (int in_edge : nodes_[i]->in_edges)
                {
                    int lat = edges_[in_edge]->latency;
                    auto child = nodes_[edges_[in_edge]->from];
                    int prev_early = child->early_time;
                    early = std::max(early, lat + prev_early);
                }

                //printf("AAAA. cycle %d; %s; early %d\n",
                        //cur_cycle, nodes_[i]->instr.text.c_str(), nodes_[i]->early_time);

                nodes_[i]->early_time = early;
                //if (nodes_[i]->early_time < cur_cycle)
                    //nodes_[i]->early_time = early;
            }
        }

        nodes_[nodes_.size() - 1]->late_time = nodes_[nodes_.size() - 1]->early_time;
        for (int i = nodes_.size() - 2; i > 0; i--)
        {
            int late = std::numeric_limits<int>::max();
            for (int out_edge : nodes_[i]->out_edges)
            {
                int lat = edges_[out_edge]->latency;
                int prev_late = nodes_[edges_[out_edge]->to]->late_time;
                late = std::min(late, prev_late - lat);
            }
            nodes_[i]->late_time = late;
        }
    }

    bool all_scheduled() const
    {
        for (auto& node : nodes_)
        {
            if (node->instr.id != end_id_ && !node->scheduled)
                return false;
        }
        return true;
    }

    std::vector<std::shared_ptr<Node>> nodes_;
    std::vector<std::shared_ptr<Edge>> edges_;
    int start_id_ = -1;
    int end_id_ = -1;

private:
    bool has_edge(int from, int to) const
    {
        for (int eid : nodes_[from]->out_edges)
        {
            auto e = edges_[eid];
            if (e->from == from && e->to == to)
            {
                return true;
            }
        }
        return false;
    }

    int add_edge(int from, int to, int latency)
    {
        assert(from >= 0);
        assert(from < nodes_.size());
        assert(to >= 0);
        assert(to < nodes_.size());

        if (has_edge(from, to))
        {
            return -1;
        }

        auto e = std::make_shared<Edge>(edges_.size(), from, to, latency);

        edges_.push_back(e);
        nodes_[from]->out_edges.push_back(e->id);
        nodes_[to]->in_edges.push_back(e->id);

        return e->id;
    }

    void build()
    {
        std::unordered_map<std::string, int> last_writer;
        int last_memory_op = -1;
        int last_store = -1;

        for (int i = 0; i < nodes_.size(); ++i)
        {
            auto& ins = nodes_[i]->instr;

            if (ins.op == Op::START || ins.op == Op::END)
            {
                continue;
            }

            for (const auto& src : ins.src_regs)
            {
                auto it = last_writer.find(src);
                if (it != last_writer.end())
                {
                    add_edge(it->second, i, nodes_[it->second]->instr.latency);
                }
            }

            if (ins.is_load)
            {
                if (last_store != -1)
                {
                    add_edge(last_store, i, nodes_[last_store]->instr.latency);
                }
            }
            else if (ins.is_store)
            {
                if (last_memory_op != -1)
                {
                    add_edge(last_memory_op, i, nodes_[last_memory_op]->instr.latency);
                }
            }

            if (nodes_[i]->in_edges.empty())
            {
                add_edge(start_id_, i, 0);
            }

            if (ins.dst_reg.has_value())
            {
                last_writer[*ins.dst_reg] = i;
            }

            if (ins.is_load || ins.is_store)
            {
                last_memory_op = i;
            }
            if (ins.is_store)
            {
                last_store = i;
            }
        }

        for (int i = 0; i < nodes_.size(); i++)
        {
            if (i == end_id_)
            {
                continue;
            }
            if (nodes_[i]->out_edges.empty())
            {
                add_edge(i, end_id_, nodes_[i]->instr.latency);
            }
        }
    }
};

}