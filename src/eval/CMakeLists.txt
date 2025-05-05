# enable_language(C) # Ensure C compiler is enabled for c_tables.c - REMOVED

# Custom command to generate the tables C++ files
# add_custom_command(
#     OUTPUT ${GEN_HEADER} ${GEN_SOURCE}
#     COMMAND ${Python3_EXECUTABLE} ${GEN_SCRIPT} --repo-path ${REPO_PATH} --header ${GEN_HEADER} --source ${GEN_SOURCE}
#     # Depend on the script itself and potentially the C source files it reads
#     # Note: Depending on files outside the source tree can be tricky.
#     # Depending on the script is usually sufficient if repo content is stable.
#     DEPENDS ${GEN_SCRIPT} # ${REPO_PATH}/src/lu_tables.c ${REPO_PATH}/src/lu_tables.h 
#     COMMENT "Generating poker lookup tables C++/HPP from external C source..."
#     VERBATIM
#     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} # Ensure script runs from project root
# )

# Custom target to drive the generation
# add_custom_target(GenerateLookupTables ALL DEPENDS ${GEN_HEADER} ${GEN_SOURCE})

# Define the static library for hand evaluation
add_library(gto_eval STATIC
    hand_evaluator.cpp
    # Supprimer POKERLIB_SRC et MTRAND_SRC d'ici
    # ${POKERLIB_SRC}
    # ${MTRAND_SRC}
)

# Supprimer LANGUAGE C car pokerlib.cpp est maintenant dans sa propre cible
# set_source_files_properties(${POKERLIB_SRC} PROPERTIES LANGUAGE C)

# Assurer que HandRanks.dat est généré avant de compiler cette bibliothèque
# add_dependencies(gto_eval generate_handranks_dat) # Supprimer car HandRanks.dat n'est plus utilisé par gto_eval

# Ensure the library includes the necessary directories
target_include_directories(gto_eval PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}            # headers locaux (hand_evaluator.hpp)
    ${CMAKE_SOURCE_DIR}/src                # pour core/cards.hpp, core/bitboard.hpp
    ${POKERLIB_INCLUDE_DIR}                # Pour poker.h (nécessaire pour la déclaration extern C)
)

# Lier gto_eval à la nouvelle bibliothèque pokerlib
target_link_libraries(gto_eval PRIVATE pokerlib)

# Supprimer l'ancien message
# message(STATUS "Added target: gto_eval (static library, uses tables loaded from HandRanks.dat)")
message(STATUS "Added target: gto_eval (static library, uses pokerlib)")

# Optionnel: Exporter la cible pour une utilisation externe
# install(TARGETS gto_eval EXPORT gto_solver-targets ...)

# --- Ancienne partie pour générer HandRanks.dat ---
# find_package(Python3 COMPONENTS Interpreter REQUIRED)

# add_custom_command(
#     OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/HandRanks.dat
#     COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_table.py ${CMAKE_CURRENT_BINARY_DIR}/HandRanks.dat
#     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_table.py
#     COMMENT "Generating HandRanks.dat lookup table"
# )

# add_custom_target(generate_handranks_dat ALL
#     DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/HandRanks.dat
# )

# Définir une variable pointant vers le fichier généré
# set(HANDRANKS_DAT_FILE ${CMAKE_CURRENT_BINARY_DIR}/HandRanks.dat CACHE INTERNAL "Path to HandRanks.dat") 