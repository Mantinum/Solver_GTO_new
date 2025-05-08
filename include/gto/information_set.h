#ifndef GTO_INFORMATION_SET_H
#define GTO_INFORMATION_SET_H

#include "gto/action_abstraction.h" // Pour Action
#include "core/cards.hpp"           // Pour Card
#include "gto/game_state.h"         // Pour Street (et potentiellement d'autres infos de GameState)
#include <string>
#include <vector>
#include <map> // Pourra être remplacé par unordered_map si performance critique
#include <numeric> // Pour std::iota, std::accumulate
#include <algorithm> // Pour std::transform, std::max
#include <sstream>   // Pour la génération de clé

namespace gto_solver {

class InformationSet {
public:
    // Clé unique identifiant ce nœud d'information.
    // Format: "[P<player_idx>]<HoleCards>|<BoardCards>|<Street>|<ActionHistory>"
    std::string key;

    // Regrets cumulés pour chaque action possible depuis cet infoset.
    // La taille correspond au nombre d'actions légales à ce nœud.
    std::vector<double> cumulative_regrets;

    // Stratégie cumulée (somme des stratégies de chaque itération où les regrets étaient positifs).
    // Utilisée pour calculer la stratégie moyenne finale.
    std::vector<double> cumulative_strategy;

    // Nombre de fois que ce nœud a été visité (utile pour certaines variantes de CFR ou debug).
    int visit_count = 0;

public:
    InformationSet() = default; // Constructeur par défaut

    // Initialise un nœud d'information avec le nombre d'actions légales possibles.
    void initialize(size_t num_actions);

    // Calcule la stratégie actuelle basée sur les regrets positifs.
    // Retourne un vecteur de probabilités pour chaque action.
    std::vector<double> get_current_strategy() const;

    // Met à jour les regrets et la stratégie cumulée.
    // strategy_profile: la stratégie effectivement jouée à cette itération (peut être la stratégie actuelle).
    // action_values: la valeur (EV) de chaque action depuis cet état.
    // current_player_reach_probability: probabilité que le joueur courant atteigne cet état.
    void update_regrets(const std::vector<double>& action_values, double node_value);
    
    void update_strategy_sum(const std::vector<double>& current_strategy_profile);

    // Génère une clé unique pour un état de jeu donné du point de vue d'un joueur.
    static std::string generate_key(
        int player_index,
        const std::vector<Card>& hole_cards, 
        const std::array<Card, 5>& board, 
        int board_cards_dealt,
        Street current_street,
        const std::vector<Action>& action_history // L'historique des actions de la main
        // TODO: Ajouter potentiellement les mises actuelles / pot si pas implicite dans action_history
    );
};

// Map pour stocker tous les nœuds d'information rencontrés.
// La clé est la string générée par InformationSet::generate_key.
using InformationSetMap = std::map<std::string, InformationSet>;
// Pourrait être: using InformationSetMap = std::unordered_map<std::string, InformationSet>;
// avec une fonction de hash custom pour std::string si performance devient un enjeu.

} // namespace gto_solver

#endif // GTO_INFORMATION_SET_H 