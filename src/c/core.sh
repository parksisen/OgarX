emcc -O3 --llvm-opts "['-O3']" -s SIDE_MODULE=1 -mbulk-memory ./core.c -o ../../public/static/wasm/server.wasm