

## Required

- `libclang.so` (which usually requires `libLLVM.so` or `libLLVM-VERSION.so`) and maybe a symlink `libclang.so.N` (which `N` being a number) that points to `libclang.so`.

-  `clang/Index.h` (required include)

##


- Don't call `CXCursor`, `CXType` or `CXTranslationUnit` functions directly. Add wrapper for them in the respective C++ classes and call those wrappers. Strongly consider dumping the result of those function in the `dump` functions for better debugging.

- If you write a `Cursor::visit_children` callback, handle *all* cursor kinds: either do something useful with the cursor (parse further), ignore it (add a comment why, add a `@todo` comment if needed), or call `Cursor::dump` on it so it shows up in the output can one can add a handler for it.



Before compiling (and running), you may need to run (in the same console):
```SH
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
```
