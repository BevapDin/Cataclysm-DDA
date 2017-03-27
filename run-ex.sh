#! /bin/bash

PYTHONPATH=/usr/src/llvm/llvm-3.8.0.src/tools/clang/bindings/python './lua/export-c++-definitions-to-lua.py'
grep -E '^ *--' lua/generated_class_definitions.lua
sed -i '/^ *--/d' lua/generated_class_definitions.lua

