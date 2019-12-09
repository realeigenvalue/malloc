# malloc

A custom memory allocator implementation of malloc.

![Application Image](malloc.png)

## Building

Prerequisites
- GCC
- Clang
- Make

```bash
sudo apt-get update && sudo apt-get install clang-5.0 libc++abi-dev libc++-dev git gdb valgrind graphviz imagemagick gnuplot
sudo apt-get install libncurses5-dev libncursesw5-dev
git clone https://github.com/realeigenvalue/malloc.git
cd malloc
make
./mcontest testers_exe/tester-1 #runs a single test
./run_all_mcontest.sh #runs all the tests
```

## Contributing
Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

Please make sure to update tests as appropriate.

## License
GNU GPLv3
