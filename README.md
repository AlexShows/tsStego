tsStego
=======

An implementation of simple stegonography in C++.
 
Given a text file input and a PNG file input, it encrypts and merges the text file into the PNG file, then outputs the new file (which will likely look just like the original to the naked eye).

Conversely, it can reverse the operation to extract a text file from a PNG file with a message encoded inside it.

![Simple Stegonography](https://raw.github.com/AlexShows/tsStego/master/SimpleStego.png) 

Future Work
===========

Add XOR of text into image.
Fix the message length check bug.

License
=======
Simple Stegonography is released under the MIT License:  

http://opensource.org/licenses/MIT

...except when documented in source otherwise (e.g. OpenSSL).