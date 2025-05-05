#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include "eval/hand_evaluator.hpp"
#include "core/cards.hpp"
#include "core/bitboard.hpp"
#include <vector>
#include <random>
#include <array>
#include <algorithm> // Pour std::shuffle, std::copy_n
#include <numeric>   // Pour std::iota

using namespace gto_solver; // Ajout de l'espace de noms

// Helper pour obtenir un jeu de cartes standard
namespace {
    std::vector<Card> get_local_standard_deck() {
        std::vector<Card> deck(NUM_CARDS); // Utilisation de NUM_CARDS (supposé défini via bitboard.hpp)
        // Utiliser gto_solver::Card explicitement si using namespace n'est pas dans ce scope
        std::iota(deck.begin(), deck.end(), static_cast<Card>(0)); 
        return deck;
    }
} // namespace anonyme

TEST_CASE("Evaluate Performance", "[evaluator][!benchmark]") {
    std::vector<Card> deck = get_local_standard_deck();
    std::mt19937 rng(std::random_device{}());

    const int num_hands_to_eval = 10000;
    std::vector<std::array<Card, 7>> random_hands;
    random_hands.reserve(num_hands_to_eval);

    for(int i = 0; i < num_hands_to_eval; ++i) {
        std::shuffle(deck.begin(), deck.end(), rng);
        std::array<Card, 7> hand;
        std::copy_n(deck.begin(), 7, hand.begin());
        random_hands.push_back(hand);
    }

    BENCHMARK("Evaluate 10k Hands (7 Cards, Array)") {
        volatile HandRank rank_sink = INVALID_HAND_RANK; // Pas de préfixe
        for(const auto& hand : random_hands) {
            rank_sink = evaluate_hand_7_card(hand); // Pas de préfixe
        }
        return rank_sink;
    };

    BENCHMARK("Evaluate 10k Hands (7 Cards, Bitboard)") {
         volatile HandRank rank_sink = INVALID_HAND_RANK; // Pas de préfixe

         for(const auto& hand_array : random_hands) {
             Bitboard mask = 0ULL; // Pas de préfixe
             // Construction du masque (identique à cards_to_board mais inline)
             for(Card c : hand_array) { // Pas de préfixe
                // Ajouter une vérification de validité de la carte si nécessaire
                 if (c < static_cast<Card>(0) || c >= static_cast<Card>(NUM_CARDS)) continue; // Utilisation de NUM_CARDS
                // Pas besoin de {1} pour Bitboard si c'est uint64_t
                 mask |= (Bitboard{1} << static_cast<int>(c)); // Pas de préfixe
             }
             // Évaluer seulement si on a 7 cartes uniques (le shuffle garantit normalement ça)
             if (count_set_bits(mask) == 7) { // Pas de préfixe
                rank_sink = evaluate_hand_7_card(mask); // Pas de préfixe
             }
         }
         return rank_sink;
    };

} 