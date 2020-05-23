#include "game.hpp"

#include <Windows.h>
#include <algorithm>

Field::Field(const wchar_t field[2026]) : std::wstring(field) {}

Field::Field() : std::wstring(81 * 25, L' ') {
    for (auto y = 0; y < 25; y++) {
        (*this)(80, y) = '\n';
    }
}

Field::reference Field::operator()(size_t x, size_t y) {
    return (*this)[x + (80 + 1) * y];
}

Field::operator LPARAM() {
    return reinterpret_cast<LPARAM>(c_str());
}

void Field::replace(wchar_t from, wchar_t to) {
    std::replace(begin(), end(), from, to);
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
        L"__|_|\\$/||      //  -111   \\ \\          //    \"-- - | | ())   \\))1## / \\      \n"
        L"        |      //       --  |_\\        //}      / \\|\" | ____)   \\_ __/   /9 \\     \n"
        L"   | |__|__   //>@        - |  \\#     //       -|                     /    \\   I\n"
        L"   (______)) / >@           |   \\-T  //         |                    /      \\  e\n"
        L"############2#####~########-#---# # ###=#### #### ######T5  T###:   #### #######\n"
        L"############ #############       -# ### #   \"    \"      #   #####\" ##### #######\n"
        L"#########£       O £        ##%           #############           ###### #######\n"
        L"#####################################3###1############   T))))##########X#######\n";
}

void Game::update(int dx = 0, int dy = 0) {
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

void Game::swap(wchar_t a, wchar_t b) {
    for (auto y = 0; y < 25; y++) {
        for (auto x = 0; x < 80; x++) {
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

bool Game::probe(int x, int y, wchar_t ch) {
    if (y >= 25 || x >= 80) {
        return true;
    }

    unsigned long ob = field_(x, y), next_ob = next_field_(x, y);

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
            if (d != 0 && probe(x, y + 1, ob) && !probe(x + d, y, ob) &&
                field_(x - d, y) == 'I') {
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

    for (unsigned y = 0; y < 25; ++y) {
        for (unsigned x = 0; x < 80; ++x) {
            unsigned long ch = field_(x, y);

            if ((ch == 'I' || ch == '[' || ch == ']' || ch == 'O' || ch == '%' || ch == L'£') &&
                y == 25 - 1) {
                next_field_(x, y) = ' ';
            }
            else if (ch >= '1' && ch <= '9' && y > 0 && field_(x, y - 1) != ' ') {
                next_field_(x, y) = ch - 1;
            }
            else {
                switch (ch) {
                case '0': {
                    next_field_(x, y) = ' ';
                    break;
                }

                case '%': {
                    if (y < 25 - 1 && field_(x, y + 1) == ';' && next_field_(x, y) ==
                        '%') {
                        next_field_(x, y) = ' ';
                        if (y < 25 - 2) {
                            next_field_(x, y + 2) = ch;
                        }
                    }
                    else if (!probe(x, y + 1, ch) ||
                        (y < 25 - 1 && field_(x, y + 1) == 'I')) {
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
                    if (y < 25 - 1 &&
                        field_(x, y + 1) == ';' && next_field_(x, y) == 'O') {
                        next_field_(x, y) = ' ';
                        if (y < 25 - 2) {
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
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            if ((x + dx) >= 0 && (y + dx) >= 0 &&
                                (x + dx) <= 80 - 1 && (y + dy) <= 25 - 1 &&
                                field_(x + dx, y + dy) == '0') {
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
                    else if (y < 25 - 1 && field_(x, y + 1) == 'I') {
                        next_field_(x, y) = ' ';
                    }
                    break;
                }

                case 'T': {
                    if (y > 0 && field_(x, y - 1) == 'I') {
                        if (x > 0 && dx_ < 0 &&
                            next_field_(x - 1, y - 1) == 'I' && !probe(x - 1, y, ch)) {
                            next_field_(x - 1, y) = ch;
                            next_field_(x, y) = ' ';
                        }
                        else if (x < 80 - 1 && dx_ > 0 &&
                            next_field_(x + 1, y - 1) == 'I' &&
                            !probe(x + 1, y, ch)) {
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
                    if (y > 0 && (field_(x, y - 1) == ':' || field_(x, y - 1) == '.')
                        && !probe(x, y + 1, 'O')) {
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
                    else if (y < 25 - 1) {
                        unsigned long fl = field_(x, y + 1);

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
                        else if (dx_ > 0 && x < 80 - 1) {
                            if (!probe(x + 1, y, ch)) {
                                next_field_(x + 1, y) = ch;
                                next_field_(x, y) = ' ';
                            }
                            else if (y > 0 && !probe(x + 1, y - 1, ch)) {
                                next_field_(x + 1, y - 1) = ch;
                                next_field_(x, y) = ' ';
                            }
                        }
                        else if (dy_ > 0) {
                            if (y > 0 && field_(x, y - 1) == '-' &&
                                !probe(x, y - 2, ch)) {
                                next_field_(x, y - 2) = 'I';
                                next_field_(x, y) = ' ';
                            }
                            else if (y > 0 && y < 25 - 1 && field_(x, y + 1) == '"' &&
                                !probe(x, y - 1, ch)) {
                                next_field_(x, y + 1) = ' ';
                                next_field_(x, y) = '"';
                                next_field_(x, y - 1) = ch;
                            }
                        }
                        else if (dy_ < 0) {
                            if (y < 25 - 1 && field_(x, y + 1) == '-' &&
                                !probe(x, y + 2, ch)) {
                                next_field_(x, y + 2) = 'I';
                                next_field_(x, y) = ' ';
                            }
                            else if (y < 25 - 1 && field_(x, y + 1) == '"' &&
                                !probe(x, y + 2, '"')) {
                                next_field_(x, y + 2) = '"';
                                next_field_(x, y + 1) = ch;
                                next_field_(x, y) = ' ';
                            }
                            else if (y < 25 - 1 && field_(x, y + 1) == '~') {
                                next_field_.replace('@', '0');
                            }
                            else if (y < 25 - 1 && field_(x, y + 1) == '`') {
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
                        unsigned long ob = field_(x, y - 1);
                        int d = (ch == ')') ? 1 : -1;
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
                            unsigned long ob = field_(x, y - 1);
                            if ((ob == 'I' || ob == '[' || ob == ']' || ob == 'O' || ob == '%' ||
                                ob == L'£') && !probe(x + d, y - 1, ob)) {
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
                    if (y < 25 - 1) {
                        int d = (ch == '[' || ch == '{') ? -1 : 1;
                        bool gr = (ch == '[' || ch == ']');
                        unsigned long od = (char)((d > 0) ? (gr ? '[' : '{') : (gr ? ']' : '}'));

                        unsigned long fl = field_(x, y + 1);
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
    }

    if (money_left == 0) {
        no_money_left_ = true;
    }
    if (players_left == 0) {
        alive_ = false;
    }
}

void Game::update_8() {
    for (auto y = 0; y < 25; ++y) {
        for (auto x = 0; x < 80; ++x) {
            unsigned long ch = field_(x, y);
            switch (ch) {
            case '=': {
                if (y == 0) {
                    continue;
                }

                unsigned long ob = field_(x, y - 1);
                if (ob != ' ' && ob != 'I' && !probe(x, y + 1, ob)) {
                    next_field_(x, y + 1) = ob;
                }
                break;
            }

            case 'b':
            case 'd': {
                int d = (ch == 'd') ? -1 : 1;
                if (probe(x + d, y, ch)) {
                    next_field_(x, y) = (char)((ch == 'd') ? 'b' : 'd');
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
