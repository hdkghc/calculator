#include <cassert>
#include <string>
#include "cas/intg.hpp"

int main() {
    CAS::Intg zero("0");
    assert(static_cast<std::string>(zero) == "0");

    CAS::Intg one("1");
    assert(static_cast<std::string>(one) == "1");

    CAS::Intg ten("10");
    assert(static_cast<std::string>(ten) == "10");

    CAS::Intg hundred("100");
    assert(static_cast<std::string>(hundred) == "100");

    CAS::Intg neg("-42");
    assert(static_cast<std::string>(neg) == "-42");

    CAS::Intg sub = CAS::Intg("10") - CAS::Intg("2");
    assert(static_cast<std::string>(sub) == "8");

    CAS::Intg sub_neg = CAS::Intg("2") - CAS::Intg("10");
    assert(static_cast<std::string>(sub_neg) == "-8");

    CAS::Intg div = CAS::Intg("123") / CAS::Intg("45");
    assert(static_cast<std::string>(div) == "2");

    CAS::Intg mod = CAS::Intg("123") % CAS::Intg("45");
    assert(static_cast<std::string>(mod) == "33");

    return 0;
}
