tsStego
=======

An implementation of simple stegonography in C++.
 
Currently this just takes a text file input and a PNG file input, merges the text file into the PNG file, and then outputs a resulting PNG file. Also, it will reverse the operation to extract a text file from a PNG file with a message encoded inside it.

![Simple Stegonography](https://raw.github.com/AlexShows/tsStego/master/SimpleStego.png) 

Future Work
===========

Add XOR of text into image.

Add encryption of plain text.

License
=======
Simple Stegonography is released under the MIT License:  

http://opensource.org/licenses/MIT

...except when documented in source otherwise. Currently experimenting with OpenSSL, but have not redistributed any components of it or reproduced any herein (just references to them and abstractions that call functions in the API). 