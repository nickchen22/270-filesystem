# 270-filesystem

makefile notes:

make debug   - makes the executable called tests, which has debug info

make nodebug - makes the executable called tests\_nodebug, which silences all debug information

make auto    - does the nodebug comand AND runs test automatically

make all     - does all of the above, mostly use this one

make clean   - deletes tempdirectory and executables

testing notes:

There's notes in tests.c about what to test. The first line of main is a function pointer array, put tests in there. If you call the program with no arguments, it will run all the tests. If you call it with numbers as arguments, it will only run those tests. Tests are functions with no arguments that print their name and return either 1 or 0. I wrote two example ones, look at those and feel free to ask any questions. Main shouldn't really need to be modified aside from adding to the test list.
