#!/bin/bash

clang++ -o eva-llvm `llvm-config --cxxflags --ldflags --system-libs --libs core` -fcxx-exceptions eva-llvm.cpp

./eva-llvm

lli ./out.ll

printf "\nReturn code: %s\n" "$?"
