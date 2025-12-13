#ifndef PASCAL_INTERPRETER_HPP
#define PASCAL_INTERPRETER_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <cctype>
#include <stdexcept>
#include <algorithm>

namespace Pascal {

    enum class TokenType {
        INTEGER, PLUS, MINUS, MUL, DIV, LPAREN, RPAREN,
        BEGIN, END, DOT, ASSIGN, SEMI, ID, END_OF_FILE
    };

    struct Token {
        TokenType type;
        std::string value;
    };

    class Lexer {
    public:
        explicit Lexer(const std::string& text) : text_(text), pos_(0) {
            current_char_ = text_.empty() ? '\0' : text_[0];
        }

        Token get_next_token() {
            while (current_char_ != '\0') {
                if (std::isspace(current_char_)) {
                    skip_whitespace();
                    continue;
                }
                if (std::isdigit(current_char_)) {
                    return {TokenType::INTEGER, number()};
                }
                if (std::isalpha(current_char_)) {
                    return id();
                }
                
                if (current_char_ == ':') {
                    advance();
                    skip_whitespace();
                    if (current_char_ == '=') {
                        advance();
                        return {TokenType::ASSIGN, ":="};
                    }
                    throw std::runtime_error("Syntax Error: expected '=' after ':'");
                }

                if (current_char_ == ';') { advance(); return {TokenType::SEMI, ";"}; }
                if (current_char_ == '+') { advance(); return {TokenType::PLUS, "+"}; }
                if (current_char_ == '-') { advance(); return {TokenType::MINUS, "-"}; }
                if (current_char_ == '*') { advance(); return {TokenType::MUL, "*"}; }
                if (current_char_ == '/') { advance(); return {TokenType::DIV, "/"}; }
                if (current_char_ == '(') { advance(); return {TokenType::LPAREN, "("}; }
                if (current_char_ == ')') { advance(); return {TokenType::RPAREN, ")"}; }
                if (current_char_ == '.') { advance(); return {TokenType::DOT, "."}; }

                throw std::runtime_error("Unknown character: " + std::string(1, current_char_));
            }
            return {TokenType::END_OF_FILE, ""};
        }

    private:
        std::string text_;
        size_t pos_;
        char current_char_;

        void advance() {
            pos_++;
            if (pos_ >= text_.length()) {
                current_char_ = '\0';
            } else {
                current_char_ = text_[pos_];
            }
        }

        void skip_whitespace() {
            while (current_char_ != '\0' && std::isspace(current_char_)) {
                advance();
            }
        }

        std::string number() {
            std::string result;
            while (current_char_ != '\0' && std::isdigit(current_char_)) {
                result += current_char_;
                advance();
            }
            return result;
        }

        Token id() {
            std::string result;
            while (current_char_ != '\0' && std::isalnum(current_char_)) {
                result += current_char_;
                advance();
            }
            std::string upper = result;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper == "BEGIN") return {TokenType::BEGIN, result};
            if (upper == "END") return {TokenType::END, result};
            return {TokenType::ID, result};
        }
    };

    
    class AST { public: virtual ~AST() = default; };

    class BinOp : public AST {
    public:
        std::unique_ptr<AST> left;
        Token op;
        std::unique_ptr<AST> right;
        BinOp(std::unique_ptr<AST> l, Token o, std::unique_ptr<AST> r) : left(std::move(l)), op(o), right(std::move(r)) {}
    };

    class UnaryOp : public AST {
    public:
        Token op;
        std::unique_ptr<AST> expr;
        UnaryOp(Token o, std::unique_ptr<AST> e) : op(o), expr(std::move(e)) {}
    };

    class Num : public AST {
    public:
        Token token;
        int value;
        explicit Num(Token t) : token(t), value(std::stoi(t.value)) {}
    };

    class Var : public AST {
    public:
        Token token;
        std::string value;
        explicit Var(Token t) : token(t), value(t.value) {}
    };

    class Assign : public AST {
    public:
        std::unique_ptr<AST> left;
        Token op;
        std::unique_ptr<AST> right;
        Assign(std::unique_ptr<AST> l, Token o, std::unique_ptr<AST> r) : left(std::move(l)), op(o), right(std::move(r)) {}
    };

    class Compound : public AST { public: std::vector<std::unique_ptr<AST>> children; };
    class NoOp : public AST {};

    
    class Parser {
    public:
        explicit Parser(Lexer& lexer) : lexer_(lexer) { current_token_ = lexer_.get_next_token(); }

        std::unique_ptr<AST> parse() {
            auto node = program();
            if (current_token_.type != TokenType::END_OF_FILE) {
                throw std::runtime_error("Unexpected token after end");
            }
            return node;
        }

    private:
        Lexer& lexer_;
        Token current_token_;

        void eat(TokenType type) {
            if (current_token_.type == type) current_token_ = lexer_.get_next_token();
            else throw std::runtime_error("Invalid syntax");
        }

        std::unique_ptr<AST> factor() {
            Token token = current_token_;
            if (token.type == TokenType::PLUS) { eat(TokenType::PLUS); return std::make_unique<UnaryOp>(token, factor()); }
            if (token.type == TokenType::MINUS) { eat(TokenType::MINUS); return std::make_unique<UnaryOp>(token, factor()); }
            if (token.type == TokenType::INTEGER) { eat(TokenType::INTEGER); return std::make_unique<Num>(token); }
            if (token.type == TokenType::LPAREN) { eat(TokenType::LPAREN); auto node = expr(); eat(TokenType::RPAREN); return node; }
            if (token.type == TokenType::ID) return variable();
            throw std::runtime_error("Unexpected token in factor");
        }

        std::unique_ptr<AST> term() {
            auto node = factor();
            while (current_token_.type == TokenType::MUL || current_token_.type == TokenType::DIV) {
                Token token = current_token_;
                eat(token.type);
                node = std::make_unique<BinOp>(std::move(node), token, factor());
            }
            return node;
        }

        std::unique_ptr<AST> expr() {
            auto node = term();
            while (current_token_.type == TokenType::PLUS || current_token_.type == TokenType::MINUS) {
                Token token = current_token_;
                eat(token.type);
                node = std::make_unique<BinOp>(std::move(node), token, term());
            }
            return node;
        }

        std::unique_ptr<AST> variable() {
            Token token = current_token_;
            eat(TokenType::ID);
            return std::make_unique<Var>(token);
        }

        std::unique_ptr<AST> assignment() {
            auto left = variable();
            Token token = current_token_;
            eat(TokenType::ASSIGN);
            return std::make_unique<Assign>(std::move(left), token, expr());
        }

        std::unique_ptr<AST> statement() {
            if (current_token_.type == TokenType::BEGIN) return compound_statement();
            if (current_token_.type == TokenType::ID) return assignment();
            return std::make_unique<NoOp>();
        }

        std::vector<std::unique_ptr<AST>> statement_list() {
            std::vector<std::unique_ptr<AST>> results;
            results.push_back(statement());
            while (current_token_.type == TokenType::SEMI) {
                eat(TokenType::SEMI);
                results.push_back(statement());
            }
            return results;
        }

        std::unique_ptr<AST> compound_statement() {
            eat(TokenType::BEGIN);
            auto nodes = statement_list();
            eat(TokenType::END);
            auto root = std::make_unique<Compound>();
            root->children = std::move(nodes);
            return root;
        }

        std::unique_ptr<AST> program() {
            auto node = compound_statement();
            eat(TokenType::DOT);
            return node;
        }
    };

    
    class Interpreter {
    public:
        Interpreter() = default;

        std::map<std::string, int> interpret(const std::string& code) {
            variables_.clear();
            Lexer lexer(code);
            Parser parser(lexer);
            auto tree = parser.parse();
            visit(tree.get());
            return variables_;
        }

        
        std::map<std::string, int> variables_;

        int visit(AST* node) {
            if (auto n = dynamic_cast<BinOp*>(node)) return visit_binop(n);
            if (auto n = dynamic_cast<UnaryOp*>(node)) return visit_unaryop(n);
            if (auto n = dynamic_cast<Num*>(node)) return visit_num(n);
            if (auto n = dynamic_cast<Compound*>(node)) return visit_compound(n);
            if (auto n = dynamic_cast<Assign*>(node)) return visit_assign(n);
            if (auto n = dynamic_cast<Var*>(node)) return visit_var(n);
            if (auto n = dynamic_cast<NoOp*>(node)) return visit_noop(n);
            throw std::runtime_error("Unknown AST node");
        }

        int visit_binop(BinOp* node) {
            int left = visit(node->left.get());
            int right = visit(node->right.get());
            if (node->op.type == TokenType::PLUS) return left + right;
            if (node->op.type == TokenType::MINUS) return left - right;
            if (node->op.type == TokenType::MUL) return left * right;
            if (node->op.type == TokenType::DIV) return left / right;
            throw std::runtime_error("Unknown operator");
        }

        int visit_unaryop(UnaryOp* node) {
            int val = visit(node->expr.get());
            if (node->op.type == TokenType::PLUS) return +val;
            if (node->op.type == TokenType::MINUS) return -val;
            throw std::runtime_error("Unknown unary operator");
        }

        int visit_num(Num* node) { return node->value; }
        
        int visit_compound(Compound* node) {
            for (const auto& child : node->children) visit(child.get());
            return 0;
        }

        int visit_assign(Assign* node) {
            std::string var_name = dynamic_cast<Var*>(node->left.get())->value;
            int val = visit(node->right.get());
            variables_[var_name] = val;
            return val;
        }

        int visit_var(Var* node) {
            std::string name = node->value;
            if (variables_.find(name) != variables_.end()) return variables_[name];
            throw std::runtime_error("Variable not found: " + name);
        }

        int visit_noop(NoOp*) { return 0; }
    };
}

#endif