// test_bnf_instruction_lexer.cpp
#include <iostream>
#include <functional>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include "camahjucunu/BNF/BNF_instruction_lexer.h"

int count_total = 0;
int count_success = 0;
int count_failure = 0;

void assertEqual(char actual, char expected, const std::string& errorMessage) {
    if (actual != expected) {
        throw std::runtime_error(errorMessage + ": Expected '" + expected + "', got '" + actual + "'");
    }
}

void assertEqual(size_t actual, size_t expected, const std::string& errorMessage) {
    if (actual != expected) {
        throw std::runtime_error(errorMessage + ": Expected '" + std::to_string(expected) + "', got '" + std::to_string(actual) + "'");
    }
}

void assertTrue(bool condition, const std::string& errorMessage) {
    if (!condition) {
        throw std::runtime_error(errorMessage);
    }
}

void runTest(const std::string& testName, const std::function<void()>& testFunc) {
    try {
        count_total++;
        testFunc();
        std::cout << "[Success] " << testName << "\n";
        count_success++;
    } catch (const std::exception& ex) {
        std::cout << "[Failure] " << testName << ": " << ex.what() << "\n";
        count_failure++;
    } catch (...) {
        std::cout << "[Failure] " << testName << ": Unknown exception\n";
        count_failure++;
    }
}

void test_InstructionLexer() {
    // Test 1: Initialization with input
    runTest("Test 1: Initialization with input", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("Hello");
        assertTrue(lexer.getInput() == "Hello", "Input does not match");
        assertEqual(lexer.getPosition(), 0, "Position should be 0 after initialization");
        assertTrue(!lexer.isAtEnd(), "Lexer should not be at end after initialization");
    });

    // Test 2: peek() and advance()
    runTest("Test 2: peek() and advance()", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("abc");
        assertEqual(lexer.peek(), 'a', "peek() at position 0 failed");
        assertEqual(lexer.advance(), 'a', "advance() at position 0 failed");
        assertEqual(lexer.peek(), 'b', "peek() at position 1 failed");
        assertEqual(lexer.advance(), 'b', "advance() at position 1 failed");
        assertEqual(lexer.peek(), 'c', "peek() at position 2 failed");
        assertEqual(lexer.advance(), 'c', "advance() at position 2 failed");
        assertTrue(lexer.isAtEnd(), "Lexer should be at end after consuming all characters");
        assertEqual(lexer.peek(), '\0', "peek() should return '\\0' at end");
        assertEqual(lexer.advance(), '\0', "advance() should return '\\0' at end");
    });

    // Test 3: reset()
    runTest("Test 3: reset()", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("test");
        lexer.advance();
        lexer.advance();
        assertEqual(lexer.getPosition(), 2, "Position should be 2 after advancing twice");
        lexer.reset();
        assertEqual(lexer.getPosition(), 0, "Position should be reset to 0");
        assertEqual(lexer.peek(), 't', "peek() after reset failed");
    });

    // Test 4: setInput()
    runTest("Test 4: setInput()", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("first");
        assertTrue(lexer.getInput() == "first", "Initial input does not match");
        lexer.setInput("second");
        assertTrue(lexer.getInput() == "second", "Input after setInput does not match");
        assertEqual(lexer.getPosition(), 0, "Position should be reset to 0 after setInput");
        assertEqual(lexer.peek(), 's', "peek() after setInput failed");
    });

    // Test 5: getPosition() and setPosition()
    runTest("Test 5: getPosition() and setPosition()", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("position");
        lexer.advance(); // pos = 1
        lexer.advance(); // pos = 2
        assertEqual(lexer.getPosition(), 2, "Position should be 2 after advancing twice");
        lexer.setPosition(5);
        assertEqual(lexer.getPosition(), 5, "Position should be set to 5");
        assertEqual(lexer.peek(), 'i', "peek() after setPosition failed");
        lexer.setPosition(0);
        assertEqual(lexer.peek(), 'p', "peek() after resetting position failed");
    });

    // Test 6: isAtEnd()
    runTest("Test 6: isAtEnd()", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("");
        assertTrue(lexer.isAtEnd(), "Lexer should be at end for empty input");
        assertEqual(lexer.peek(), '\0', "peek() should return '\\0' for empty input");
        assertEqual(lexer.advance(), '\0', "advance() should return '\\0' for empty input");

        cuwacunu::camahjucunu::BNF::InstructionLexer lexer2("a");
        assertTrue(!lexer2.isAtEnd(), "Lexer should not be at end after initialization with non-empty input");
        lexer2.advance(); // pos = 1
        assertTrue(lexer2.isAtEnd(), "Lexer should be at end after consuming all characters");
    });

    // Test 7: advance() beyond end
    runTest("Test 7: advance() beyond end", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("end");
        lexer.advance(); // 'e'
        lexer.advance(); // 'n'
        lexer.advance(); // 'd'
        assertTrue(lexer.isAtEnd(), "Lexer should be at end after consuming all characters");
        assertEqual(lexer.advance(), '\0', "advance() should return '\\0' when at end");
        assertEqual(lexer.advance(), '\0', "advance() should return '\\0' when at end");
    });

    // Test 8: setPosition() beyond input length
    runTest("Test 8: setPosition() beyond input length", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("short");
        lexer.setPosition(10); // Beyond length
        assertTrue(lexer.isAtEnd(), "Lexer should be at end after setting position beyond input length");
        assertEqual(lexer.peek(), '\0', "peek() should return '\\0' when position is beyond input length");
        assertEqual(lexer.advance(), '\0', "advance() should return '\\0' when position is beyond input length");
    });

    // Test 9: advance(size_t delta)
    runTest("Test 9: advance(size_t delta)", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("abcdef");
        lexer.advance(3); // Should advance to 'd'
        assertEqual(lexer.peek(), 'd', "peek() after advance(3) failed");
        assertEqual(lexer.getPosition(), 3, "Position should be 3 after advance(3)");
        lexer.advance(10); // Should reach the end
        assertTrue(lexer.isAtEnd(), "Lexer should be at end after advancing beyond input length");
        assertEqual(lexer.peek(), '\0', "peek() should return '\\0' when at end");
    });

    // Test 10: Empty input
    runTest("Test 10: Empty input", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("");
        assertTrue(lexer.isAtEnd(), "Lexer should be at end for empty input");
        assertEqual(lexer.peek(), '\0', "peek() should return '\\0' for empty input");
        assertEqual(lexer.advance(), '\0', "advance() should return '\\0' for empty input");
    });

    // Test 11: Single-character input
    runTest("Test 11: Single-character input", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer("x");
        assertTrue(!lexer.isAtEnd(), "Lexer should not be at end after initialization");
        assertEqual(lexer.peek(), 'x', "peek() failed for single-character input");
        assertEqual(lexer.advance(), 'x', "advance() failed for single-character input");
        assertTrue(lexer.isAtEnd(), "Lexer should be at end after consuming the character");
    });

    // Test 12: Whitespace input
    runTest("Test 12: Whitespace input", []() {
        cuwacunu::camahjucunu::BNF::InstructionLexer lexer(" \t\n");
        assertEqual(lexer.peek(), ' ', "peek() failed at position 0");
        assertEqual(lexer.advance(), ' ', "advance() failed at position 0");
        assertEqual(lexer.peek(), '\t', "peek() failed at position 1");
        assertEqual(lexer.advance(), '\t', "advance() failed at position 1");
        assertEqual(lexer.peek(), '\n', "peek() failed at position 2");
        assertEqual(lexer.advance(), '\n', "advance() failed at position 2");
        assertTrue(lexer.isAtEnd(), "Lexer should be at end after consuming all whitespace characters");
    });
}

int main() {
    test_InstructionLexer();
    // Summary
    std::cout << "----------------------------------------\n";
    std::cout << "Test Summary: " << count_success << " Passed, " << count_failure << " Failed. Total: " << count_total << "\n";

    return (count_failure == 0) ? 0 : 1;
}
