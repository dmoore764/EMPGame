HEADER_SEARCH_PATHS="-I/Users/danielmoore/Github/EMPGame/dependencies/include -I/Users/danielmoore/Github/EMPGame/dependencies/include/SDL"

clang++ -g -std=c++11 $HEADER_SEARCH_PATHS /Users/danielmoore/Github/EMPGame/preprocessor/preprocessor.cpp -o /Users/danielmoore/Github/EMPGame/build/preprocessor -framework CoreServices

DEBUG="true"
HEADER_SEARCH_PATHS="-I/Users/danielmoore/Github/EMPGame/dependencies/include -I/Users/danielmoore/Github/EMPGame/dependencies/include/SDL"
AUDIO_FRAMEWORK_SEARCH_PATHS="-L/Users/danielmoore/Github/EMPGame/dependencies/lib"

if [ "$DEBUG" = "true" ]; then

OPTIMIZATION="-O0"

else

OPTIMIZATION="-Os"

fi

../build/preprocessor

clang++ -g -O0 -std=c++11 /Users/danielmoore/Github/EMPGame/src/datapacker.cpp -o /Users/danielmoore/Github/EMPGame/build/datapacker

../build/datapacker NO_OUTPUT

clang++ -g -glldb $OPTIMIZATION -Wno-shift-negative-value -Wno-format-security -Wno-format -Wno-writable-strings -std=c++11 $HEADER_SEARCH_PATHS /Users/danielmoore/Github/EMPGame/src/main.cpp -o /Users/danielmoore/Github/EMPGame/build/game -framework CoreServices -framework OpenGL -framework OpenAL -lSDL2 -logg -lvorbis -lvorbisenc -lvorbisfile
