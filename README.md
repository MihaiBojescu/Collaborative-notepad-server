# Collaborative notepad server
A server for the collaborative notepad project.

# Dependencies:
* glibc (GNU C Library)
* jsoncpp
Dependencies can be installed like this:
* `sudo apt install glibc libjsoncpp-dev libjsoncpp1` for Debian based distributions
* `sudo pacman -Sy glibc jsoncpp` for Arch based distributions

# Compiling:
Run `make` or `make debug` (for debugging support with GDB).

# Usage
Run the created executable created in the `build/` directory:
* `build/notepadserver` for release build;
* `build/notepadserver-debug` for debug build;
**Disclaimer:** The server isn't interactive.
