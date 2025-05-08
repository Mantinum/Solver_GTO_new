#include <iostream>
#include "spdlog/spdlog.h"
// #include "gto/config.h" // Pour GameConfig (si utilisé plus tard)
#include "gto/game_state.h"
#include "gto/action_abstraction.h"
#include "gto/cfr_engine.h"
#include "core/cards.hpp" // Pour MAKE_CARD, etc. si on veut créer un état spécifique

int main(int argc, char* argv[]) {
    // Initialiser spdlog pour la journalisation
    spdlog::set_level(spdlog::level::debug); // Configurer le niveau de log (trace, debug, info, etc.)
    spdlog::info("Démarrage du solveur GTO...");

    // 1. Définir les paramètres du jeu
    int num_players = 2;
    int initial_stack = 200; // Par exemple, 100 BB pour des blinds 1/2
    int ante = 0;
    int button_pos = 0; // Joueur 0 est au bouton (et SB en HU)

    // 2. Créer un état de jeu initial (template)
    // Ce GameState sera copié à chaque début de main dans CFR, et le deck sera mélangé.
    gto_solver::GameState initial_game_template(num_players, initial_stack, ante, button_pos);
    spdlog::info("État de jeu initial (template) créé.");
    initial_game_template.print_state();

    // 3. Définir l'abstraction d'actions
    // Utiliser l'abstraction par défaut pour commencer:
    // - Fold autorisé
    // - Check/Call autorisé
    // - Raises: 33%, 50%, 75%, 100% du pot (après call)
    // - All-in autorisé
    gto_solver::ActionAbstraction default_action_abstraction;
    spdlog::info("Abstraction d'actions par défaut créée.");

    // 4. Initialiser le moteur CFR
    // Note: CFREngine ne prend plus HandEvaluator dans son constructeur pour l'instant.
    // Les fonctions d'évaluation sont appelées statiquement.
    gto_solver::CFREngine cfr_engine(default_action_abstraction);
    spdlog::info("Moteur CFR initialisé.");

    // 5. Lancer les itérations CFR
    int num_iterations = 5; // Commencer avec un petit nombre pour tester
    spdlog::info("Lancement de {} itérations CFR...", num_iterations);
    
    cfr_engine.run_iterations(num_iterations, initial_game_template);

    spdlog::info("Fin de l'entraînement CFR.");
    spdlog::info("Nombre total d'infosets explorés: {}", cfr_engine.get_infoset_map().size());

    // 6. (Optionnel) Afficher quelques stratégies apprises
    if (!cfr_engine.get_infoset_map().empty()) {
        spdlog::info("Affichage de quelques stratégies (si des infosets existent) :");
        int count = 0;
        for (const auto& pair : cfr_engine.get_infoset_map()) {
            const std::string& key = pair.first;
            const gto_solver::InformationSet& infoset = pair.second;
            std::vector<double> avg_strategy = cfr_engine.get_average_strategy(key);
            
            std::string strategy_str;
            for(size_t i=0; i < avg_strategy.size(); ++i) {
                char buffer[100];
                snprintf(buffer, sizeof(buffer), "A%zu:%.3f ", i, avg_strategy[i]);
                strategy_str += buffer;
            }
            spdlog::info("  Infoset [{}...]: Visites={}, Stratégie Avg: {}", 
                         key.substr(0, std::min((size_t)30, key.length())), // Afficher début de clé
                         infoset.visit_count, 
                         strategy_str);
            count++;
            if (count >= 5) { // Limiter l'affichage
                spdlog::info("... et {} autres infosets.", cfr_engine.get_infoset_map().size() - count);
                break;
            }
        }
    } else {
        spdlog::info("Aucun infoset n'a été généré.");
    }

    spdlog::info("Exécution terminée.");
    return 0;
} 