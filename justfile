run *args="--debug": build
  ./build/lodge {{args}}

build:
  ninja -C build
