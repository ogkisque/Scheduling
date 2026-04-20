#pragma once

#include "graph.hpp"

#include <string>
#include <cassert>

namespace parser
{

class LineParser
{
public:
    explicit LineParser(const std::string& s) : s_(s) {}

    void skip_spaces()
    {
        while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_])))
        {
            ++pos_;
        }
    }

    void expect(char c)
    {
        skip_spaces();
        assert(pos_ < s_.size());
        assert(s_[pos_] == c);
        ++pos_;
    }

    std::string parse_word()
    {
        skip_spaces();
        assert(pos_ < s_.size());
        assert(is_word_start(s_[pos_]));

        size_t begin = pos_;
        pos_++;

        while (pos_ < s_.size() && is_word_char(s_[pos_]))
        {
            pos_++;
        }

        return ascii_upper(s_.substr(begin, pos_ - begin));
    }

    bool next_is_int()
    {
        skip_spaces();
        if (pos_ >= s_.size())
        {
            return false;
        }

        size_t p = pos_;
        if (s_[p] == '-')
        {
            ++p;
        }
        if (p >= s_.size())
        {
            return false;
        }

        if (std::isdigit(static_cast<unsigned char>(s_[p])))
        {
            return true;
        }

        return false;
    }

    int parse_int()
    {
        skip_spaces();
        assert(pos_ < s_.size());

        size_t begin = pos_;

        if (s_[pos_] == '-') {
            ++pos_;
        }

        assert(pos_ < s_.size());
        assert(std::isdigit(static_cast<unsigned char>(s_[pos_])));

        if (s_[pos_] == '0' &&
            pos_ + 1 < s_.size() &&
            (s_[pos_ + 1] == 'x' || s_[pos_ + 1] == 'X')) {
            pos_ += 2;
            assert(pos_ < s_.size());
            assert(std::isxdigit(static_cast<unsigned char>(s_[pos_])));

            while (pos_ < s_.size() && std::isxdigit(static_cast<unsigned char>(s_[pos_])))
            {
                pos_++;
            }
        }
        else
        {
            while (pos_ < s_.size() && std::isdigit(static_cast<unsigned char>(s_[pos_])))
            {
                pos_++;
            }
        }

        const std::string token = s_.substr(begin, pos_ - begin);
        return std::stoi(token, nullptr, 0);
    }

    bool next_is_word()
    {
        skip_spaces();
        return pos_ < s_.size() && is_word_start(s_[pos_]);
    }

    char peek()
    {
        skip_spaces();
        assert(pos_ < s_.size());
        return s_[pos_];
    }

private:
    static std::string ascii_upper(std::string s)
    {
        for (char& c : s)
        {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return s;
    }

    static bool is_word_start(char c)
    {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
    }

    static bool is_word_char(char c)
    {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
    }

    const std::string& s_;
    size_t pos_ = 0;
};

static std::string trim(const std::string& s)
{
    size_t l = 0;
    while (l < s.size() && std::isspace(static_cast<unsigned char>(s[l]))) {
        ++l;
    }

    size_t r = s.size();
    while (r > l && std::isspace(static_cast<unsigned char>(s[r - 1]))) {
        --r;
    }

    return s.substr(l, r - l);
}

class Parser
{
public:
    static std::vector<graph::Instruction> parse(std::istream& in)
    {
        std::vector<graph::Instruction> result;

        std::string line;
        int lineNo = 0;
        int id = 0;

        while (std::getline(in, line))
        {
            ++lineNo;

            const std::string s = trim(line);
            if (s.empty())
            {
                continue;
            }
            if (s.rfind("//", 0) == 0 || s.rfind("#", 0) == 0)
            {
                continue;
            }

            result.push_back(parse_line(s, id, lineNo));
            ++id;
        }

        assert(!result.empty());
        assert(result.front().op == Op::START);
        assert(result.back().op == Op::END);

        return result;
    }

private:
    static graph::Instruction parse_line(const std::string& s, int id, int lineNo)
    {
        LineParser p(s);

        graph::Instruction ins;
        ins.id = id;
        ins.lineNo = lineNo;
        ins.text = s;

        const std::string opcode = p.parse_word();

        if (opcode == "START")
        {
            ins.op = Op::START;
            ins.form = Form::NONE;
            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        if (opcode == "END")
        {
            ins.op = Op::END;
            ins.form = Form::NONE;
            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        if (opcode == "LD")
        {
            ins.op = Op::LD;

            ins.dst_reg = p.parse_word();
            p.expect(',');
            p.expect('[');

            if (p.next_is_word())
            {
                const std::string base = p.parse_word();
                ins.src_regs.push_back(base);

                p.skip_spaces();
                if (p.peek() == '+')
                {
                    p.expect('+');
                    p.parse_int();
                }

                p.expect(']');
                ins.form = Form::LD_BASE_IMM;
            }
            else
            {
                p.parse_int();
                p.expect(']');
                ins.form = Form::LD_IMM;
            }

            ins.is_load = true;
            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        if (opcode == "ST")
        {
            ins.op = Op::ST;

            const std::string valueReg = p.parse_word();
            ins.src_regs.push_back(valueReg);

            p.expect(',');
            p.expect('[');

            if (p.next_is_word())
            {
                const std::string base = p.parse_word();
                ins.src_regs.push_back(base);

                p.skip_spaces();
                if (p.peek() == '+')
                {
                    p.expect('+');
                    p.parse_int();
                }

                p.expect(']');
                ins.form = Form::ST_BASE_IMM;
            }
            else
            {
                p.parse_int();
                p.expect(']');

                ins.form = Form::ST_IMM;
            }

            ins.is_store = true;
            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        if (opcode == "ADD" || opcode == "SUB")
        {
            ins.op = (opcode == "ADD") ? Op::ADD : Op::SUB;

            ins.dst_reg = p.parse_word();
            p.expect(',');
            ins.src_regs.push_back(p.parse_word());
            p.expect(',');

            if (p.next_is_word())
            {
                ins.src_regs.push_back(p.parse_word());
                ins.form = Form::RD_RS_RS;
            }
            else
            {
                assert(p.next_is_int());
                p.parse_int();
                ins.form = Form::RD_RS_IMM;
            }

            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        if (opcode == "SHL" || opcode == "SHR")
        {
            ins.op = (opcode == "SHL") ? Op::SHL : Op::SHR;

            ins.dst_reg = p.parse_word();
            p.expect(',');
            ins.src_regs.push_back(p.parse_word());
            p.expect(',');

            if (p.next_is_word())
            {
                ins.src_regs.push_back(p.parse_word());
                ins.form = Form::RD_RS_RS;
            }
            else
            {
                assert(p.next_is_int());
                p.parse_int();
                ins.form = Form::RD_RS_IMM;
            }

            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        if (opcode == "MOV")
        {
            ins.op = Op::MOV;

            ins.dst_reg = p.parse_word();
            p.expect(',');

            if (p.next_is_word())
            {
                ins.src_regs.push_back(p.parse_word());
                ins.form = Form::RD_RS_IMM;
            }
            else
            {
                assert(p.next_is_int());
                p.parse_int();
                ins.form = Form::RD_IMM;
            }

            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        if (opcode == "MUL" || opcode == "DIV")
        {
            ins.op = (opcode == "MUL") ? Op::MUL : Op::DIV;

            ins.dst_reg = p.parse_word();
            p.expect(',');
            ins.src_regs.push_back(p.parse_word());
            p.expect(',');

            if (p.next_is_word())
            {
                ins.src_regs.push_back(p.parse_word());
                ins.form = Form::RD_RS_RS;
            }
            else
            {
                assert(p.next_is_int());
                p.parse_int();
                ins.form = Form::RD_RS_IMM;
            }

            ins.latency = get_latency(ins.op, ins.form);
            return ins;
        }

        assert(0);
    }
};

}