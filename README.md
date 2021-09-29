# LazyDAW

This project is to become a simple audio mixer, it is still in the *very* early stages however and has no real functionality yet.

The image in my head is a Dear ImGui node graph where each node is dynamically configurable with various features. So one node might represent a hi/lo pass filter, or volume boost, or a FFT, etc. Each node should thus have inputs and outputs, linked to other nodes, and a transformation function. So far all I've written is a very simplistic computation graph class, and so far it is largely untested outside of the simplest cases. The API for the computation graph class is what can only be described as atrocious. Still to be added is the GUI component, audio component, lua bindings, and I would also like to add some automated testing.

