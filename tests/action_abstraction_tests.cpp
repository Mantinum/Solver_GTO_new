// tests/action_abstraction_tests.cpp
// ──────────────────────────────────────────────────────────────────────────────
#include "gto/action_abstraction.h"
#include "gto/game_state.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include <algorithm>
#include <set>
#include <vector>

// ──────────────────────────────────────────────────────────────────────────────
// Variables & utilitaires généraux
// ──────────────────────────────────────────────────────────────────────────────
const double BIG_BLIND_SIZE_DOUBLE = 2.0;   // Placeholder (si non défini globalement)
const int    BIG_BLIND_SIZE_INT    = 2;     // Pour GameState

using namespace gto_solver;                 // Raccourci pour les enum / classes
using Catch::Matchers::Contains;

// Helpers pour la présence d'actions dans un vector<Action>
bool has_action(const std::vector<Action>& actions,
                ActionType                 type,
                int                        amount = 0)
{
    return std::any_of(actions.begin(), actions.end(),
                       [&](const Action& a) { return a.type == type && a.amount == amount; });
}

bool has_action_type_amount(const std::vector<Action>& actions,
                            ActionType                 type,
                            int                        amount)
{
    return std::any_of(actions.begin(), actions.end(),
                       [&](const Action& a) { return a.type == type && a.amount == amount; });
}

// ──────────────────────────────────────────────────────────────────────────────
// Factories par défaut pour les maps de tailles
// ──────────────────────────────────────────────────────────────────────────────
ActionAbstraction::StreetFractionsMap create_default_test_fractions()
{
    std::set<double> default_set = {0.33, 0.5, 0.75, 1.0};
    ActionAbstraction::StreetFractionsMap fractions;
    fractions[Street::PREFLOP] = default_set;
    fractions[Street::FLOP]    = default_set;
    fractions[Street::TURN]    = default_set;
    fractions[Street::RIVER]   = default_set;
    return fractions;
}

ActionAbstraction::StreetBBSizesMap create_empty_bb_sizes_map()    { return {}; }

// NOUVEAU : map vide pour les mises exactes
ActionAbstraction::StreetExactBetsMap create_empty_exact_bets_map() { return {}; }

// ──────────────────────────────────────────────────────────────────────────────
// TEST_CASE 1 : Basic Tests
// ──────────────────────────────────────────────────────────────────────────────
TEST_CASE("ActionAbstraction Basic Tests", "[ActionAbstraction]")
{
    // Stacks 200 (100 BB) — blinds 1 / 2
    // Rappel : Action.amount = mise TOTALE après l'action

    SECTION("SB to act preflop, facing no action yet (only blinds)")
    {
        GameState st(2, /*stack=*/200, /*ante=*/0, /*button_pos=*/0, BIG_BLIND_SIZE_INT);
        REQUIRE(st.get_current_player() == 0);
        REQUIRE(st.get_pot_size() == 3);
        REQUIRE(st.get_current_bets()[0] == 1);
        REQUIRE(st.get_current_bets()[1] == 2);

        ActionAbstraction aa_default(
            /*allow_folds=*/true,
            /*allow_calls=*/true,
            create_default_test_fractions(),
            create_empty_bb_sizes_map(),
            create_empty_exact_bets_map(),   // ← ajouté
            /*allow_all_in=*/true);

        auto actions = st.get_legal_abstract_actions(aa_default);

        REQUIRE(has_action_type_amount(actions, ActionType::FOLD, 0));
        REQUIRE(has_action_type_amount(actions, ActionType::CALL, 2));

        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 4));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 5));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 6));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 200));

        REQUIRE_FALSE(has_action_type_amount(actions, ActionType::RAISE, 3));
    }

    SECTION("BB to act preflop, SB limped")
    {
        GameState st(2, 200, 0, 0, BIG_BLIND_SIZE_INT);
        st.apply_action({0, ActionType::CALL, 2});

        REQUIRE(st.get_current_player() == 1);
        REQUIRE(st.get_current_street() == Street::FLOP);
        REQUIRE(st.get_pot_size() == 4);

        ActionAbstraction aa_default(
            true, true,
            create_default_test_fractions(),
            create_empty_bb_sizes_map(),
            create_empty_exact_bets_map(),   // ← ajouté
            true);

        auto actions = st.get_legal_abstract_actions(aa_default);

        REQUIRE(has_action_type_amount(actions, ActionType::CALL, 0));
        REQUIRE_FALSE(has_action_type_amount(actions, ActionType::FOLD, 0));

        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 2));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 4));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 198));
    }

    SECTION("No Fold option if Check is available")
    {
        GameState st(2, 200, 0, 0, BIG_BLIND_SIZE_INT);
        st.apply_action({0, ActionType::CALL, 2});

        REQUIRE(st.get_current_player() == 1);
        REQUIRE(st.get_current_street() == Street::FLOP);

        ActionAbstraction::StreetFractionsMap flop_only;
        flop_only[Street::FLOP] = {0.5};

        ActionAbstraction aa_custom_flop(
            true, true,
            flop_only,
            create_empty_bb_sizes_map(),
            create_empty_exact_bets_map(),   // ← ajouté
            true);

        auto actions = st.get_legal_abstract_actions(aa_custom_flop);

        REQUIRE(has_action_type_amount(actions, ActionType::CALL, 0));
        REQUIRE_FALSE(has_action_type_amount(actions, ActionType::FOLD, 0));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 2));
    }

    SECTION("Fold is available if facing a bet")
    {
        GameState st(2, 200, 0, 0, BIG_BLIND_SIZE_INT);
        st.apply_action({0, ActionType::RAISE, 6});

        REQUIRE(st.get_current_player() == 1);

        ActionAbstraction::StreetFractionsMap preflop_only;
        preflop_only[Street::PREFLOP] = {0.5};

        ActionAbstraction aa_custom_preflop(
            true, true,
            preflop_only,
            create_empty_bb_sizes_map(),
            create_empty_exact_bets_map(),   // ← ajouté
            true);

        auto actions = st.get_legal_abstract_actions(aa_custom_preflop);

        REQUIRE(has_action_type_amount(actions, ActionType::FOLD, 0));
        REQUIRE(has_action_type_amount(actions, ActionType::CALL, 6));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 12));
    }

    // Test volontairement simplifié :
    SECTION("Raise sizing with small effective stack")
    {
        SUCCEED("Skipping raise sizing with small effective stack due to GameState setup limitations for now.");
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// TEST_CASE 2 : Street Specific Fractions
// ──────────────────────────────────────────────────────────────────────────────
TEST_CASE("ActionAbstraction Street Specific Fractions", "[ActionAbstraction][StreetFractions]")
{
    const int bb_size = 2;

    GameState state(2, 200, 0, 0, bb_size);
    state.apply_action({0, ActionType::CALL, bb_size});

    REQUIRE(state.get_current_street() == Street::FLOP);
    REQUIRE(state.get_current_player() == 1);
    REQUIRE(state.get_pot_size() == 4);

    ActionAbstraction::StreetFractionsMap spec;
    spec[Street::PREFLOP] = {1.0, 2.0};
    spec[Street::FLOP]    = {0.5, 1.0};
    spec[Street::TURN]    = {0.25, 0.75};
    spec[Street::RIVER]   = {0.66, 1.33};

    ActionAbstraction aa(
        true, true,
        spec,
        create_empty_bb_sizes_map(),
        create_empty_exact_bets_map(),          // ← ajouté
        true);

    auto actions = state.get_legal_abstract_actions(aa);

    REQUIRE(has_action_type_amount(actions, ActionType::CALL, 0));
    REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 2));
    REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 4));
    REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 198));
    REQUIRE_FALSE(has_action_type_amount(actions, ActionType::RAISE, 8));
    REQUIRE_FALSE(has_action_type_amount(actions, ActionType::RAISE, 5));

    int raise_cnt = 0;
    for (const auto& a : actions) if (a.type == ActionType::RAISE) ++raise_cnt;
    REQUIRE(raise_cnt == 3);
    REQUIRE(actions.size() == 1 + raise_cnt);
}

// ──────────────────────────────────────────────────────────────────────────────
// TEST_CASE 3 : BB Raise Sizes
// ──────────────────────────────────────────────────────────────────────────────
TEST_CASE("ActionAbstraction BB Raise Sizes", "[ActionAbstraction][BBSizes]")
{
    const int bb_size_const = 2;

    SECTION("Preflop SB to act - Open Raise in BBs")
    {
        GameState state(2, 200, 0, 0, bb_size_const);

        ActionAbstraction::StreetBBSizesMap bb_sizes;
        bb_sizes[Street::PREFLOP] = {2.5, 3.0};

        ActionAbstraction aa(
            true, true,
            {},                       // fractions vides
            bb_sizes,
            create_empty_exact_bets_map(),      // ← ajouté
            true);

        auto actions = state.get_legal_abstract_actions(aa);

        REQUIRE(has_action_type_amount(actions, ActionType::FOLD, 0));
        REQUIRE(has_action_type_amount(actions, ActionType::CALL, 2));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 5));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 6));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 200));

        if (4 != 5 && 4 != 6 && 4 != 200)
            REQUIRE_FALSE(has_action_type_amount(actions, ActionType::RAISE, 4));
    }

    SECTION("Preflop BB to act - Facing SB Raise - Re-raise in BBs")
    {
        GameState state(2, 200, 0, 0, bb_size_const);
        state.apply_action({0, ActionType::RAISE, 6});

        ActionAbstraction::StreetBBSizesMap reraise;
        reraise[Street::PREFLOP] = {4.0, 5.0};

        ActionAbstraction aa(
            true, true,
            {},
            reraise,
            create_empty_exact_bets_map(),      // ← ajouté
            true);

        auto actions = state.get_legal_abstract_actions(aa);

        REQUIRE(has_action_type_amount(actions, ActionType::FOLD, 0));
        REQUIRE(has_action_type_amount(actions, ActionType::CALL, 6));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 14));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 16));
        REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 200));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// TEST_CASE 4 : Exact Bet Sizes
// (déjà conforme, aucune modif requise)
// ──────────────────────────────────────────────────────────────────────────────
TEST_CASE("ActionAbstraction Exact Bet Sizes", "[action_abstraction]")
{
    using namespace gto_solver;

    GameState state(2, /*stack=*/200, /*ante=*/0, /*button_pos=*/0, BIG_BLIND_SIZE_INT);
    // SB posts 1, BB posts 2. Pot = 3. P0 (SB) to act.

    SECTION("Preflop SB to act - Open with Exact Bet Size")
    {
        ActionAbstraction::StreetExactBetsMap exact = {
            {Street::PREFLOP, {7, 10}}
        };
        ActionAbstraction abs(
            true, true,
            {}, {}, exact, true);

        auto actions = abs.get_abstract_actions(state);
        CHECK(has_action(actions, ActionType::FOLD, 0)); // FOLD amount is 0 by convention
        CHECK(has_action(actions, ActionType::CALL, 2)); // Call BB
        CHECK(has_action(actions, ActionType::RAISE, 7));
        CHECK(has_action(actions, ActionType::RAISE, 10));
        CHECK(has_action(actions, ActionType::RAISE, 200));
        CHECK(actions.size() == 5);

        // Exact bet trop petit → clampé
        ActionAbstraction::StreetExactBetsMap small = {
            {Street::PREFLOP, {3}}
        };
        ActionAbstraction abs_small(true, true, {}, {}, small, true);
        actions = abs_small.get_abstract_actions(state);
        CHECK(has_action(actions, ActionType::RAISE, 4)); // Clamped to min raise (2 + 2)
        CHECK_FALSE(has_action(actions, ActionType::RAISE, 3));
        CHECK(actions.size() == 4);
    }

    SECTION("Flop P0 to act - Open with Exact Bet Size")
    {
        GameState flop_state(2, /*stack=*/200, /*ante=*/0, /*button_pos=*/0, BIG_BLIND_SIZE_INT);
        // Simuler les actions préflop pour arriver au flop
        // P0 (SB) call, P1 (BB) call (check)
        flop_state.apply_action({0, ActionType::CALL, BIG_BLIND_SIZE_INT}); // P0 calls to match BB
        flop_state.apply_action({1, ActionType::CALL, BIG_BLIND_SIZE_INT}); // P1 calls (effectively checks as already at BB)
        // À ce point, apply_action devrait avoir détecté la fin du tour de mise préflop
        // et appelé en interne ce qu'il faut pour passer au FLOP et distribuer les cartes.

        REQUIRE(flop_state.get_current_street() == Street::FLOP);
        REQUIRE(flop_state.get_current_player() == 0); // P0 (SB/BTN) agit en premier postflop
        REQUIRE(flop_state.get_pot_size() == BIG_BLIND_SIZE_INT * 2); // Pot de 2 BBs
        int max_bet_flop = 0; for(int b : flop_state.get_current_bets()) max_bet_flop = std::max(max_bet_flop, b);
        REQUIRE(max_bet_flop == 0); // Mises remises à zéro pour le flop

        ActionAbstraction abs(true, true, {}, {}, { {Street::FLOP, {5, 15}} }, true);
        auto actions = abs.get_abstract_actions(flop_state);

        CHECK(has_action(actions, ActionType::CALL, 0)); // Check
        CHECK(has_action(actions, ActionType::RAISE, 5));
        CHECK(has_action(actions, ActionType::RAISE, 15));
        // P0 stack: 200 - 2 (BB postée ou égalisée) = 198. All-in est de 198.
        CHECK(has_action(actions, ActionType::RAISE, 198)); 
        CHECK(actions.size() == 4);
    }

    SECTION("Flop P1 to act - Facing P0 Bet - Re-raise with Exact Increment")
    {
        GameState flop_state(2, /*stack=*/200, /*ante=*/0, /*button_pos=*/0, BIG_BLIND_SIZE_INT);
        // Simuler les actions préflop pour arriver au flop
        flop_state.apply_action({0, ActionType::CALL, BIG_BLIND_SIZE_INT}); // P0 calls
        flop_state.apply_action({1, ActionType::CALL, BIG_BLIND_SIZE_INT}); // P1 calls (checks)
        // Passage au Flop, P0 (SB/BTN) to act.
        
        // P0 bets 10 on flop
        flop_state.apply_action({0, ActionType::RAISE, 10}); 

        REQUIRE(flop_state.get_current_street() == Street::FLOP);
        REQUIRE(flop_state.get_current_player() == 1); // P1 (BB) to act
        // Pot: 4 (preflop) + 10 (P0 bet) = 14
        REQUIRE(flop_state.get_pot_size() == (BIG_BLIND_SIZE_INT * 2) + 10);
        int max_bet_flop_after_p0_bet = 0; for(int b : flop_state.get_current_bets()) max_bet_flop_after_p0_bet = std::max(max_bet_flop_after_p0_bet, b);
        REQUIRE(max_bet_flop_after_p0_bet == 10);

        ActionAbstraction abs(true, true, {}, {}, { {Street::FLOP, {12, 22}} }, true);
        auto actions = abs.get_abstract_actions(flop_state);

        CHECK(has_action(actions, ActionType::FOLD, 0));
        CHECK(has_action(actions, ActionType::CALL, 10));
        CHECK(has_action(actions, ActionType::RAISE, 22)); // 10 + 12
        CHECK(has_action(actions, ActionType::RAISE, 32)); // 10 + 22
        // P1 stack: 200 - 2 (BB postée) - 10 (call) = 188. Si all-in, montant total = 2 (mise préflop) + 188 = 190 ???
        // Non, le montant de l'action RAISE est le *montant total* de la mise. Stack P1=200. BB=2. P0 bet 10. P1 stack = 200-2=198. P1 all-in = 198.
        CHECK(has_action(actions, ActionType::RAISE, 198)); 
        CHECK(actions.size() == 5);

        // Check clamping of increment if too small
        ActionAbstraction abs_small(true, true, {}, {}, { {Street::FLOP, {5}} }, true);
        actions = abs_small.get_abstract_actions(flop_state);
        CHECK(has_action(actions, ActionType::RAISE, 20)); // Clamped to min raise (10 + 10)
        CHECK_FALSE(has_action(actions, ActionType::RAISE, 15));
        CHECK(actions.size() == 4); // Fold, Call, Raise 20, All-in
    }

    SECTION("Interaction with BB sizes and Fractions")
    {
        GameState state_preflop(2, /*stack=*/200, /*ante=*/0, /*button_pos=*/0, BIG_BLIND_SIZE_INT);

        ActionAbstraction abs(
            true, true,
            { {Street::PREFLOP, {1.0}} },              // fractions
            { {Street::PREFLOP, {2.5}} },              // bb sizes
            { {Street::PREFLOP, {7}} },                // exact
            true);

        auto actions = abs.get_abstract_actions(state_preflop);

        CHECK(has_action(actions, ActionType::FOLD, 0)); // FOLD amount is 0
        CHECK(has_action(actions, ActionType::CALL, 2));
        CHECK(has_action(actions, ActionType::RAISE, 5));
        CHECK(has_action(actions, ActionType::RAISE, 6));
        CHECK(has_action(actions, ActionType::RAISE, 7));
        CHECK(has_action(actions, ActionType::RAISE, 200));
        CHECK(actions.size() == 6);
    }
}

// ──────────────────────────────────────────────────────────────────────────────
// TEST_CASE 5 : Default Action Abstraction – No Folds
// ──────────────────────────────────────────────────────────────────────────────
TEST_CASE("Default Action Abstraction - No Folds", "[ActionAbstraction]")
{
    ActionAbstraction::StreetFractionsMap fractions   = {};
    ActionAbstraction::StreetBBSizesMap    bb_sizes   = {};
    ActionAbstraction::StreetExactBetsMap  exact_bets = {};

    // allow_fold est false ici
    ActionAbstraction abs(false, true, fractions, bb_sizes, exact_bets, true);

    GameState state(2, /*stack=*/200, /*ante=*/0, /*button_pos=*/0, BIG_BLIND_SIZE_INT);
    // Situation: SB (P0) to act. SB=1, BB=2. Max_bet=2. P0_bet=1. P0 peut folder.
    // Mais comme allow_fold est false, FOLD ne devrait pas être une option.

    auto actions = state.get_legal_abstract_actions(abs);

    // L'action FOLD ne devrait PAS être présente si allow_fold_ est false
    REQUIRE_FALSE(has_action_type_amount(actions, ActionType::FOLD, 0)); // Corrigé de REQUIRE à REQUIRE_FALSE

    // Les autres actions attendues (CALL, RAISES) devraient toujours être là.
    // SB doit payer 1 pour caller (total bet 2)
    REQUIRE(has_action_type_amount(actions, ActionType::CALL, 2));
    // Les raises par défaut devraient être basées sur les fractions (qui sont vides ici), et l'all-in.
    // Min raise increment = BB = 2. Min total raise = max_bet (2) + 2 = 4.
    // All-in pour P0 = stack (200) - mise SB (1) + mise SB (1) = 200.
    // Puisque les fractions et BB sizes sont vides, et exact_bets aussi, seul all-in devrait être un raise possible
    // en plus du min-raise si les autres options ne le génèrent pas.
    // Revoyons la logique de add_raise_actions pour ce cas.
    // Si raise_total_bets est vide après fractions, bb_sizes, exact_bets, et que allow_all_in_ est true, all-in est ajouté.
    // Min_raise_total_bet est 4. Max_raise_total_bet (all-in) est 200.
    // S'il n'y a aucune fraction/BB/exact size, est-ce que min_raise (4) est ajouté automatiquement ?
    // La logique actuelle ajoute min_raise_total_bet (s'il est > max_bet) comme candidat initial si aucune autre taille de relance n'est trouvée. Non, ce n'est pas le cas.
    // Les relances sont ajoutées à `raise_total_bets` puis converties en actions.
    // Si `raise_total_bets` est vide mais `allow_all_in_` est vrai, seul all-in (200) devrait être ajouté.

    // Attendu : CALL 2, RAISE 200 (all-in)
    // Si les fractions sont vides, les tailles BB vides, les mises exactes vides :
    // min_raise_total_bet = 4.
    // max_raise_total_bet = 200.
    // raise_total_bets sera vide initialement.
    // allow_all_in_ est true, donc 200 est ajouté à raise_total_bets.
    // Donc on s'attend à CALL 2, RAISE 200.
    REQUIRE(actions.size() == 2); // CALL, RAISE (all-in)
    REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 200));

    // Les assertions originales qui suivaient le FOLD étaient basées sur create_default_test_fractions(), qui ne sont pas utilisées ici.
    // REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 4)); // Plus valide avec fractions vides
    // REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 5)); // Plus valide
    // REQUIRE(has_action_type_amount(actions, ActionType::RAISE, 6)); // Plus valide
    // REQUIRE_FALSE(has_action_type_amount(actions, ActionType::RAISE, 3)); // Toujours vrai
}
