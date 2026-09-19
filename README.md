# MiniC Compiler

一个从零手写的 C 语言子集编译器，用 C++17 实现。

目标后端是**自定义栈式虚拟机字节码**：源码 → 词法分析 → 语法分析(AST) → 语义分析 → 字节码 → 虚拟机执行。
不依赖 flex/bison，也不依赖外部汇编器，编译出来的 `minic.exe` 自己就能跑 MiniC 程序。

## 当前进度

- [x] **词法分析器** Lexer：把源码切成 token 流
- [x] **语法分析器** Parser：递归下降，产生 AST
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

```
usage: minic [--tokens | --ast] <file.c>
  --tokens   print the token stream
  --ast      print the syntax tree (default)
```

默认输出语法树，每层缩进两个空格：

```
Program
  Function int gcd(a, b)
    Block
      While
        Binary !=
          Var b
          Int 0
```

`--tokens` 输出 token 表，三列分别是行号、token 类型、原文。

出错时输出一行到 stderr，退出码为 1，格式是 `<阶段> error: line <行号>: <说明>`：

```
syntax error: line 2: expected ';' after variable declaration, found 'return'
```

## 测试

```bat
test.bat          跑全部测试
test.bat update   用当前输出重写所有 .expected，之后用 git diff 检查改动
```

`tests\` 下每个 `.c` 都有同名的 `.expected`，内容是 `minic.exe` 的完整输出。文件名前缀表示用途：

| 前缀 | 内容 |
| --- | --- |
| `ok_` | 合法程序，预期输出是语法树 |
| `lex_` | 词法错误，预期输出是报错信息 |
| `syntax_` | 语法错误，预期输出是报错信息 |

`gcd.c` 是示例程序，同样有预期输出。

## 语言

第一阶段只做核心子集，够写出递归、循环和分支。

### 文法

每条规则对应 [src/parser.cpp](src/parser.cpp) 里的一个函数。表达式部分优先级从上到下递增，所以 `*` 比 `+` 结合得更紧。

```
program     = function*
function    = ("int" | "void") IDENT "(" params ")" block
params      = ε | "void" | "int" IDENT ("," "int" IDENT)*
block       = "{" block_item* "}"
block_item  = declaration | statement
declaration = "int" IDENT ("=" expr)? ";"
statement   = "if" "(" expr ")" statement ("else" statement)?
            | "while" "(" expr ")" statement
            | "return" expr? ";"
            | block
            | expr? ";"

expr        = assign
assign      = or ("=" assign)?            右结合，左边必须是变量
or          = and ("||" and)*
and         = equality ("&&" equality)*
equality    = relational (("==" | "!=") relational)*
relational  = additive (("<" | "<=" | ">" | ">=") additive)*
additive    = term (("+" | "-") term)*
term        = unary (("*" | "/" | "%") unary)*
unary       = ("-" | "!") unary | primary
primary     = INTEGER | IDENT | IDENT "(" args? ")" | "(" expr ")"
args        = expr ("," expr)*
```

几处和 C 一致、容易踩到的地方：

- **赋值是表达式**：`a = b = 0;` 合法，等价于 `a = (b = 0)`。
- **声明不是语句**：`if (x) int y;` 在 C 里也是错的，要写成 `if (x) { int y; }`。
- **else 跟最近的 if**：`if (a) if (b) x; else y;` 里的 else 属于内层 if。
- **`f(void)`** 表示没有参数，和 `f()` 相同。

### 类型与字面量

`int` 是 32 位，也是唯一的值类型。`void` 只能作为函数返回类型。

整数字面量最大 2147483647，超过就是词法错误。和 C 一样，负号是一元运算符而不是字面量的一部分，所以最小值要写成 `-2147483647 - 1`。

### 刻意不支持的写法

这些写法遇到时直接报错，不会被静默接受：

| 写法 | 原因 |
| --- | --- |
| `&` `\|` 位运算 | 没有位运算；单个 `&` 报错并提示应为 `&&` |
| `123abc` | 数字后紧跟字母是畸形字面量，不是两个 token |
| `010` | C 里这是八进制的 8，MiniC 没有八进制，读成十会和 C 结果不同 |
| 全局变量 | 变量只能声明在函数里 |
| `int a, b;` | 每条声明只能有一个变量 |
| `int f(int x);` 原型 | 函数可以在定义之前调用，不需要原型 |

数组、指针、`char`、字符串、`struct`、`for`、全局变量留到后续阶段。

### 限制

嵌套最多 256 层，括号、函数调用参数、一元运算符、连续赋值、语句块、if/else/while 的分支体都算一层。超过会报语法错误，而不是让递归撑爆栈。256 和 clang 默认的括号嵌套上限相同，手写代码不会碰到。

`1 + 1 + ... + 1` 这种平铺长链不算嵌套，但会生成很深的树，后续阶段需要递归遍历它。所以 `build.bat` 把栈保留空间设为 64 MB，默认的 1 MB 在大约 4000 项时就会溢出。

## 源码结构

| 文件 | 内容 |
| --- | --- |
| `src/token.hpp` / `src/token.cpp` | `TokenType` 枚举、`Token` 结构（含行号、整数值），以及 token 名称和拼写表 |
| `src/lexer.hpp` / `src/lexer.cpp` | 手写扫描器，单字符前看，支持 `//` 与 `/* */` 注释 |
| `src/ast.hpp` / `src/ast.cpp` | AST 节点定义与缩进打印 |
| `src/parser.hpp` / `src/parser.cpp` | 递归下降语法分析器 |
| `src/error.hpp` | 各阶段共用的 `CompileError`，带阶段名和行号 |
| `src/main.cpp` | 命令行入口 |
| `build.bat` / `test.bat` | 构建与回归测试 |
