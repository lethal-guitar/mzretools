dostrace
========

dostrace is a collection of utilities to help with reverse engineering MS-DOS games. 

It can analyze MZ executables containing 8086 opcodes and provide information that is useful in reverse engineering. For now, .com files and newer Intel CPUs are not supported.

This is still work in progress, and I am adding features as needed in my own reversing efforts. Currently, the array of useful tools consists of the following:

- **mzhdr**: shows information from the .exe file MZ header

ninja@dell:debug$ ./mzhdr bin/hello.exe
--- bin/hello.exe MZ header (28 bytes)
        [0x0] signature = 0x5a4d ('MZ')
        [0x2] last_page_size = 0x43 (67 bytes)
        [0x4] pages_in_file = 15 (7680 bytes)
        [0x6] num_relocs = 4
        [0x8] header_paragraphs = 32 (512 bytes)
        [0xa] min_extra_paragraphs = 227 (3632 bytes)
        [0xc] max_extra_paragraphs = 65535
        [0xe] ss:sp = 207:800
        [0x12] checksum = 0x156e
        [0x16] cs:ip = 0:20
        [0x18] reloc_table_offset = 0x1e
        [0x1a] overlay_number = 0
--- extra data (overlay info?)
        0x01, 0x00
--- relocations:
        [0]: 173:1a, linear: 0x174a, file offset: 0x194a, file value = 0x16f
        [1]: 0:2b, linear: 0x2b, file offset: 0x22b, file value = 0x16f
        [2]: 0:b5, linear: 0xb5, file offset: 0x2b5, file value = 0x16f
        [3]: 173:ae, linear: 0x17de, file offset: 0x19de, file value = 0x16f
--- load module @ 0x200, size = 0x1a43 / 6723 bytes

- **mzmap**: scans and interprets instructions in the executable, traces jump/call destinations and return instructions in order to try and determine the boundaries of subroutines. It can do limited register value tracing to figure out register-dependent calls and jumps. Reachable blocks are either attributed to a subroutine's main body, or it can be marked as a disconnected chunk. The map is saved to a file in a text format.

ninja@dell:debug$ ./mzmap bin/hello.exe
usage: mzmap <file.exe[:entrypoint]> <output.map> [options]
Scans a DOS MZ executable and tries to find routine boundaries, saves output into a file
Options:
--verbose:      show more detailed information, including compared instructions
--debug:        show additional debug information
--nocpu:        omit CPU-related information like instruction decoding
--noanal:       omit analysis-related information
--load segment: overrride default load segment (0x1000)
ninja@dell:debug$ ./mzmap bin/hello.exe hello.map --verbose
Loading executable bin/hello.exe at segment 0x1000
Analyzing code within extents: 1000:0000-11a4:0003/001a44
Done analyzing code
Dumping visited map of size 0x1a43 starting at 0x10000 to routines.visited
Dumping visited map of size 0x1a43 starting at 0x10000 to search_queue.dump
Building routine map from search queue contents (39 routines)
--- Routine map containing 39 routines
STACK/0x1207
CODE/0x1000
DATA/0x116f
1000:0000-1000:000f/000010: unclaimed_1
1000:0010-1000:001f/000010: routine_7
1000:0020-1000:00d1/0000b2: start
1000:00d2-1000:0195/0000c4: routine_4
        1000:00d2-1000:0195/0000c4: main
        1000:0262-1000:0267/000006: chunk
1000:0196-1000:01ac/000017: routine_8
1000:01ad-1000:01f1/000045: routine_9
1000:01f2-1000:021e/00002d: routine_13
1000:021f-1000:022d/00000f: routine_10
[...]
1000:1574-1000:15e1/00006e: routine_34
1000:15e2-1000:1637/000056: routine_35
1000:1638-1000:165d/000026: unclaimed_13
1000:165e-1000:1680/000023: routine_19
1000:1681-1000:1a42/0003c2: unclaimed_14
Saving routine map (size = 39) to hello.map

- **mzdiff** takes two executable files as input and compares their instructions one by one to verify if they match, which is useful when trying to recreate the source code of a game in a high level programming language. After compiling the recreation, this tool can instantly check to see if the generated code matches the original. It accounts for data layout differences, so if one executable accesses a value at one memory offset, and the other accesses one at a different offset, the mapping between the two is saved, and not counted as a mismatch as long as its use is consistent. It can optionally take the map generated by mzmap as an input, which enables assigning meaningful names to the compared subroutines, as well as to exclude some subroutines from the comparison - locations not found in the map will not be compared. This is useful to ignore subroutines which are known to be standard library functions, assembly subroutines or others that are not eligible for comparison for some different reason.

ninja@dell:debug$ ./mzdiff
usage: mzdiff [options] base.exe[:entrypoint] compare.exe[:entrypoint]
Compares two DOS MZ executables instruction by instruction, accounting for differences in code layout
Options:
--map basemap  map file of base executable to use, if present
--verbose      show more detailed information, including compared instructions
--debug        show additional debug information
--dbgcpu       include CPU-related debug information like instruction decoding
--idiff        ignore differences completely
--nocall       do not follow calls, useful for comparing single functions
--sdiff count  skip differences, ignore up to 'count' consecutive mismatching instructions in the base executable
--loose        non-strict matching, allows e.g for literal argument differences
ninja@dell:debuf$ ./mzdiff original.exe:10 my.exe:10 --verbose --loose --sdiff 1 --map original.map
Comparing code between reference (entrypoint 1000:0010/010010) and other (entrypoint 1000:0010/010010) executables
Reference location @1000:0010/010010, routine 1000:0010-1000:0482/000473: main, block 010010-010482/000473, other @1000:0010/010010
1000:0010/010010: push bp                          == 1000:0010/010010: push bp
1000:0011/010011: mov bp, sp                       == 1000:0011/010011: mov bp, sp
1000:0013/010013: sub sp, 0x1c                     =~ 1000:0013/010013: sub sp, 0xe 🟡 The 'loose' option ignores the stack layout difference
1000:0016/010016: push si                          == 1000:0016/010016: push si
1000:0017/010017: mov word [0x628c], 0x0           ~= 1000:0017/010017: mov word [0x418], 0x0 🟡 same value placed at different offsets in the two .exe-s
1000:001d/01001d: mov word [0x628a], 0x4f2         ~= 1000:001d/01001d: mov word [0x416], 0x4f2
1000:0023/010023: mov word [0x45fc], 0x0           ~= 1000:0023/010023: mov word [0x410], 0x0
[...]
1000:0111/010111: or ax, ax                        == 1000:010e/01010e: or ax, ax
1000:0113/010113: jnz 0x11c (down)                 ~= 1000:0110/010110: jnz 0x102 (up)
1000:0115/010115: call far 0x16b50c7f              ~= 1000:0112/010112: call far 0x10650213
1000:011a/01011a: jmp 0x11e (down)                 != 1000:0117/010117: cmp byte [0x40c], 0x78 🔴 the 'skip difference = 1' option allows one mismatching instruction
1000:011c/01011c: jmp 0x105 (up)                   != 1000:0117/010117: cmp byte [0x40c], 0x78 🔴 but the next one still doesn't match
ERROR: Instruction mismatch in routine main at 1000:011c/01011c: jmp 0x105 != 1000:0117/010117: cmp byte [0x40c], 0x78

The output shows `==` for an exact match, `~=` and `=~` for a "soft" difference in either the first or second operand, and `!=` for a mismatch. The idea is to iterate on the reconstruction process as long as the tool finds discrepancies, until the reconstructed code perfectly matches the original, with a margin for the different layout resulting in offset value differences.

I originally intended dostrace to be a limited emulator, which would be able to run and trace the original and reconstructed game executables and compare the traces, but after understanding that the game is [not guaranteed to be deteministic](https://gafferongames.com/post/fix_your_timestep/), I abandoned the idea. The `dostrace` executable is a remnant of those efforts, but it currently serves no useful purpose.

Patches and improvement suggestions are welcome. I hope this can be of use to somebody.
