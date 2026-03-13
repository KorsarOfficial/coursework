# Financial Analytics

## Project Overview

A C++ console application for personal financial analytics over transaction data. Features include top-K transaction retrieval by amount, category-based spending within a 90-day sliding window, per-card aggregation with cashback calculation, and substring search using the Knuth-Morris-Pratt algorithm. All data is hardcoded (18 sample transactions in RUB). A companion Python implementation exists in `main.py`.

## Mathematical Formalization

### Data Model

Let T be the set of transactions:

    t = (date, status, amount, currency, category, desc, card)
    |T| = 18, status in {"OK", "FAILED"}

### Date Parsing and Serial Day Number

Parse function:

    d(s) : "DD.MM.YYYY" -> (y, m, d) in Z^3

Rata Die serial conversion (monotonic):

    f(y, m, d) = 365y + floor(y/4) - floor(y/100) + floor(y/400) + floor((153(m-3)+2)/5) + d

Used for range comparisons: delta_d = |f(d1) - f(d2)|

### Greeting Lookup

    g(h) = G[i] where R[i].first <= h < R[i].second
    R = {[6,12), [12,18), [18,23)}, G = {morning, afternoon, evening, night}
    T = O(1), 4 buckets

### Per-Card Aggregation

    S_card(c) = sum over {i : card(i)=c AND status(i)="OK"} of amount(i)
    cashback(c) = S_card(c) / 100
    T(n) = O(n), S(k) = O(k), k = |{card(i) : i in T}|

### Top-K by Amount

    TopK(T, k=5):
      1. Filter: R = {t in T | t.status = "OK"}
      2. partial_sort R[0..k) by amount descending
      T(n) = O(n + k log k)

### Spending by Category (90-Day Window)

    S_c = sum over {i : cat(i)=c AND serial(d(i)) in [ref-90, ref] AND status(i)="OK"} of amount(i)
    T(n) = O(n), single pass with copy_if

### KMP String Search

Failure function:

    pi(q) = max{k < q : P[0..k-1] = P[q-k..q-1]}
    T(m) = O(m), S(m) = O(m)

Search:

    KMP(text, pattern) : P is substring of text
    T(n, m) = O(n + m) vs naive O(n * m)

Full search over transactions:

    Search(T, q) = {t in T | q is substring of lower(desc(t)) OR q is substring of lower(cat(t))}
    T(n, m) = O(n * (|desc| + |cat| + m))

## Original Code Quality Analysis (Python `main.py`)

The following bugs and structural issues were identified in the original Python source:

### Bug 1: Incorrect Import Causing Startup Crash

```python
from turtle import pd
```

The `turtle` module is Python's graphics library and has no export named `pd`. This raises `ImportError` immediately at module load time, preventing any code from executing. The intended import was almost certainly `import pandas as pd`.

### Bug 2: Duplicate `spending_by_category` Definition

Two functions with the identical name `spending_by_category` are defined in the same module. Python silently overwrites the first with the second definition. The first function's logic is completely unreachable. Only the second definition ever executes. This is a copy-paste error that would have been caught by any linter or basic test.

### Bug 3: Unreachable Code After Early Return

```python
return list_by_category
# ... more code below ...
```

Code following an unconditional `return` statement is dead code. The interpreter never reaches the subsequent lines. This indicates that the function was edited without understanding its control flow.

### Bug 4: `top_five_transaction` Misnaming

The function is named `top_five_transaction` suggesting it finds the global top-5 transactions by amount. Instead, it finds the maximum transaction per category -- a fundamentally different operation. The name does not match the semantics:

- Expected: sort all transactions by amount descending, take first 5
- Actual: group by category, take max from each group

The C++ rewrite fixes this by using `partial_sort` to correctly retrieve the global top-5.

### Bug 5: Excessive Copy-Paste Logging

Every function contains nearly identical logging boilerplate:

```python
logger.info("Starting function_name...")
# ... logic ...
logger.info("Finished function_name.")
```

This mechanical repetition across all functions is a structural indicator of AI-generated code. A human developer would extract this into a decorator:

```python
@log_call
def some_function(...):
    ...
```

### AI Generation Markers Summary

- Import error (`from turtle import pd`) that fails on first run -- never tested
- Duplicate function definitions with no test catching the shadow
- Unreachable code after return statements
- Misleading function names that do not match behavior
- Mechanical copy-paste logging in every function

## Build & Run

### Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Standard library only (no external dependencies)
- Uses `<bits/stdc++.h>` (GCC/MinGW; for MSVC, replace with individual headers)

### Compile

```bash
g++ -std=c++17 -O2 -o finance main.cpp
```

### Run

```bash
./finance
```

On Windows, the program sets the console to UTF-8 (`chcp 65001`) for Cyrillic text display.

### Menu Options

1. Top-5 transactions globally by amount
2. Spending by category within a 90-day window
3. Per-card aggregation with cashback
4. Substring search over descriptions/categories (KMP)
5. Time-of-day greeting
0. Exit
