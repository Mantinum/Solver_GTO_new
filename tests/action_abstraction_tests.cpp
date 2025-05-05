#include "gto/action_abstraction.h"
#include "gto/game_state.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// Assurez-vous que les headers nécessaires sont inclus (ex: BIG_BLIND_SIZE si défini ailleurs)
// Si BIG_BLIND_SIZE n'est pas global, il faudra peut-être l'injecter ou le récupérer autrement.
// Pour cet exemple, on suppose qu'il est accessible ou défini (e.g., via une constante globale ou un include).
// Si ce n'est pas le cas, le test devra être adapté.
const double BIG_BLIND_SIZE = 2.0; // Placeholder si non défini globalement


using namespace gto_solver; // Assurez-vous que ce namespace est correct
using Catch::Approx;

TEST_CASE("ActionAbstraction Tests", "[ActionAbstraction]") {

    SECTION("RaiseMultiplierX Preflop BB vs SB Limp") {
        // Jeu simple : 2 joueurs, stacks 100 BB, blinds 1/2 (stack=200 jetons)
        GameState st(2, /*stack=*/200, /*ante=*/0, /*button=*/0);

        // --- Scénario --- 
        // Blinds postés automatiquement (supposé): Pot=3 (SB=1, BB=2). Joueur 0 (SB) à parler.
        REQUIRE(st.get_current_player() == 0); 
        REQUIRE(st.get_pot_size() == 3);
        REQUIRE(st.get_current_bets().size() == 2);
        REQUIRE(st.get_current_bets()[0] == 1);
        REQUIRE(st.get_current_bets()[1] == 2);

        // SB limp (call 1 jeton pour compléter à 2)
        st.apply_action({0, ActionType::CALL, 1}); 
        REQUIRE(st.get_current_player() == 1); // BB à parler
        REQUIRE(st.get_pot_size() == 4); // Pot: 1(SB)+2(BB)+1(call SB) = 4
        REQUIRE(st.get_current_bets()[0] == 2);
        REQUIRE(st.get_current_bets()[1] == 2);

        ActionAbstraction aa; 
        auto specs = aa.get_possible_action_specs(st);

        // Cherche une relance 3x
        // raise_base_amount = current_bet + amount_to_call = 2 + 0 = 2.
        bool found = false;
        for (const auto& s : specs) {
            if (s.type == ActionType::RAISE &&
                s.unit == SizingUnit::MULTIPLIER_X &&
                std::abs(s.value - 3.0) < 1e-6) 
            {
                int amt = aa.get_action_amount(s, st);

                // Calcul attendu :
                // raise_base_amount = 2
                // target_total_bet = round(2 * 3.0) = 6.
                // min_raise_total = 2 + max(2, 0) = 4 
                // target_total_bet = max(6, 4) = 6.
                // amt = min(200 + 2, 6) = 6.
                REQUIRE(amt == 6); 
                found = true;
                break; 
            }
        }
        REQUIRE(found); // Vérifie que l'action RAISE 3x a été trouvée et validée
    }
    
    // Ajoutez d'autres SECTIONs ici pour tester d'autres aspects de ActionAbstraction

}

// Ajoutez ici d'autres tests pour ActionAbstraction si nécessaire 