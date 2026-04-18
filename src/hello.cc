#include "common.h"

extern "C" void go(char * args, int length) {
   datap  parser;
   char * str_arg;
   int    num_arg;

   formatp buffer;
   BeaconFormatAlloc(&buffer,1024);
   
   BeaconDataParse(&parser, args, length);
   str_arg = BeaconDataExtract(&parser, NULL);
   num_arg = BeaconDataInt(&parser);

   
   // BeaconPrintf(CALLBACK_OUTPUT, "Message is %s with %d arg", str_arg, num_arg);
   FPRINT(&buffer, "[+] BOF compiled with clang-cl: %04d-%02d-%02d %02d:%02d:%02d\n", BUILD_YEAR, BUILD_MONTH, BUILD_DAY, BUILD_HOUR, BUILD_MIN, BUILD_SEC);
   FPRINT(&buffer, "[+] CLI Args are: %s, %d", str_arg, num_arg);
   
   FLUSHFREE(&buffer);
}