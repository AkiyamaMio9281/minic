# MiniC Compiler

一个从零手写的 C 语言子集编译器，用 C++17 实现。

目标后端是**自定义栈式虚拟机字节码**：源码 → 词法分析 → 语法分析(AST) → 语义分析 → 字节码 → 虚拟机执行。
不依赖 flex/bison，也不依赖外部汇编器，编译出来的 `minic.exe` 自己就能跑 MiniC 程序。

## 当前进度

- [x] **词法分析器** Lexer：把源码切成 token 流
- [x] **语法分析器** Parser：递归下降，产生 AST
- [x] **语义分析**：符号表、作用域、名字解析与各项检查
- [x] **代码生成**：AST → 自定义栈式虚拟机字节码
- [x] **虚拟机**：调用帧、局部变量槽、操作数栈、跳转与函数调用

## 构建与运行

需要 Visual Studio 的 C++ 工具链（Community / Professional / BuildTools 任一即可）。
`build.bat` 通过 vswhere 自动定位，不用手动开 Developer Command Prompt。

```bat
build.bat
minic.exe --run tests\gcd.c
```

```text
usage: minic [--tokens | --ast | --check | --bytecode | --run] <file.c>
  --tokens    print the token stream
  --ast       parse, then print the syntax tree
  --check     run semantic analysis, then print the resolved tree (default)
  --bytecode  compile and print stack-machine bytecode
  --run       compile and execute the program, then print main's result
```

默认仍然是 `--check`。`--run` 会完整经过 lexer、parser、semantic analysis、code generation 和 VM，并打印 `main` 的返回值。`--bytecode` 可以查看生成的栈机指令。

## 测试

```bat
test.bat
test.bat update
```

`tests\` 下每个 `.c` 都有同名的 `.expected`。新增的 `run_` 前缀测试会用 `--run` 执行，覆盖算术、赋值表达式、循环、递归、短路逻辑、void 调用、函数末尾行为和整数溢出。

## 字节码与虚拟机

代码生成直接消费语义分析已经解析好的 `slot`、函数 `target` 和 `localCount`，不再做名字查找。虚拟机使用 operand stack 和 call stack；每个 frame 保存函数、instruction pointer、局部变量槽和进入函数时的 operand-stack 基准位置。

函数调用按从左到右顺序计算实参；`CALL` 建立新 frame 并把参数放入局部槽 `0..n-1`。`RET` 清掉被调用函数留下的临时值，只把返回值交回调用者。`if`、`while`、`&&` 和 `||` 通过 `JMP`、`JZ`、`JNZ` 与回填跳转目标实现，其中逻辑运算保持短路求值。

虚拟机对除零、求余除零、整数除法溢出、取负溢出以及 `+`/`-`/`*` 的 32 位溢出报告 `runtime error`。`main` 走到函数末尾时按 C 的规则返回 0；`void` 函数走到末尾等价于 `return;`；其他 `int` 函数如果实际执行到末尾而没有返回值，则报告运行时错误。

## 源码结构

| 文件 | 内容 |
| --- | --- |
| `src/token.hpp` / `src/token.cpp` | token 定义与打印 |
| `src/lexer.hpp` / `src/lexer.cpp` | 手写 lexer |
| `src/ast.hpp` / `src/ast.cpp` | AST |
| `src/parser.hpp` / `src/parser.cpp` | 递归下降 parser |
| `src/semantic.hpp` / `src/semantic.cpp` | 作用域、名字解析、槽位分配 |
| `src/bytecode.hpp` / `src/bytecode.cpp` | 栈式 VM 指令与 bytecode 打印 |
| `src/codegen.hpp` / `src/codegen.cpp` | AST → bytecode |
| `src/vm.hpp` / `src/vm.cpp` | bytecode 解释执行 |
| `src/error.hpp` | 共用错误类型 |
| `src/main.cpp` | 命令行入口 |
| `build.bat` / `test.bat` | 构建与回归测试 |
