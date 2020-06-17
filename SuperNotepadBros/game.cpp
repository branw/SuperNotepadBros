#include "game.hpp"

#include <algorithm>

auto const ROWS = 25;
auto const COLUMNS = 80;

Field::Field() : buffer_{ 0 } {
    for (auto y = 0; y < ROWS; y++) {
        for (auto x = 0; x < COLUMNS; x++) {
            (*this)(x, y) = ' ';
        }
        (*this)(COLUMNS, y) = '\n';
    }
}

Field::Field(wchar_t const field[2026]) {
    std::copy(&field[0], &field[2026], buffer_);
}

Field& Field::operator=(Field const field) {
    std::copy(field.begin(), field.end(), buffer_);

    return *this;
}

wchar_t &Field::operator()(size_t x, size_t y) {
    return buffer_[x + (COLUMNS + 1) * y];
}

wchar_t* Field::begin() {
    return &buffer_[0];
}

wchar_t* Field::end() {
    return &buffer_[2026];
}

wchar_t const * Field::begin() const {
    return &buffer_[0];
}

wchar_t const * Field::end() const {
    return &buffer_[2026];
}

Game::Game() {
    field_ =
        L"                                                                                \n"
        L"                                                        /\\                      \n"
        L"       |                                               /__\\                     \n"
        L"    \\ ___ /  }                                        /%%| \\                    \n"
        L"     /   \\                                           /|== \\ \\                   \n"
        L"   - |   | -                                        /_|@@@ \\ \\                  \n"
        L"     \\___/                                         O£||)))=|_ \\                 \n"
        L"    /     \\                                       :--\\|    __| \\                \n"
        L"       |                                         / |  | (((____#\\               \n"
        L"                                                /  |-          \\ \\              \n"
        L"                                               /___/   =  T-----\\ \\             \n"
        L"                      /\\                      // O             O \\ \\            \n"
        L"                     /  \\                    // 1##- --)=------# #  \\           \n"
        L"    ____            /____\\                  //       { | \\     #1#   \\          \n"
        L"   / ___\\          //£@ \\ \\                //|    ## ###= \\    # #    \\         \n"
        L"  | |O>O||        /_)))  \\ \\              // |  --|| | |   \\ ###1£#   _\\        \n"
        L"  | |(=)||       // O     \\ \\            //  |    || | |    \\#£@ (#  /  1       \n"
        L"__|_|\\$/||      //  -111   \\ \\          //    \"--- | | ())   \\))1## /    \\      \n"
        L"        |      //       --  |_\\        //}      / \\|\"|____)   \\_ __/   /9 \\     \n"
        L"   | |__|__   //>@        - |  \\#     //       -|                     /    \\   I\n"
        L"   (______)) / >@           |   \\-T  //         |                    /      \\  e\n"
        L"############2#####~########-#---# # ###=#### #### ######T5  T###:   #### #######\n"
        L"############ #############       -# ### #   \"    \"      #   #####\" ##### #######\n"
        L"#########£       O £        ##%           #############           ###### #######\n"
        L"#####################################3###1############   T))))##########X#######\n";;
}

void Game::update(int dx, int dy) {
    dx_ = dx;
    dy_ = dy;

    next_field_ = field_;

    if (reverse_) {
        swap('{', '}');
        swap('[', ']');
        swap('<', '>');
        swap('(', ')');

        reverse_ = false;
    }

    update_1();
    if (ticks_++ % 8 == 7) {
        update_8();
    }

    field_ = next_field_;

    if (tired_) {
        tired_ = (tired_ + 1) % 3;
    }
}

Field const& Game::field() const {
    return field_;
}

void Game::replace(wchar_t from, wchar_t to) {
    for (auto y = 0; y < ROWS; y++) {
        for (auto x = 0; x < COLUMNS; x++) {
            if (field_(x, y) == from) {
                next_field_(x, y) = to;
            }
        }
    }
}

void Game::swap(wchar_t a, wchar_t b) {
    for (auto y = 0; y < ROWS; y++) {
        for (auto x = 0; x < COLUMNS; x++) {
            if (field_(x, y) == a) {
                next_field_(x, y) = b;
                field_(x, y) = L' ';
            }
            else if (field_(x, y) == b) {
                next_field_(x, y) = a;
                field_(x, y) = L' ';
            }
        }
    }
}

bool Game::probe(size_t x, size_t y, wchar_t ch) {
    if (y >= ROWS || x >= COLUMNS) {
        return true;
    }

    auto ob = field_(x, y), next_ob = next_field_(x, y);

    switch (ch) {
    case 'I': {
        switch (ob) {
        case 'E': {
            won_ = true;
            return false;
        }

        case L'£': {
            next_field_(x, y) = ' ';
            return false;
        }

        case '0': {
            return true;
        }

        case '[':
        case ']':
        case '{':
        case '}':
        case '%': {
            alive_ = false;
            return true;
        }

        case 'O': {
            int d = dx_;
            if (d != 0 && probe(x, y + 1, ob) && !probe(x + d, y, ob) && field_(x - d, y) == 'I') {
                next_field_(x + d, y) = ob;
                next_field_(x, y) = ' ';

                if (field_(x, y + 1) == ';') {
                    next_field_(x, y + 2) = ' ';
                }

                if (!tired_) {
                    tired_ = 1;
                }
                return false;
            }
            return true;
        }

        default:
            break;
        }

        break;
    }

    case '<':
    case '>': {
        if (ob == 'I') {
            return false;
        }
        break;
    }

    case '{':
    case '}':
    case '[':
    case ']': {
        switch (ob) {
        case 'O':
        case L'£': {
            int d = (ch == '}' || ch == ']') ? 1 : -1;
            if (probe(x, y + 1, ob) && !probe(x + d, y, ob)) {
                next_field_(x + d, y) = ob;
                return false;
            }
            break;
        }

        case 'I': {
            alive_ = false;
            return false;
        }

        default:
            break;
        }
        break;
    }

    default:
        break;
    }

    return !(ob == ' ' && next_ob == ' ');
}

void Game::update_1() {
    int money_left = 0;
    int players_left = 0;

    for (size_t y = 0; y < ROWS; ++y) {
        for (size_t x = 0; x < COLUMNS; ++x) {
            auto ch = field_(x, y);

            if ((ch == 'I' || ch == '[' || ch == ']' || ch == 'O' || ch == '%' || ch == L'£') && y == ROWS - 1) {
                next_field_(x, y) = ' ';
                continue;
            }
            else if (ch >= '1' && ch <= '9' && y > 0 && field_(x, y - 1) != ' ') {
                next_field_(x, y) = ch - 1;
                continue;
            }
            
            switch (ch) {
            case '0': {
                next_field_(x, y) = ' ';
                break;
            }

            case '%': {
                if (y < ROWS - 1 && field_(x, y + 1) == ';' && next_field_(x, y) == '%') {
                    next_field_(x, y) = ' ';
                    if (y < ROWS - 2) {
                        next_field_(x, y + 2) = ch;
                    }
                }
                else if (!probe(x, y + 1, ch) || (y < ROWS - 1 && field_(x, y + 1) == 'I')) {
                    next_field_(x, y + 1) = ch;
                    next_field_(x, y) = ' ';
                }
                break;
            }

            case ':': {
                if (y > 0) {
                    if (field_(x, y - 1) == 'O' || field_(x, y - 1) == '%') {
                        next_field_(x, y) = ';';
                    }
                    else if (field_(x, y - 1) == 'X' || field_(x, y - 1) == '.') {
                        next_field_(x, y) = '.';
                    }
                }
                break;
            }

            case ';': {
                if (y > 0 && field_(x, y - 1) != 'O' && field_(x, y - 1) != '%') {
                    next_field_(x, y) = ':';
                }
                break;
            }

            case 'O': {
                if (y < ROWS - 1 && field_(x, y + 1) == ';' && next_field_(x, y) == 'O') {
                    next_field_(x, y) = ' ';
                    if (y < ROWS - 2) {
                        next_field_(x, y + 2) = ch;
                    }
                }
                else if (!probe(x, y + 1, ch)) {
                    next_field_(x, y + 1) = ch;
                    field_(x, y) = ' ';
                    next_field_(x, y) = ' ';
                }
                break;
            }

            case '.': {
                next_field_(x, y) = ':';
                break;
            }

            case '&':
            case '?': {
                for (auto dy = -1; dy <= 1; ++dy) {
                    for (auto dx = -1; dx <= 1; ++dx) {
                        if ((x + dx) >= 0 && (y + dx) >= 0 && (x + dx) <= COLUMNS - 1 && (y + dy) <= ROWS - 1 && field_(x + dx, y + dy) == '0') {
                            next_field_(x, y) = '0';
                        }
                    }
                }
                break;
            }

            case L'£': {
                ++money_left;

                if (!probe(x, y + 1, ch)) {
                    next_field_(x, y + 1) = ch;
                    next_field_(x, y) = ' ';
                }
                else if (y < ROWS - 1 && field_(x, y + 1) == 'I') {
                    next_field_(x, y) = ' ';
                }
                break;
            }

            case 'T': {
                if (y > 0 && field_(x, y - 1) == 'I') {
                    if (x > 0 && dx_ < 0 && next_field_(x - 1, y - 1) == 'I' && !probe(x - 1, y, ch)) {
                        next_field_(x - 1, y) = ch;
                        next_field_(x, y) = ' ';
                    }
                    else if (x < COLUMNS - 1 && dx_ > 0 && next_field_(x + 1, y - 1) == 'I' && !probe(x + 1, y, ch)) {
                        next_field_(x + 1, y) = ch;
                        next_field_(x, y) = ' ';
                    }
                }
                break;
            }

            case L'¦': {
                if (y > 0 && field_(x, y - 1) == '.') {
                    next_field_(x, y) = 'A';
                }
                break;
            }

            case 'A': {
                if (y > 0 && (field_(x, y - 1) == ':' || field_(x, y - 1) == '.') && !probe(x, y + 1, 'O')) {
                    next_field_(x, y + 1) = 'O';
                    next_field_(x, y) = L'¦';
                }
                break;
            }

            case 'I': {
                ++players_left;

                if (!probe(x, y + 1, ch)) {
                    next_field_(x, y + 1) = ch;
                    next_field_(x, y) = ' ';
                }
                else if (y < ROWS - 1) {

                    auto fl = field_(x, y + 1);
                    if (fl == '(' || fl == ')' || tired_) {
                        break;
                    }

                    if (dx_ < 0 && x > 0) {
                        if (!probe(x - 1, y, ch)) {
                            next_field_(x - 1, y) = ch;
                            next_field_(x, y) = ' ';
                        }
                        else if (y > 0 && !probe(x - 1, y - 1, ch)) {
                            next_field_(x - 1, y - 1) = ch;
                            next_field_(x, y) = ' ';
                        }
                    }
                    else if (dx_ > 0 && x < COLUMNS - 1) {
                        if (!probe(x + 1, y, ch)) {
                            next_field_(x + 1, y) = ch;
                            next_field_(x, y) = ' ';
                        }
                        else if (y > 0 && !probe(x + 1, y - 1, ch)) {
                            next_field_(x + 1, y - 1) = ch;
                            next_field_(x, y) = ' ';
                        }
                    }
                    else if (dy_ > 0 && y > 0) {
                        if (field_(x, y - 1) == '-' && !probe(x, y - 2, ch)) {
                            next_field_(x, y - 2) = 'I';
                            next_field_(x, y) = ' ';
                        }
                        else if (y < ROWS - 1 && field_(x, y + 1) == '"' && !probe(x, y - 1, ch)) {
                            next_field_(x, y + 1) = ' ';
                            next_field_(x, y) = '"';
                            next_field_(x, y - 1) = ch;
                        }
                    }
                    else if (dy_ < 0 && y < ROWS - 1) {
                        if (field_(x, y + 1) == '-' && !probe(x, y + 2, ch)) {
                            next_field_(x, y + 2) = 'I';
                            next_field_(x, y) = ' ';
                        }
                        else if (field_(x, y + 1) == '"' && !probe(x, y + 2, '"')) {
                            next_field_(x, y + 2) = '"';
                            next_field_(x, y + 1) = ch;
                            next_field_(x, y) = ' ';
                        }
                        else if (field_(x, y + 1) == '~') {
                            replace('@', '0');
                        }
                        else if (field_(x, y + 1) == '`') {
                            reverse_ = true;
                        }
                    }
                }
                break;
            }

            case 'x': {
                if (y > 0 && field_(x, y - 1) != ' ') {
                    next_field_(x, y - 1) = ' ';
                    next_field_(x, y) = 'X';
                }
                break;
            }

            case 'X': {
                next_field_(x, y) = 'x';
                break;
            }

            case 'e': {
                if (no_money_left_) {
                    next_field_(x, y) = 'E';
                }
                break;
            }

            case 'E': {
                if (!no_money_left_) {
                    next_field_(x, y) = 'e';
                }
                break;
            }

            case '(':
            case ')': {
                if (y > 0) {
                    auto ob = field_(x, y - 1);
                    auto d = (ch == ')') ? 1 : -1;
                    if ((ob == 'I' || ob == '[' || ob == ']' || ob == 'O' || ob == '%' ||
                        ob == L'£') && !probe(x + d, y - 1, ob)) {
                        next_field_(x, y - 1) = ' ';
                        next_field_(x + d, y - 1) = ob;
                    }
                }
                break;
            }

            case '<':
            case '>': {
                int d = (ch == '<') ? -1 : 1;
                if (!probe(x + d, y, ch)) {
                    next_field_(x, y) = ' ';
                    next_field_(x + d, y) = ch;

                    if (y > 0) {
                        auto ob = field_(x, y - 1);
                        if ((ob == 'I' || ob == '[' || ob == ']' || ob == 'O' || ob == '%' || ob == L'£') && !probe(x + d, y - 1, ob)) {
                            next_field_(x, y - 1) = ' ';
                            next_field_(x + d, y - 1) = ob;
                        }
                    }
                }
                break;
            }

            case '{':
            case '}':
            case '[':
            case ']': {
                if (y < ROWS - 1) {
                    auto d = (ch == '[' || ch == '{') ? -1 : 1;
                    auto gr = (ch == '[' || ch == ']');
                    auto od = (d > 0) ? (gr ? '[' : '{') : (gr ? ']' : '}');

                    auto fl = field_(x, y + 1);
                    if (!(gr && (fl == '(' || fl == ')'))) {
                        if (probe(x + d, y, ch) && (!gr || probe(x, y + 1, ch))) {
                            next_field_(x, y) = od;
                        }
                        else if (gr && !probe(x, y + 1, ch)) {
                            next_field_(x, y) = ' ';
                            next_field_(x, y + 1) = ch;
                        }
                        else {
                            next_field_(x, y) = ' ';
                            next_field_(x + d, y) = ch;
                        }
                    }
                }
                break;
            }

            default:
                break;
            }
        }
    }

    if (money_left == 0) {
        no_money_left_ = true;
    }
    if (players_left == 0) {
        alive_ = false;
    }
}

void Game::update_8() {
    for (size_t y = 0; y < ROWS; ++y) {
        for (size_t x = 0; x < COLUMNS; ++x) {
            auto ch = field_(x, y);
            switch (ch) {
            case '=': {
                if (y == 0) {
                    continue;
                }

                auto ob = field_(x, y - 1);
                if (ob != ' ' && ob != 'I' && !probe(x, y + 1, ob)) {
                    next_field_(x, y + 1) = ob;
                }
                break;
            }

            case 'b':
            case 'd': {
                auto d = (ch == 'd') ? -1 : 1;
                if (probe(x + d, y, ch)) {
                    next_field_(x, y) = (ch == 'd') ? 'b' : 'd';
                }
                else {
                    next_field_(x, y) = ' ';
                    next_field_(x + d, y) = ch;
                }
                break;
            }

            default:
                break;
            }
        }
    }
}
