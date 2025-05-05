#include "gto/action.h"
#include "core/deck.hpp"
#include "core/cards.hpp"
#include <vector>
#include <array>

namespace gto_solver {

enum class Street {
    PREFLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

std::string street_to_string(Street s); // Déclaration si besoin

class GameState {
public:
    GameState(int num_players, int initial_stack, int ante, int button_pos);
    ~GameState();

    // Accesseurs
    int                              get_current_player()      const;
    int                              get_player_stack(int i)   const;
    const std::vector<int>&          get_current_bets()        const;
    int                              get_pot_size()            const;
    int                              get_last_raise_size()     const;
    Street                           get_current_street()      const;
    const std::array<Card, 5>&       get_board()               const;
    int                              get_board_cards_dealt()   const;
    const std::vector<Card>&         get_player_hand(int i)    const;
    bool                             is_player_folded(int i)   const;
    int                              get_num_players()         const;
    int                              get_num_active_players()  const; // Joueurs non foldés avec des jetons

    // Actions
    void apply_action(const Action& action);

    // TODO: Ajouter des fonctions pour obtenir les actions légales, etc.
    // std::vector<Action> get_legal_actions() const;

    // Utilitaires
    std::string toString() const;
    void print_state() const;

private:
    // Données d'état
    int                           num_players_;
    std::vector<int>              stacks_;
    std::vector<int>              current_bets_;
    int                           pot_size_;
    int                           current_player_index_;
    int                           last_raise_size_;
    int                           button_pos_;
    int                           ante_;
    Street                        current_street_;
    Deck                          deck_;
    std::vector<std::vector<Card>> player_hands_;       // Taille [num_players][2]
    std::array<Card, 5>           board_;
    int                           board_cards_dealt_;
    std::vector<bool>             has_folded_;         // Taille [num_players]

    // --- Fonctions Helper Privées ---
    bool is_betting_round_over() const;
    void end_betting_round();

    // TODO: Autres helpers (ex: déterminer le vainqueur)
};

} // namespace gto_solver 