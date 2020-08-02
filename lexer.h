//
// Created by Alex
//

#ifndef MYLIBS_LEXER_H
#define MYLIBS_LEXER_H

#include "regex.h"

namespace alex {
    //template <typename char_t>
    class Lexer {
    public:
        using iter_t = const char *;
        using char_t = char;
        using viewer_t = std::string;
        RegexGenerator generator;
        std::vector<std::unique_ptr<RegexState>> state_machine;
        std::vector<std::unique_ptr<RegexState>> whitespace;
        iter_t first;
        iter_t last;
        iter_t current;
        int token_symbol = -1;
        int token_line = 0;
        int token_length = 0;
        iter_t token_start = 0;
        iter_t token_line_start = 0;
        void set_whitespace(const char *pattern) {
            RegexParser parser(pattern);
            RegexGenerator space_generator;
            space_generator.feed(parser.parse_concat());
            whitespace = std::move(space_generator.generate());
        }
        int add_pattern(const char *pattern) {
            RegexParser parser(pattern);
            return generator.feed(parser.parse_concat());
        }
        void generate_states() {
            state_machine = std::move(generator.generate());
        }
        void reset(iter_t begin) {
            reset(begin, begin + strlen(begin));
        }
        void reset(iter_t begin, iter_t end) {
            this->current = this->first = this->token_line_start = begin;
            this->last = end;
        }
        void skip() {
            if (whitespace.empty()) {
                return;
            }
            auto *state = whitespace[0].get();
            do {
                auto *trans = state->find_trans(*current);
                if (!trans) {
                    break;
                }
                if (*current++ == '\n') {
                    token_line++;
                    token_line_start = current;
                }
                state = trans->state;
            } while (*current != '\0');
        }
        void advance() {
            skip();
            auto *state = state_machine[0].get();
            token_start = current;
            do {
                auto *trans = state->find_trans(*current);
                if (trans == nullptr) {
                    break;
                }
                state = trans->state;
            } while (*++current != '\0');
            token_symbol = state->symbol;
            token_length = current - token_start;
        }
        inline bool good() { return current < last; }
        inline viewer_t lexme() { return viewer_t(&*token_start, token_length); }
        inline int symbol() { return token_symbol; }
        inline int line() { return token_line; }
        inline int column() { return current - token_line_start; }

    };

}
#endif //MYLIBS_LEXER_H
