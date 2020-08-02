//
// Created by Alex on 8/2/2020.
//
#include <iostream>
#include "regex.h"
int main() {
    using namespace alex;
    std::cout << regex_match(regex_compile("[0-9]+"), "1234") << " ";
    std::cout << regex_match(regex_compile("[a-z]+"), "asdf") << " ";
    std::cout << regex_match(regex_compile("'.*'"), "'asdf'") << " ";
    std::cout << regex_match(regex_compile("aa[a-z]+"), "dfa") << " ";
    //std::cout << regex_emit_c(regex_compile("[0-9]+"));

    return 0;
}
