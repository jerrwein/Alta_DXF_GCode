Code to generate G-Code from DXF drawing.


Possible Issues:

$> make
...
Invoking: GCC C++ Compiler
g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"src/Main.d" -MT"src/Main.d" -o "src/Main.o" "src/Main.cpp"
src/Main.cpp:21:10: fatal error: GL/freeglut.h: No such file or directory
   21 | #include "GL/freeglut.h"

Ans: $> sudo add-apt-repository universe (optional)
     $> sudo apt-get update
     $> sudo apt install freeglut3-dev
$> make

  => Success

  
