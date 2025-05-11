#ifndef GTO_GAME_STATE_H
#define GTO_GAME_STATE_H

#include <vector>
#include <string> // Pour les noms de joueurs ou autres infos
#include <array> // Pour le board
#include "core/cards.hpp"
#include "core/deck.hpp" // Supposant l'existence de ce fichier
#include "gto/common_types.h"
#include "gto/action_abstraction.h"

namespace gto_solver {

class ActionAbstraction;

class GameState {
public:
    static const int MAX_PLAYERS = 6;
    
    // Constructeur
    GameState(int num_players, int initial_stack, int ante, int button_pos, int big_blind_size);
    virtual ~GameState();

    // Getters
    int get_current_player() const;
    int get_player_stack(int player_index) const;
    const std::vector<int>& get_current_bets() const;
    int get_pot_size() const;
    int get_last_raise_size() const;
    int get_num_active_players() const;
    void apply_action(const Action& action);
    Street get_current_street() const;
    const std::vector<Card>& get_player_hand(int player_index) const;
    const std::array<Card, 5>& get_board() const;
    int get_board_cards_dealt() const;
    std::vector<Card> get_board_vector() const;
    bool is_player_folded(int player_index) const;
    int get_num_players() const;
    int get_big_blind_size() const;
    std::vector<Action> get_legal_abstract_actions(const ActionAbstraction& abstraction) const;
    std::vector<Card> get_remaining_deck_cards() const;

    // Getters pour le 6-max
    Position get_player_position(int player_index) const;
    int get_position_index(Position pos) const;
    bool is_position_active(Position pos) const;
    int get_ante() const;
    int get_button_position() const;
    std::vector<Position> get_active_positions() const;
    bool is_valid_position(Position pos) const;

    // Utilitaires
    std::string toString() const;
    void print_state() const;
    std::string position_to_string(Position pos) const;

private:
    // Membres privés
    int num_players_;
    std::vector<int> stacks_;
    std::vector<int> current_bets_;
    int pot_size_;
    int current_player_index_;
    int last_raise_size_;
    int button_pos_;
    int ante_;
    int big_blind_size_;
    Street current_street_;
    Deck deck_;
    std::vector<std::vector<Card>> player_hands_; // [player_idx][card_idx]
    std::array<Card, 5> board_;
    int board_cards_dealt_;
    std::vector<bool> has_folded_; // Taille [num_players]
    int last_aggressor_index_;

    // Membres pour le 6-max
    std::array<Position, MAX_PLAYERS> positions_;  // Mapping joueur -> position
    std::array<int, MAX_PLAYERS> position_to_player_;  // Mapping position -> joueur
    std::vector<Position> active_positions_;  // Positions actives dans l'ordre

    // Méthodes privées
    void progress_to_next_street();
    std::string board_to_string() const;
    bool is_betting_round_over() const;
    void end_betting_round();

    // Méthodes privées pour le 6-max
    void initialize_positions();
    void update_active_positions();
    bool is_valid_player_index(int player_index) const;
    void validate_player_index(int player_index) const;
    void collect_antes();
};

} // namespace gto_solver

#endif // GTO_GAME_STATE_H 