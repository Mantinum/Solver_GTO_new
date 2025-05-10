#include "gto/game_state.h"
#include "gto/action_abstraction.h"
#include "gto/cfr_engine.h"
#include "spdlog/spdlog.h"

#include <iostream>   // std::cerr
#include <string>     // std::string
#include <vector>     // std::vector
#include <exception>  // std::exception
#include <sstream>    // std::stringstream
#include <iomanip>    // std::setprecision
#include <set>        // std::set

int main(int /*argc*/, char* /*argv*/[])
{
    // ─────────────────────────────────────────────────────────────
    // Logging
    // ─────────────────────────────────────────────────────────────
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Démarrage du solveur GTO…");

    // ─────────────────────────────────────────────────────────────
    // Paramètres généraux
    // ─────────────────────────────────────────────────────────────
    const int         num_players      = 2;
    const int         initial_stack    = 200;
    const int         ante             = 0;
    const int         button_pos       = 0;
    const int         big_blind        = 2;
    const int         num_iterations   = 4;            // Valeur basse pour test
    const std::string infoset_filename = "infoset_map.dat";

    try
    {
        // 1. État de jeu « template »
        gto_solver::GameState initial_state_template(
            num_players, initial_stack, ante, button_pos, big_blind);
        spdlog::info("État de jeu initial (template) créé.");

        // 2. Créer l’abstraction d’action (configuration enrichie)
        // --------------------------------------------------------

        // Fractions de pot par street
        gto_solver::ActionAbstraction::StreetFractionsMap fractions = {
            {gto_solver::Street::PREFLOP, {0.5, 0.75, 1.0, 1.25}},
            {gto_solver::Street::FLOP,    {0.25, 0.33, 0.5, 0.66, 0.75, 1.0, 1.25, 1.5}},
            {gto_solver::Street::TURN,    {0.33, 0.5, 0.66, 0.75, 1.0, 1.25, 1.5, 2.0}},
            {gto_solver::Street::RIVER,   {0.33, 0.5, 0.75, 1.0, 1.5, 2.0, 2.5}}
        };

        // Tailles en BB par street
        gto_solver::ActionAbstraction::StreetBBSizesMap bb_sizes = {
            {gto_solver::Street::PREFLOP, {2.2, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0}},
            {gto_solver::Street::FLOP,    {1.0, 1.5, 2.0}},
            {gto_solver::Street::TURN,    {1.5, 2.0, 2.5}},
            {gto_solver::Street::RIVER,   {2.0, 2.5, 3.0}}
        };

        // Mises exactes par street
        gto_solver::ActionAbstraction::StreetExactBetsMap exact_bets = {
            {gto_solver::Street::FLOP,  {5, 8, 10, 12, 15, 20, 25, 30}},
            {gto_solver::Street::TURN,  {10, 15, 20, 25, 30, 40, 50}},
            {gto_solver::Street::RIVER, {20, 30, 40, 50, 75, 100}}
        };

        gto_solver::ActionAbstraction abstraction(
            /*allow_folds*/       true,
            /*allow_check_call*/  true,
            fractions,
            bb_sizes,
            exact_bets,
            /*allow_all_in*/      true);

        spdlog::info("Abstraction d’actions enrichie créée.");

        // 3. Initialiser le moteur CFR
        gto_solver::CFREngine engine(abstraction);
        spdlog::info("Moteur CFR initialisé.");

        // 4. Charger une éventuelle map d’infosets
        if (engine.load_infoset_map(infoset_filename))
            spdlog::info("Infosets chargés depuis {} ({} entrées).",
                         infoset_filename, engine.get_infoset_map().size());
        else
            spdlog::info("Pas de map d’infosets existante ({}) – nouvel entraînement.",
                         infoset_filename);

        // 5. Exécuter les itérations CFR
        spdlog::info("Lancement de {} itérations CFR…", num_iterations);
        engine.run_iterations(num_iterations, initial_state_template);
        spdlog::info("Entraînement CFR terminé.");

        // 6. Sauvegarder la map d’infosets
        spdlog::info("Infosets après entraînement : {}",
                     engine.get_infoset_map().size());

        if (engine.get_infoset_map().empty())
        {
            spdlog::warn("Map d’infosets vide ; sauvegarde annulée.");
        }
        else if (engine.save_infoset_map(infoset_filename))
        {
            spdlog::info("Map d’infosets sauvegardée dans {}.",
                         infoset_filename);
        }
        else
        {
            spdlog::error("Échec de la sauvegarde de {}.", infoset_filename);
        }

        // 7. Afficher quelques infosets (optionnel)
        spdlog::info("Aperçu de stratégies :");
        int shown = 0;
        for (const auto& [key, node] : engine.get_infoset_map())
        {
            if (shown >= 5) break;
            const auto& strat = engine.get_average_strategy(key);

            std::stringstream ss; ss << std::fixed << std::setprecision(3);
            for (size_t i = 0; i < strat.size(); ++i)
                ss << "A" << i << ':' << strat[i]
                   << (i + 1 == strat.size() ? "" : " ");

            std::string k = key.size() > 60 ? key.substr(0, 57) + "…" : key;
            spdlog::info("  [{}] Visites={}  AvgStrat: {}", k,
                         node.visit_count, ss.str());
            ++shown;
        }
        if (engine.get_infoset_map().size() > shown)
            spdlog::info("… et {} autres infosets.",
                         engine.get_infoset_map().size() - shown);
    }
    catch (const std::exception& e)
    {
        spdlog::critical("Erreur critique : {}", e.what());
        std::cerr << "Erreur critique : " << e.what() << '\n';
        return 1;
    }
    catch (...)
    {
        spdlog::critical("Erreur critique inconnue interceptée.");
        std::cerr << "Erreur critique inconnue interceptée.\n";
        return 1;
    }

    spdlog::info("Exécution terminée.");
    return 0;
}
