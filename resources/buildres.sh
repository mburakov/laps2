#!/bin/bash

echo '#include <cstdint>' > ../resources.h
echo >> ../resources.h

for item in *.bin
do
  echo "$item" | sed 's/^/const uint8_t /;s/\.bin/[] = {/'
  hexdump -C "$item" | sed 's/  |.*//;s/ \+/ /g;s/........//;s/ /, 0x/g;s/^, /  /g;s/$/,/;s/, 0x,$//'
done | sed 's/^,$/};\n/' >> ../resources.h
