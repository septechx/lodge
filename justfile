run *args: build
  ./build/lodge {{args}}

run-dev: (run "--debug")

build:
  ninja -C build

test:
  meson test -C build --verbose
