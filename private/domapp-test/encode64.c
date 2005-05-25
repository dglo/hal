#include <stdio.h>

#include "base64.h"

int main(int argc, char *argv[]) {
   while (!feof(stdin)) {
      char in[4096];
      char out[8172];
      int nr = fread(in, 1, sizeof(in), stdin);

      if (nr>0) {
         int ne = base64EncodeBuffer(out, in, nr);
         if (ne>0) fwrite(out, 1, ne, stdout);
      }
   }

   return 0;
}
