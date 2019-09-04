# tarcraft
Tool for crafting impossible tar files

This tool can be used to create tar files for penetration testing.

Implemented scenarios:

1. Extracting a payload to the destination of a symlink. Tar can dereference symlinks but not pack files in a symlinked directory. In this scenario a tar is generated that creates a symlink and extracts the payload into the directory the symlink is linking to.
2. Scenario 1 but in two steps, some versions of tar might complain about extracting into a symlink.
3. Creating a double symlink. In certain cases the extracted files might be checked and removed when linking to a directory outside of where the file is extracted. It might be possible to avoid detection by embedding special characters in the filename. When the output of tar is used as the input for the checking process a newline might be enough to avoid detection. A second symlink is then created linking to the 'illegal' symlink to give access to the attacker (webservers don't like newlines in a filename).
