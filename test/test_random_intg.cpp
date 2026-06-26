#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <unistd.h>

#include "cas/intg.hpp"

namespace {
std::string make_number(std::mt19937_64& rng, bool allow_negative, bool non_zero, int max_length = 40) {
    std::uniform_int_distribution<int> len_dist(1, max_length);
    std::uniform_int_distribution<int> digit_dist(0, 9);

    const int len = len_dist(rng);
    std::string s;
    if (allow_negative && (rng() % 2 == 0)) {
        s.push_back('-');
    }

    if (non_zero) {
        s.push_back(static_cast<char>('1' + (digit_dist(rng) % 9)));
    } else {
        s.push_back(static_cast<char>('0' + digit_dist(rng)));
    }

    for (int i = 1; i < len; ++i) {
        s.push_back(static_cast<char>('0' + digit_dist(rng)));
    }

    return s;
}

std::string shell_quote(const std::string& value) {
    std::string quoted = "'";
    for (char ch : value) {
        if (ch == '\'') {
            quoted += "'\"'\"'";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back('\'');
    return quoted;
}

std::string python_expect(const std::string& op, const std::string& a, const std::string& b) {
    const std::string code =
        "import sys;"
        "a=int(sys.argv[1]);"
        "b=int(sys.argv[2]);"
        "op=sys.argv[3];"
        "expr={'+':a+b,'-':a-b,'*':a*b,'/':a//b,'%':a%b,'^':pow(a,b),"
        "'==':(a==b),'!=':(a!=b),'<':(a<b),'>':(a>b),'<=':(a<=b),'>=':(a>=b)}[op];"
        "sys.stdout.write(str(int(expr)))";

    const std::string cmd = std::string("python3 -c ") + shell_quote(code) + " " +
                            shell_quote(a) + " " + shell_quote(b) + " " + shell_quote(op);
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("failed to launch python");
    }

    char buffer[4096] = {};
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    const int rc = pclose(pipe);
    if (rc != 0) {
        throw std::runtime_error("python returned non-zero: " + std::to_string(rc) + ", cmd: " + cmd);
    }

    return output;
}
}  // namespace

int main() {
    std::mt19937_64 rng(static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()));

    const std::vector<std::string> ops = {"+", "-", "*", "/", "%", "^", "==", "!=", "<", ">", "<=", ">="};
    for (int i = 0; ; ++i) {
        const std::string op = ops[static_cast<std::size_t>(rng() % ops.size())];

        const bool comparison_op = (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=");
        const bool positive_only = (op == "/" || op == "%") && !comparison_op;
        std::string a = make_number(rng, !positive_only, false, i * 2 + 1);
        std::string b = make_number(rng, !positive_only, positive_only, i * 2 + 1);
        if (positive_only && b == "0") {
            b = "1";
        }
        if (op == "^") {
            a = make_number(rng, false, false);
            b = std::to_string(1 + static_cast<int>(rng() % 10));
        }

        std::string expected;
        try {
            expected = python_expect(op, a, b);
        } catch (const std::exception& ex) {
            std::cerr << "python failed for case " << i << ": " << ex.what() << std::endl;
            return 1;
        }

        CAS::Intg lhs(a);
        CAS::Intg rhs(b);

        if (comparison_op) {
            bool got = false;
            if (op == "==") {
                got = (lhs == rhs);
            } else if (op == "!=") {
                got = (lhs != rhs);
            } else if (op == "<") {
                got = (lhs < rhs);
            } else if (op == ">") {
                got = (lhs > rhs);
            } else if (op == "<=") {
                got = (lhs <= rhs);
            } else if (op == ">=") {
                got = (lhs >= rhs);
            }

            const std::string got_str = got ? "1" : "0";
            if (got_str != expected) {
                std::cerr << "mismatch at case " << i << ": " << a << ' ' << op << ' ' << b
                          << "\nexpected=" << expected << "\nactual=" << got_str << std::endl;
                return 1;
            }
            continue;
        }

        CAS::Intg actual;
        if (op == "+") {
            actual = lhs + rhs;
        } else if (op == "-") {
            actual = lhs - rhs;
        } else if (op == "*") {
            actual = lhs * rhs;
        } else if (op == "/") {
            actual = lhs / rhs;
        } else if (op == "%") {
            actual = lhs % rhs;
        } else if (op == "^") {
            actual = lhs.pow(rhs);
        } else {
            std::cerr << "unexpected op" << std::endl;
            return 1;
        }

        const std::string got = static_cast<std::string>(actual);
        if (got != expected) {
            std::cerr << "mismatch at case " << i << ": " << a << ' ' << op << ' ' << b
                      << "\nexpected=" << expected << "\nactual=" << got << std::endl;
            return 1;
        }
        else
        {
            std::cout << "#" << i << "OK " << a << ' ' << op << ' ' << b << "=" << got << std::endl;
        }
    }
    return 0;
}
