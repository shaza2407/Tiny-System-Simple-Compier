# Mini Compiler & Interpreter (C++)

## 📌 Overview
This project is a **mini compiler and interpreter** implemented in **C++** for a small, statically-typed programming language.  
It demonstrates the **full compiler pipeline**, including:

- Lexical analysis
- Parsing (recursive-descent)
- Abstract Syntax Tree (AST) construction
- Semantic analysis (symbol table & type checking)
- Runtime interpretation and execution

The project was built for **educational purposes** to understand how programming languages are designed and executed internally.

---

## 🧠 Language Features
The supported language includes:

### Data Types
- `int`
- `real`
- `bool`

### Statements
- Variable declaration
- Assignment
- `if / else`
- `repeat ... until`
- `read`
- `write`

### Expressions
- Arithmetic: `+ - * / ^`
- Relational: `< =`
- Logical: `and`

---

## 🏗️ Compiler Architecture

### 1️⃣ Lexical Analysis
- Converts source code into tokens
- Supports identifiers, keywords, literals, and operators

### 2️⃣ Parsing
- Recursive-descent parser
- Grammar enforces:
  - **Declarations appear before statements**
- Builds an **Abstract Syntax Tree (AST)**

### 3️⃣ Semantic Analysis
- Symbol table creation
- Type checking
- Undeclared variable detection
- Assignment compatibility checks
- Expression type inference

### 4️⃣ Interpretation (Execution)
- AST-based interpreter
- Runtime memory model using `memloc`
- Unified expression evaluation using `double`
- Correct handling of `int`, `real`, and `bool` values

---

## 🌳 AST Design
- Nodes are connected using:
  - `child[]` for hierarchical structure
  - `sibling` for sequential statements
- Each node stores:
  - Node kind
  - Line number
  - Expression type
  - Variable type (for declarations)

---
