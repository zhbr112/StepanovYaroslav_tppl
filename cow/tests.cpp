#include <gtest/gtest.h>
#include <sstream>
#include "CowInterpreter.h"

class CowTest : public ::testing::Test {
protected:
    std::stringstream ss_in;
    std::stringstream ss_out;
    CowInterpreter* cow;

    void SetUp() override {
        cow = new CowInterpreter(ss_in, ss_out);
    }

    void TearDown() override {
        delete cow;
    }
};

// 1. Тест базовой арифметики (MoO = +1, MOo = -1)
TEST_F(CowTest, BasicArithmetic) {
    // +1, +1, -1 = 1
    cow->run("MoO MoO MOo");
    EXPECT_EQ(cow->getValueAt(0), 1);
}

// 2. Тест перемещения указателя (moO ->, mOo <-)
TEST_F(CowTest, PointerMovement) {
    // inc, right, inc, inc, left
    cow->run("MoO moO MoO MoO mOo");
    
    EXPECT_EQ(cow->getPointer(), 0);
    EXPECT_EQ(cow->getValueAt(0), 1);
    EXPECT_EQ(cow->getValueAt(1), 2);
}

// 3. Тест очистки ячейки (OOO)
TEST_F(CowTest, ZeroInstruction) {
    cow->run("MoO MoO OOO");
    EXPECT_EQ(cow->getValueAt(0), 0);
}

// 4. Тест работы с регистром (MMM)
TEST_F(CowTest, RegisterLogic) {
    // Установим 5
    std::string make5 = "MoO MoO MoO MoO MoO";
    
    // 1. Копируем 5 в регистр. Ячейка 5. Регистр 5.
    // 2. Обнуляем ячейку. Ячейка 0.
    // 3. Копируем из регистра. Ячейка 5. Регистр пуст.
    cow->run(make5 + " MMM OOO MMM");
    
    EXPECT_EQ(cow->getValueAt(0), 5);
    EXPECT_FALSE(cow->isRegisterLoaded());
}

// 5. Тест ввода/вывода чисел (oom, OOM)
TEST_F(CowTest, InputOutputInt) {
    // oom читает ЦЕЛОЕ ЧИСЛО, а не char.
    // Поэтому подаем "65", а не "A".
    ss_in << "65"; 
    
    // Читаем int (65), печатаем int (65)
    cow->run("oom OOM");
    
    EXPECT_EQ(cow->getValueAt(0), 65);
    EXPECT_EQ(ss_out.str(), "65");
}

// 6. Тест условного оператора Moo (Char I/O)
TEST_F(CowTest, ConditionalMoo_Input) {
    // Если значение 0 -> Moo читает char
    ss_in << "X";
    cow->run("Moo"); 
    EXPECT_EQ(cow->getValueAt(0), 88);
}

TEST_F(CowTest, ConditionalMoo_Output) {
    // Если значение !0 -> Moo выводит char
    // Используем oom, чтобы записать число 89 ('Y')
    ss_in << "89"; 
    
    // oom считает 89. Moo увидит не-ноль и выведет char(89) -> 'Y'
    cow->run("oom Moo");
    
    EXPECT_EQ(ss_out.str(), "Y");
}

// 7. Тест циклов (MOO ... moo)
TEST_F(CowTest, LoopExecution) {
    cow->run("MoO MoO MoO MOO MOo moo");
    EXPECT_EQ(cow->getValueAt(0), 0);
}

// 8. Тест пропуска цикла
TEST_F(CowTest, LoopSkip) {
    cow->run("MOO MoO moo");
    EXPECT_EQ(cow->getValueAt(0), 0);
}

// 9. Тест косвенного выполнения (mOO)
TEST_F(CowTest, IndirectExecution) {
    std::string set6 = "MoO MoO MoO MoO MoO MoO";
    cow->run(set6 + " mOO");
    
    EXPECT_EQ(cow->getValueAt(0), 7);
}

// Тест "Hello, World!"
TEST_F(CowTest, HelloWorld) {
    std::string source = 
        "MoO MoO MoO MoO MoO MoO MoO MoO MOO moO MoO MoO MoO MoO MoO moO MoO MoO MoO MoO moO MoO MoO MoO MoO moO MoO MoO MoO MoO MoO MoO MoO "
        "MoO MoO moO MoO MoO MoO MoO mOo mOo mOo mOo mOo MOo moo moO moO moO moO Moo moO MOO mOo MoO moO MOo moo mOo MOo MOo MOo Moo MoO MoO "
        "MoO MoO MoO MoO MoO Moo Moo MoO MoO MoO Moo MMM mOo mOo mOo MoO MoO MoO MoO Moo moO Moo MOO moO moO MOo mOo mOo MOo moo moO moO MoO "
        "MoO MoO MoO MoO MoO MoO MoO Moo MMM MMM Moo MoO MoO MoO Moo MMM MOo MOo MOo Moo MOo MOo MOo MOo MOo MOo MOo MOo Moo mOo MoO Moo";
    
    cow->run(source);
    
    EXPECT_EQ(ss_out.str(), "Hello, World!");
}

// Тест "Фибоначи"
TEST_F(CowTest, Fibonaci) {
    std::string source = R"(
MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO 
MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO 
                                            c1v44 : ASCII code of comma
moO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO 
MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO
                                            c2v32 : ASCII code of space
moO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO MoO
                                            c3v11 : quantity of numbers to be calculated
moO                                         c4v0  : zeroth Fibonacci number (will not be printed)
moO MoO                                     c5v1  : first Fibonacci number
mOo mOo                                     c3    : loop counter
MOO                                         block : loop to print (i)th number and calculate next one
moO moO OOM                                 c5    : the number to be printed
mOo mOo mOo mOo Moo moO Moo                 c1c2  : print comma and space
                                            block : actually calculate next Fibonacci in c6
moO moO MOO moO moO MoO mOo mOo MOo moo     c4v0  : move c4 to c6 (don't need to preserve it)
moO MOO moO MoO mOo mOo MoO moO MOo moo     c5v0  : move c5 to c6 and c4 (need to preserve it)
moO MOO mOo MoO moO MOo moo                 c6v0  : move c6 with sum to c5
mOo mOo mOo MOo                             c3    : decrement loop counter
moo 
mOo mOo MoO MoO Moo Moo Moo                 c1    : output three dots
)";

    cow->run(source);

    std::string expected = "1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, ...";
    EXPECT_EQ(ss_out.str(), expected);
}

// ==========================================
// Негативные тесты (Exceptions)
// ==========================================

TEST_F(CowTest, ErrorUnderflow) {
    EXPECT_THROW(cow->run("mOo"), std::runtime_error);
}

// moo - это конец цикла (End Loop). Если встретили без начала - ошибка.
TEST_F(CowTest, ErrorUnmatchedEnd) {
    EXPECT_THROW(cow->run("moo"), std::runtime_error);
}

// MOO - это начало цикла (Start Loop). Если не закрыли - ошибка.
TEST_F(CowTest, ErrorUnmatchedStart) {
    EXPECT_THROW(cow->run("MOO"), std::runtime_error);
}

TEST_F(CowTest, ErrorLoopInIndirect) {
    // 0 - это код moo (End Loop). Нельзя выполнять переходы через mOO
    EXPECT_THROW(cow->run("mOO"), std::runtime_error);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
