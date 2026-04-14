#include <fstream>
#include <cassert>
#include "parser.hpp"
#include "graph.hpp"

int main(int argc, char* argv[])
{
    assert(argc <= 2);

    std::ifstream file;
    std::istream* in = &std::cin;

    if (argc == 2) {
        file.open(argv[1]);
        assert(file.is_open());
        in = &file;
    }

    auto instructions = parser::Parser::parse(*in);
    DataFlowGraph dfg(instructions);
    dfg.print(std::cout);

    return 0;
}