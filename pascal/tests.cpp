#include <gtest/gtest.h>
#include <string>
#include <memory>
#include "interpreter.hpp"

using namespace Pascal;

// Стандартные примеры

TEST(InterpreterTest, Example1_Empty) {
    Interpreter interp;
    auto result = interp.interpret("BEGIN END.");
    EXPECT_TRUE(result.empty());
}

TEST(InterpreterTest, Example2_Math) {
    std::string code = 
        "BEGIN\n"
        "   x:= 2 + 3 * (2 + 3);\n"
        "   y:= 2 / 2 - 2 + 3 * ((1 + 1) + (1 + 1));\n"
        "END.";
    Interpreter interp;
    auto result = interp.interpret(code);
    EXPECT_EQ(result.at("x"), 17);
    EXPECT_EQ(result.at("y"), 11);
}

TEST(InterpreterTest, Example3_ScopeAndColon) {
    std::string code = 
        "BEGIN\n"
        "    y: = 2;\n"
        "    BEGIN\n"
        "        a := 3;\n"
        "        a := a;\n"
        "        b := 10 + a + 10 * y / 4;\n"
        "        c := a - b\n"
        "    END;\n"
        "    x := 11;\n"
        "END.";
    Interpreter interp;
    auto result = interp.interpret(code);
    EXPECT_EQ(result.at("y"), 2);
    EXPECT_EQ(result.at("b"), 18);
    EXPECT_EQ(result.at("c"), -15);
    EXPECT_EQ(result.at("x"), 11);
}

// Тесты на Лексер

TEST(LexerTest, UnknownCharacter) {
    Interpreter interp;
    EXPECT_THROW(interp.interpret("BEGIN @ END."), std::runtime_error);
}

TEST(LexerTest, AssignmentSyntaxError) {
    Interpreter interp;
    EXPECT_THROW(interp.interpret("BEGIN x : 10; END."), std::runtime_error);
}

TEST(LexerTest, PeekColonBoundary) {
    Lexer l(":");
    EXPECT_THROW(l.get_next_token(), std::runtime_error);
}

// Тесты на Парсер

TEST(ParserTest, GarbageAfterDot) {
    Interpreter interp;
    EXPECT_THROW(interp.interpret("BEGIN END. garbage"), std::runtime_error);
}

TEST(ParserTest, MismatchedParens) {
    Interpreter interp;
    EXPECT_THROW(interp.interpret("BEGIN x := 1 )"), std::runtime_error);
}

TEST(ParserTest, UnexpectedFactor) {
    Interpreter interp;
    EXPECT_THROW(interp.interpret("BEGIN x := * 2; END."), std::runtime_error);
}

// есты на Интерпретатор

TEST(InterpreterTest, UninitializedVariable) {
    Interpreter interp;
    EXPECT_THROW(interp.interpret("BEGIN x := y + 1; END."), std::runtime_error);
}

TEST(InterpreterTest, UnknownBinOp) {
    Interpreter interp;
    auto left = std::make_unique<Num>(Token{TokenType::INTEGER, "1"});
    auto right = std::make_unique<Num>(Token{TokenType::INTEGER, "1"});
    Token badOp{TokenType::DOT, "."};
    
    auto node = std::make_unique<BinOp>(std::move(left), badOp, std::move(right));
    
    try {
        interp.visit(node.get());
        FAIL() << "Expected std::runtime_error";
    } catch(const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), "Unknown operator");
    }
}

TEST(InterpreterTest, UnknownUnaryOp) {
    Interpreter interp;
    auto expr = std::make_unique<Num>(Token{TokenType::INTEGER, "1"});
    Token badOp{TokenType::MUL, "*"};
    
    auto node = std::make_unique<UnaryOp>(badOp, std::move(expr));
    
    try {
        interp.visit(node.get());
        FAIL() << "Expected std::runtime_error";
    } catch(const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), "Unknown unary operator");
    }
}

TEST(InterpreterTest, UnknownASTNode) {
    class UnknownNode : public AST {};
    Interpreter interp;
    UnknownNode node;
    
    try {
        interp.visit(&node);
        FAIL() << "Expected std::runtime_error";
    } catch(const std::runtime_error& e) {
        EXPECT_EQ(std::string(e.what()), "Unknown AST node");
    }
}

// Тесты жизненного цикла AST

TEST(ASTLifeCycle, Destructors) {
    { std::unique_ptr<AST> node = std::make_unique<NoOp>(); }
    { std::unique_ptr<AST> node = std::make_unique<Num>(Token{TokenType::INTEGER, "1"}); }
    { std::unique_ptr<AST> node = std::make_unique<Var>(Token{TokenType::ID, "x"}); }
}