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
        int final_pot_size = current_state.get_pot_size();
        int p0_investment = current_state.get_player_total_investment_this_hand(0);
        int p1_investment = (current_state.get_num_players() > 1) ? current_state.get_player_total_investment_this_hand(1) : 0;

        bool p0_active = !current_state.is_player_folded(0);
        bool p1_active = (current_state.get_num_players() > 1) ? !current_state.is_player_folded(1) : false;

        if (current_state.get_num_players() == 2) { // Logique pour 2 joueurs
            if (p0_active && !p1_active) { // P1 a foldé, P0 gagne le pot
                // P0 gagne ce que P1 a investi.
                p0_utility = static_cast<double>(p1_investment);
                spdlog::trace("CFR Terminal: P1 folded. P0 wins pot. P0 utility (P1 investment): {}", p0_utility);
            } else if (!p0_active && p1_active) { // P0 a foldé, P1 gagne le pot
                // P0 perd ce que P0 a investi.
                p0_utility = -static_cast<double>(p0_investment);
                spdlog::trace("CFR Terminal: P0 folded. P1 wins pot. P0 utility (-P0 investment): {}", p0_utility);
            } else if (p0_active && p1_active) { // Showdown entre P0 et P1
                spdlog::debug("CFR Terminal: Showdown P0 vs P1. Board cards: {}", current_state.get_board_cards_dealt());
                if (current_state.get_board_cards_dealt() == 5) { // Board complet
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
                        } else if (rank_p0 < rank_p1) { // P0 gagne
                            p0_utility = static_cast<double>(p1_investment); // P0 gagne l'investissement de P1
                        } else if (rank_p1 < rank_p0) { // P1 gagne
                            p0_utility = -static_cast<double>(p0_investment); // P0 perd son investissement
                        } else { // Égalité
                            // P0 récupère son investissement, P1 récupère le sien. Gain net de P0 = 0.
                            p0_utility = 0.0;
                        }
                        spdlog::trace("CFR Showdown: P0 rank {}, P1 rank {}. P0 utility (net): {}", rank_p0, rank_p1, p0_utility);
                    } else {
                        spdlog::error("CFR Showdown: Incorrect card counts for eval. P0 hand: {}, P1 hand: {}, Board: {}. State:\n{}", 
                                      p0_hand_vec.size(), p1_hand_vec.size(), board_vec.size(), current_state.toString());
                        p0_utility = 0.0; // Erreur
                    }
                } else { // Cas où le board n'a PAS 5 cartes (showdown au Flop ou au Turn) -> Calcul d'équité
                    spdlog::debug("CFR Showdown: Board non complet ({} cartes). Calcul d'équité.", current_state.get_board_cards_dealt());
                    const auto& p0_hand_cards = current_state.get_player_hand(0);
                    const auto& p1_hand_cards = current_state.get_player_hand(1);
                    
                    if (p0_hand_cards.size() != 2 || p1_hand_cards.size() != 2) {
                        spdlog::error("CFR Equity Calc Prereq: Incorrect player hand size. P0: {}, P1: {}. State:\n{}", 
                                      p0_hand_cards.size(), p1_hand_cards.size(), current_state.toString());
                        p0_utility = 0.0; // Erreur
                    } else {
                        std::vector<Card> board_cards_for_equity = current_state.get_board_vector();
                        std::vector<Card> deck_for_equity = current_state.get_remaining_deck_cards();
                        // Pour calculate_equity, le pot_size argument est le pot *total* disputé.
                        // L'utilité retournée par calculate_equity est déjà le gain net moyen pour P0.
                        // Exemple: si P0 a 60% d'équité sur un pot de 100, et que P0 et P1 ont chacun mis 50.
                        // P0 s'attend à gagner 0.6 * 100 = 60. Son gain net est 60 - 50 = 10.
                        // calculate_equity(..., pot_size=100) devrait retourner 10.
                        // Si calculate_equity retourne +/- pot_halved, alors p0_utility est directement cette valeur.
                        // La version actuelle de calculate_equity retourne (p0_wins - p1_wins) * pot_halved / total_runouts.
                        // Ce qui est déjà l'EV net pour P0 si pot_halved est (investissement_P0+investissement_P1)/2.
                        // Ici, p0_investment et p1_investment sont les investissements totaux.
                        // Le pot total est p0_investment + p1_investment (si pas d'argent mort avant).
                        // L'appel original à calculate_equity utilisait current_state.get_pot_size().
                        // Il faut s'assurer que l'utilité retournée par calculate_equity est cohérente avec gain/perte net.
                        // calculate_equity reçoit le pot total qui sera joué. Elle retourne l'EV de P0 sur ce pot total
                        // par rapport à un partage équitable de ce pot (donc +/- EV(pot_total)/2 si HU)
                        // Donc, l'utilité retournée est déjà l'EV nette de P0, en supposant une contribution équitable au pot disputé.
                        // Si le pot_size passé à calculate_equity est `final_pot_size` (p0_inv + p1_inv),
                        // et que calculate_equity retourne `valeur_attendue_P0_du_pot_total - contribution_P0_au_pot_total`,
                        // alors c'est bon. 
                        // La version actuelle de calculate_equity: (p0_wins - p1_wins) * (pot_size_arg / 2.0) / total_runouts.
                        // Donc si on lui passe final_pot_size, elle calcule bien l'EV nette de P0.
                        p0_utility = this->calculate_equity(p0_hand_cards, p1_hand_cards, board_cards_for_equity, deck_for_equity, final_pot_size);
                        spdlog::trace("CFR Equity Calc Result for P0 (net): {:.4f}", p0_utility);
                    }
                }
            } else { // Cas imprévu à 2 joueurs
                 spdlog::warn("CFR Terminal: État ambigu pour 2 joueurs (P0 active: {}, P1 active: {}). Pot: {}. Utility = 0.", p0_active, p1_active, final_pot_size);
                 p0_utility = 0.0;
            }
        } else { // Plus de 2 joueurs ou moins de 2 (ne devrait pas arriver pour un jeu standard)
            // TODO: Implémenter pour N joueurs.
            // Pour l'instant, si P0 a foldé et num_players > 2, son utilité est -p0_investment.
            if (!p0_active) { // P0 a foldé
                p0_utility = -static_cast<double>(p0_investment);
            } else {
                 // P0 est actif, mais situation N-joueurs non gérée pour le showdown/equity.
                spdlog::warn("CFR Terminal: Calcul d'utilité non implémenté pour {} joueurs actifs (P0 actif). Utility = 0.", current_state.get_num_players());
                p0_utility = 0.0;
            }
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
        
        // Log de l'action seulement si le niveau de log est suffisant
        spdlog::debug("CFR Traverse: P{} action {} ({}). Key: {}", 
                      current_player, i, gto_solver::action_to_string(action), infoset_key);

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

        // Format: cle<TAB>visit_count<TAB>regret1,regret2,...<TAB>strat1,strat2,...
        outfile << key << "\t" << node.visit_count << "\t";

        // Écrire les regrets cumulés
        for (size_t i = 0; i < node.cumulative_regrets.size(); ++i) {
            outfile << node.cumulative_regrets[i] << (i == node.cumulative_regrets.size() - 1 ? "" : ",");
        }
        outfile << "\t";

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

        // Séparer par le délimiteur principal '\t'
        while (std::getline(ss_line, segment, '\t')) {
            parts.push_back(segment);
        }

        if (parts.size() != 4) { // clé<TAB>visits<TAB>regrets<TAB>strategie
            spdlog::error("Erreur de format ligne {}: Nombre incorrect de segments ({} attendus 4). Ligne: {}", line_count, parts.size(), line);
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

double CFREngine::calculate_equity(
    const std::vector<Card>& p0_hand,             // Main de P0
    const std::vector<Card>& p1_hand,             // Main de P1
    const std::vector<Card>& current_board_cards, // Cartes déjà sur le board
    const std::vector<Card>& remaining_deck_cards,  // Cartes restantes dans le deck
    int pot_size                                // Taille du pot
) {
    const int num_board_cards = current_board_cards.size();
    const double pot_halved = static_cast<double>(pot_size) / 2.0;

    if (p0_hand.size() != 2 || p1_hand.size() != 2) {
        spdlog::error("CFREngine::calculate_equity: Les mains des joueurs doivent avoir 2 cartes.");
        return 0.0; // Ou gérer l'erreur autrement
    }

    long p0_wins = 0;
    long p1_wins = 0;
    long ties = 0;
    long total_runouts = 0;

    std::vector<Card> actual_board_cards = current_board_cards; // Utiliser directement l'argument

    if (num_board_cards == 5) { // Showdown avec board complet
        HandRank p0_rank = gto_solver::evaluate_hand_7_card(p0_hand[0], p0_hand[1], actual_board_cards);
        HandRank p1_rank = gto_solver::evaluate_hand_7_card(p1_hand[0], p1_hand[1], actual_board_cards);
        if (p0_rank < p1_rank) return pot_halved;  // P0 a un meilleur rang (plus bas), P0 gagne
        if (p1_rank < p0_rank) return -pot_halved; // P1 a un meilleur rang (plus bas), P1 gagne
        return 0.0; // Égalité
    } else if (num_board_cards == 4) { // Turn -> River (tirer 1 carte)
        if (remaining_deck_cards.empty()) {
            spdlog::warn("CFREngine::calculate_equity: Deck vide au turn, impossible de tirer la river.");
            // Cela ne devrait pas arriver si le jeu est configuré correctement.
            // Retourner une valeur neutre ou basée sur l'équité actuelle ? Pour l'instant, 0.
            return 0.0; 
        }
        for (size_t i = 0; i < remaining_deck_cards.size(); ++i) {
            std::vector<Card> final_board = actual_board_cards;
            final_board.push_back(remaining_deck_cards[i]);
            // TODO: S'assurer que deck[i] n'est pas déjà dans p0_hand, p1_hand, actual_board_cards
            //       Normalement, get_remaining_deck_cards() devrait garantir cela.

            HandRank p0_rank = gto_solver::evaluate_hand_7_card(p0_hand[0], p0_hand[1], final_board);
            HandRank p1_rank = gto_solver::evaluate_hand_7_card(p1_hand[0], p1_hand[1], final_board);

            if (p0_rank < p1_rank) p0_wins++;
            else if (p1_rank < p0_rank) p1_wins++;
            else ties++;
            total_runouts++;
        }
    } else if (num_board_cards == 3) { // Flop -> Turn & River (tirer 2 cartes)
        if (remaining_deck_cards.size() < 2) {
            spdlog::warn("CFREngine::calculate_equity: Deck avec <2 cartes au flop, impossible de tirer turn/river.");
            return 0.0;
        }
        for (size_t i = 0; i < remaining_deck_cards.size(); ++i) {
            for (size_t j = i + 1; j < remaining_deck_cards.size(); ++j) {
                std::vector<Card> final_board = actual_board_cards;
                final_board.push_back(remaining_deck_cards[i]);
                final_board.push_back(remaining_deck_cards[j]);

                HandRank p0_rank = gto_solver::evaluate_hand_7_card(p0_hand[0], p0_hand[1], final_board);
                HandRank p1_rank = gto_solver::evaluate_hand_7_card(p1_hand[0], p1_hand[1], final_board);

                if (p0_rank < p1_rank) p0_wins++;
                else if (p1_rank < p0_rank) p1_wins++;
                else ties++;
                total_runouts++;
            }
        }
    } else {
        spdlog::error("CFREngine::calculate_equity: Non implémenté pour {} cartes sur le board.", num_board_cards);
        return 0.0; // Cas non géré (Preflop)
    }

    if (total_runouts == 0) {
        spdlog::warn("CFREngine::calculate_equity: Aucun runout possible ? Mains: P0[{},{}], P1[{},{}], Board({}): {}, Deck size: {}", 
            to_string(p0_hand[0]), to_string(p0_hand[1]), to_string(p1_hand[0]), to_string(p1_hand[1]), 
            num_board_cards, vec_to_string(actual_board_cards), remaining_deck_cards.size());
        return 0.0; // Éviter division par zéro, situation anormale
    }

    double p0_net_utility = static_cast<double>(p0_wins - p1_wins) * pot_halved;
    return p0_net_utility / static_cast<double>(total_runouts);
}

} 