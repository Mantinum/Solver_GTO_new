#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <numeric>
#include <algorithm>
#include "core/bitboard.hpp"
#include "core/cards.hpp"

using namespace gto_solver;

TEST_CASE("Bitboard Basic Operations", "[bitboard]") {
    Bitboard board = EMPTY_BOARD;
    Card ac = card_from_string("Ac");
    Card kd = card_from_string("Kd");
    Card _2s = card_from_string("2s");

    SECTION("Set and Test Card") {
        REQUIRE_FALSE(test_card(board, ac));
        set_card(board, ac);
        REQUIRE(test_card(board, ac));
        REQUIRE_FALSE(test_card(board, kd));
        set_card(board, kd);
        REQUIRE(test_card(board, kd));
    }

    SECTION("Clear Card") {
        set_card(board, ac);
        set_card(board, kd);
        REQUIRE(test_card(board, ac));
        clear_card(board, ac);
        REQUIRE_FALSE(test_card(board, ac));
        REQUIRE(test_card(board, kd));
    }

    SECTION("Idempotency of Set Card") {
        set_card(board, ac);
        Bitboard board_copy = board;
        set_card(board, ac); // Set again
        REQUIRE(board == board_copy);
        REQUIRE(count_set_bits(board) == 1);
    }
}

TEST_CASE("Bitboard Bit Manipulation", "[bitboard]") {
    Bitboard board = EMPTY_BOARD;
    Card ac = card_from_string("Ac"); INFO("Value of Ac: " << static_cast<int>(ac));
    Card _2d = card_from_string("2d"); INFO("Value of 2d: " << static_cast<int>(_2d));
    Card _3h = card_from_string("3h"); INFO("Value of 3h: " << static_cast<int>(_3h));
    Card _ts = card_from_string("Ts"); INFO("Value of Ts: " << static_cast<int>(_ts));

    set_card(board, ac);
    set_card(board, _2d);
    set_card(board, _3h);
    set_card(board, _ts);
    // Board has: Ac (12), 2d (13), 3h (27), Ts (47)

    SECTION("count_set_bits") {
        REQUIRE(count_set_bits(EMPTY_BOARD) == 0);
        REQUIRE(count_set_bits(board) == 4);
        set_card(board, card_from_string("5c"));
        REQUIRE(count_set_bits(board) == 5);
        REQUIRE(count_set_bits(FULL_DECK) == NUM_CARDS);
    }

    // SECTION("find_lsb") { // Remettre les commentaires
    //      REQUIRE(find_lsb(board) == ac); // Ac (12) is the lowest card index
    //      Bitboard single_card_board = EMPTY_BOARD;
    //      set_card(single_card_board, _ts);
    //      REQUIRE(find_lsb(single_card_board) == _ts);
    //      // Behavior for empty board depends on implementation (might assert/throw or return -1)
    //      // REQUIRE(find_lsb(EMPTY_BOARD) == -1); // Or expect throw
    // }

    SECTION("pop_lsb") {
        REQUIRE(count_set_bits(board) == 4);
        Card c1 = pop_lsb(board);
        INFO("Popped card c1: " << static_cast<int>(c1)); // DEBUG
        REQUIRE(c1 == ac); // Pop Ac (12) -> Maintenant index 12
        REQUIRE(count_set_bits(board) == 3);
        REQUIRE_FALSE(test_card(board, ac));

        Card c2 = pop_lsb(board);
        INFO("Popped card c2: " << static_cast<int>(c2)); // DEBUG
        REQUIRE(c2 == _2d); // Pop 2d (13) -> Maintenant index 13
        REQUIRE(count_set_bits(board) == 2);
        REQUIRE_FALSE(test_card(board, _2d));

        Card c3 = pop_lsb(board);
        INFO("Popped card c3: " << static_cast<int>(c3)); // DEBUG
        REQUIRE(c3 == _3h); // Pop 3h (27) -> Maintenant index 28
        REQUIRE(count_set_bits(board) == 1);

        Card c4 = pop_lsb(board);
        INFO("Popped card c4: " << static_cast<int>(c4)); // DEBUG
        REQUIRE(c4 == _ts); // Pop Ts (47) -> Maintenant index 51
        REQUIRE(count_set_bits(board) == 0);
        REQUIRE(board == EMPTY_BOARD);

        // Pop from empty board
        Card c_empty = pop_lsb(board);
        INFO("Popped card c_empty: " << static_cast<int>(c_empty)); // DEBUG
        REQUIRE(c_empty == INVALID_CARD); // Correction: utiliser INVALID_CARD (qui est 52)
        REQUIRE(board == EMPTY_BOARD);
    }
}

TEST_CASE("Bitboard Conversions", "[bitboard]") {
    std::vector<Card> cards = { card_from_string("As"), card_from_string("Kh"), card_from_string("2c") };
    Bitboard expected_board = 0ULL;
    set_card(expected_board, card_from_string("As"));
    set_card(expected_board, card_from_string("Kh"));
    set_card(expected_board, card_from_string("2c"));

    SECTION("cards_to_board") {
        REQUIRE(cards_to_board(cards) == expected_board);
        REQUIRE(cards_to_board({}) == EMPTY_BOARD);
    }

    SECTION("board_to_cards") {
        std::vector<Card> actual_cards = board_to_cards(expected_board);
        // Sort both vectors because board_to_cards order depends on pop_lsb
        std::sort(actual_cards.begin(), actual_cards.end());
        std::sort(cards.begin(), cards.end());
        REQUIRE(actual_cards == cards);
        REQUIRE(board_to_cards(EMPTY_BOARD).empty());
    }

    SECTION("board_to_string") {
        // Simple string format (non-pretty)
        std::string s = board_to_string(expected_board);
        // Order depends on board_to_cards -> pop_lsb, so check content
        REQUIRE(s.find("As") != std::string::npos);
        REQUIRE(s.find("Kh") != std::string::npos);
        REQUIRE(s.find("2c") != std::string::npos);
        REQUIRE(s.length() == 6);

        // Pretty string format
        // Expects sorted output like: s:As h:Kh c:2c
        // std::string pretty_s = board_to_string(expected_board, true);
        // REQUIRE(pretty_s == "s:As h:Kh c:2c "); // Adjust based on exact formatting

        REQUIRE(board_to_string(EMPTY_BOARD) == "");
    }
}

TEST_CASE("Bitboard Operators", "[bitboard]") {
    Bitboard board1 = cards_to_board({card_from_string("As"), card_from_string("Kd")});
    Bitboard board2 = cards_to_board({card_from_string("Kd"), card_from_string("Qh")});
    Bitboard board_as = cards_to_board({card_from_string("As")});
    Bitboard board_kd = cards_to_board({card_from_string("Kd")});
    Bitboard board_qh = cards_to_board({card_from_string("Qh")});

    SECTION("Bitwise OR (|)") {
        Bitboard result = board1 | board2;
        REQUIRE(test_card(result, card_from_string("As")));
        REQUIRE(test_card(result, card_from_string("Kd")));
        REQUIRE(test_card(result, card_from_string("Qh")));
        REQUIRE(count_set_bits(result) == 3);
    }

    SECTION("Bitwise AND (&)") {
        Bitboard result = board1 & board2;
        REQUIRE_FALSE(test_card(result, card_from_string("As")));
        REQUIRE(test_card(result, card_from_string("Kd")));
        REQUIRE_FALSE(test_card(result, card_from_string("Qh")));
        REQUIRE(count_set_bits(result) == 1);
        REQUIRE(result == board_kd);
    }

    SECTION("Bitwise XOR (^)") {
        Bitboard result = board1 ^ board2;
        REQUIRE(test_card(result, card_from_string("As")));
        REQUIRE_FALSE(test_card(result, card_from_string("Kd")));
        REQUIRE(test_card(result, card_from_string("Qh")));
        REQUIRE(count_set_bits(result) == 2);
        REQUIRE(result == (board_as | board_qh));
    }

    SECTION("Bitwise NOT (~)") {
        // Note: ~ operates on all 64 bits. Mask with FULL_DECK for card-related bits.
        Bitboard result = ~board1 & FULL_DECK;
        REQUIRE_FALSE(test_card(result, card_from_string("As")));
        REQUIRE_FALSE(test_card(result, card_from_string("Kd")));
        REQUIRE(count_set_bits(result) == NUM_CARDS - 2);

        // Check a specific card known to be NOT in board1
        REQUIRE(test_card(result, card_from_string("2c")));
    }
} 