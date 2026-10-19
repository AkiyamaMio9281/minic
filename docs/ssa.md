# Stack slots to SSA

The backend does not build SSA itself. Every MiniC local becomes an
`alloca` in the function's entry block, and every read or write of it
becomes a `load` or a `store`. LLVM promotes those away.

The source is [bench/sum_loop.c](../bench/sum_loop.c):

```c
int sum(int n) {
    int acc = 0;
    int i = 0;

    while (i < n) {
        acc = acc + i;
        i = i + 1;
    }

    return acc;
}
```

## What the backend emits

Three slots, one per local, with `n` stored into its slot on entry. The
loop reads and writes memory on every iteration.

```llvm
define i32 @mc_sum(i32 %p0) {
entry:
  %s0 = alloca i32
  %s1 = alloca i32
  %s2 = alloca i32
  store i32 %p0, ptr %s0
  store i32 0, ptr %s1
  store i32 0, ptr %s2
  br label %cond.0
cond.0:
  %t3 = load i32, ptr %s2
  %t4 = load i32, ptr %s0
  %t5 = icmp slt i32 %t3, %t4
  %t6 = zext i1 %t5 to i32
  %t7 = icmp ne i32 %t6, 0
  br i1 %t7, label %body.1, label %endwhile.2
body.1:
  %t8 = load i32, ptr %s1
  %t9 = load i32, ptr %s2
  %t10 = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %t8, i32 %t9)
  %t11 = extractvalue {i32, i1} %t10, 0
  %t12 = extractvalue {i32, i1} %t10, 1
  br i1 %t12, label %trap.13, label %ok.14
trap.13:
  call void @minic_trap(i32 0, i32 8)
  unreachable
ok.14:
  store i32 %t11, ptr %s1
  %t15 = load i32, ptr %s2
  %t16 = call {i32, i1} @llvm.sadd.with.overflow.i32(i32 %t15, i32 1)
  %t17 = extractvalue {i32, i1} %t16, 0
  %t18 = extractvalue {i32, i1} %t16, 1
  br i1 %t18, label %trap.19, label %ok.20
trap.19:
  call void @minic_trap(i32 0, i32 9)
  unreachable
ok.20:
  store i32 %t17, ptr %s2
  br label %cond.0
endwhile.2:
  %t21 = load i32, ptr %s1
  ret i32 %t21
}
```

Reproduce with:

```bat
minic.exe --emit-llvm bench\sum_loop.c
```

## After promotion

```llvm
define i32 @mc_sum(i32 %p0) {
entry:
  br label %cond.0

cond.0:                                           ; preds = %ok.20, %entry
  %s1.0 = phi i32 [ 0, %entry ], [ %t11, %ok.20 ]
  %s2.0 = phi i32 [ 0, %entry ], [ %t17, %ok.20 ]
  %t5 = icmp slt i32 %s2.0, %p0
  %t6 = zext i1 %t5 to i32
  %t7 = icmp ne i32 %t6, 0
  br i1 %t7, label %body.1, label %endwhile.2

body.1:                                           ; preds = %cond.0
  %t10 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %s1.0, i32 %s2.0)
  %t11 = extractvalue { i32, i1 } %t10, 0
  %t12 = extractvalue { i32, i1 } %t10, 1
  br i1 %t12, label %trap.13, label %ok.14

trap.13:                                          ; preds = %body.1
  call void @minic_trap(i32 0, i32 8)
  unreachable

ok.14:                                            ; preds = %body.1
  %t16 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %s2.0, i32 1)
  %t17 = extractvalue { i32, i1 } %t16, 0
  %t18 = extractvalue { i32, i1 } %t16, 1
  br i1 %t18, label %trap.19, label %ok.20

trap.19:                                          ; preds = %ok.14
  call void @minic_trap(i32 0, i32 9)
  unreachable

ok.20:                                            ; preds = %ok.14
  br label %cond.0

endwhile.2:                                       ; preds = %cond.0
  ret i32 %s1.0
}
```

Reproduce with:

```bat
minic.exe --emit-llvm bench\sum_loop.c > sum.ll
clang -S -emit-llvm -Wno-override-module -O1 -mllvm -print-after=sroa sum.ll -o sum.O1.ll
```

The dump goes to stderr, and the pass runs more than once in the
pipeline, so take the first block for the function you care about.

## What changed

All three `alloca` instructions are gone, and with them every `load` and
`store`. In their place the loop header gained two `phi` nodes, one per
variable the loop carries:

```llvm
%s1.0 = phi i32 [ 0, %entry ], [ %t11, %ok.20 ]
%s2.0 = phi i32 [ 0, %entry ], [ %t17, %ok.20 ]
```

A `phi` names one value per incoming edge: on entry both variables are
zero, and on the back edge they take what the previous iteration
computed. SSA requires this, because a value is assigned exactly once and
a merge point has to say which definition reached it.

Two details worth knowing:

- The pass that does this in the default pipeline is **SROA**, not
  `mem2reg`. `mem2reg` still exists as a standalone pass inside `opt`,
  which the Windows LLVM package does not ship. SROA subsumes it.
- The shape of the code decides whether a `phi` survives the passes that
  follow. A merge of two constants, as in `if (c) x = 1; else x = 2;`, is
  usually rewritten into a `select`. A loop-carried variable stays a
  `phi`.

The overflow checks survive promotion, because they are real control flow
rather than something a pass may drop. Only a pass able to prove the
addition cannot overflow would remove them.
