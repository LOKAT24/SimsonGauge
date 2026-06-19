# # Plik: components/UI/CMakeLists.txt

# # Definiujemy ścieżkę do plików generowanych przez SquareLine
# set(SLS_DIR "sls_generated")

# # Definiujemy listę wszystkich plików źródłowych (.c) w podkatalogu
# set(SOURCES 
#     "${SLS_DIR}/screens/ui_Screen1.c"
#     "${SLS_DIR}/screens/ui_Screen2.c"
#     "${SLS_DIR}/ui.c"
#     "${SLS_DIR}/components/ui_comp_hook.c"
#     "${SLS_DIR}/ui_helpers.c"
#     "${SLS_DIR}/images/ui_img_mazdalogoodszumione_png.c"
#     # "${SLS_DIR}/images/ui_img_nfsu2_gauge_png.c"
# )


# # Rejestrujemy komponent
# idf_component_register(
#     SRCS "${SOURCES}"
#     # Dodajemy katalogi z plikami nagłówkowymi (.h) z podkatalogu
#     INCLUDE_DIRS "." "${SLS_DIR}" "${SLS_DIR}/screens" "${SLS_DIR}/components" "${SLS_DIR}/images"
#     REQUIRES "lvgl"
# )

# Plik: components/UI/CMakeLists.txt

# Określ ścieżkę do plików generowanych przez SquareLine Studio
set(SLS_GENERATED_DIR "sls_generated")

# Ta komenda automatycznie znajdzie WSZYSTKIE pliki .c
# we wszystkich podkatalogach wewnątrz folderu sls_generated.
file(GLOB_RECURSE UI_SOURCES "${SLS_GENERATED_DIR}/*.c")
# i cpp
file(GLOB_RECURSE UI_SOURCES_CPP "${SLS_GENERATED_DIR}/*.cpp")

# Zarejestruj komponent
idf_component_register(
    # Przekaż listę automatycznie znalezionych plików
    SRCS "${UI_SOURCES}"
            "${UI_SOURCES_CPP}"

    # Dodaj katalogi z plikami nagłówkowymi (.h) do ścieżki include.
    # Dzięki temu kompilator znajdzie ui.h, ui_helpers.h, itp.
    # Ustawiamy tutaj główny folder generowanych plików.
    INCLUDE_DIRS "." "${SLS_GENERATED_DIR}"

    # Określ, że ten komponent wymaga biblioteki LVGL
    REQUIRES "lvgl"
)