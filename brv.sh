cmake -B build-valgrind -DUSE_SANITIZERS=OFF -DUSE_VALGRIND=ON -G Ninja
cmake --build build-valgrind --clean-first
valgrind --leak-check=full ./build-valgrind/aso
