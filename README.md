# Rolling Stone

A Sokoban solver program originally developed at the University of Alberta in the 1990s.

**Web Page:**
* https://github.com/carlos-montiers/rolling-stone/releases

**Original Source Code:**
* https://webdocs.cs.ualberta.ca/~games/Sokoban/Src/RS_src.tgz

## Credits

- **Andreas Junghanns:** Original author.
- **Jonathan Schaeffer:** Ph.D. advisor.
- **Jason Hood (Australia):** Ported the program from Unix to Windows, added 64-bit support, and resolved a significant performance degradation issue.
- **Brian Damgaard (Denmark):** Added solution printing in standard LURD notation to facilitate easier solution verification and interoperability with other programs.
- **Carlos Montiers (Chile):** Project management and contributions to the Windows port.

## Basic Usage

### Interactive Usage

- `?` - Displays the menu options.
- `S 1` - Solves puzzle 1 (located in the standard `screens` folder as `screens/screen.1`).
- `S "puzzles\my puzzle.txt"` - Solves a puzzle from a custom file outside the `screens` folder.
- `Q` - Quits the program.

### Note on Loading Custom Puzzles

To load custom puzzles, place your puzzle files in a folder of your choice. If the folder or file name contains spaces, enclose the path in double quotes when running the program, for example:

`S "puzzles\my puzzle.txt"`

**Important:** The file should contain only the puzzle itself—any additional data will not be parsed by Rolling Stone.
