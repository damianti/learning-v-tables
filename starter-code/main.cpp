#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/mman.h>
#include <unistd.h>

// ============================================================
//  Game Objects
// ============================================================

class IGameObject {
public:
    virtual std::vector<std::string> draw() const = 0;
    virtual int type() const = 0;  // 0 = Rock, 1 = Paper, 2 = Scissors
    virtual ~IGameObject() = default;
};

class Rock : public IGameObject {
public:
    std::vector<std::string> draw() const override {
        return {
            "    ___  ",
            "   /   \\ ",
            "  (     )",
            "   \\___/ ",
            "   ROCK  ",
        };
    }
    int type() const override { return 0; }
};

class Paper : public IGameObject {
public:
    std::vector<std::string> draw() const override {
        return {
            "  +-------+",
            "  |       |",
            "  | PAPER |",
            "  |       |",
            "  +-------+",
        };
    }
    int type() const override { return 1; }
};

class Scissors : public IGameObject {
public:
    std::vector<std::string> draw() const override {
        return {
            "    \\ /   ",
            "     X    ",
            "    / \\   ",
            " SCISSORS ",
        };
    }
    int type() const override { return 2; }
};

class FunnyRock : public IGameObject {
public:
    std::vector<std::string> draw() const override {
        return {
            "  \\(^o^)/ ",
            "   PARTY  ",
            "   ROCK!  ",
        };
    }
    int type() const override { return 0; }
};

// ============================================================
//  Players
// ============================================================

class IPlayer {
public:
    virtual IGameObject* pick() = 0;
    virtual ~IPlayer() = default;
};

class RandomPlayer : public IPlayer {
public:
    IGameObject* pick() override {
        int r = rand() % 3;
        if (r == 0) return new Rock();
        if (r == 1) return new Paper();
        return new Scissors();
    }
};

class RockPlayer : public IPlayer {
public:
    IGameObject* pick() override { return new Rock(); }
};

// ============================================================
//  Display helpers
// ============================================================

static void printSideBySide(const IGameObject* left, const IGameObject* right) {
    const int col_width = 16;

    std::cout << "\n  "
              << std::left << std::setw(col_width) << "--- You ---"
              << "--- Rival ---\n";

    auto ll = left->draw();
    auto rl = right->draw();
    std::size_t rows = std::max(ll.size(), rl.size());

    // Bottom-align: pad shorter drawing with empty lines at the top.
    while (ll.size() < rows) ll.insert(ll.begin(), "");
    while (rl.size() < rows) rl.insert(rl.begin(), "");

    for (std::size_t i = 0; i < rows; ++i) {
        std::cout << "  " << std::left << std::setw(col_width) << ll[i] << rl[i] << "\n";
    }
}

// ============================================================
//  Game logic
// ============================================================

static const char* whoWins(const IGameObject* user, const IGameObject* rival) {
    int u = user->type(), r = rival->type();
    if (u == r) return "Draw!";
    // Rock(0) > Scissors(2) > Paper(1) > Rock(0)
    if ((u == 0 && r == 2) || (u == 2 && r == 1) || (u == 1 && r == 0))
        return "You win! :)";
    return "Rival wins! :(";
}

// ============================================================
//  TODO 1 — vptr swap (per-instance)
// ============================================================
//
//  Goal: make `player` always pick Rock WITHOUT creating a new
//  object or changing the class definition.
//
//  How: every polymorphic object starts with a hidden vptr
//  (the first sizeof(void*) bytes). Overwrite `player`'s vptr
//  with the vptr taken from a RockPlayer instance.
//
//  After this call, calling player->pick() will dispatch to
//  RockPlayer::pick() even though `player` is a RandomPlayer.
//
//  Hints:
//    - Create a local RockPlayer on the stack.
//    - Use memcpy to copy the first sizeof(void*) bytes from
//      the RockPlayer into `player`.
//
void makeAlwaysRock(IPlayer* player) {
    // TODO: implement here
}

// ============================================================
//  TODO 2 — vtable patch (per-class, affects all Rock objects)
// ============================================================
//
//  Goal: make EVERY Rock object draw like FunnyRock, globally,
//  without touching any Rock instance directly.
//
//  How: the vtable is a read-only array of function pointers
//  shared by all instances of a class. Patch the draw() entry
//  inside Rock's vtable to point to FunnyRock::draw() instead.
//
//  Vtable layout for IGameObject hierarchy (GCC/Clang, x86-64):
//    vtable[0] -> draw()   <-- the one to patch
//    vtable[1] -> type()
//    vtable[2] -> ~IGameObject() (complete)
//    vtable[3] -> ~IGameObject() (deleting)
//
//  Steps:
//    1. Get Rock's vtable:
//         void** rock_vt = *reinterpret_cast<void***>(&rock_obj);
//    2. Get FunnyRock's vtable the same way.
//    3. Use mprotect() to mark the vtable page read+write.
//         uintptr_t page = (uintptr_t)rock_vt & ~(getpagesize()-1);
//         mprotect((void*)page, getpagesize(), PROT_READ | PROT_WRITE);
//    4. Copy FunnyRock's draw() pointer into rock_vt[0].
//    5. Restore the page to read-only with mprotect().
//
void patchRockDraw() {
    // TODO: implement here
}

// ============================================================
//  main
// ============================================================

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    RandomPlayer rival;

    // ---- Uncomment these lines to activate the two tasks ----
    // makeAlwaysRock(&rival);   // Task 1: rival always picks Rock
    // patchRockDraw();          // Task 2: all Rocks draw as FunnyRock
    // ---------------------------------------------------------

    char input;
    while (true) {
        std::cout << "\nYour move (r=Rock, p=Paper, s=Scissors, q=Quit): ";
        std::cin >> input;

        if (input == 'q') break;

        IGameObject* user_choice = nullptr;
        if      (input == 'r') user_choice = new Rock();
        else if (input == 'p') user_choice = new Paper();
        else if (input == 's') user_choice = new Scissors();
        else { std::cout << "Unknown input, try again.\n"; continue; }

        IGameObject* rival_choice = rival.pick();

        printSideBySide(user_choice, rival_choice);
        std::cout << "\n  >>> " << whoWins(user_choice, rival_choice) << "\n";

        delete user_choice;
        delete rival_choice;
    }

    std::cout << "Goodbye!\n";
    return 0;
}
