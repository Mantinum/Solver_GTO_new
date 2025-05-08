#include "gto/information_set.h"
#include "core/cards.hpp" // Pour MAKE_CARD et string_to_card
#include "gto/action_abstraction.h" // Pour Action, ActionType
#include "gto/game_state.h" // Pour Street et street_to_string (indirectement)
#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <string>
#include <array>
#include <algorithm> // pour std::sort dans certains tests si besoin de comparer des vecteurs

using namespace gto_solver;

// Helper pour créer des cartes plus facilement dans les tests
Card C(const std::string& s) { return gto_solver::card_from_string(s); }

TEST_CASE("InformationSet::generate_key Tests", "[InformationSet]") {

    std::array<Card, 5> empty_board = {INVALID_CARD, INVALID_CARD, INVALID_CARD, INVALID_CARD, INVALID_CARD};
    std::array<Card, 5> flop_board = {C("Ah"), C("Kd"), C("Qc"), INVALID_CARD, INVALID_CARD};
    std::array<Card, 5> turn_board = {C("Ah"), C("Kd"), C("Qc"), C("Ts"), INVALID_CARD};
    std::array<Card, 5> river_board = {C("Ah"), C("Kd"), C("Qc"), C("Ts"), C("2h")};

    std::vector<Action> empty_history = {};
    std::vector<Action> history1 = {
        {0, ActionType::RAISE, 6}, // P0 raises to 6
        {1, ActionType::CALL, 6}   // P1 calls 6
    };
    std::vector<Action> history2 = {
        {0, ActionType::CALL, 2},  // P0 calls 2 (limp)
        {1, ActionType::RAISE, 8} // P1 raises to 8
    };
     std::vector<Action> history1_reordered_for_test = { // Ne devrait pas arriver en vrai, mais pour tester la sensibilité à l'ordre
        {1, ActionType::CALL, 6},
        {0, ActionType::RAISE, 6}  
    };


    SECTION("Canonicity of Hole Cards") {
        std::vector<Card> hc1 = {C("As"), C("Kc")};
        std::vector<Card> hc2 = {C("Kc"), C("As")};
        std::string key1 = InformationSet::generate_key(0, hc1, flop_board, 3, Street::FLOP, history1);
        std::string key2 = InformationSet::generate_key(0, hc2, flop_board, 3, Street::FLOP, history1);
        REQUIRE(key1 == key2);

        // Avec une seule carte (ne devrait pas arriver en Holdem mais testons)
        std::vector<Card> hc_single1 = {C("As")};
        std::vector<Card> hc_single2 = {C("As")}; // Pas d'ordre à tester ici
        key1 = InformationSet::generate_key(0, hc_single1, flop_board, 3, Street::FLOP, history1);
        key2 = InformationSet::generate_key(0, hc_single2, flop_board, 3, Street::FLOP, history1);
        REQUIRE(key1 == key2);
    }

    SECTION("Canonicity of Board Cards") {
        std::vector<Card> hc = {C("As"), C("Ks")};
        std::array<Card, 5> board1 = {C("Ah"), C("Kd"), C("Qc"), INVALID_CARD, INVALID_CARD};
        std::array<Card, 5> board2 = {C("Kd"), C("Qc"), C("Ah"), INVALID_CARD, INVALID_CARD}; // Ordre différent
        std::array<Card, 5> board3 = {C("Qc"), C("Ah"), C("Kd"), INVALID_CARD, INVALID_CARD}; // Autre ordre
        
        std::string key1 = InformationSet::generate_key(0, hc, board1, 3, Street::FLOP, history1);
        std::string key2 = InformationSet::generate_key(0, hc, board2, 3, Street::FLOP, history1);
        std::string key3 = InformationSet::generate_key(0, hc, board3, 3, Street::FLOP, history1);
        REQUIRE(key1 == key2);
        REQUIRE(key1 == key3);

        // Turn
        std::array<Card, 5> turn_b1 = {C("Ah"), C("Kd"), C("Qc"), C("Ts"), INVALID_CARD};
        std::array<Card, 5> turn_b2 = {C("Ts"), C("Qc"), C("Kd"), C("Ah"), INVALID_CARD};
        key1 = InformationSet::generate_key(0, hc, turn_b1, 4, Street::TURN, history1);
        key2 = InformationSet::generate_key(0, hc, turn_b2, 4, Street::TURN, history1);
        REQUIRE(key1 == key2);
    }

    SECTION("Differentiation by Player Index") {
        std::vector<Card> hc = {C("Ac"), C("Kc")};
        std::string key_p0 = InformationSet::generate_key(0, hc, flop_board, 3, Street::FLOP, history1);
        std::string key_p1 = InformationSet::generate_key(1, hc, flop_board, 3, Street::FLOP, history1);
        REQUIRE(key_p0 != key_p1);
    }

    SECTION("Differentiation by Street") {
        std::vector<Card> hc = {C("Td"), C("9d")};
        std::string key_preflop = InformationSet::generate_key(0, hc, empty_board, 0, Street::PREFLOP, history1);
        std::string key_flop = InformationSet::generate_key(0, hc, flop_board, 3, Street::FLOP, history1);
        std::string key_turn = InformationSet::generate_key(0, hc, turn_board, 4, Street::TURN, history1);
        std::string key_river = InformationSet::generate_key(0, hc, river_board, 5, Street::RIVER, history1);
        REQUIRE(key_preflop != key_flop);
        REQUIRE(key_flop != key_turn);
        REQUIRE(key_turn != key_river);
    }

    SECTION("Differentiation by Action History") {
        std::vector<Card> hc = {C("7h"), C("8h")};
        std::string key_hist_empty = InformationSet::generate_key(0, hc, flop_board, 3, Street::FLOP, empty_history);
        std::string key_hist1 = InformationSet::generate_key(0, hc, flop_board, 3, Street::FLOP, history1);
        std::string key_hist2 = InformationSet::generate_key(0, hc, flop_board, 3, Street::FLOP, history2);
        
        REQUIRE(key_hist_empty != key_hist1);
        REQUIRE(key_hist1 != key_hist2);

        // Test sensibilité à l'ordre (notre format de clé est sensible à l'ordre)
        std::string key_hist1_reordered = InformationSet::generate_key(0, hc, flop_board, 3, Street::FLOP, history1_reordered_for_test);
        REQUIRE(key_hist1 != key_hist1_reordered); 
    }

    SECTION("Differentiation by Board Cards (same street, same num_dealt)") {
        std::vector<Card> hc = {C("5s"), C("6s")};
        std::array<Card, 5> board_alt_flop = {C("2s"), C("3h"), C("4d"), INVALID_CARD, INVALID_CARD};
        std::string key_flop1 = InformationSet::generate_key(0, hc, flop_board, 3, Street::FLOP, history1);
        std::string key_flop2 = InformationSet::generate_key(0, hc, board_alt_flop, 3, Street::FLOP, history1);
        REQUIRE(key_flop1 != key_flop2);
    }

    SECTION("Empty Board (Preflop)") {
        std::vector<Card> hc = {C("Qh"), C("Js")};
        std::string key = InformationSet::generate_key(1, hc, empty_board, 0, Street::PREFLOP, history1);
        // Pour un board vide (preflop), la section board est vide, mais les délimiteurs restent.
        // Format: P<idx>;<hole_cards>||<street>|<history>
        std::string expected_key = "P1;Qh-Js||Preflop|A0R6,A1C6,"; // Ajout du deuxième pipe pour board vide
        
        // Debugging output -- Peut être supprimé maintenant
        /*
        INFO("Generated key: '" << key << "' (Length: " << key.length() << ")");
        INFO("Expected key:  '" << expected_key << "' (Length: " << expected_key.length() << ")");
        if (key.length() != expected_key.length()) {
            INFO("Lengths differ!");
        }
        for(size_t i = 0; i < std::min(key.length(), expected_key.length()); ++i) {
            if (key[i] != expected_key[i]) {
                INFO("Diff at index " << i << ": Gen='" << key[i] << "' (" << int(key[i]) << ") vs Exp='" << expected_key[i] << "' (" << int(expected_key[i]) << ")");
                break;
            }
        }
        if (key.length() > expected_key.length()) {
            INFO("Generated key has extra char at index " << expected_key.length() << ": '" << key[expected_key.length()] << "' (" << int(key[expected_key.length()]) << ")");
        } else if (expected_key.length() > key.length()) {
            INFO("Expected key has extra char at index " << key.length() << ": '" << expected_key[key.length()] << "' (" << int(expected_key[key.length()]) << ")");
        }
        */

        REQUIRE(key == expected_key);
    }
    
    SECTION("Full Board and History") {
        std::vector<Card> hc = {C("2c"), C("3d")}; // 2c(1), 3d(14) -> trié 2c-3d
        // river_board = {Ah(38), Kd(24), Qc(10), Ts(47), 2h(28)}
        // trié numériquement: Qc(10)-Kd(24)-2h(28)-Ah(38)-Ts(47)
        std::string key = InformationSet::generate_key(0, hc, river_board, 5, Street::RIVER, history2);
        std::string expected_key = "P0;2c-3d|Qc-Kd-2h-Ah-Ts|River|A0C2,A1R8,";
        REQUIRE(key == expected_key);
    }

    // TODO: Tester avec des actions de type FOLD dans l'historique.
    // TODO: Tester la robustesse avec des vecteurs/arrays vides où ce n'est pas attendu (si la fonction ne valide pas en amont).
} 