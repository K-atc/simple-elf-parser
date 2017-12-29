Simple ELF Parser
====

C++ script to parse ELF file.

The Parser parses:

* Entry Point
* Sections
* Segments
* Symbols


Usage
----
```bash
./simple-elf-parser ELF_FILE
```

sample run:

```
./simple-elf-parser hello
 (addr=0x0, size=0x0)
.bss (addr=0x601030, size=0x8)
[...]
.rodata (addr=0x4001b0, size=0x11)
.shstrtab (addr=0x0, size=0x103)
.strtab (addr=0x0, size=0x20a)
.symtab (addr=0x0, size=0x600)
.text (addr=0x400000, size=0x1a2)
```


Build
----
```bash
make
```


Test
---
usuful for debugging purpose.

```bash
make test
```


TODO
----
* Dinamic Library
* Relocs