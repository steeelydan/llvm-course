clang++ -o eva-llvm `llvm-config --cxxflags --ldflags --system-libs --libs core` eva-llvm.cpp

./eva-llvm

lli ./out.ll