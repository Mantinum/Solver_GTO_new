# Add test executable
add_executable(gto_tests
    test_main.cpp
    cards_tests.cpp
    bitboard_tests.cpp
    eval_tests.cpp
    bench_eval.cpp
    # hand_evaluator_tests.cpp # <-- SUPPRIMÉ car fichier introuvable et eval_tests.cpp existe déjà
    action_abstraction_tests.cpp
    information_set_tests.cpp
)

# Définir le chemin vers HandRanks.dat comme une macro C++
# HANDRANKS_DAT_FILE est défini dans external/2p2/CMakeLists.txt
# if(DEFINED HANDRANKS_DAT_FILE)
#     target_compile_definitions(gto_tests PRIVATE "HANDRANKS_DAT_PATH=\"${HANDRANKS_DAT_FILE}\"")
# else()
#     message(FATAL_ERROR "Variable HANDRANKS_DAT_FILE is not defined. Ensure external/2p2 is included.")
# endif()

# Include directory for headers in src/
target_include_directories(gto_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/eval # Also need eval headers
    # ${CMAKE_BINARY_DIR}/generated # Include generated headers dir - Supprimé car non utilisé
    # Include Catch2 headers (FetchContent usually adds this automatically, but explicitly doesn't hurt)
    # ${catch2_SOURCE_DIR}/src - Supprimé car géré par FetchContent
)

# Link Catch2 using the target provided by FetchContent
# Lier aussi gto_core et gto_eval
target_link_libraries(gto_tests PRIVATE 
    gto_solver_lib          # <-- AJOUTÉ: Contient GameState et ActionAbstraction
    gto_eval                # Pour evaluator functions (contient eval_7hand)
    gto_core                # Nouvelle bibliothèque core (utilise eval_7hand)
    Catch2::Catch2WithMain # Target from FetchContent (provides main)
)

# Ensure test data is accessible when running tests
# Copy the data directory to the build directory where tests are run
# Note: This might not be the most robust way, alternatives exist (e.g., configure_file)
add_custom_command(TARGET gto_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/tests/data $<TARGET_FILE_DIR:gto_tests>/data
    COMMENT "Copying test data to build directory"
)

# Include CTest module
include(CTest)

# Include Catch module for test discovery
include(Catch)

# Discover tests in the executable
catch_discover_tests(gto_tests) 