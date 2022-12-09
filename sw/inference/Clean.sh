#!/bin/bash

/bin/rm -rf obj
/bin/rm -f  compile.log
/bin/rm -f  resized_*.png
/bin/rm -f  library/libutils.a
/bin/rm -f  library/libutils.so
/bin/rm -f  infer
/bin/rm -rf results

for F in *; do
    if [[ -d "${F}" && ! -L "${F}" ]]; then
    if [ -f ${F}/Clean.sh ]; then
       ( cd ${F}; ./Clean.sh )
    fi
    fi
done
