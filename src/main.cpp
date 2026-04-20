#include <fstream>
#include <cassert>
#include "parser.hpp"
#include "graph.hpp"
#include "scheduler.hpp"

int main(int argc, char* argv[])
{
    assert(argc <= 2);

    std::ifstream file;
    std::istream* in = &std::cin;

    if (argc == 2)
    {
        file.open(argv[1]);
        assert(file.is_open());
        in = &file;
    }

    auto instructions = parser::Parser::parse(*in);
    auto dfg = std::make_shared<graph::DataFlowGraph>(instructions);
    dfg->print(std::cout);
    scheduler::Scheduler scheduler(dfg);

    //dfg->print(std::cout);
    //std::string dot_name = "test";
    //dfg->print_dotter(dot_name);

    //scheduler.dump();

    scheduler.schedule();

    return 0;
}