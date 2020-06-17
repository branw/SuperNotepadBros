# SuperNotepadBros

A game that renders in an instance of Windows's `notepad.exe`.

![Screenshot](https://user-images.githubusercontent.com/2104778/84940242-8d057f80-b0ad-11ea-82f2-48f03811f615.png)

## Usage

This project has no dependencies and should compile in any Visual Studio 2019
version. To play, place `Loader.exe` and `SuperNotepadBros.dll` in the same
directory, then run `Loader.exe` (`SuperNotepadBros.dll` should be in the same
working directory). Note that the binaries need to have the same architecture
as `notepad.exe`, e.g. 64-bit Windows requires this project to be built for
the x64 platform.

Move your `I` around with the arrow keys, collecting all of the `£` until the
exit `e` opens into an `E`.

## TODO

- Figure out why enabling Word Wrap causes a crash
- Allow seamless level saving/loading
- Add more levels and reloading
- Add a level editor

## License

SuperNotepadBros is released under the MIT License.
