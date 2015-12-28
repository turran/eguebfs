What is it?
===========
EguebFS is a FUSE based filesystem for [Egüeb](https://github.com/turran/egueb) documents. It maps a XML tree into the filesystem

How?
====
1. Using the provided binary eguebfs in the form
  ```bash
  eguebfs FILE MOUNTPOINT
  ```
  Where FILE has to be any XML file that [Egüeb](https://github.com/turran/egueb) knows.

2. Embedding it into your own application
  
  Check the contents of [eguebfs.c](https://github.com/turran/eguebfs/blob/master/src/bin/eguebfs.c) to see how it can be done.

Examples
========
On the video you will see a screencast of a mounted SVG file and the live editing.
[![Foo](https://i.vimeocdn.com/video/549595310_640.webp)](https://vimeo.com/150186589)
 

Documentation
=============
For API reference check out the automatically generated [doxygen](https://turran.github.io/eguebfs/docs/index.html) documentation

