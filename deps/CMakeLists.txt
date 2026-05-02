# --- cute_framework ---
set(CF_FRAMEWORK_STATIC ON CACHE BOOL "")
set(CF_FRAMEWORK_BUILD_SAMPLES OFF CACHE BOOL "")
set(CF_FRAMEWORK_BUILD_TESTS OFF CACHE BOOL "")

add_subdirectory("cute_framework" EXCLUDE_FROM_ALL)

# Make the include path system so that targets linking to cute framework can
# use a stricter warning settings.
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/include")
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/libraries")
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/libraries/cimgui")
target_include_directories(cute SYSTEM PUBLIC BEFORE "cute_framework/libraries/cimgui/imgui")

get_target_property(S2N_OPTS s2n COMPILE_OPTIONS)
list(REMOVE_ITEM S2N_OPTS "-std=gnu99")
set_target_properties(s2n PROPERTIES COMPILE_OPTIONS "${S2N_OPTS}")

set(PHYSFS_TARGET physfs-static)
set_target_properties("${PHYSFS_TARGET}" PROPERTIES
	C_STANDARD 99
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS OFF
)
target_compile_definitions("${PHYSFS_TARGET}" PRIVATE _POSIX_C_SOURCE=200112L)

# --- blibs ---
add_library(blibs INTERFACE)
target_include_directories(blibs INTERFACE blibs)

# --- byteshift_strstr ---
add_library(byteshift_strstr STATIC "byteshift_strstr/byteshift_memmem.c" "byteshift_strstr/byteshift_strstr.c")
target_include_directories(byteshift_strstr PUBLIC byteshift_strstr)
target_compile_definitions(byteshift_strstr PRIVATE _BSD_SOURCE)
