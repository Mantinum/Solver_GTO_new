#include "gto/cfr_engine.h"
#include "spdlog/spdlog.h"

namespace gto_solver {

CFREngine::CFREngine() {
    spdlog::warn("CFREngine constructor - Placeholder implementation");
}

CFREngine::~CFREngine() {}

void CFREngine::run_iterations(int num_iterations) {
    spdlog::warn("CFREngine::run_iterations - Placeholder implementation");
    for (int i = 0; i < num_iterations; ++i) {
        // Logique d'une itération CFR
        // - Traverser l'arbre
        // - Mettre à jour les regrets
        // - Mettre à jour la stratégie
    }
}

} 