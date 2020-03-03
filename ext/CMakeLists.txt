set(ASSIMP_BUILD_TESTS        OFF CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL " " FORCE)
# disable a bunch of importers that we wont use. Just add
# them to the list if you want it enabled again
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL " " FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER     ON CACHE BOOL "Enable format OBJ"     FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER     ON CACHE BOOL "Enable format FBX"     FORCE)
set(ASSIMP_BUILD_3DS_IMPORTER     ON CACHE BOOL "Enable format 3DS"     FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER     ON CACHE BOOL "Enable format PLY"     FORCE)
set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "Enable format COLLADA" FORCE)
set(ASSIMP_BUILD_STL_IMPORTER     ON CACHE BOOL "Enable format STL"     FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER    ON CACHE BOOL "Enable format GLTF"    FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL " " FORCE)
# assimp doesnt compile if all exporters are disabled -.-
set(ASSIMP_BUILD_OBJ_EXPORTER    ON CACHE BOOL "Enable format OBJ"    FORCE)

# disable all warnings
add_definitions(-w)

add_subdirectory(assimp)