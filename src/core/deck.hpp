#ifndef GTO_CORE_DECK_HPP
#define GTO_CORE_DECK_HPP

#include "core/cards.hpp"
#include <vector>
#include <random>
#include <algorithm> // Pour std::shuffle
#include <stdexcept> // Pour std::runtime_error

namespace gto_solver {

class Deck {
public:
    Deck();
    ~Deck() = default;

    Card deal_card();
    void burn_card();
    void shuffle();
    void reset();

    void initialize() {
        cards_.clear();
        cards_.reserve(52);
        for (int s = 0; s < 4; ++s) {
            for (int r = 0; r < 13; ++r) {
                Card card = make_card(static_cast<Rank>(r), static_cast<Suit>(s));
                cards_.push_back(card);
            }
        }
        next_card_index_ = 0;
        shuffled_ = false;
    }

    // Peut-être ajouter une méthode pour retirer des cartes spécifiques (dead cards)
    // void remove_card(const Card& card);

private:
    std::vector<Card> cards_;
    size_t            next_card_index_;
    bool shuffled_ = false;
    std::mt19937      rng_;
};

} // namespace gto_solver

#endif // GTO_CORE_DECK_HPP 