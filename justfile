init:
    mkdir build
build:
    cd build && cmake .. && make
clean:
    rm -r build