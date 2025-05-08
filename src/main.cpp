#include "gto/game_state.h"
#include "gto/action_abstraction.h"
#include "gto/cfr_engine.h"
#include "spdlog/spdlog.h"
#include <iostream> // Pour std::cerr
#include <string>   // Pour std::string
#include <vector>   // Pour std::vector
#include <exception> // Pour std::exception
#include <sstream>   // Pour std::stringstream
#include <iomanip>   // Pour std::setprecision

int main(int argc, char* argv[]) {
    // Configuration du logging (exemple: niveau info)
    spdlog::set_level(spdlog::level::info);
    // Mettre spdlog::level::debug ou spdlog::level::trace pour plus de détails
    spdlog::info("Démarrage du solveur GTO...");

    // Paramètres du jeu (exemple)
    int num_players = 2;
    int initial_stack = 200; // Exemple, ajuster si besoin
    int ante = 0;
    int button_pos = 0;
    int num_iterations = 5; // GARDER BAS POUR LES TESTS DE SAVE/LOAD
    const std::string infoset_map_filename = "infoset_map.dat"; // Nom du fichier
    int big_blind = 2; // Définir la taille de la Big Blind ici

    try {
        // 1. Créer l'état de jeu initial (template)
        gto_solver::GameState initial_state_template(num_players, initial_stack, ante, button_pos, big_blind);
        spdlog::info("État de jeu initial (template) créé.");
        // Afficher l'état initial peut être verbeux, on peut le commenter si besoin
        // initial_state_template.print_state();

        // 2. Créer l'abstraction d'action
        gto_solver::ActionAbstraction default_abstraction;
        spdlog::info("Abstraction d'actions par défaut créée.");

        // 3. Initialiser le moteur CFR
        gto_solver::CFREngine engine(default_abstraction);
        spdlog::info("Moteur CFR initialisé.");

        // 4. Essayer de charger la map d'infosets
        if (engine.load_infoset_map(infoset_map_filename)) {
            spdlog::info("Map d'infosets chargée avec succès depuis {}. Nombre d'infosets chargés: {}",
                         infoset_map_filename, engine.get_infoset_map().size());
        } else {
            spdlog::info("Aucune map d'infosets existante trouvée ({}) ou erreur de chargement. Démarrage d'un nouvel entraînement.", infoset_map_filename);
        }
        spdlog::info("Nombre d'infosets avant entraînement: {}", engine.get_infoset_map().size());

        // 5. Lancer les itérations CFR
        spdlog::info("Lancement de {} itérations CFR...", num_iterations);
        engine.run_iterations(num_iterations, initial_state_template);
        spdlog::info("Fin de l'entraînement CFR.");

        // 6. Sauvegarder la map d'infosets mise à jour
        spdlog::info("Nombre d'infosets après entraînement: {}", engine.get_infoset_map().size());
        if (engine.get_infoset_map().empty()) {
             spdlog::warn("La map d'infosets est vide, sauvegarde annulée.");
        } else if (engine.save_infoset_map(infoset_map_filename)) {
            spdlog::info("Map d'infosets sauvegardée avec succès dans {}.", infoset_map_filename);
        } else {
            spdlog::error("Échec de la sauvegarde de la map d'infosets dans {}.", infoset_map_filename);
        }

        // 7. Afficher quelques résultats (Optionnel)
        spdlog::info("Affichage de quelques stratégies (si des infosets existent) :");
        int count = 0;
        for (const auto& pair : engine.get_infoset_map()) {
            if (count >= 5) break; // Limiter l'affichage
            const std::string& key = pair.first;
            const auto& avg_strategy = engine.get_average_strategy(key);
            std::stringstream ss_strat;
             ss_strat << std::fixed << std::setprecision(3); // Appliquer la précision au stream
            for(size_t i = 0; i < avg_strategy.size(); ++i) {
                ss_strat << "A" << i << ":" << avg_strategy[i] << (i == avg_strategy.size() - 1 ? "" : " ");
            }
            // Tronquer la clé si elle est trop longue pour l'affichage
            std::string truncated_key = key;
            if (truncated_key.length() > 60) {
                 truncated_key = truncated_key.substr(0, 57) + "...";
            }
            spdlog::info("  Infoset [{}]: Visites={}, Stratégie Avg: {}", truncated_key, pair.second.visit_count, ss_strat.str());
            count++;
        }
        if (engine.get_infoset_map().size() > count) {
            spdlog::info("... et {} autres infosets.", engine.get_infoset_map().size() - count);
        }

    } catch (const std::exception& e) {
        spdlog::critical("Erreur critique: {}", e.what());
        // Afficher sur cerr aussi au cas où spdlog ne fonctionne pas
        std::cerr << "Erreur critique: " << e.what() << std::endl;
        return 1;
    } catch (...) {
         spdlog::critical("Erreur critique inconnue interceptée.");
         std::cerr << "Erreur critique inconnue interceptée." << std::endl;
         return 1;
    }


    spdlog::info("Exécution terminée.");
    return 0;
}