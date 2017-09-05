# pars: a parsing tool

Licensed under the [GPL version 2] (http://www.gnu.org/licenses/)

## Introduction

Well, there's not much to it yet, just some running tests.
They test a radix tree, a tokenizer for grammar files, and an incomplete
state machine constructing functions.

## How to build

You need to install the following dependencies:

* cmake
* g++

Then build it like any other cmake project:

    mkdir build
    cd build
    cmake ..
    make

You can also build the project with tracing information:

    mkdir build
    cd build
    cmake .. -DTRACE=ON
    make

You can also run test.sh to run all tests and grammars.sh to run test grammars.
