//
// Desktop test
// Reads from STDIN, outputs to STDOUT
//

#ifndef ARDUINO

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cstdint>

#include "MYLYTSJA.ino"

int main(int argc, char *argv[])
{
    while(1)
    {
      char c = getchar(); // getch() ???

      if (c<0)
        break;

      char_received(c);
    }

    return 0;
}

#endif // !ARDUINO