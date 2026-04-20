#pragma once

#include <set>

namespace Latency
{
    const int START         = 0;
    const int LD            = 3;
    const int ST            = 1;
    const int ADD_SUB_REG   = 1;
    const int ADD_SUB_IMM   = 1;
    const int SHL_SHR       = 1;
    const int MOV           = 1;
    const int MUL_REG       = 4;
    const int MUL_IMM       = 4;
    const int DIV_REG       = 7;
    const int DIV_IMM       = 7;
    const int END           = 0;
}

enum class Op
{
    START,
    LD,
    ST,
    ADD,
    SUB,
    SHL,
    SHR,
    MOV,
    MUL,
    DIV,
    END
};

enum class Form
{
    NONE,
    LD_BASE_IMM,
    LD_IMM,
    ST_BASE_IMM,
    ST_IMM,
    RD_RS_RS,
    RD_RS_IMM,
    RD_IMM
};

namespace Units
{
    const int NUM_LSU       = 1;
    const int NUM_ARITH     = 1;
    const int NUM_ARITH_C   = 1;

    const std::set<Op> LSU_OPS = {Op::LD, Op::ST};
    const std::set<Op> ARITH_OPS = {Op::ADD, Op::SUB, Op::SHL, Op::SHR, Op::MOV};
    const std::set<Op> ARITH_C_OPS = {Op::ADD, Op::SUB, Op::SHL, Op::SHR, Op::MOV, Op::MUL, Op::DIV};
}