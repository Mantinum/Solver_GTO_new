#ifndef GTO_COMMON_TYPES_H
#define GTO_COMMON_TYPES_H

namespace gto_solver {

// Enum pour les positions
enum class Position {
    BTN,    // Button
    SB,     // Small Blind
    BB,     // Big Blind
    UTG,    // Under the Gun
    MP,     // Middle Position
    CO,     // Cutoff
    INVALID // Position invalide
};

// Enum pour les tours de jeu
enum class Street {
    PREFLOP,
    FLOP,
    TURN,
    RIVER,
    SHOWDOWN
};

// Types d'action possibles
enum class ActionType {
    FOLD,
    CALL,
    RAISE
};

// Structure pour représenter une action concrète
struct Action {
    int player_index = -1;
    ActionType type = ActionType::FOLD;
    int amount = 0;

    bool operator<(const Action& other) const {
        if (player_index != other.player_index) return player_index < other.player_index;
        if (type != other.type) return static_cast<int>(type) < static_cast<int>(other.type);
        return amount < other.amount;
    }

    bool operator==(const Action& other) const {
        return player_index == other.player_index &&
               type == other.type &&
               amount == other.amount;
    }
};

// Fonction utilitaire pour convertir une position en string
inline const char* position_to_string(Position pos) {
    switch (pos) {
        case Position::BTN: return "BTN";
        case Position::SB: return "SB";
        case Position::BB: return "BB";
        case Position::UTG: return "UTG";
        case Position::MP: return "MP";
        case Position::CO: return "CO";
        default: return "INVALID";
    }
}

} // namespace gto_solver

#endif // GTO_COMMON_TYPES_H 