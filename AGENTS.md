# Project overview
# codebase instructions
- Prefer C++20.
- Use clang-format with LLVM style.
- Avoid Python unless explicitly requested.
- Use Makefiles instead of CMake when possible.

# Build commands
- Spec_1: Do not run/build when confidence in changes is high.
- Spec_2: When building, use `make -j12` (build instructions take very long in this environment).

# Test commands
# Code style
# Architecture notes
- Spec_3: leaving legacy ghosts in the code is strongly discouraged. 

# Security constraints
# PR rules
- Update the .man files if you add or change the behaviour of a configuration setting. 
# Known pitfalls