# MiniC Compiler

一个从零手写的 C 语言子集编译器，用 C++17 实现。

目标后端是**自定义栈式虚拟机字节码**：源码 → 词法分析 → 语法分析(AST) → 语义分析 → 字节码 → 虚拟机执行。
不依赖 flex/bison，也不依赖外部汇编器，编译出来的 `minic.exe` 自己就能跑 MiniC 程序。

## 当前进度

- [x] **词法分析器** Lexer：把源码切成 token 流
- [ ] 语法分析器 Parser：递归下降，产生 AST
- [ ] 语义分析：符号表、作用域、类型与实参个数检查
- [ ] 代码生成：AST → 栈式虚拟机字节码
- [ ] 虚拟机：执行字节码

## 构建与运行

需要 Visual Studio 的 C++ 工具链（Community / Professional / BuildTools 任一即可）。
`build.bat` 通过 vswhere 自动定位，不用手动开 Developer Command Prompt。

```bat
build.bat
minic.exe tests\gcd.c
```

不带参数运行 `minic.exe` 会处理一段内置的示例源码。

当前阶段的输出是 token 表，三列分别是行号、token 类型、原文：

```
   2  KwInt        "int"
   2  Identifier   "gcd"
   2  LParen       "("
   4  Integer      "24"  value=24
```

## 语言特性范围

第一阶段只做核心子集，够写出递归、循环和分支：

- 类型：`int`、`void`（`void` 仅用于函数返回类型）
- 语句：变量声明、赋值、`if` / `else`、`while`、`return`、语句块
- 表达式：`+ - * / %`、`== != < <= > >=`、`&& || !`、一元负号、函数调用
- 函数：多参数、递归、互相调用

刻意**不支持**的东西，遇到时直接在词法阶段报错而不是静默接受：

- 位运算 `&` `|`：单个 `&` 会报错，提示应为 `&&`
- `123abc` 这种数字紧跟标识符的写法

数组、指针、`char`、字符串、`struct`、`for`、全局变量留到后续阶段。

## 源码结构

| 文件 | 内容 |
| --- | --- |
| `src/token.hpp` | `TokenType` 枚举与 `Token` 结构（含行号、整数值） |
| `src/lexer.hpp` / `src/lexer.cpp` | 手写扫描器，单字符前看，支持 `//` 与 `/* */` 注释 |
| `src/main.cpp` | 命令行入口，目前负责打印 token 表 |
| `tests/` | 示例程序与各类错误输入 |

## 测试输入

`tests/gcd.c` 是正常程序。其余四个是故意写错的，用来确认报错带正确行号：

| 文件 | 预期报错 |
| --- | --- |
| `bad_number.c` | 第 2 行，数字后紧跟标识符字符 |
| `bad_comment.c` | 第 2 行，块注释未闭合（报注释开始的行，不是文件结尾） |
| `bad_ampersand.c` | 第 3 行，应为 `&&` |
| `bad_char.c` | 第 2 行，未知字符 `#` |
