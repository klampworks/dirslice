dirslice
========

A utility for splitting directories into multiple smaller subdirectories.


usage
=====

Usage dirslice &lt;num&gt; &lt;seg&gt; &lt;directory&gt;
 Where &lt;num&gt; = Number of slices to create, 0 = as many as possible
 and &lt;seg&gt; = The size of each slice

If test_dir has 550 files then the following command would create two subdirectories inside test_dir and move 200 files into each one, leaving 150 in test_dir.

    dirslice 2 200 test_dir/

If test_dir has 550 files then the following command would create three subdirectories inside test_dir and move 200 files into the first two and the remaining 150 files into the third.

    dirslice 0 200 test_dir/


design
======

Dirslice uses dirent to enumerates all files inside the target directory into a linked list and then iterates through it until the required number of files have been moved or the end of the list is reached. This may cause problems on servilely memory constricted systems with several million files inside the target directory. Files are inserted into the list in order of modification date and are therefore grouped into subdirectories in order of modification date too.
