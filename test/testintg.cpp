#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include "cas/intg.hpp"

using namespace std;
using namespace CAS;

int main() {

    string s1, s2, op;
    while (true) {
        cout << "Input I1: ";
        cin >> s1;
        if (s1 == "exit") break;
        cout << "Input I2: ";
        cin >> s2;
        cout << "Input operator: ";
        cin >> op;
        if (op == "exit") break;

        Intg I1(s1);
        Intg I2(s2);

        cout << "I1 = " << string(I1) << endl;
        cout << "I2 = " << string(I2) << endl;


        cout << "\n[Ans] ";
        if (op == "+") {
            Intg res = I1 + I2;
            cout << string(res) << endl;
        }
        else if (op == "-") {
            Intg res = I1 - I2;
            cout << string(res) << endl;
        }
        else if (op == "*") {
            Intg res = I1 * I2;
            cout << string(res) << endl;
        }
        else if (op == "/") {
            Intg res = I1 / I2;
            cout << string(res) << endl;
        }
        else if (op == "%") {
            Intg res = I1 % I2;
            cout << string(res) << endl;
        }
        else if (op == "^") {
            Intg res = I1.pow(I2);
            cout << string(res) << endl;
        }
        else if (op == "r") {
            auto pr = I1.divmod(I2);
            cout << "Q= " << string(pr.first) << "  R= " << string(pr.second) << endl;
        }
        else if (op == ">") {
            cout << (I1 > I2 ? "true" : "false") << endl;
        }
        else if (op == "<") {
            cout << (I1 < I2 ? "true" : "false") << endl;
        }
        else if (op == ">=") {
            cout << (I1 >= I2 ? "true" : "false") << endl;
        }
        else if (op == "<=") {
            cout << (I1 <= I2 ? "true" : "false") << endl;
        }
        else if (op == "==") {
            cout << (I1 == I2 ? "true" : "false") << endl;
        }
        else if (op == "!=") {
            cout << (I1 != I2 ? "true" : "false") << endl;
        }
        else {
            cout << "Unsupported operator" << endl;
        }
        cout << "-----------------------------------------" << endl;
    }
    return 0;
}