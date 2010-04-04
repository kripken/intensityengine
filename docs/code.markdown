Code Conventions, Contributions, Etc.
=====================================

Sauer Code
----------

/engine and /shared contains sauer code, which we try to modify as little as possible - to make updating to the latest sauer code as easy as we can. Towards that goal,

* As mentioned, we modify as little as possible.
* When we do modify, we add comments of the form // INTENSITY - which clearly mark what is changed/added.
* If we have large amounts of code to add, we add an #include in sauer code of our code.

Conventions
-----------

TODO (meanwhile, see existing code)


Code Contributions
------------------

Code contributions are very welcome.

License-wise, the simplest thing is to contribute code under a permissive license like zlib, BSD, etc. - we already use a lot of such code anyhow, and it is easiest to combine.

The code should follow our approach to dealing with sauer code, and our coding conventions as much as possible.

We should also differentiate core from non-core additions ('contrib' in Django parlance). Core stuff is to fix bugs and possibly add essential functionality. Non-core is everything else. Non-core code should be added in a 'plugin' manner - changing as little core code as possible:

* JavaScript code can usually be made into a plugin, using the JavaScript API methods for exactly that. Additionally, use the dynamic aspects of JavaScript (for example, if you want to add new members to the Map global, add them in your plugin code using Map.member = value, instead of modifying the core code which defines Map).
* C++ code can be placed in separate files, with #includes from core files (similarly to what we do with our code in sauer code).
* Python code can be written as a component
* Consider placing all the relevant code in a single directory, if possible. Something under src/intensity/contrib/ would make sense.
* Clearly mark in each part of the code what other code is relevant to that patch (i.e., in C++, mention what JavaScript is needed, and vice versa).


