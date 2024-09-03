# RVL_MEM

RVL_MEM is a reimplementation of the EGG heap system in C++, and has been adapted to work on modern hardware. This project is based off of [vabold/bba-wd](https://github.com/vabold/bba-wd), with some parts moved from C to C++.

The main function contains a set of tests to ensure everything is working correctly.

## Building

### Dependencies

- g++
- ninja
- Python 3

### Process

Generate the ninja file:

```bash
./configure.py
```

Execute it:

```bash
ninja
```

## Contributing

The codebase uses C++ for the engine and Python for any external scripts.

Pull requests resolving an issue or element of a tracking issue should reference the issue or item in the description.

Any commits should be formatted using the project's clang-format file.
