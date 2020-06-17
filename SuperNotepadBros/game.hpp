#pragma once

#include <string>

class Field {
public:
    Field();

    Field(wchar_t const field[2026]);

    Field& operator=(Field const field);

    wchar_t &operator()(size_t x, size_t y);

    wchar_t* begin();

    wchar_t* end();

    wchar_t const * begin() const;

    wchar_t const * end() const;

private:
    wchar_t buffer_[2026];
};

struct Game {
    Game();

    void update(int dx = 0, int dy = 0);

    Field const& field() const;

private:
    int dx_ = 0, dy_ = 0;

    bool no_money_left_ = false;
    bool won_ = false;
    bool alive_ = true;
    int ticks_ = 0;

    bool reverse_ = false;
    int tired_ = 0;

    Field field_, next_field_;

    void replace(wchar_t from, wchar_t to);

    void swap(wchar_t a, wchar_t b);

    bool probe(size_t x, size_t y, wchar_t ch);

    void update_1();

    void update_8();
};
