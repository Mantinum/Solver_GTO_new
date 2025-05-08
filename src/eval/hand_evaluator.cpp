// ─────────────────────────────────────────────────────────────────────────────
//  src/eval/hand_evaluator.cpp
//  Implémentation (temporaire) de l'évaluateur 7-cartes.
//  Les rangs retournés sont encore fictifs (TODO), mais l'API est complète
//  et satisfait le linkage des tests.
//
//  NB : les vraies tables de lookup (hash_values / hash_adjust) sont
//  définies dans external/2p2/pokerlib.cpp et simplement référencées ici.
// ─────────────────────────────────────────────────────────────────────────────
#include "hand_evaluator.hpp"
#include "core/bitboard.hpp"
#include "core/cards.hpp"
#include <stdexcept>                // Pour std::runtime_error
#include <iostream>                 // Pour std::cerr
#include <array>                    // Pour std::array
#include <numeric>                  // Pour std::accumulate (potentiellement)
#include <vector>
#include <atomic> // Pour std::atomic_flag
#include <mutex> // Pour std::mutex et std::lock_guard
#include <cstdio> // Pour fprintf
#include <algorithm>    // Pour std::transform
#include <bit>          // Pour std::popcount

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace gto_solver {

// --- Suppression des Variables globales, Initialisation, Offsets --- 
// const uint16_t* HR = nullptr; 
// unsigned short* hash_values = nullptr;
// unsigned short* hash_adjust = nullptr;
// std::atomic_flag evaluator_initialized = ATOMIC_FLAG_INIT;
// std::mutex init_mutex;
// constexpr size_t HR_SIZE = ...;
// ... etc ...
// void initialize_evaluator(...) { ... } 

// --- Déclaration de la fonction C --- 
extern "C" short eval_7hand(int* hand);

// --- Fonction utilitaire pour convertir Card -> int 2p2 --- 
int card_to_2p2_int(const Card& card) {
    static const int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41}; // Primes pour 2..A
    if (card == INVALID_CARD) {
        // eval_7hand ne gère probablement pas les cartes invalides.
        // Retourner 0 ou une autre valeur pourrait fausser l'évaluation.
        // Lancer une exception est plus sûr pour indiquer une erreur de programmation.
        throw std::invalid_argument("Cannot convert INVALID_CARD to 2p2 int format.");
    }
    int rank_int = static_cast<int>(get_rank(card)); // 0=2, ..., 12=A
    int suit_mask;
    // IMPORTANT: Les masques correspondent à c=0x8000, d=0x4000, h=0x2000, s=0x1000
    // Notre enum Suit est { SPADES, HEARTS, DIAMONDS, CLUBS } (0, 1, 2, 3)
    switch (get_suit(card)) {
        case Suit::CLUBS:    suit_mask = 0x8000; break; // 3 -> 0x8000
        case Suit::DIAMONDS: suit_mask = 0x4000; break; // 2 -> 0x4000
        case Suit::HEARTS:   suit_mask = 0x2000; break; // 1 -> 0x2000
        case Suit::SPADES:   suit_mask = 0x1000; break; // 0 -> 0x1000
        default: throw std::logic_error("Invalid suit in card_to_2p2_int"); // Ne devrait jamais arriver
    }
    int prime = primes[rank_int];
    int rank_bit_mask = 1 << (16 + rank_int);

    return prime | (rank_int << 8) | suit_mask | rank_bit_mask;
}

// --- Évaluation --- 

// Implémentation pour la surcharge avec Bitboard
HandRank evaluate_hand_7_card(Bitboard seven_card_mask) {
    // Vérifier le nombre de cartes AVANT de tenter la conversion/évaluation
    if (std::popcount(seven_card_mask) != 7) {
         // L'attente du test 10 est de retourner INVALID_HAND_RANK, pas de lancer une exception.
         // throw std::invalid_argument("Bitboard must contain exactly 7 cards.");
         return INVALID_HAND_RANK; // Retourner 0 comme attendu par les tests
    }
    std::array<int, 7> hand_int;
    int count = 0;
    Bitboard temp_mask = seven_card_mask;
    while(temp_mask > 0 && count < 7) {
        Card card = pop_lsb(temp_mask); // pop_lsb retourne l'index 0-51
        // Vérifier si la carte est invalide APRES pop_lsb (sécurité, ne devrait pas arriver si popcount = 7)
        if (card == INVALID_CARD) { 
            // Ne devrait pas se produire si popcount est correct
             return INVALID_HAND_RANK; 
        }
        hand_int[count++] = card_to_2p2_int(card); // card_to_2p2_int lance une exception si card==INVALID_CARD
    }
     if (count != 7) {
         // Safety check, should not happen if popcount was 7
         // throw std::runtime_error("Internal error extracting cards from bitboard.");
         return INVALID_HAND_RANK;
    }

    short rank_value = eval_7hand(hand_int.data());
    return static_cast<HandRank>(rank_value);
}

// Implémentation pour la surcharge avec 2 cartes privées + 5 board
HandRank evaluate_hand_7_card(Card c1, Card c2, const std::vector<Card>& board) {
    // Vérifier la taille du board
    if (board.size() != 5) {
        // L'attente du test 10 est INVALID_HAND_RANK (0), pas une exception.
        // throw std::invalid_argument("Board must contain exactly 5 cards."); 
        return INVALID_HAND_RANK;
    }
    
    // Vérifier les cartes invalides et les doublons AVANT la conversion
    Bitboard mask = EMPTY_BOARD;
    if (c1 >= INVALID_CARD) return INVALID_HAND_RANK;
    set_card(mask, c1);
    if (c2 >= INVALID_CARD || test_card(mask, c2)) return INVALID_HAND_RANK;
    set_card(mask, c2);
    for(const auto& card : board) {
        if (card >= INVALID_CARD || test_card(mask, card)) return INVALID_HAND_RANK;
        set_card(mask, card);
    }
    // Si on arrive ici, on a 7 cartes valides et uniques

    std::array<int, 7> hand_int;
    hand_int[0] = card_to_2p2_int(c1);
    hand_int[1] = card_to_2p2_int(c2);
    std::transform(board.begin(), board.end(), hand_int.begin() + 2, card_to_2p2_int);

    short rank_value = eval_7hand(hand_int.data());
    return static_cast<HandRank>(rank_value);
}

// Implémentation pour la surcharge avec un std::array<Card, 7>
HandRank evaluate_hand_7_card(const std::array<Card, 7>& cards) {
    // Vérifier les cartes invalides et les doublons AVANT la conversion
    Bitboard mask = EMPTY_BOARD;
    for(const auto& card : cards) {
        if (card >= INVALID_CARD || test_card(mask, card)) return INVALID_HAND_RANK;
        set_card(mask, card);
    }
    // Si on arrive ici, on a 7 cartes valides et uniques

    std::array<int, 7> hand_int;
    std::transform(cards.begin(), cards.end(), hand_int.begin(), card_to_2p2_int);

    short rank_value = eval_7hand(hand_int.data());
    return static_cast<HandRank>(rank_value);
}

// --- Implémentation de l'évaluation (basée sur l'algo HR/2p2) ---

// --- Implémentations des fonctions utilitaires (placeholders) ---
std::string hand_rank_to_string(HandRank rank) {
    // Cette implémentation dépendra de la signification exacte des valeurs retournées
    // par eval_7hand et comment elles correspondent à HandRank.
    // Les valeurs typiques sont 1 (Royal Flush) à 7462 (7-5-4-3-2 offsuit).
    // Voir: http://suffe.cool/poker/evaluator.html
    if (rank == INVALID_HAND_RANK) return "Invalid Rank";
    if (rank == 1) return "Royal Flush"; // Typiquement 1, mais à vérifier
    if (rank <= 10) return "Straight Flush"; // 1 à 10
    if (rank <= 166) return "Four of a Kind"; // 11 à 166
    if (rank <= 322) return "Full House";     // 167 à 322
    if (rank <= 1599) return "Flush";         // 323 à 1599
    if (rank <= 1609) return "Straight";      // 1600 à 1609
    if (rank <= 2467) return "Three of a Kind";// 1610 à 2467
    if (rank <= 3325) return "Two Pair";      // 2468 à 3325
    if (rank <= 6185) return "One Pair";      // 3326 à 6185
    if (rank <= 7462) return "High Card";     // 6186 à 7462

    return "Unknown Rank (" + std::to_string(rank) + ")";
}

std::string hand_type_to_string(HandRank rank) {
    // Similaire à hand_rank_to_string, mais retourne juste la catégorie
    if (rank == INVALID_HAND_RANK) return "Invalid";
    if (rank <= 10) return "Straight Flush";
    if (rank <= 166) return "Four of a Kind";
    if (rank <= 322) return "Full House";
    if (rank <= 1599) return "Flush";
    if (rank <= 1609) return "Straight";
    if (rank <= 2467) return "Three of a Kind";
    if (rank <= 3325) return "Two Pair";
    if (rank <= 6185) return "One Pair";
    if (rank <= 7462) return "High Card";
    return "Unknown";
}

} // namespace gto_solver
 