#include "gto/cfr_engine.h"
#include "eval/hand_evaluator.hpp" // Assurer la définition complète pour l'utilisation
#include "gto/information_set.h" // Déjà inclus via cfr_engine.h mais explicite
#include "gto/game_utils.hpp"      // Pour street_to_string
#include "spdlog/spdlog.h"
#include <numeric> // Pour std::accumulate
#include <stdexcept> // Pour std::runtime_error
#include <fstream> // Pour std::ofstream
#include <sstream> // Pour std::stringstream (pourrait être utile)
#include <iomanip> // Pour std::setprecision

namespace gto_solver {

CFREngine::CFREngine(const ActionAbstraction& action_abstraction)
    : action_abstraction_(action_abstraction) {}

void CFREngine::run_iterations(int num_iterations, GameState initial_state_template) {
    if (initial_state_template.get_num_players() <= 0) {
        spdlog::error("CFREngine: Nombre de joueurs invalide dans l'état initial.");
        return;
    }

    for (int i = 0; i < num_iterations; ++i) {
        spdlog::info("CFR Iteration {}/{}", i + 1, num_iterations);
        current_hand_action_history_.clear(); // Historique pour la clé d'infoset
        
        // Créer une copie de l'état initial pour cette itération
        // car GameState contient le deck et distribue les cartes.
        GameState current_hand_state = initial_state_template; 
        // Il est crucial que initial_state_template soit "frais" ou que GameState
        // gère correctement le re-shuffle et la re-distribution des cartes pour chaque main.
        // Si initial_state_template est toujours le même, le deck sera le même à chaque itération.
        // GameState::GameState constructeur fait un shuffle.

        std::vector<double> initial_player_reach_probs(current_hand_state.get_num_players(), 1.0);
        cfr_traverse(current_hand_state, initial_player_reach_probs, i);
    }
    spdlog::info("CFR Entraînement terminé. {} infosets explorés.", infoset_map_.size());
}

std::vector<double> CFREngine::get_average_strategy(const std::string& infoset_key) const {
    auto it = infoset_map_.find(infoset_key);
    if (it == infoset_map_.end()) {
        spdlog::warn("CFREngine: Infoset key '{}' non trouvée.", infoset_key);
        return {}; // Retourner une stratégie vide ou lancer une exception
    }
    const InformationSet& infoset = it->second;
    if (infoset.visit_count == 0 || infoset.cumulative_strategy.empty()) {
        spdlog::warn("CFREngine: Infoset '{}' n'a pas été visité ou n'a pas de stratégie cumulative.", infoset_key);
        // Retourner stratégie uniforme si pas de visites ?
        size_t num_actions = infoset.cumulative_regrets.size(); // On se base sur la taille des regrets
        if (num_actions == 0) return {};
        return std::vector<double>(num_actions, 1.0 / num_actions);
    }

    std::vector<double> avg_strategy = infoset.cumulative_strategy;
    double sum_cumulative_strategy = 0;
    for(double prob : avg_strategy) sum_cumulative_strategy += prob;

    if (sum_cumulative_strategy > 0) {
        for (double& prob : avg_strategy) {
            prob /= sum_cumulative_strategy;
        }
    } else {
        // Devrait être rare si visit_count > 0 et strategy_sum a été mis à jour.
        // Retourner uniforme par défaut.
        size_t num_actions = infoset.cumulative_strategy.size();
        if (num_actions == 0) return {};
        std::fill(avg_strategy.begin(), avg_strategy.end(), 1.0 / num_actions);
        spdlog::debug("CFREngine: sum_cumulative_strategy est 0 pour '{}', retour stratégie uniforme.", infoset_key);
    }
    return avg_strategy;
}

double CFREngine::cfr_traverse(GameState current_state, std::vector<double>& player_reach_probs, int iteration_num) {
    // 1. Vérifier si c'est un nœud terminal (fin de la main)
    if (current_state.get_current_player() < 0 || current_state.get_current_street() == Street::SHOWDOWN) {
        spdlog::trace("CFR: Nœud terminal atteint. Pot: {}. Street: {}", 
                      current_state.get_pot_size(), street_to_string(current_state.get_current_street()));

        double p0_utility = 0.0;
        int pot_size = current_state.get_pot_size();

        // Pour un jeu à 2 joueurs, déterminer le statut de chaque joueur
        bool p0_active = !current_state.is_player_folded(0);
        bool p1_active = false;
        if (current_state.get_num_players() > 1) {
            p1_active = !current_state.is_player_folded(1);
        }

        if (current_state.get_num_players() == 2) { // Logique pour 2 joueurs
            if (p0_active && !p1_active) { // P1 a foldé, P0 gagne
                p0_utility = static_cast<double>(pot_size) / 2.0;
                spdlog::trace("CFR Terminal: P1 folded. P0 utility: {}", p0_utility);
            } else if (!p0_active && p1_active) { // P0 a foldé, P1 gagne
                p0_utility = -static_cast<double>(pot_size) / 2.0;
                spdlog::trace("CFR Terminal: P0 folded. P0 utility: {}", p0_utility);
            } else if (p0_active && p1_active) { // Showdown entre P0 et P1
                spdlog::trace("CFR Terminal: Showdown P0 vs P1. Board cards: {}", current_state.get_board_cards_dealt());
                if (current_state.get_board_cards_dealt() == 5) {
                    const auto& p0_hand_vec = current_state.get_player_hand(0);
                    const auto& p1_hand_vec = current_state.get_player_hand(1);
                    const auto& board_array = current_state.get_board();
                    std::vector<Card> board_vec;
                    for(int k=0; k < current_state.get_board_cards_dealt(); ++k) {
                        if(board_array[k] != INVALID_CARD) board_vec.push_back(board_array[k]);
                    }

                    if (p0_hand_vec.size() == 2 && p1_hand_vec.size() == 2 && board_vec.size() == 5) {
                        short rank_p0 = gto_solver::evaluate_hand_7_card(p0_hand_vec[0], p0_hand_vec[1], board_vec);
                        short rank_p1 = gto_solver::evaluate_hand_7_card(p1_hand_vec[0], p1_hand_vec[1], board_vec);
                        
                        if (rank_p0 == INVALID_HAND_RANK || rank_p1 == INVALID_HAND_RANK) {
                           spdlog::error("CFR Showdown: Invalid hand rank P0 ({}) or P1 ({}). State:\n{}", rank_p0, rank_p1, current_state.toString());
                           p0_utility = 0.0; // Erreur, traiter comme une égalité pour l'instant
                        } else if (rank_p0 > rank_p1) { // P0 gagne
                            p0_utility = static_cast<double>(pot_size) / 2.0;
                        } else if (rank_p1 > rank_p0) { // P1 gagne
                            p0_utility = -static_cast<double>(pot_size) / 2.0;
                        } else { // Égalité
                            p0_utility = 0.0;
                        }
                        spdlog::trace("CFR Showdown: P0 rank {}, P1 rank {}. P0 utility: {}", rank_p0, rank_p1, p0_utility);
                    } else {
                        spdlog::error("CFR Showdown: Incorrect card counts for eval. P0 hand: {}, P1 hand: {}, Board: {}. State:\n{}", 
                                      p0_hand_vec.size(), p1_hand_vec.size(), board_vec.size(), current_state.toString());
                        p0_utility = 0.0; // Erreur
                    }
                } else { // Cas où le board n'a PAS 5 cartes (showdown au Flop ou au Turn)
                    // TODO: CRITICAL - Implémenter le calcul d'équité pour les boards incomplets.
                    // spdlog::warn("CFR Showdown: Board non complet ({} cartes). Début calcul d'équité (placeholder).", current_state.get_board_cards_dealt());

                    const auto& p0_hand_cards = current_state.get_player_hand(0); // Devrait être std::vector<Card>
                    const auto& p1_hand_cards = current_state.get_player_hand(1); // Devrait être std::vector<Card>
                    
                    std::vector<Card> current_board_cards_vec;
                    const auto& board_array_incomplete = current_state.get_board();
                    for(int k=0; k < current_state.get_board_cards_dealt(); ++k) {
                        if(board_array_incomplete[k] != INVALID_CARD) current_board_cards_vec.push_back(board_array_incomplete[k]);
                    }

                    int num_board_cards_dealt = current_state.get_board_cards_dealt();
                    int cards_to_deal_for_full_board = 5 - num_board_cards_dealt;

                    // Récupérer les cartes restantes dans le deck.
                    // Cela nécessitera une nouvelle méthode dans GameState ou Deck.
                    // Exemple: std::vector<Card> remaining_deck = current_state.get_remaining_deck_cards();
                    // Pour l'instant, on simule un deck vide pour éviter des erreurs de compilation,
                    // mais cela doit être remplacé par le vrai deck restant.
                    std::vector<Card> remaining_deck = current_state.get_remaining_deck_cards(); // MODIFIÉ ICI

                    if (p0_hand_cards.size() != 2 || p1_hand_cards.size() != 2) {
                        spdlog::error("CFR Equity Calc: Incorrect player hand size for equity calculation.");
                        p0_utility = 0.0; // Erreur
                    } else if (remaining_deck.empty() && cards_to_deal_for_full_board > 0) {
                         // Si on a besoin de cartes mais que le deck (simulé) est vide.
                         // En réalité, si on arrive ici avec un deck vraiment vide, c'est une erreur de logique en amont.
                         spdlog::error("CFR Equity Calc: Remaining deck is empty, cannot deal {} cards.", cards_to_deal_for_full_board);
                         p0_utility = 0.0;
                    } else if (cards_to_deal_for_full_board < 0) {
                        spdlog::error("CFR Equity Calc: cards_to_deal_for_full_board is negative ({}), board already has {} cards.",
                                       cards_to_deal_for_full_board, num_board_cards_dealt);
                        p0_utility = 0.0;
                    }
                    else if (cards_to_deal_for_full_board == 0) { // Devrait avoir été géré par le if (board_cards_dealt == 5)
                        spdlog::error("CFR Equity Calc: cards_to_deal_for_full_board is 0 but not caught by 5-card board logic.");
                        p0_utility = 0.0; // Fallback, mais c'est une erreur de logique
                    }
                    else {
                        // Ici, la logique pour itérer sur les combinaisons de `remaining_deck`
                        // pour tirer `cards_to_deal_for_full_board` cartes.

                        long p0_wins = 0;
                        long p1_wins = 0;
                        long ties = 0;
                        long total_runouts_simulated = 0;

                        // Exemple simplifié pour 1 carte à tirer (showdown au Turn)
                        if (cards_to_deal_for_full_board == 1) {
                            if (remaining_deck.empty()) {
                                spdlog::error("CFR Equity Calc (1 card): Remaining deck is empty, cannot deal turn card.");
                            } else {
                                for (Card turn_card : remaining_deck) { 
                                    std::vector<Card> final_board = current_board_cards_vec;
                                    final_board.push_back(turn_card);
                                    // Pas besoin de vérifier la validité de turn_card ici car get_remaining_deck_cards
                                    // ne devrait retourner que des cartes valides et non utilisées.
                                    // Il faut s'assurer que final_board a bien 4 cartes pour l'évaluation 7 cartes (2+2+3 initial + 1)
                                    // Non, evaluate_hand_7_card attend 5 cartes de board. Il faut ajuster.
                                    // Notre `evaluate_hand_7_card` prend 2 cartes main + 5 board.
                                    // Ici, current_board_cards_vec a 3 cartes (Flop), on ajoute la Turn, il en manque une pour River.
                                    // Ah, non, si cards_to_deal_for_full_board == 1, c'est qu'on est au Turn et il manque la River.
                                    // Donc current_board_cards_vec a 4 cartes (Flop+Turn), et final_board aura 5 cartes.
                                    // Donc, il faut que `current_board_cards_vec` ait 4 cartes (Flop+Turn) si on est ici.
                                    // Et on ajoute la River card (appelée `turn_card` dans la boucle, renommons là `river_card_iter`)

                                    if (final_board.size() == 5) { // Vérification cruciale
                                        short rank_p0 = gto_solver::evaluate_hand_7_card(p0_hand_cards[0], p0_hand_cards[1], final_board);
                                        short rank_p1 = gto_solver::evaluate_hand_7_card(p1_hand_cards[0], p1_hand_cards[1], final_board);

                                        if (rank_p0 == INVALID_HAND_RANK || rank_p1 == INVALID_HAND_RANK) {
                                            spdlog::error("CFR Equity Calc (1 card): Invalid hand rank during runout. P0:{}, P1:{}", rank_p0, rank_p1);
                                            // Que faire? On peut ignorer ce runout ou compter comme égalité
                                        } else if (rank_p0 > rank_p1) {
                                            p0_wins++;
                                        } else if (rank_p1 > rank_p0) {
                                            p1_wins++;
                                        } else {
                                            ties++;
                                        }
                                        total_runouts_simulated++;
                                    } else {
                                        spdlog::error("CFR Equity Calc (1 card): Final board size is not 5 ({}) after adding river card. Initial board size: {}. River card: {}", 
                                                      final_board.size(), current_board_cards_vec.size(), gto_solver::to_string(turn_card));
                                    }
                                }
                            }
                            // spdlog::debug("Equity calc for 1 card to come: Placeholder. Need actual remaining_deck and iteration.");
                            spdlog::debug("Equity calc (1 card): total_runouts_simulated={}, p0_wins={}, p1_wins={}, ties={}",
                                          total_runouts_simulated, p0_wins, p1_wins, ties);
                        } 
                        // Exemple pour 2 cartes à tirer (showdown au Flop)
                        else if (cards_to_deal_for_full_board == 2) {
                            if (remaining_deck.size() < 2) {
                                spdlog::error("CFR Equity Calc (2 cards): Remaining deck size ({}) is less than 2, cannot deal turn and river.", remaining_deck.size());
                            } else {
                                // Combinatorics: itérer sur toutes paires de cartes de remaining_deck
                                for (size_t i = 0; i < remaining_deck.size(); ++i) {
                                    for (size_t j = i + 1; j < remaining_deck.size(); ++j) {
                                        Card turn_card_candidate = remaining_deck[i];
                                        Card river_card_candidate = remaining_deck[j];
                                        
                                        std::vector<Card> final_board = current_board_cards_vec; // current_board_cards_vec a 3 cartes (Flop)
                                        final_board.push_back(turn_card_candidate);
                                        final_board.push_back(river_card_candidate);

                                        if (final_board.size() == 5) { // Vérification cruciale
                                            short rank_p0 = gto_solver::evaluate_hand_7_card(p0_hand_cards[0], p0_hand_cards[1], final_board);
                                            short rank_p1 = gto_solver::evaluate_hand_7_card(p1_hand_cards[0], p1_hand_cards[1], final_board);

                                            if (rank_p0 == INVALID_HAND_RANK || rank_p1 == INVALID_HAND_RANK) {
                                                spdlog::error("CFR Equity Calc (2 cards): Invalid hand rank during runout. P0:{}, P1:{}", rank_p0, rank_p1);
                                            } else if (rank_p0 > rank_p1) {
                                                p0_wins++;
                                            } else if (rank_p1 > rank_p0) {
                                                p1_wins++;
                                            } else {
                                                ties++;
                                            }
                                            total_runouts_simulated++;
                                        } else {
                                            spdlog::error("CFR Equity Calc (2 cards): Final board size is not 5 ({}) after adding turn/river. Initial board size: {}. Turn: {}, River: {}", 
                                                          final_board.size(), current_board_cards_vec.size(), 
                                                          gto_solver::to_string(turn_card_candidate), gto_solver::to_string(river_card_candidate));
                                        }
                                    }
                                }
                            }
                            // spdlog::debug("Equity calc for 2 cards to come: Placeholder. Need actual remaining_deck and iteration.");
                            spdlog::debug("Equity calc (2 cards): total_runouts_simulated={}, p0_wins={}, p1_wins={}, ties={}",
                                          total_runouts_simulated, p0_wins, p1_wins, ties);
                        }
                        // ... autres cas si plus de cartes (ne devrait pas arriver pour Texas Holdem 5-card board)

                        if (total_runouts_simulated > 0) {
                            // Utility pour P0 : (gain si P0 gagne - gain si P1 gagne (perte pour P0))
                            // Chaque joueur a contribué pot_size / 2 au pot s'ils sont all-in.
                            // Ce calcul suppose que le pot est partagé équitablement en cas d'égalité.
                            // L'utilité est du point de vue de P0.
                            // Si P0 gagne, il récupère sa mise et gagne celle de P1 => gain net = mise_P1 = pot_size / 2
                            // Si P0 perd, il perd sa mise => perte net = -mise_P0 = -pot_size / 2
                            p0_utility = (static_cast<double>(p0_wins - p1_wins) / total_runouts_simulated) * (static_cast<double>(pot_size) / 2.0);
                            spdlog::debug("CFR Equity Calc Result: P0 wins {}, P1 wins {}, Ties {}. Total runouts {}. P0 Utility: {:.4f}", 
                                          p0_wins, p1_wins, ties, total_runouts_simulated, p0_utility);
                        } else {
                            spdlog::warn("CFR Equity Calc: No runouts simulated for board with {} cards. Utility set to 0.", num_board_cards_dealt);
                            p0_utility = 0.0; // Fallback si aucune simulation n'a eu lieu
                        }
                    }
                }
            } else { // Cas imprévu à 2 joueurs (ex: les deux inactifs mais pas de fold clair?)
                 spdlog::warn("CFR Terminal: État ambigu pour 2 joueurs (P0 active: {}, P1 active: {}). Pot: {}. Utility = 0.", p0_active, p1_active, pot_size);
                 p0_utility = 0.0;
            }
        } else { // Plus de 2 joueurs ou moins de 2 (ne devrait pas arriver pour un jeu standard)
            spdlog::warn("CFR Terminal: Calcul d'utilité non implémenté pour {} joueurs. Utility = 0.", current_state.get_num_players());
            p0_utility = 0.0;
        }
        return p0_utility;
    }

    int current_player = current_state.get_current_player();

    // 2. Générer la clé de l'infoset et récupérer/créer le nœud d'information
    std::string infoset_key = InformationSet::generate_key(
        current_player,
        current_state.get_player_hand(current_player),
        current_state.get_board(),
        current_state.get_board_cards_dealt(),
        current_state.get_current_street(),
        current_hand_action_history_
    );
    
    InformationSet& infoset_node = infoset_map_[infoset_key]; // Crée si n'existe pas

    std::vector<Action> legal_actions = current_state.get_legal_abstract_actions(action_abstraction_);    
    if (legal_actions.empty()) {
        // Cela ne devrait pas arriver si le nœud n'est pas terminal.
        // GameState ou ActionAbstraction pourrait avoir un problème.
        spdlog::error("CFR: Aucune action légale pour un nœud non terminal! Infoset: {}. State:\n{}", infoset_key, current_state.toString());
        // Forcer un abandon ou retourner une valeur d'erreur?
        // Pour l'instant, on suppose que ça n'arrive pas ou que c'est un bug à corriger ailleurs.
        return 0.0; // Valeur d'erreur ou de repli
    }

    if (infoset_node.cumulative_regrets.empty() || infoset_node.cumulative_regrets.size() != legal_actions.size()) {
        infoset_node.initialize(legal_actions.size());
        infoset_node.key = infoset_key; // S'assurer que la clé est stockée
    }

    // 3. Obtenir la stratégie actuelle pour cet infoset
    std::vector<double> current_strategy = infoset_node.get_current_strategy();
    double node_value = 0.0; // Valeur de cet état pour le joueur courant
    std::vector<double> action_values(legal_actions.size(), 0.0);

    // 4. Itérer sur chaque action légale
    for (size_t i = 0; i < legal_actions.size(); ++i) {
        const Action& action = legal_actions[i];
        GameState next_state = current_state; // Copie pour simuler l'action
        
        current_hand_action_history_.push_back(action); // Ajouter l'action à l'historique
        next_state.apply_action(action);
        
        // Mettre à jour les probabilités d'atteinte pour le joueur qui vient d'agir
        std::vector<double> next_player_reach_probs = player_reach_probs;
        next_player_reach_probs[current_player] *= current_strategy[i];

        double child_node_value_for_current_player = cfr_traverse(next_state, next_player_reach_probs, iteration_num);
        
        current_hand_action_history_.pop_back(); // Retirer l'action de l'historique (backtrack)

        action_values[i] = child_node_value_for_current_player; // Valeur de prendre cette action
        node_value += current_strategy[i] * child_node_value_for_current_player;
    }

    // 5. Mettre à jour les regrets et la stratégie cumulée pour le joueur courant (Vanilla CFR)
    double p_i = player_reach_probs[current_player]; // Probabilité que le joueur courant atteigne ce nœud
    double p_opp = 1.0; // Probabilité que les opposants atteignent ce nœud
    for(int p = 0; p < current_state.get_num_players(); ++p) {
        if (p != current_player) {
            p_opp *= player_reach_probs[p];
        }
    }

    // Mettre à jour les regrets cumulés
    // regret_for_action = action_value - node_value
    // cumulative_regret += p_opp * regret_for_action
    std::vector<double> immediate_regrets(legal_actions.size());
    for (size_t i = 0; i < legal_actions.size(); ++i) {
        immediate_regrets[i] = action_values[i] - node_value;
        infoset_node.cumulative_regrets[i] += p_opp * immediate_regrets[i];
    }
    // infoset_node.update_regrets(action_values, node_value); // L'ancienne méthode ne pondère pas
    // On pourrait créer une nouvelle méthode: update_cumulative_regrets(const std::vector<double>& immediate_regrets, double opponent_reach_prob)
    // Pour l'instant, mise à jour directe.

    // Mettre à jour la somme des stratégies
    // cumulative_strategy += p_i * current_strategy_action_prob
    std::vector<double> weighted_strategy_to_sum = current_strategy;
    for(double& prob_s : weighted_strategy_to_sum) {
        prob_s *= p_i;
    }
    infoset_node.update_strategy_sum(weighted_strategy_to_sum);
    // Le visit_count est incrémenté dans update_strategy_sum, ce qui est ok.
    // Si on voulait un visit_count pondéré, il faudrait le passer.

    return node_value;
}

// --- Sauvegarde / Chargement --- 

bool CFREngine::save_infoset_map(const std::string& filename) const {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        spdlog::error("Impossible d'ouvrir le fichier de sauvegarde : {}", filename);
        return false;
    }

    spdlog::info("Sauvegarde de {} infosets dans {}...", infoset_map_.size(), filename);
    outfile << std::fixed << std::setprecision(10); // Précision pour les doubles

    for (const auto& pair : infoset_map_) {
        const std::string& key = pair.first;
        const InformationSet& node = pair.second;

        // Format: cle;visit_count;regret1,regret2,...;strat1,strat2,...
        outfile << key << ";" << node.visit_count << ";";

        // Écrire les regrets cumulés
        for (size_t i = 0; i < node.cumulative_regrets.size(); ++i) {
            outfile << node.cumulative_regrets[i] << (i == node.cumulative_regrets.size() - 1 ? "" : ",");
        }
        outfile << ";";

        // Écrire la stratégie cumulée
        for (size_t i = 0; i < node.cumulative_strategy.size(); ++i) {
            outfile << node.cumulative_strategy[i] << (i == node.cumulative_strategy.size() - 1 ? "" : ",");
        }
        outfile << "\n"; // Nouvelle ligne pour le prochain infoset
    }

    outfile.close();
    if (outfile.fail()) { // Vérifier si la fermeture ou une écriture a échoué
        spdlog::error("Erreur lors de l'écriture ou de la fermeture du fichier : {}", filename);
        return false;
    }
    
    spdlog::info("Sauvegarde terminée.");
    return true;
}

bool CFREngine::load_infoset_map(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        spdlog::warn("Impossible d'ouvrir le fichier de chargement : {}. Aucun infoset chargé.", filename);
        return false; // Pas forcément une erreur si le fichier n'existe pas au premier lancement
    }

    infoset_map_.clear(); // Vider la map actuelle avant de charger
    std::string line;
    long line_count = 0;
    long loaded_count = 0;

    spdlog::info("Chargement des infosets depuis {}...", filename);

    while (std::getline(infile, line)) {
        line_count++;
        bool parse_error = false; // Drapeau pour gérer les erreurs sans goto
        std::stringstream ss_line(line);
        std::string segment;
        std::vector<std::string> parts;

        // Séparer par le délimiteur principal ';'
        while (std::getline(ss_line, segment, ';')) {
            parts.push_back(segment);
        }

        if (parts.size() != 4) { // clé;visits;regrets;strategie
            spdlog::error("Erreur de format ligne {}: Nombre incorrect de segments ({}). Ligne: {}", line_count, parts.size(), line);
            continue; // Passer à la ligne suivante
        }

        InformationSet node;
        node.key = parts[0];

        // Parser visit_count
        try {
            node.visit_count = std::stoll(parts[1]); // Utiliser stoll pour long long
        } catch (const std::exception& e) {
            spdlog::error("Erreur de format ligne {}: Impossible de parser visit_count '{}'. Raison: {}. Ligne: {}", 
                          line_count, parts[1], e.what(), line);
            continue;
        }

        // Parser les regrets (séparés par ',')
        std::stringstream ss_regrets(parts[2]);
        std::string regret_val_str;
        while (std::getline(ss_regrets, regret_val_str, ',')) {
            try {
                node.cumulative_regrets.push_back(std::stod(regret_val_str)); // Utiliser stod pour double
            } catch (const std::exception& e) {
                spdlog::error("Erreur de format ligne {}: Impossible de parser la valeur de regret '{}'. Raison: {}. Ligne: {}", 
                              line_count, regret_val_str, e.what(), line);
                parse_error = true;
                break; // Sortir de la boucle de parsing des regrets
            }
        }

        if (parse_error) continue; // Passer à la ligne suivante si erreur dans les regrets

        // Parser la stratégie (séparée par ',')
        std::stringstream ss_strategy(parts[3]);
        std::string strategy_val_str;
        while (std::getline(ss_strategy, strategy_val_str, ',')) {
            try {
                node.cumulative_strategy.push_back(std::stod(strategy_val_str));
            } catch (const std::exception& e) {
                spdlog::error("Erreur de format ligne {}: Impossible de parser la valeur de stratégie '{}'. Raison: {}. Ligne: {}", 
                              line_count, strategy_val_str, e.what(), line);
                parse_error = true;
                break; // Sortir de la boucle de parsing de la stratégie
            }
        }

        if (parse_error) continue; // Passer à la ligne suivante si erreur dans la stratégie

        // Vérifier la cohérence des tailles (optionnel mais utile)
        if (node.cumulative_regrets.size() != node.cumulative_strategy.size()) {
             spdlog::error("Erreur de format ligne {}: Tailles incohérentes pour regrets ({}) et stratégie ({}). Ligne: {}", 
                           line_count, node.cumulative_regrets.size(), node.cumulative_strategy.size(), line);
             continue; // Passer à la ligne suivante
        }

        // Si tout s'est bien passé, ajouter à la map
        infoset_map_[node.key] = std::move(node); // Utiliser move pour efficacité
        loaded_count++;
    }

    infile.close();
    spdlog::info("Chargement terminé. {} infosets chargés depuis {} lignes.", loaded_count, line_count);
    return true;
}

} 