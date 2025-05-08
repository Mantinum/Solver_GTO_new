#ifndef GTO_CFR_ENGINE_H
#define GTO_CFR_ENGINE_H

#include "gto/game_state.h"
#include "gto/action_abstraction.h"
#include "gto/information_set.h"
#include "eval/hand_evaluator.hpp" // Pour évaluer les mains au showdown
#include <vector>
#include <string>
#include <map>

namespace gto_solver {

class CFREngine {
public:
    CFREngine(const ActionAbstraction& action_abstraction);

    // Lance N itérations de l'algorithme CFR.
    void run_iterations(int num_iterations, GameState initial_state);

    // Récupère la stratégie moyenne pour un infoset donné (après entraînement).
    std::vector<double> get_average_strategy(const std::string& infoset_key) const;

    // Permet d'accéder à la map des infosets (pour analyse ou debug)
    const InformationSetMap& get_infoset_map() const { return infoset_map_; }

    // Méthodes pour sauvegarder et charger la map des infosets
    bool save_infoset_map(const std::string& filename) const;
    bool load_infoset_map(const std::string& filename);

private:
    // Méthode CFR récursive principale.
    // player_reach_probs: vecteur des probabilités que chaque joueur atteigne cet état.
    // P0_reach_prob, P1_reach_prob, ...
    // Returns: La valeur (EV) de l'état pour le joueur dont c'est le tour.
    double cfr_traverse(GameState current_state, std::vector<double>& player_reach_probs, int iteration_num);

    InformationSetMap infoset_map_;
    const ActionAbstraction& action_abstraction_; // Référence à une abstraction constante

    // Historique des actions pour la main courante (utile pour générer la clé d'infoset)
    // Est vidé au début de chaque nouvelle main dans run_iterations.
    std::vector<Action> current_hand_action_history_;
};

} // namespace gto_solver

#endif // GTO_CFR_ENGINE_H 