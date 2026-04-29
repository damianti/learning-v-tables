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
//  Helper — print vptr and vtable entries
// ============================================================

static void printVTable(const char* label, const void* obj, int entries = 4) {
    void** vtable = *reinterpret_cast<void***>(const_cast<void*>(obj));
    std::cout << "[" << label << "] vptr = " << static_cast<void*>(vtable) << "\n";
    for (int i = 0; i < entries; ++i)
        std::cout << "  vtable[" << i << "] = " << vtable[i] << "\n";
}

// ============================================================
//  Helper — mprotect wrappers
// ============================================================

static void setPageWritable(void* addr) {
    uintptr_t page = reinterpret_cast<uintptr_t>(addr) & ~(static_cast<uintptr_t>(getpagesize()) - 1);
    mprotect(reinterpret_cast<void*>(page), getpagesize(), PROT_READ | PROT_WRITE);
}

static void setPageReadOnly(void* addr) {
    uintptr_t page = reinterpret_cast<uintptr_t>(addr) & ~(static_cast<uintptr_t>(getpagesize()) - 1);
    mprotect(reinterpret_cast<void*>(page), getpagesize(), PROT_READ);
}

// ============================================================
//  Task 1 — vptr swap (per-instance)
// ============================================================

void makeAlwaysRock(IPlayer* player) {
    RockPlayer rock;
    memcpy(player, &rock, sizeof(void*));
}

// ============================================================
//  Task 2 — vtable patch (per-class, affects all Rock objects)
// ============================================================

void patchRockDraw() {
    Rock rock_obj;
    FunnyRock funny_obj;

    void** rock_vt  = *reinterpret_cast<void***>(&rock_obj);
    void** funny_vt = *reinterpret_cast<void***>(&funny_obj);

    setPageWritable(rock_vt);
    rock_vt[0] = funny_vt[0];
    setPageReadOnly(rock_vt);
}

// ============================================================
//  main
// ============================================================

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    RandomPlayer rival;
    RockPlayer rock_player;
    Rock rock_obj;
    FunnyRock funny_obj;
    IPlayer* rival_ptr = &rival;

    std::cout << "\n=== vtable dump (before patches) ===\n";
    printVTable("RandomPlayer", &rival);
    printVTable("RockPlayer",   &rock_player);
    printVTable("Rock",         &rock_obj);
    printVTable("FunnyRock",    &funny_obj);

    makeAlwaysRock(rival_ptr);
    std::cout << "\n=== after makeAlwaysRock ===\n";
    printVTable("RandomPlayer (vptr swapped)", &rival);

    patchRockDraw();
    std::cout << "\n=== after patchRockDraw ===\n";
    printVTable("Rock (vtable patched)", &rock_obj);
    std::cout << "\n";

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

        IGameObject* rival_choice = rival_ptr->pick();

        printSideBySide(user_choice, rival_choice);
        std::cout << "\n  >>> " << whoWins(user_choice, rival_choice) << "\n";

        delete user_choice;
        delete rival_choice;
    }

    std::cout << "Goodbye!\n";
    return 0;
}
