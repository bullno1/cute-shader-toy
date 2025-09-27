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

# --- blibs ---
add_library(blibs INTERFACE)
target_include_directories(blibs INTERFACE blibs)

# --- byteshift_strstr ---
add_library(byteshift_strstr STATIC "byteshift_strstr/byteshift_memmem.c" "byteshift_strstr/byteshift_strstr.c")
target_include_directories(byteshift_strstr PUBLIC byteshift_strstr)
