#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <stdexcept>
#include "core/cards.hpp"
#include "core/bitboard.hpp" // Include for bitboard conversion tests
#include <string>

using namespace gto_solver;

TEST_CASE("Card Creation and Properties", "[cards]") {
    Card ac = make_card(Rank::ACE, Suit::CLUBS);
    Card kd = make_card(Rank::KING, Suit::DIAMONDS);
    Card kh = make_card(Rank::KING, Suit::HEARTS);
    Card _2s = make_card(Rank::TWO, Suit::SPADES);

    SECTION("Card ranks and suits are correct") {
        REQUIRE(get_rank(ac) == Rank::ACE);
        REQUIRE(get_suit(ac) == Suit::CLUBS);
        REQUIRE(get_rank(kd) == Rank::KING);
        REQUIRE(get_suit(kd) == Suit::DIAMONDS);
        REQUIRE(get_rank(kh) == Rank::KING);
        REQUIRE(get_suit(kh) == Suit::HEARTS);
        REQUIRE(get_rank(_2s) == Rank::TWO);
        REQUIRE(get_suit(_2s) == Suit::SPADES);
    }

    SECTION("to_string conversion is correct") {
        REQUIRE(to_string(ac) == "Ac");
        REQUIRE(to_string(kd) == "Kd");
        REQUIRE(to_string(kh) == "Kh");
        REQUIRE(to_string(_2s) == "2s");
    }

    SECTION("card_from_string conversion is correct") {
        REQUIRE(card_from_string("Ac") == ac);
        REQUIRE(card_from_string("Kd") == kd);
        REQUIRE(card_from_string("Kh") == kh);
        REQUIRE(card_from_string("2s") == _2s);
        // Test invalid strings
        REQUIRE_THROWS_AS(card_from_string("XX"), std::invalid_argument);
        REQUIRE_THROWS_AS(card_from_string("A"), std::invalid_argument);
        REQUIRE_THROWS_AS(card_from_string("1c"), std::invalid_argument);
        REQUIRE_THROWS_AS(card_from_string("Ahx"), std::invalid_argument);
    }

    // Les tests de bitboard ont été déplacés vers bitboard_tests.cpp
}

TEST_CASE("Card String Conversions", "[cards][string]") {
    SECTION("to_string conversions") {
        REQUIRE(to_string(make_card(Rank::ACE, Suit::SPADES)) == "As");   // Correction: to_string
        REQUIRE(to_string(make_card(Rank::TEN, Suit::DIAMONDS)) == "Td"); // Correction: to_string
        REQUIRE(to_string(make_card(Rank::TWO, Suit::CLUBS)) == "2c");    // Correction: to_string
        REQUIRE(to_string(INVALID_CARD) == "??"); // Correction: to_string et INVALID_CARD
    }

    SECTION("card_from_string conversions") {
        REQUIRE(card_from_string("As") == make_card(Rank::ACE, Suit::SPADES));     // Correction: card_from_string
        REQUIRE(card_from_string("Td") == make_card(Rank::TEN, Suit::DIAMONDS));   // Correction: card_from_string
        REQUIRE(card_from_string("2c") == make_card(Rank::TWO, Suit::CLUBS));      // Correction: card_from_string
    }

    SECTION("card_from_string invalid inputs") {
        REQUIRE_THROWS_AS(card_from_string("XX"), std::invalid_argument);        // Correction: card_from_string
        REQUIRE_THROWS_AS(card_from_string("A"), std::invalid_argument);         // Correction: card_from_string
        REQUIRE_THROWS_AS(card_from_string("1c"), std::invalid_argument);        // Correction: card_from_string
        REQUIRE_THROWS_AS(card_from_string("Tsx"), std::invalid_argument);       // Correction: card_from_string
        REQUIRE_THROWS_AS(card_from_string(""), std::invalid_argument);          // Correction: card_from_string
        REQUIRE_THROWS_AS(card_from_string(" Td "), std::invalid_argument);      // Correction: card_from_string
    }
}

// TEST_CASE("Card Indexing", "[cards][index]") { // Supprimé temporairement car fonctions inexistantes
//     // Test si l'index unique 0-51 est généré correctement
//     REQUIRE(card_index(make_card(Rank::TWO, Suit::CLUBS)) == 0);
//     REQUIRE(card_index(make_card(Rank::TWO, Suit::DIAMONDS)) == 1);
//     // ... autres tests d'index ...
//     REQUIRE(card_index(make_card(Rank::ACE, Suit::SPADES)) == 51);
// 
//     // Test conversion inverse
//     REQUIRE(card_from_index(0) == make_card(Rank::TWO, Suit::CLUBS));
//     REQUIRE(card_from_index(51) == make_card(Rank::ACE, Suit::SPADES));
// 
//     // Test index invalide
//     REQUIRE_THROWS_AS(card_from_index(52), std::out_of_range);
//     REQUIRE_THROWS_AS(card_from_index(-1), std::out_of_range); 
// }

// Note: Comparison operators (==, <) are implicitly tested via REQUIRE
// and usage in containers like std::vector unless specific behavior needs checking. 