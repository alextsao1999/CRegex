//
// Created by Alex
//

#ifndef ALEX_LIBS_REGEX_H
#define ALEX_LIBS_REGEX_H

#include <utility>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <string>
#include <string.h>

#define RegexNodeDecl() int accept(RegexVisitor *) override;
#define RegexNodeList(V) \
            V(RegexConcat) \
            V(RegexBracket) \
            V(RegexOr) \
            V(RegexRange) \
            V(RegexPlus) \
            V(RegexRepeat) \
            V(RegexQuestion) \
            V(RegexStar)
namespace alex {
    class RegexVisitor;
    class RegexNode {
    public:
        int index = 0;
        std::vector<int> firstpos;
        std::vector<int> lastpos;
        std::vector<int> followpos;
        inline void set_index(int idx) { this->index = idx; }
        inline int get_index(int idx) const { return this->index; }
        virtual ~RegexNode() = default;
        virtual int accept(RegexVisitor *) = 0;
        virtual void print() {}
        virtual bool nullable() { return false; }
    };
    class RegexConcat : public RegexNode {
    public:
        RegexNodeDecl();
        RegexConcat() = default;

        RegexConcat(const std::shared_ptr<RegexNode> &lhs, const std::shared_ptr<RegexNode> &rhs) {
            add(lhs);
            add(rhs);
        }

        std::vector<std::shared_ptr<RegexNode>> nodes;

        void add(const std::shared_ptr<RegexNode> &node) {
            nodes.push_back(node);
        }
        void print() override {
            std::cout << "(";
            for (auto &item : nodes) {
                item->print();
            }
            std::cout << ")";
        }
        bool nullable() override {
            for (auto &item : nodes) {
                if (!item->nullable()) {
                    return false;
                }
            }
            return true;
        }
    };
    class RegexBracket : public RegexNode {
    public:
        RegexNodeDecl();
        RegexBracket() = default;
        std::vector<std::shared_ptr<RegexNode>> nodes;
        void add(const std::shared_ptr<RegexNode>& node) {
            nodes.push_back(node);
        }
        void print() override {
            std::cout << "[";
            for (auto &item : nodes) {
                item->print();
            }
            std::cout << "]";
        }
    };
    class RegexOr : public RegexNode {
    public:
        RegexNodeDecl();
        RegexOr() = default;
        std::shared_ptr<RegexNode> lhs;
        std::shared_ptr<RegexNode> rhs;
        RegexOr(std::shared_ptr <RegexNode> lhs, std::shared_ptr <RegexNode> rhs) : lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        void print() override {
            lhs->print();
            std::cout << "|";
            rhs->print();
        }
        bool nullable() override { return rhs->nullable() || rhs->nullable(); }
    };
    class RegexRange : public RegexNode {
    public:
        RegexNodeDecl();
        RegexRange(int begin, int end) : begin(begin), end(end) {}
        RegexRange(int chr) : begin(chr), end(chr)  {}
        int begin;
        int end;
        int symbol = -1;
    };
    class RegexStar : public RegexNode {
    public:
        RegexNodeDecl();
        std::shared_ptr <RegexNode> node;
        RegexStar(std::shared_ptr <RegexNode> node) : node(std::move(node)) {}
        bool nullable() override { return true; }
    };
    class RegexPlus : public RegexNode {
    public:
        RegexNodeDecl();
        std::shared_ptr <RegexNode> node;
        RegexPlus(std::shared_ptr <RegexNode> node) : node(std::move(node)) {}
        void print() override {
            node->print();
            std::cout << "+";
        }
    };
    class RegexQuestion : public RegexNode {
    public:
        RegexNodeDecl();
        std::shared_ptr <RegexNode> node; // 问号
        RegexQuestion(std::shared_ptr <RegexNode> node) : node(std::move(node)) {}
        void print() override {
            node->print();
            std::cout << "?";
        }
        bool nullable() override { return true; }
    };
    class RegexRepeat : public RegexNode {
    public:
        RegexNodeDecl();
        std::shared_ptr <RegexNode> node;
        int begin;
        int end;
        RegexRepeat(std::shared_ptr<RegexNode> node, int begin, int end) : node(std::move(node)), begin(begin), end(end) {}
        void print() override {
            node->print();
            std::cout << "{" << begin << ", " << end << "}";
        }
        bool nullable() override { return begin == 0; }
    };
    class RegexVisitor {
    public:
#define DefineVisitor(Type) virtual int visit(Type *node) { return node->accept(this); }
        RegexNodeList(DefineVisitor)
#undef  DefineVisitor
    };
#define DefineVisitor(Type) int Type::accept(RegexVisitor *visitor) { return visitor->visit(this); }
    RegexNodeList(DefineVisitor)
#undef  DefineVisitor
    class RegexState;
    struct RegexTransition {
        RegexState *state = nullptr;
        int index = -1;
        int begin;
        int end;
        RegexTransition(RegexState *state, int index, int begin, int end) :
        state(state), index(index), begin(begin), end(end) {}
        RegexTransition(int begin, int end) : begin(begin), end(end) {}
    };
    struct RegexState {
        int symbol = -1;
        bool visited = false;
        std::vector<RegexTransition> transitions;
        std::vector<int> go_to; // list index of the leaf
        inline const RegexTransition *find_trans(const int chr) const {
            const RegexTransition *dot = nullptr;
            for (auto &item : transitions) {
                if (item.begin == -1) {
                    dot = &item;
                }
                if (chr >= item.begin  && chr <= item.end) {
                    return &item;
                }
            }
            return dot;
        }
    };
    class RegexGenerator : private RegexVisitor {
    public:
        int index = 0;
        int visit_count = 0;
        int symbol_count = 0;
        std::vector<int> firstpos; // all first pos
        std::vector<RegexRange *> lists; // record all of the leaf node
        std::vector<std::unique_ptr<RegexState>> states;
        RegexGenerator() = default;
        int feed(std::shared_ptr<RegexNode> node) {
            node->accept(this);
            firstpos.insert(firstpos.end(), node->firstpos.begin(), node->firstpos.end());
            for (auto &item : node->lastpos) {
                lists[item]->symbol = symbol_count;
            }
            return symbol_count++;
        }
        std::vector<std::unique_ptr<RegexState>> generate() {
            get_goto_state(firstpos);
            while (visit_count < states.size()) {
                if (!states[visit_count]->visited) {
                    generate_transition(states[visit_count].get());
                    ++visit_count;
                }
            }
            return std::move(states);
        }
    private:
        static bool contains(std::vector<int> vec, int value) {
            for (auto &item : vec) {
                if (item == value) {
                    return true;
                }
            }
            return false;
        }
        bool is_same(std::vector<int> &lhs, std::vector<int> &rhs) {
            for (auto &slice : rhs) {
                if (!contains(lhs, slice)) {
                    return false;
                }
            }
            if (!lhs.empty() && rhs.empty()) {
                return false;
            }
            return true;
        }
        int get_goto_state(std::vector<int> &go_to, int symbol = -1) {
            //int symbol = get_symbol(go_to);
            for (int i = 0; i < states.size(); ++i) {
                if (is_same(states[i]->go_to, go_to) && symbol == states[i]->symbol) {
                    return i;
                }
            }
            auto *state = new RegexState;
            state->symbol = symbol;
            state->go_to = go_to;
            states.emplace_back(state);
            return states.size() - 1;
        }
        void insert_goto(RegexState *state, char begin, char end) {
            std::vector<int> go_to;
            std::vector<int> matched;
            int symbol = -1;
            for (auto &id : state->go_to) {
                if (begin >= lists[id]->begin && end <= lists[id]->end) {
                    for (auto &item : lists[id]->followpos) {
                        if (!contains(go_to, item)) {
                            go_to.push_back(item);
                        }
                    }
                    if (begin == lists[id]->begin && end == lists[id]->end && lists[id]->symbol != -1)
                        matched.push_back(id);
                    if (lists[id]->symbol != -1) {
                        symbol = lists[id]->symbol;
                    }
                }
            }
            if (!matched.empty()) {
                for (auto &item : matched) {
                    symbol = lists[item]->symbol;
                }
            }
            auto idx = get_goto_state(go_to, symbol);
            if (idx >= 0) {
                auto *goto_state = states[idx].get();
                state->transitions.push_back({goto_state, idx, begin, end});
            }
        }
        void generate_transition(RegexState *state) {
            state->visited = true;
            std::map<int, int> bounds;
            for (auto &id : state->go_to) { // 将go_to相同的状态合并 之后生成新go_to
                bounds.insert(std::pair<int, int>(lists[id]->begin, 1));
                bounds.insert(std::pair<int, int>(lists[id]->end + 1, -1));
            }
            auto iter = bounds.begin();
            while (iter != bounds.end()) {
                int count = iter->second;
                int begin = (iter++)->first;
                if (iter == bounds.end()) {
                    break;
                }
                if (count < 0 && iter->second > 0) {
                    continue;
                }
                int end = (iter)->first - 1;
                insert_goto(state, begin, end);
            }
        }
        void add_follow(std::vector<int> &elements, std::vector<int> &follow) {
            for (auto &item : elements) {
                lists[item]->followpos.insert(lists[item]->followpos.end(), follow.begin(), follow.end());
            }
        }
        int visit(RegexConcat *node) override {
            for (auto &item : node->nodes) {
                item->accept(this);
            }
            for (auto &item : node->nodes) {
                node->firstpos.insert(node->firstpos.end(), item->firstpos.begin(), item->firstpos.end());
                if (!item->nullable()) {
                    break;
                }
            }
            for (auto iter = node->nodes.rbegin();iter != node->nodes.rend();++iter) {
                node->lastpos.insert(node->lastpos.end(), (*iter)->lastpos.begin(), (*iter)->lastpos.end());
                if (!(*iter)->nullable()) {
                    break;
                }
            }
            for (int i = 1; i < node->nodes.size(); ++i) {
                for (int j = i; j < node->nodes.size(); ++j) {
                    add_follow(node->nodes[i - 1]->lastpos, node->nodes[j]->firstpos);
                    if (!node->nodes[j]->nullable()) {
                        break;
                    }
                }
                //add_follow(node->nodes[i - 1]->lastpos, node->nodes[i]->firstpos);
            }
            return 0;
        }
        int visit(RegexBracket *node) override {
            for (auto &item : node->nodes) {
                item->accept(this);
                node->firstpos.insert(node->firstpos.end(), item->firstpos.begin(), item->firstpos.end());
                node->lastpos.insert(node->lastpos.end(), item->lastpos.begin(), item->lastpos.end());
            }
            return 0;
        }
        int visit(RegexRange *node) override {
            node->set_index(index);
            node->firstpos.push_back(index);
            node->lastpos.push_back(index);
            lists.push_back(node);
            index++;
            return 0;
        }
        int visit(RegexOr *node) override {
            node->lhs->accept(this);
            node->rhs->accept(this);
            node->firstpos.insert(node->firstpos.end(), node->lhs->firstpos.begin(), node->lhs->firstpos.end());
            node->firstpos.insert(node->firstpos.end(), node->rhs->firstpos.begin(), node->rhs->firstpos.end());
            node->lastpos.insert(node->lastpos.end(), node->lhs->lastpos.begin(), node->lhs->lastpos.end());
            node->lastpos.insert(node->lastpos.end(), node->rhs->lastpos.begin(), node->rhs->lastpos.end());
            return 0;
        }
        int visit(RegexPlus *node) override {
            node->node->accept(this);
            node->firstpos = node->node->firstpos;
            node->lastpos = node->node->lastpos;
            add_follow(node->node->lastpos, node->node->firstpos);
            return 0;
        }
        int visit(RegexStar *node) override {
            node->node->accept(this);
            node->firstpos = node->node->firstpos;
            node->lastpos = node->node->lastpos;
            add_follow(node->node->lastpos, node->node->firstpos);
            return 0;
        }
        int visit(RegexRepeat *node) override {
            node->node->accept(this);
            node->firstpos = node->node->firstpos;
            node->lastpos = node->node->lastpos;
            add_follow(node->node->lastpos, node->node->firstpos);
            return 0;
        }
        int visit(RegexQuestion *node) override {
            node->node->accept(this);
            node->firstpos = node->node->firstpos;
            node->lastpos = node->node->lastpos;
            return 0;
        }
    };
    class RegexParser {
    public:
        using char_t = char;
        const char_t *m_first;
        const char_t *m_last;
        const char_t *m_current;
        RegexParser() = default;
        RegexParser(const char_t *first, const char_t *last) : m_first(first), m_last(last), m_current(first) {}
        RegexParser(const char_t *s) : m_first(s), m_last(s + strlen(s)), m_current(s) {}
        inline void reset(const char_t *string) {
            m_first = m_current = string;
            m_last = string + strlen(string);
        }
        inline bool has() const { return m_current < m_last; }
        int parse_character() {
            if (*m_current == '\\') {
                switch (*++m_current) {
                    case 't':
                        m_current++;
                        return '\t';
                    case 'n':
                        m_current++;
                        return '\n';
                    case '\\':
                        m_current++;
                        return '\\';
                    case 'a':
                        m_current++;
                        return '\a';
                    case 'b':
                        m_current++;
                        return '\b';
                    case 'f':
                        m_current++;
                        return '\f';
                    case 'r':
                        m_current++;
                        return '\r';
                    case 'u':
                    case 'x':
                        m_current += 3;
                        return (FromHex(*(m_current - 2)) << 4) + FromHex(*(m_current - 1));
                    default:
                        return *m_current++;
                }
            }
            return *m_current++;
        }
        int parse_integer() {
            int value = 0;
            if (isdigit(*m_current)) {
                do {
                    value *= 10;
                    value += (*m_current - '0');
                } while (isdigit(*++m_current));
            }
            if (*m_current == ',' || *m_current == '}') {
                m_current++;
            }
            return value;
        }
        std::shared_ptr<RegexNode> parse_choice() {
            auto chr = parse_character();
            if (*m_current == '-') {
                m_current++;
                return std::make_shared<RegexRange>(chr, parse_character());
            } else {
                return std::make_shared<RegexRange>(chr);
            }
        }
        std::shared_ptr<RegexNode> parse_bracket() {
            std::shared_ptr<RegexBracket> bracket(new RegexBracket());
            while (has()) {
                switch (*m_current) {
                    case ']':
                        m_current++;
                        return bracket;
                    default: {
                        bracket->add(parse_choice());
                    }
                }
            }
            return bracket;
        }
        std::shared_ptr<RegexNode> parse_char() {
            switch (*m_current) {
                case '.':
                    m_current++;
                    return std::make_shared<RegexRange>(-1);
                default:
                    return std::make_shared<RegexRange>(parse_character());
            }
        }
        std::shared_ptr<RegexNode> parse_item() {
            std::shared_ptr<RegexNode> node;
            switch(*m_current) {
                case '(':
                    m_current++;
                    node = parse_concat();
                    break;
                case '[':
                    m_current++;
                    node = parse_bracket();
                    break;
                default:
                    node = parse_char();
                    break;
            }
            return parse_postfix(node);
        }
        std::shared_ptr<RegexNode> parse_concat() {
            std::shared_ptr<RegexConcat> concat(new RegexConcat());
            while (has()) {
                switch (*m_current) {
                    case ')':
                        m_current++;
                        return concat;
                    case '|':
                        m_current++;
                        return std::make_shared<RegexOr>(concat, parse_concat());
                    default: {
                        concat->add(parse_item());
                    }
                }
            }
            return concat;
        }
        std::shared_ptr<RegexNode> parse_postfix(const std::shared_ptr<RegexNode> &node) {
            switch (*m_current) {
                case '*':
                    m_current++;
                    return std::make_shared<RegexStar>(node);
                case '+':
                    m_current++;
                    return std::make_shared<RegexPlus>(node);
                case '?':
                    m_current++;
                    return std::make_shared<RegexQuestion>(node);
                case '{':
                    m_current++;
                    {
                        int v1 = parse_integer();
                        int v2 = parse_integer();
                        return std::make_shared<RegexRepeat>(node, v1, v2);
                    }
                default:
                    break;
            }
            return node;
        }
        inline static uint8_t FromHex(const char digit) {
            if (digit >= '0' && digit <= '9')
                return digit - '0';
            if (digit >= 'a' && digit <= 'f')
                return digit - 'a' + 10;
            if (digit >= 'A' && digit <= 'F')
                return digit - 'A' + 10;
            return 0;
        }
    };

    std::vector<std::unique_ptr<RegexState>> regex_compile(const char *regex) {
        RegexParser parser(regex);
        auto re = parser.parse_concat();
        RegexGenerator generator;
        generator.feed(re);
        return std::move(generator.generate());
    }
    template <typename It>
    int regex_match(const std::vector<std::unique_ptr<RegexState>> &state_machine, It string) {
        auto *state = state_machine[0].get();
        do {
            auto *trans = state->find_trans(*(string));
            if (trans == nullptr) {
                return state->symbol;
            }
            state = trans->state;
            if ((*++string) == '\0') {
                return state->symbol;
            }
        } while (*string != '\0');
        return state->symbol;
    }
    std::string regex_emit_c(const std::vector<std::unique_ptr<RegexState>> &state_machine) {
        std::string result;
        result.append("int GetNextToken() {\n");
        result.append("    int chr = 0;\n");
        for (int i = 0; i < state_machine.size(); ++i) {
            result += "    state_";
            result += std::to_string(i);
            result += ":\n";
            int break_to = -1;
            for (auto &item : state_machine[i]->transitions) {
                if (item.begin == -1) {
                    break_to = item.index;
                    continue;
                }
                result += ("    chr = getchar();\n");
                result += ("    if (chr >= ");
                result += std::to_string(item.begin);
                result += (" && chr <= ");
                result += std::to_string(item.end);
                result += ("){\n");
                result += ("        goto state_");
                result += std::to_string(item.index);
                result += (";\n    }\n");
            }
            if (break_to != -1) {
                result += "    goto state_";
                result += std::to_string(break_to);
                result += ";\n";
            } else {
                result += "    return ";
                result += std::to_string(state_machine[i]->symbol);
                result += ";\n";

            }
        }
        result.append("}");
        return std::move(result);
    }

}

#endif //ALEX_LIBS_REGEX_H