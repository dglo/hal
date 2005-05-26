#include <stdio.h>

#include "base64.h"

int main(int argc, char *argv[]) {
   while (!feof(stdin)) {
      char in[4096];
      char out[8172];
      const int nr = fread(in, 1, sizeof(in), stdin);
      const int ne = base64EncodeLength(nr);

      if (ne>0) {
         base64EncodeBuffer(out, in, nr);
         fwrite(out, 1, ne, stdout);
      }
   }

   return 0;
}
