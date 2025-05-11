// tests/cfr_engine_tests.cpp
#include "gto/cfr_engine.h"
#include "gto/action_abstraction.h"
// #include "gto/game_state.h" // Moins nécessaire directement si on ne construit pas de GameState complexe
#include "core/cards.hpp" // Pour card_from_string et combinaisons de cartes
#include "eval/hand_evaluator.hpp" // Pour evaluate_hand_7_card
#include "gto/game_utils.hpp"      // Pour vec_to_string

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/catch_approx.hpp>

#include <cstdio> // Pour std::remove
#include <fstream>

using namespace gto_solver;

// Helper pour créer une ActionAbstraction simple pour les tests
ActionAbstraction create_simple_aa() {
    // Permet toutes les actions, pas de fractions/BBs/exacts spécifiques
    return ActionAbstraction(true, true, {}, {}, {}, true);
}

// La classe TestCFREngine n'est plus nécessaire car calculate_equity est public
/*
class TestCFREngine : public CFREngine {
public:
    TestCFREngine(const ActionAbstraction& aa) : CFREngine(aa) {}

    using CFREngine::calculate_equity; // Rendre la méthode accessible
};
*/

TEST_CASE("CFREngine calculate_equity Tests", "[CFREngine][Equity]") {
    ActionAbstraction aa = create_simple_aa();
    CFREngine cfr_engine(aa); // Utiliser CFREngine directement
    const double pot_size_for_tests = 100.0;

    SECTION("Board Complet - P0 gagne") {
        std::vector<Card> p0_h = { card_from_string("As"), card_from_string("Ks") };
        std::vector<Card> p1_h = { card_from_string("Qh"), card_from_string("Qd") };
        std::vector<Card> board = {
            card_from_string("Ac"), card_from_string("Kc"), card_from_string("2h"),
            card_from_string("3d"), card_from_string("4s")
        };
        std::vector<Card> deck = {}; // Deck vide car board complet

        double equity_p0 = cfr_engine.calculate_equity(p0_h, p1_h, board, deck, static_cast<int>(pot_size_for_tests));
        // P0 (AKs) a deux paires (A,K), P1 (QQ) a une paire de Q. P0 gagne.
        // Utility = pot_size / 2
        REQUIRE_THAT(equity_p0, Catch::Matchers::WithinAbs(pot_size_for_tests / 2.0, 0.001));
    }

    SECTION("Board Complet - P1 gagne") {
        std::vector<Card> p0_h = { card_from_string("As"), card_from_string("Ks") };
        std::vector<Card> p1_h = { card_from_string("Qh"), card_from_string("Qd") };
        std::vector<Card> board = {
            card_from_string("Qc"), card_from_string("Js"), card_from_string("2h"),
            card_from_string("3d"), card_from_string("4s")
        };
        std::vector<Card> deck = {};

        double equity_p0 = cfr_engine.calculate_equity(p0_h, p1_h, board, deck, static_cast<int>(pot_size_for_tests));
        // P0 (AKs) a hauteur AK, P1 (QQ) a une paire de Q. P1 gagne.
        // Utility = -pot_size / 2
        REQUIRE_THAT(equity_p0, Catch::Matchers::WithinAbs(-pot_size_for_tests / 2.0, 0.001));
    }

    SECTION("Board Complet - Égalité") {
        std::vector<Card> p0_h = { card_from_string("As"), card_from_string("Kc") }; // AKo
        std::vector<Card> p1_h = { card_from_string("Ad"), card_from_string("Kh") }; // AKo
        std::vector<Card> board = {
            card_from_string("2c"), card_from_string("3d"), card_from_string("4h"),
            card_from_string("5s"), card_from_string("6c") // Board anodin
        };
        std::vector<Card> deck = {};
        double equity_p0 = cfr_engine.calculate_equity(p0_h, p1_h, board, deck, static_cast<int>(pot_size_for_tests));
        // Égalité, hauteur AK pour les deux.
        REQUIRE_THAT(equity_p0, Catch::Matchers::WithinAbs(0.0, 0.001));
    }

    SECTION("Board au Turn - 2 outs possibles") {
        std::vector<Card> p0_h = { card_from_string("Ah"), card_from_string("Ad") }; // AA
        std::vector<Card> p1_h = { card_from_string("Ks"), card_from_string("Kc") }; // KK
        std::vector<Card> board = { // Board: Ac Kd Qs Js
            card_from_string("Ac"), card_from_string("Kd"), 
            card_from_string("Qs"), card_from_string("Js")
        };
        // Deck restant: 2 cartes. Une qui fait gagner P0, une qui fait gagner P1.
        Card p0_win_card = card_from_string("2h"); // Carte neutre, AA gagne si P1 ne fait pas KKK
        Card p1_win_card = card_from_string("Kh"); // Donne KKK à P1, P1 gagne.
                                                 
        std::vector<Card> deck = { p0_win_card, p1_win_card }; // Un out pour chaque

        double equity_p0 = cfr_engine.calculate_equity(p0_h, p1_h, board, deck, static_cast<int>(pot_size_for_tests));
        // P0 gagne une fois (+50), P1 gagne une fois (-50). Moyenne = 0.
        REQUIRE_THAT(equity_p0, Catch::Matchers::WithinAbs(0.0, 0.001));
    }

    SECTION("Board au Flop - 3 outs possibles pour Turn/River") {
        std::vector<Card> p0_h = { card_from_string("Ah"), card_from_string("Kh") }; // AKs (hearts)
        std::vector<Card> p1_h = { card_from_string("7d"), card_from_string("7c") }; // Paire de 7
        std::vector<Card> board_flop = { 
            card_from_string("As"), card_from_string("Ks"), card_from_string("Qs") 
        }; // P0 a TPTK + Flush draw (spades)
           // P1 a besoin d'un 7 pour un set.

        // Deck restant simulé (3 cartes)
        Card card_set_for_p1 = card_from_string("7h"); // Donne un set à P1
        Card card_neutral_1 = card_from_string("2c");
        Card card_neutral_2 = card_from_string("3d");

        std::vector<Card> deck = { card_set_for_p1, card_neutral_1, card_neutral_2 };

        // Calcul manuel des issues des 3 runouts possibles (deck.size() C 2 = 3 C 2 = 3)
        // Pot = 100.0, pot_halved = 50.0

        // Runout 1: Turn 7h, River 2c. Board: As Ks Qs 7h 2c
        // P0 Hand: Ah Kh As Ks Qs 7h 2c (Deux paires As, Ks)
        // P1 Hand: 7d 7c As Ks Qs 7h 2c (Set de 7)
        // P1 gagne. P0 utility = -50.0

        // Runout 2: Turn 7h, River 3d. Board: As Ks Qs 7h 3d
        // P0 Hand: Ah Kh As Ks Qs 7h 3d (Deux paires As, Ks)
        // P1 Hand: 7d 7c As Ks Qs 7h 3d (Set de 7)
        // P1 gagne. P0 utility = -50.0

        // Runout 3: Turn 2c, River 3d. Board: As Ks Qs 2c 3d
        // P0 Hand: Ah Kh As Ks Qs 2c 3d (Deux paires As, Ks)
        // P1 Hand: 7d 7c As Ks Qs 2c 3d (Paire de 7, P0 gagne)
        // P0 gagne. P0 utility = +50.0

        // Total utility P0 = -50 - 50 + 50 = -50
        // Average utility P0 = -50 / 3 = -16.6666...
        double expected_equity_p0 = (-50.0 - 50.0 + 50.0) / 3.0;

        double equity_p0 = cfr_engine.calculate_equity(p0_h, p1_h, board_flop, deck, static_cast<int>(pot_size_for_tests));
        REQUIRE_THAT(equity_p0, Catch::Matchers::WithinAbs(expected_equity_p0, 0.001));
    }

    // Plus de sections de test ici pour flop et turn...
}

// Classe helper pour accéder/modifier infoset_map_ pour les tests de sauvegarde/chargement
class TestCFREngineWithMapAccess : public CFREngine {
public:
    TestCFREngineWithMapAccess(const ActionAbstraction& aa) : CFREngine(aa) {}

    // Méthode pour insérer ou modifier directement un infoset (pour les tests)
    void upsert_infoset(const InformationSet& infoset) {
        infoset_map_[infoset.key] = infoset;
    }

    // Accès public à la map pour vérification (const)
    // const InformationSetMap& get_internal_map() const { return infoset_map_; }
    // Note: CFREngine a déjà get_infoset_map() const
};

TEST_CASE("CFREngine Save/Load InfosetMap", "[CFREngine][SaveLoad]") {
    ActionAbstraction aa = create_simple_aa();
    std::string test_filename = "test_infoset_map.dat";

    InformationSet node1;
    node1.key = "P0;AsKh|QcJsTd|F|R20,C,";
    node1.visit_count = 10;
    node1.cumulative_regrets = {1.5, -0.5, 2.0};
    node1.cumulative_strategy = {10.0, 20.0, 70.0};

    InformationSet node2;
    node2.key = "P1;7s7h|QcJsTd2c|T|B50,C,";
    node2.visit_count = 5;
    node2.cumulative_regrets = {3.0, 1.0};
    node2.cumulative_strategy = {60.0, 40.0};

    SECTION("Save and Load basic map") {
        {
            TestCFREngineWithMapAccess cfr_saver(aa);
            cfr_saver.upsert_infoset(node1);
            cfr_saver.upsert_infoset(node2);
            
            bool save_ok = cfr_saver.save_infoset_map(test_filename);
            REQUIRE(save_ok == true);
        }

        CFREngine cfr_loader(aa);
        bool load_ok = cfr_loader.load_infoset_map(test_filename);
        REQUIRE(load_ok == true);

        const InformationSetMap& loaded_map = cfr_loader.get_infoset_map();
        REQUIRE(loaded_map.size() == 2);

        // Vérifier Node1
        auto it1 = loaded_map.find(node1.key);
        REQUIRE(it1 != loaded_map.end());
        const InformationSet& loaded_node1 = it1->second;
        REQUIRE(loaded_node1.visit_count == node1.visit_count);
        REQUIRE(loaded_node1.cumulative_regrets.size() == node1.cumulative_regrets.size());
        for(size_t i=0; i<node1.cumulative_regrets.size(); ++i) {
            REQUIRE(loaded_node1.cumulative_regrets[i] == Catch::Approx(node1.cumulative_regrets[i]).margin(0.00001));
        }
        REQUIRE(loaded_node1.cumulative_strategy.size() == node1.cumulative_strategy.size());
        for(size_t i=0; i<node1.cumulative_strategy.size(); ++i) {
            REQUIRE(loaded_node1.cumulative_strategy[i] == Catch::Approx(node1.cumulative_strategy[i]).margin(0.00001));
        }

        // Vérifier Node2
        auto it2 = loaded_map.find(node2.key);
        REQUIRE(it2 != loaded_map.end());
        const InformationSet& loaded_node2 = it2->second;
        REQUIRE(loaded_node2.visit_count == node2.visit_count);
        REQUIRE(loaded_node2.cumulative_regrets.size() == node2.cumulative_regrets.size());
        for(size_t i=0; i<node2.cumulative_regrets.size(); ++i) {
            REQUIRE(loaded_node2.cumulative_regrets[i] == Catch::Approx(node2.cumulative_regrets[i]).margin(0.00001));
        }
        REQUIRE(loaded_node2.cumulative_strategy.size() == node2.cumulative_strategy.size());
        for(size_t i=0; i<node2.cumulative_strategy.size(); ++i) {
            REQUIRE(loaded_node2.cumulative_strategy[i] == Catch::Approx(node2.cumulative_strategy[i]).margin(0.00001));
        }
    }

    SECTION("Load non-existent file") {
        CFREngine cfr_engine(aa);
        // S'assurer que le fichier n'existe pas
        std::remove(test_filename.c_str()); 
        bool load_ok = cfr_engine.load_infoset_map("non_existent_file.dat");
        REQUIRE(load_ok == false); // Doit retourner false, pas crasher
        REQUIRE(cfr_engine.get_infoset_map().empty());
    }

    SECTION("Load malformed file") {
        std::string malformed_filename = "malformed_infoset_map.dat";
        {
            std::ofstream outfile(malformed_filename);
            outfile << "Key1\t10\t1.0,2.0\t3.0,4.0\n"; // Ligne correcte
            outfile << "Key2\tBAD_COUNT\t1.0\t2.0\n";    // Mauvais visit_count
            outfile << "Key3\t10\tBAD,REGRET\t1.0\n"; // Mauvais regret
            outfile << "Key4\t5\t1.0\tBAD,STRAT\n";   // Mauvaise stratégie
            outfile << "Key5\t20\t1.0,2.0,3.0\t4.0,5.0\n"; // Incohérence tailles regrets/stratégie
            outfile << "Key6\t30\t1.0\n"; // Pas assez de segments
            outfile.close();
        }

        CFREngine cfr_engine(aa);
        bool load_ok = cfr_engine.load_infoset_map(malformed_filename);
        // load_ok pourrait être true si au moins une ligne est bien chargée et que le parsing continue
        // La spécification actuelle de load_infoset_map est de continuer en cas d'erreur sur une ligne.
        // Elle logue une erreur et passe à la suite.
        // Donc, on vérifie si la map contient uniquement les entrées valides.

        const InformationSetMap& loaded_map = cfr_engine.get_infoset_map();
        if (load_ok) { // Si le fichier s'est ouvert etc.
            REQUIRE(loaded_map.size() == 1); // Seule Key1 devrait être chargée
            auto it = loaded_map.find("Key1");
            REQUIRE(it != loaded_map.end());
            if (it != loaded_map.end()) {
                REQUIRE(it->second.visit_count == 10);
                const std::vector<double> expected_regrets = {1.0, 2.0};
                const std::vector<double> expected_strategy = {3.0, 4.0};
                REQUIRE(it->second.cumulative_regrets.size() == expected_regrets.size());
                for(size_t i=0; i<expected_regrets.size(); ++i) {
                    REQUIRE(it->second.cumulative_regrets[i] == Catch::Approx(expected_regrets[i]).margin(0.00001));
                }
                REQUIRE(it->second.cumulative_strategy.size() == expected_strategy.size());
                for(size_t i=0; i<expected_strategy.size(); ++i) {
                    REQUIRE(it->second.cumulative_strategy[i] == Catch::Approx(expected_strategy[i]).margin(0.00001));
                }
            }
        } else {
            // Si load_ok est false (ex: impossible d'ouvrir le fichier), la map doit être vide.
            // Ce cas ne devrait pas arriver ici car on crée le fichier.
            REQUIRE(loaded_map.empty()); 
        }
        // Nettoyage
        std::remove(malformed_filename.c_str());
    }
} 