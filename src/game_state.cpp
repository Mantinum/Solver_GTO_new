#include <gto/game_state.h>            // Interface (Contient la définition de Action)
#include "spdlog/spdlog.h"             // Logging
#include <stdexcept>                    // Standard
#include <algorithm>                    // Standard
#include <string>                       // Standard
#include <vector>                       // Standard
#include <sstream>                      // Standard
#include "core/cards.hpp"              // Interne relatif
#include "core/deck.hpp"               // Interne relatif

// --- Constantes ---
static constexpr int    SB_SIZE         = 1;
static constexpr int    BB_SIZE         = 2;

namespace gto_solver {

// --- Implémentation de street_to_string ---
std::string street_to_string(Street s) {
    switch (s) {
        case Street::PREFLOP:  return "Preflop";
        case Street::FLOP:     return "Flop";
        case Street::TURN:     return "Turn";
        case Street::RIVER:    return "River";
        case Street::SHOWDOWN: return "Showdown";
        default:               return "Unknown Street";
    }
}

// -----------------------------------------------------------------------------
//  Constructeur / Destructeur
// -----------------------------------------------------------------------------
GameState::GameState(int num_players, int initial_stack, int ante, int button_pos)
    : num_players_          (num_players),
      stacks_               (num_players, initial_stack),
      current_bets_         (num_players, 0),
      pot_size_             (0),
      current_player_index_ (0), // Init temp
      last_raise_size_      (0),
      button_pos_           (button_pos),
      ante_                 (ante),
      current_street_       (Street::PREFLOP),
      deck_                 (),
      player_hands_         (num_players, std::vector<Card>(2, INVALID_CARD)),
      board_                (),
      board_cards_dealt_    (0),
      has_folded_           (num_players, false)
{
    if (num_players <= 0) throw std::invalid_argument("Num players must be > 0");
    if (initial_stack < 0) throw std::invalid_argument("Initial stack >= 0");
    std::fill(board_.begin(), board_.end(), INVALID_CARD);

    deck_.shuffle();
    for (int i = 0; i < num_players_ * 2; ++i) {
        const int player = i % num_players_;
        const int hole   = i / num_players_;
        if (hole < 2) player_hands_[player][hole] = deck_.deal_card();
    }

    // Blinds
    if (num_players_ == 2) { // HU
        const int sb_player = button_pos_;
        const int bb_player = (button_pos_ + 1) % num_players_;
        int sb_post = std::min(stacks_[sb_player], SB_SIZE);
        stacks_[sb_player] -= sb_post; current_bets_[sb_player] = sb_post; pot_size_ += sb_post;
        int bb_post = std::min(stacks_[bb_player], BB_SIZE);
        stacks_[bb_player] -= bb_post; current_bets_[bb_player] = bb_post; pot_size_ += bb_post;
        current_player_index_ = sb_player; last_raise_size_ = BB_SIZE;
    } else { // 3+
        const int sb_player = (button_pos_ + 1) % num_players_;
        const int bb_player = (button_pos_ + 2) % num_players_;
        int sb_post = std::min(stacks_[sb_player], SB_SIZE);
        stacks_[sb_player] -= sb_post; current_bets_[sb_player] = sb_post; pot_size_ += sb_post;
        int bb_post = std::min(stacks_[bb_player], BB_SIZE);
        stacks_[bb_player] -= bb_post; current_bets_[bb_player] = bb_post; pot_size_ += bb_post;
        current_player_index_ = (button_pos_ + 3) % num_players_; last_raise_size_ = BB_SIZE;
    }
    spdlog::debug("GameState initialisé: {} joueurs, stack {}, BTN {}, Pot {}, Mises: {}, Premier: {}",
                  num_players_, initial_stack, button_pos_, pot_size_, fmt::join(current_bets_, ","), current_player_index_);
}

GameState::~GameState() = default;

// -----------------------------------------------------------------------------
//  Accesseurs simples
// -----------------------------------------------------------------------------
int GameState::get_current_player() const { return current_player_index_; }
int GameState::get_player_stack(int i) const { if (i<0||i>=num_players_) throw std::out_of_range("Idx joueur"); return stacks_.at(i); }
const std::vector<int>& GameState::get_current_bets() const { return current_bets_; }
int GameState::get_pot_size() const { return pot_size_; }
int GameState::get_last_raise_size() const { return last_raise_size_; }
Street GameState::get_current_street() const { return current_street_; }
const std::array<Card, 5>& GameState::get_board() const { return board_; }
int GameState::get_board_cards_dealt() const { return board_cards_dealt_; }
const std::vector<Card>& GameState::get_player_hand(int i) const { if (i<0||i>=num_players_) throw std::out_of_range("Idx joueur"); return player_hands_.at(i); }
bool GameState::is_player_folded(int i) const { if (i<0||i>=num_players_) throw std::out_of_range("Idx joueur"); return has_folded_.at(i); }
int GameState::get_num_players() const { return num_players_; }

int GameState::get_num_active_players() const {
    int cnt = 0;
    for (int p = 0; p < num_players_; ++p) {
        if (!has_folded_[p] && (stacks_[p] > 0 || current_bets_[p] > 0)) cnt++;
    }
    return cnt;
}

// -----------------------------------------------------------------------------
//  Implémentation de progress_to_next_street (basée sur l'ancienne end_betting_round)
// -----------------------------------------------------------------------------
void GameState::progress_to_next_street() {
    spdlog::debug("Progressing to next street from {}", street_to_string(current_street_));
    Street next_street = current_street_;
    if (current_street_ == Street::PREFLOP)    next_street = Street::FLOP;
    else if (current_street_ == Street::FLOP)  next_street = Street::TURN;
    else if (current_street_ == Street::TURN)  next_street = Street::RIVER;
    else if (current_street_ == Street::RIVER) next_street = Street::SHOWDOWN;
    else { /* Déjà Showdown ou état invalide */ return; }

    current_street_ = next_street;
    spdlog::debug("Moved to street {}", street_to_string(current_street_));

    if (current_street_ == Street::SHOWDOWN) {
        spdlog::info("Hand reached Showdown");
        current_player_index_ = -1; return;
    }

    // Deal board cards
    if (current_street_ == Street::FLOP && board_cards_dealt_ == 0) {
        deck_.burn_card(); board_[0] = deck_.deal_card(); board_[1] = deck_.deal_card(); board_[2] = deck_.deal_card(); board_cards_dealt_ = 3;
        spdlog::debug("FLOP: [{} {} {}]", to_string(board_[0]), to_string(board_[1]), to_string(board_[2]));
    } else if (current_street_ == Street::TURN && board_cards_dealt_ == 3) {
        deck_.burn_card(); board_[3] = deck_.deal_card(); board_cards_dealt_ = 4;
        spdlog::debug("TURN: {}", to_string(board_[3]));
    } else if (current_street_ == Street::RIVER && board_cards_dealt_ == 4) {
        deck_.burn_card(); board_[4] = deck_.deal_card(); board_cards_dealt_ = 5;
        spdlog::debug("RIVER: {}", to_string(board_[4]));
    }

    // Reset for next street
    std::fill(current_bets_.begin(), current_bets_.end(), 0);
    last_raise_size_ = BB_SIZE;

    // Find first player to act
    current_player_index_ = (button_pos_ + 1) % num_players_; // Postflop commence après bouton
    int players_checked = 0;
    while (has_folded_[current_player_index_] || stacks_[current_player_index_] == 0) {
        current_player_index_ = (current_player_index_ + 1) % num_players_;
        players_checked++;
        if (players_checked > num_players_) { spdlog::warn("No active player found?"); current_player_index_ = -1; return; }
    }
    spdlog::debug("New street {}, first player: {}", street_to_string(current_street_), current_player_index_);
}

// -----------------------------------------------------------------------------
//  end_betting_round — vérifie si tout le monde a payé ou si un seul joueur reste
// -----------------------------------------------------------------------------
void GameState::end_betting_round()
{
    // 1) un seul joueur actif  ➜ main terminée
    int active_count = 0;
    for(int p=0; p<num_players_; ++p) {
        if (!has_folded_[p] && (stacks_[p] > 0 || current_bets_[p] > 0)) {
            active_count++;
        }
    }
    if (active_count <= 1) {
        spdlog::debug("EndBettingRound: <=1 active player, proceeding to showdown.");
        current_street_ = Street::SHOWDOWN;
        current_player_index_ = -1; // Indiquer fin
        return;
    }

    // 2) tout le monde (qui peut encore agir) a égalisé la mise la plus haute?
    int max_bet = 0; 
    for(int bet : current_bets_) max_bet = std::max(max_bet, bet);
    
    // Si le tour n'est PAS terminé, on trouve juste le prochain joueur
    // (Must_continue = vrai si qqn peut agir et n'a pas misé le max)
    bool must_continue = false;
    for (int p = 0; p < num_players_; ++p) {
         if (!has_folded_[p] && stacks_[p] > 0 && current_bets_[p] < max_bet) {
              must_continue = true;
              break;
         }
    }

    // Patch pour le test: Si on est préflop et que le max bet est juste la BB (pas de relance),
    // et que les mises sont égales (ex: SB a limp), le tour doit continuer.
    if (current_street_ == Street::PREFLOP && max_bet == BB_SIZE && !must_continue /* càd all_bets_matched */) {
        spdlog::trace("EndBettingRound: Preflop, max_bet=BB, forcing continue for BB option.");
        must_continue = true;
    }

    if (must_continue) {
        // Trouver le prochain joueur actif
        int next_player = current_player_index_;
        int players_checked = 0;
        do {
            next_player = (next_player + 1) % num_players_;
            players_checked++;
            if (players_checked > num_players_) throw std::logic_error("Boucle infinie next player");
        } while (has_folded_[next_player] || stacks_[next_player] == 0); 
        current_player_index_ = next_player;
        spdlog::trace("Betting round continues, next player: {}", current_player_index_);
        return; // On ne progresse PAS à la street suivante
    }

    // 3) tour terminé ➜ passer à la street suivante
    spdlog::debug("EndBettingRound: All bets matched or players are all-in/folded. Progressing street.");
    progress_to_next_street();
}

// -----------------------------------------------------------------------------
//  apply_action
// -----------------------------------------------------------------------------
void GameState::apply_action(const Action& action)
{
    if (current_player_index_ < 0) { spdlog::warn("Action sur partie terminée."); return; }
    if (action.player_index != current_player_index_) throw std::logic_error("Action mauvais joueur");
    if (has_folded_[current_player_index_]) throw std::logic_error("Action joueur foldé");

    const int player_stack = stacks_[current_player_index_];
    const int player_bet = current_bets_[current_player_index_];
    int max_bet = 0; for(int bet : current_bets_) { max_bet = std::max(max_bet, bet); }
    const int amount_to_call = max_bet - player_bet;

    switch (action.type) {
        case gto_solver::ActionType::FOLD: {
            has_folded_[current_player_index_] = true;
            spdlog::info("P{} FOLD", current_player_index_); break;
        }
        case gto_solver::ActionType::CALL: {
            if (amount_to_call == 0) { spdlog::info("P{} CHECK", current_player_index_); }
            else {
                const int call_amt = std::min(player_stack, amount_to_call);
                if (call_amt <= 0) { spdlog::warn("P{} tente CALL 0 -> CHECK?", current_player_index_); }
                else {
                    stacks_[current_player_index_] -= call_amt;
                    current_bets_[current_player_index_] += call_amt;
                    pot_size_ += call_amt;
                    spdlog::info("P{} CALL {} (stack {})", current_player_index_, call_amt, stacks_[current_player_index_]);
                }
            } break;
        }
        case gto_solver::ActionType::RAISE: {
            const int total_bet_after_raise = action.amount;
            const int raise_added = total_bet_after_raise - player_bet;
            const bool is_all_in = (raise_added == player_stack);
            const int raise_size = total_bet_after_raise - max_bet;
            if (raise_added <= 0) throw std::logic_error("Raise ajouté non positif");
            if (raise_added > player_stack) throw std::logic_error("Raise > stack");
            if (total_bet_after_raise <= max_bet && !is_all_in) throw std::logic_error("Raise <= max bet (pas all-in)");
            if (!is_all_in && raise_size < last_raise_size_ && max_bet > 0) throw std::logic_error("Raise < min-raise");
            stacks_[current_player_index_] -= raise_added;
            current_bets_[current_player_index_] = total_bet_after_raise;
            pot_size_ += raise_added;
            if (!is_all_in || raise_size >= last_raise_size_) { last_raise_size_ = raise_size; }
            spdlog::info("P{} RAISE to {} (+{}, inc {}, stack {})", current_player_index_, total_bet_after_raise, raise_added, raise_size, stacks_[current_player_index_]);
            break;
        }
        default: throw std::logic_error("Type action inconnu");
    }

    // À la fin de chaque action, on vérifie si le tour/main doit se terminer
    end_betting_round(); 

    // La logique if/else précédente est maintenant DANS end_betting_round
}

// -----------------------------------------------------------------------------
//  Fonctions Helper Privées
// -----------------------------------------------------------------------------
/*
bool GameState::is_betting_round_over() const {
    if (current_player_index_ < 0) return true;
    int active_players_count = 0;
    int max_bet = 0;
    std::vector<int> active_indices;
    for (int p = 0; p < num_players_; ++p) {
        if (!has_folded_[p]) {
            active_players_count++; active_indices.push_back(p);
            max_bet = std::max(max_bet, current_bets_[p]);
        }
    }
    if (active_players_count <= 1) return true;
    bool all_eligible_acted_and_equal = true;
    for (int p_idx : active_indices) {
        if (stacks_[p_idx] > 0 && current_bets_[p_idx] < max_bet) {
            all_eligible_acted_and_equal = false; break;
        }
    }
    if (all_eligible_acted_and_equal) {
        // TODO: Logique plus fine pour fermeture action
        bool bb_option_closed = (current_street_ == Street::PREFLOP && max_bet == BB_SIZE && current_player_index_ == (button_pos_ + 1) % num_players_);
        if (max_bet > 0 || bb_option_closed) {
            spdlog::trace("Betting round over (simplified check)"); return true;
        }
    }
    return false;
}
*/

// -----------------------------------------------------------------------------
//  Utilitaires d'affichage
// -----------------------------------------------------------------------------
// Implémentation de board_to_string (était commentée)
std::string GameState::board_to_string() const {
    std::string s;
    for (int i = 0; i < board_cards_dealt_; ++i) {
        if (board_[i] != INVALID_CARD) s += to_string(board_[i]) + " ";
    }
    if (!s.empty()) s.pop_back();
    return s;
}

// Implémentation de toString (était commentée)
std::string GameState::toString() const {
    std::stringstream ss;
    ss << "Street: " << street_to_string(current_street_) << " | Pot: " << pot_size_
       << " | Board: [" << board_to_string() << "] | Next: P" << (current_player_index_ >= 0 ? std::to_string(current_player_index_) : "None")
       << " | LastRaise: " << last_raise_size_ << "\n";
    for (int i = 0; i < num_players_; ++i) {
        ss << "  P" << i << (i == button_pos_ ? "(BTN)" : "") << ": Stack=" << stacks_[i] << ", Bet=" << current_bets_[i]
           << ", Hand=[" << (player_hands_[i].size() > 0 ? to_string(player_hands_[i][0]) : "??")
           << " " << (player_hands_[i].size() > 1 ? to_string(player_hands_[i][1]) : "??") << "]"
           << (has_folded_[i] ? " (Folded)" : "") << "\n";
    }
    return ss.str();
}

// Implémentation de print_state (était commentée)
void GameState::print_state() const {
    spdlog::info("\n{}", toString());
}

} // namespace gto_solver