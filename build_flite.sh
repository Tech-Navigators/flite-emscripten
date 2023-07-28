#!/bin/sh
export set LDFLAGS='-s INITIAL_MEMORY=128MB'
export set CFLAGS='-s EXPORTED_RUNTIME_METHODS=ccall -s EXPORT_ES6 -s MODULARIZE -s EXPORT_NAME=flite -s EXPORT_ALL -s LINKABLE'

echo "
#
#
#  The base languages, lexicons and voices
LEXES += cmulex
LANGS += usenglish
VOXES += cmu_us_rms
" > config/default.lv

emconfigure ./configure --host=wasm32-wasi
emmake make clean
emmake make

cp bin/flite example/flite.mjs
cp bin/flite.wasm example
