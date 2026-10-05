# MiniC Compiler

一个从零手写的 C 语言子集编译器，用 C++17 实现。

目标后端是**自定义栈式虚拟机字节码**：源码 → 词法分析 → 语法分析(AST) → 语义分析 → 字节码 → 虚拟机执行。
不依赖 flex/bison，也不依赖外部汇编器，编译出来的 `minic.exe` 自己就能跑 MiniC 程序。

## 当前进度

- [x] **词法分析器** Lexer：把源码切成 token 流
- [x] **语法分析器** Parser：递归下降，产生 AST
- [x] **语义分析**：符号表、作用域、名字解析与各项检查
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
usage: minic [--tokens | --ast | --check] <file.c>
  --tokens   print the token stream
  --ast      parse, then print the syntax tree
  --check    also run semantic analysis, then print the tree with the
             slots and call targets it resolved (default)
```

默认输出语法树，每层缩进两个空格。方括号里是语义分析的结果：变量解析到了哪个局部变量槽位，函数调用指向第几个函数，以及每个函数的调用帧需要多少槽位。

```
Program
  Function int gcd(a, b)  [locals 3]
    Block
      While
        Binary !=
          Var b  [slot 1]
          Int 0
```

`--ast` 只做到语法分析，因此不带方括号标注，可以用来单独观察语法树。`--tokens` 输出 token 表，三列分别是行号、token 类型、原文。

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
| `ok_` | 合法程序，预期输出是带标注的语法树 |
| `lex_` | 词法错误，预期输出是报错信息 |
| `syntax_` | 语法错误，预期输出是报错信息 |
| `semantic_` | 语义错误，预期输出是报错信息 |

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

### 语义检查

语法树建好之后，语义分析遍历一遍，解析每个名字并检查下面这些规则。任何一条不满足就报错，错误信息同样带行号。

| 检查 | 例子 |
| --- | --- |
| 必须有 `main`，返回 `int` 且没有参数 | `void main()` 报错 |
| 变量先声明后使用 | `return y;` 而 y 没声明 |
| 同一作用域里不能重复声明 | `int x; int x;` |
| 参数和函数体最外层是同一个作用域 | `int f(int a) { int a; }` 报错，和 C 一致 |
| 变量不能出现在自己的初始化式里 | `int x = x + 1;` |
| 变量名不能和函数名相同 | 函数 `f` 存在时再写 `int f;` |
| 函数不能重复定义 | 两个 `int f()` |
| 调用的函数必须存在，实参个数必须对上 | `add(1)` 而 `add` 有两个参数 |
| `void` 函数的返回值不能参与表达式 | `int x = noop();` |
| `return` 带不带值要和函数返回类型一致 | `int f() { return; }` 报错 |

内层作用域可以遮蔽外层的同名变量，这是合法的，`tests/ok_scoping.c` 专门测这一点。

分析的另一半工作是为代码生成做准备：每个变量解析到一个局部变量槽位，每个调用解析到目标函数的下标，每个函数记下调用帧需要的槽位数。相邻的语句块会复用槽位，所以槽位数是峰值，不是声明的个数。函数名在分析函数体之前先收集一遍，所以函数可以在定义之前被调用，互相递归也没问题。

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
| `src/semantic.hpp` / `src/semantic.cpp` | 语义分析：作用域、名字解析、槽位分配 |
| `src/error.hpp` | 各阶段共用的 `CompileError`，带阶段名和行号 |
| `src/main.cpp` | 命令行入口 |
| `build.bat` / `test.bat` | 构建与回归测试 |
