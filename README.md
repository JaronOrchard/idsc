#idsc
====

Implementation of the Deformable Simplicial Complex method for representing and evolving geometric interfaces.

###Linux:
Make sure you have dependencies: g++ libx11-dev libxrandr-dev libopenal-dev libsndfile1-dev libglew-dev libjpeg-dev

To build run:

    python waf configure
    python waf get_deps
    python waf build

###OSX:
Install Homebrew with the command:

    ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"

Install glew with the command:

    brew install glew
