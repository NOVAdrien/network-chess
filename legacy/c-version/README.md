# Legacy C Version

This folder contains the original C implementation of the project.

---

## Context

This repository is a continuation and refactor of an earlier academic project written in C.

Instead of deleting or rewriting everything from scratch, the original codebase has been preserved here to provide context and traceability.

---

## Files

```text
ClientTCP.c
ClientUDP.c
Makefile
README.md
ServerTCP.c
ServerUDP.c
arial.ttf
black_bishop.webp
black_king.webp
black_knight.webp
black_pawn.webp
black_queen.webp
black_rook.webp
white_bishop.webp
white_king.webp
white_knight.webp
white_pawn.webp
white_queen.webp
white_rook.webp
```

---

## Purpose

The legacy version is not the main version of the project anymore.

It is kept to:

- Preserve the original academic implementation;
- Document the evolution from a monolithic C implementation to a modular architecture;
- Avoid losing working code during the refactor.

---

## Original architecture

The initial implementation was functional but relied heavily on large, monolithic files:

```text
ClientTCP.c
ServerTCP.c
```

While effective for a first version, this made the code difficult to maintain, extend, and reason about.

---

## Refactor objective

The new version of the project (outside this folder):

- Introduces a modular architecture;
- Separates concerns: game logic, networking, server, client;
- Improves maintainability and readability;
- Moves towards a cleaner and more scalable design.

The refactored version separates the code into several modules:

```text
chess/
network/
server/
client/
```

---

## Important design change

In the new version, the server should be authoritative.

This means:

- The server owns the official board state;
- The server validates moves;
- The server checks whose turn it is;
- The server sends the updated state to both clients;
- The clients only send player actions and display the result.
- The clients should not be responsible for deciding whether a move is legal.

⚠️ This code is no longer maintained.
⚠️ It is kept for reference only.
⚠️ Any new development happens outside of this folder.
