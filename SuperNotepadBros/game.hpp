#pragma once

#include <string>

class Field : public std::wstring {
public:
    Field(const wchar_t field[2026]);

    Field();

    reference operator()(size_t x, size_t y);

    operator LPARAM();

    void replace(wchar_t from, wchar_t to);
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

    void swap(wchar_t a, wchar_t b);

    bool probe(int x, int y, wchar_t ch);

    void update_1();

    void update_8();
};
