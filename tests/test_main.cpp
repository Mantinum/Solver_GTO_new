#define CATCH_CONFIG_RUNNER
#define CATCH_CONFIG_ENABLE_BENCHMARKING   // <- active les benchmarks
// #define CATCH_CONFIG_MAIN // Ne pas utiliser CATCH_CONFIG_MAIN, nous définissons notre propre main
#include <catch2/catch_session.hpp> // Pour Catch::Session
#include <catch2/catch_test_macros.hpp>
// Supprimé: #include <catch2/catch_reporter_registrars.hpp> 
#include <string>   // Pour std::string
#include "catch2/catch_all.hpp"
// #include "eval/hand_ranks_loader.hpp" // Supprimé
#include <spdlog/spdlog.h> // Pour spdlog
// #include "eval/hand_evaluator.hpp" // Pour initialize_evaluator (supprimé)

// Chemin vers le fichier handranks.dat (supprimé)
// #ifndef HANDRANKS_DAT_PATH
//     #define HANDRANKS_DAT_PATH "handranks.dat" 
// #endif

// Notre propre fonction main
int main( int argc, char* argv[] ) {
    // Charger les tables de rangs une seule fois au début
    // try {
    //     // load_hand_ranks charge les données et définit les pointeurs C (hash_values, hash_adjust)
    //     // gto_solver::load_hand_ranks(HANDRANKS_DAT_PATH); // Supprimé
    //     // spdlog::info("Tables de rangs chargées avec succès depuis {}", HANDRANKS_DAT_PATH);
        
    //     // Initialiser l'évaluateur (assigne HR aux données chargées)
    //     // gto_solver::initialize_evaluator(HANDRANKS_DAT_PATH); // Supprimé
    //     // spdlog::info("Évaluateur de mains initialisé.");

    // } catch (const std::exception& e) {
    //     spdlog::error("Erreur lors du chargement de {}: {}", HANDRANKS_DAT_PATH, e.what());
    //     return 1; // Échouer si les tables ne peuvent pas être chargées
    // }

    // Initialisation de Catch2
    Catch::Session session;

    // Lancer la session de tests Catch2
    int result = session.run( argc, argv );

    return result;
}

// Supprimé: Global Fixture et Listener

// Placeholder test case (peut être mis dans un autre fichier)
TEST_CASE("Basic check", "[skeleton]") {
    REQUIRE(1 == 1);
}

TEST_CASE("Skeleton Test") {
    REQUIRE(1 == 1);
} 