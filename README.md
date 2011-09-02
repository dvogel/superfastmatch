README
======

This is a new version of Superfastmatch written in C++ to improve matching performance and with an index running totally in memory to improve response times.

The point of the software is to index large amounts of text in memory. Therefore there isn't much reason to run it on a 32-bit OS with a 4GB cap on memory and a 64-bit OS is assumed

The process for installation is as follows:


Dependencies
------------

Superfastmatch depends on these libraries:

[Google gflags](http://code.google.com/p/google-gflags/)

[Google perftools](http://code.google.com/p/google-perftools/)

[Google ctemplate](http://code.google.com/p/google-ctemplate/)

[Google sparsehash](http://code.google.com/p/google-sparsehash/)

[Kyoto Cabinet](http://fallabs.com/kyotocabinet/)

[Kyoto Tycoon](http://fallabs.com/kyototycoon/)

You might be able to get away with installing the .deb packages on the listed project pages, but this is untested. The trunk of sparsehash is required for this [patch](http://code.google.com/p/google-sparsehash/source/detail?r=76);

The easier route is to run:

    ./scripts/bootstrap.sh

and wait for everything to build. The script will ask you for your sudo password, which is required to install the libraries.

On Ubuntu you'll need to do this first:

    sudo apt-get install libunwind7

Which is a dependency for perftools. And you might also need a:

    sudo ldconfig

after the script has finished.

Test
----

After the libraries are installed, you can run:

    make check

to run the currently lonely unit test for the index code.

Build
-----

After that you can run:

    make run

to get a superfastmatch instance running. Nothing is currently configurable from the command line yet. Coming soon...

Visit http://127.0.0.1:8080 to test the interface.

Data
----

For a quick introduction to what can be found with superfastmatch try this:

If you have a machine with less than 8GB of memory and less than 4 cores run:

    ./superfastmatch -debug -hash_width 24 -reset -slot_count 2 -thread_count 2 -window_size 30

otherwise this will be much faster:

    ./superfastmatch -debug -reset -window_size 30

in another terminal window run:

    tail -f debug.log
    
to see what is happening. And then finally, in another terminal window, run:

    ./scripts/gutenberg.sh
    
to load some example documents and associate them with each other. You can view the results in the browser at:

    http://127.0.0.1:8080/document/




