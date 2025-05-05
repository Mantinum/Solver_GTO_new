#ifndef GTO_GAME_STATE_H
#define GTO_GAME_STATE_H

#include <vector>
#include <string> // Pour les noms de joueurs ou autres infos
#include <array> // Pour le board

// Inclure d'autres dépendances si nécessaire (ex: Action)
// #include "gto/action.h" // Si Action est défini ailleurs

// Inclure les cartes et le deck
#include "core/cards.hpp"
#include "core/deck.hpp" // Supposant l'existence de ce fichier

namespace gto_solver {

// Supposer une structure ou classe Action simple pour l'instant
// Si ActionType est défini dans action_abstraction.h, inclure ce header
#include "gto/action_abstraction.h" // Pour ActionType
struct Action {
    int player_index;
    gto_solver::ActionType type; // <-- AJOUTÉ gto_solver::
    int amount; // Montant de l'action (pour CALL, RAISE)
};

// Enum pour les tours de jeu
enum class Street {
    PREFLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN // Ou POST_RIVER
};

class GameState {
public:
    // Constructeur (basé sur l'utilisation dans le test)
    GameState(int num_players, int initial_stack, int ante, int button_pos);
    virtual ~GameState();

    // Méthodes publiques (basées sur l'utilisation dans action_abstraction.cpp et le test)
    int get_current_player() const;
    int get_player_stack(int player_index) const;
    const std::vector<int>& get_current_bets() const; // Ou retourne une copie?
    int get_pot_size() const;
    int get_last_raise_size() const;
    int get_num_active_players() const; // Utilisé dans le calcul PCT_POT
    void apply_action(const Action& action);
    Street get_current_street() const;
    const std::vector<Card>& get_player_hand(int player_index) const;
    const std::array<Card, 5>& get_board() const; // Board max 5 cartes
    int get_board_cards_dealt() const;
    bool is_player_folded(int player_index) const;
    int get_num_players() const;

    // Méthodes de test (placeholders)
    int get_current_player_placeholder() const { return 0; }
    int get_player_stack_placeholder(int player_index) const { return 200; }
    std::vector<int> get_current_bets_placeholder() const { return {1, 2}; }
    int get_pot_size_placeholder() const { return 3; }
    int get_last_raise_size_placeholder() const { return 0; } // Ou BIG_BLIND_SIZE?
    int get_num_active_players_placeholder() const { return 2; }
    void apply_action_placeholder(const Action& action) {}

    // std::vector<Action> get_legal_actions() const;

    // Utilitaires
    std::string toString() const;
    void print_state() const;

private:
    // Membres privés pour stocker l'état du jeu
    int num_players_;
    std::vector<int> stacks_;
    std::vector<int> current_bets_;
    int pot_size_;
    int current_player_index_;
    int last_raise_size_;
    int button_pos_;
    int ante_;
    Street current_street_;
    Deck deck_;
    std::vector<std::vector<Card>> player_hands_; // [player_idx][card_idx]
    std::array<Card, 5> board_;
    int board_cards_dealt_;
    std::vector<bool> has_folded_; // Pour suivre les folds
    // ... autres états ...

    // Méthodes privées
    void progress_to_next_street();
    std::string board_to_string() const;
    bool is_betting_round_over() const;
    void end_betting_round();
};

} // namespace gto_solver

#endif // GTO_GAME_STATE_H 