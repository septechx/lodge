run *args="--debug": build
  ./build/lodge {{args}}

build:
  ninja -C build

test:
  meson test -C build --verbose
