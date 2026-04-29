# In-Class Exercise: V-Tables in C++

**Topic:** Virtual dispatch internals — vtables and vptrs  
**File:** `starter/main.cpp`  
**Build:** `make` → `./rps`

---

## Background: What is a V-Table?

When a C++ class has at least one `virtual` method, the compiler creates a **vtable** (virtual dispatch table): a static, read-only array of function pointers — one entry per virtual method declared in the class.

```
      Rock's vtable (read-only, lives in .rodata segment)
      ┌─────────────────────┐
[0]   │ ptr → Rock::draw()  │
[1]   │ ptr → Rock::type()  │
[2]   │ ptr → Rock::~Rock() │  (complete object destructor)
[3]   │ ptr → Rock::~Rock() │  (deleting destructor)
      └─────────────────────┘
```

Every **instance** of the class holds a hidden **vptr** — a pointer to its class's vtable. The vptr is **the very first field** of the object in memory, before any data members.

```
      A Rock object in memory:
      ┌──────────┬────────────────┐
      │  vptr    │  (Rock fields) │
      │ → vtable │                │
      └──────────┴────────────────┘
        8 bytes
```

When you call `obj->draw()`, the compiler does:
1. Read the vptr from the object.
2. Index into the vtable: `vtable[0]`.
3. Call the function pointer stored there.

---

## The Game

A terminal Rock-Paper-Scissors game against a computer opponent (`RandomPlayer`).

```
Your move (r=Rock, p=Paper, s=Scissors, q=Quit): r

  --- You ---          --- Rival ---
    ___
   /   \
  (     )               +-------+
   \___/                | PAPER |
   ROCK                 +-------+

  >>> Rival wins! :(
```

Build and play:
```bash
make
./rps
```

---

## Task 1 — Vptr Swap (per-instance)

**Goal:** make the `RandomPlayer rival` always pick Rock, **without** creating a new object and without modifying any class definition.

**How:** every polymorphic object starts with a hidden vptr. If you overwrite the vptr of `rival` with the vptr from a `RockPlayer` instance, calls to `rival.pick()` will dispatch to `RockPlayer::pick()` instead of `RandomPlayer::pick()`.

```
Before:                         After makeAlwaysRock():

rival (RandomPlayer)            rival (same object in memory)
┌──────────┬──────────┐         ┌──────────┬──────────┐
│  vptr ───┼──► RndVT │         │  vptr ───┼──► RockVT│
└──────────┴──────────┘         └──────────┴──────────┘

RndVT:                          RockVT:
 [0] RandomPlayer::pick()        [0] RockPlayer::pick()
```

**Implement** `makeAlwaysRock(IPlayer* player)` in `main.cpp`:

```cpp
void makeAlwaysRock(IPlayer* player) {
    // Hints:
    // 1. Create a local RockPlayer on the stack.
    // 2. The vptr occupies the first sizeof(void*) bytes of the object.
    // 3. Use memcpy to copy the vptr from RockPlayer into *player.
}
```

Then uncomment `makeAlwaysRock(&rival);` in `main()` and verify that the rival always picks Rock.

> **To win every round:** what should you play once Task 1 is active?

---

## Task 2 — Vtable Patch (per-class, global effect)

**Goal:** make **every** `Rock` object — past and future — draw like `FunnyRock`, without touching any individual instance.

**How:** the vtable is shared by all instances of a class. Patching `rock_vtable[0]` (the `draw()` pointer) affects every `Rock` that has ever been or will ever be created.

The catch: the vtable lives on a **read-only** memory page. You must use `mprotect()` to temporarily make it writable.

```
Before:                         After patchRockDraw():

Rock vtable (.rodata)           Rock vtable (patched)
┌─────────────────────┐         ┌──────────────────────────┐
│[0] → Rock::draw()   │   →     │[0] → FunnyRock::draw()   │
│[1] → Rock::type()   │         │[1] → Rock::type()        │
│[2] → ~Rock()        │         │[2] → ~Rock()             │
└─────────────────────┘         └──────────────────────────┘
```

**Implement** `patchRockDraw()` in `main.cpp`. The exact steps are documented as comments inside the function body.

```cpp
void patchRockDraw() {
    // See the comments in the source file for the step-by-step guide.
    // Key functions: mprotect(), getpagesize()
    // Headers already included: <sys/mman.h>, <unistd.h>
}
```

Uncomment `patchRockDraw();` in `main()`. Pick Rock and confirm you see `\(^o^)/`.

---

## Reflection Questions

1. In Task 1, if you create **two** `RandomPlayer` instances and call `makeAlwaysRock` on only one of them, does the other one change? Why or why not?

2. In Task 2, does the patch affect `Rock` objects that were created **before** the call to `patchRockDraw()`? What about objects created **after**?

3. Why is the vtable placed on a read-only page by default? What security implications does the ability to patch it have?

4. Is this code portable to Windows? What would need to change?

5. Task 1 copies the vptr but not the internal data of `RockPlayer`. Could that be a problem if `RockPlayer` had its own data members? What about `RandomPlayer`?

---

## Quick Reference

| Function | Header | Description |
|----------|--------|-------------|
| `memcpy(dst, src, n)` | `<cstring>` | Copy `n` bytes from `src` to `dst` |
| `getpagesize()` | `<unistd.h>` | Return the OS page size (e.g. 4096) |
| `mprotect(addr, len, prot)` | `<sys/mman.h>` | Change memory region permissions |
| `PROT_READ \| PROT_WRITE` | `<sys/mman.h>` | Page is readable and writable |
| `PROT_READ` | `<sys/mman.h>` | Page is read-only |
