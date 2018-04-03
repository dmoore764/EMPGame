HEADER_SEARCH_PATHS="-I/Users/danielmoore/XCodeProjects/ninja-game/dependencies/include -I/Users/danielmoore/XCodeProjects/ninja-game/dependencies/include/SDL"

clang++ -g -std=c++11 $HEADER_SEARCH_PATHS /Users/danielmoore/XCodeProjects/ninja-game/preprocessor/preprocessor.cpp -o /Users/danielmoore/XCodeProjects/ninja-game/build/preprocessor -framework CoreServices
