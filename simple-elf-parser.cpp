#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <string>
#include <map>
#include <list>

typedef enum {
    ERR_OK = 0,
    ERR_EXIST,
    ERR_FORMAT,
} err_t;

struct header {
    long unsigned int entry_point;
};
struct section {
    long unsigned int addr;
    long unsigned int offset;    
    long unsigned int size;
};
typedef std::map<std::string, struct section> sections;
struct segment {
    long unsigned int addr;
    long unsigned int offset;    
    long unsigned int size;
    Elf32_Word type;
};
typedef std::map<int, struct segment> segments;
struct symbol {
    long unsigned int value; // offset or address  
    long unsigned int size;
};
typedef std::map<std::string, struct symbol> symbols;

#define IS_ELF(h) (h->e_ident[0] == 0x7f && h->e_ident[1] == 'E' && h->e_ident[2] == 'L' && h->e_ident[3] == 'F')

err_t parse_elf(char *file_name, header *header, sections *sections, segments *segments, symbols *symbols)
{
    // open elf file
    int fd = open(file_name, O_RDONLY);
    if (fd <= 0) {
        return ERR_EXIST;
    }

    // map file to memory
    struct stat sb;
    fstat(fd, &sb);
    void *head = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

    // check the file's magic number
    Elf32_Ehdr *e32hdr = (Elf32_Ehdr *)head; // parse this file as 32-bit elf
    if (!IS_ELF(e32hdr)) {
        return ERR_FORMAT;
    }

    if (e32hdr->e_ident[EI_CLASS] == ELFCLASS64) { // this is 64-bit elf
        Elf64_Ehdr *ehdr;
        Elf64_Shdr *shdr;
        Elf64_Shdr *shstr;
        Elf64_Shdr *str;        
        Elf64_Phdr *phdr;

        // re-parse elf header
        ehdr = (Elf64_Ehdr *) e32hdr;

        // parse header
        header->entry_point = ehdr->e_entry;
        
        // parse sections
        shstr = (Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shstrndx);
        for (int i = 0; i < ehdr->e_shnum; i++) {
            shdr = (Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
            std::string section_name = std::string((char*) head + shstr->sh_offset + shdr->sh_name);
            struct section s = {(long unsigned int) shdr->sh_addr, (long unsigned int) shdr->sh_offset, (long unsigned int) shdr->sh_size};
            (*sections)[section_name] = s;  
            if (section_name == ".strtab") str = shdr; // used in symbols
        }

        // parse segments
        for (int i = 0; i < ehdr->e_phnum; i++) {
            phdr = (Elf64_Phdr *)(head + ehdr->e_phoff + ehdr->e_phentsize * i);
            struct segment s = {(long unsigned int) phdr->p_vaddr, (long unsigned int) phdr->p_offset, (long unsigned int) phdr->p_filesz, (Elf32_Word) phdr->p_type};
            (*segments)[i] = s;
        }

        // parse symbols
        for (int i = 0; i < ehdr->e_shnum; i++) {
            shdr = (Elf64_Shdr *)(head + ehdr->e_shoff + ehdr->e_shentsize * i);
            if (shdr->sh_type != SHT_SYMTAB) continue;
            Elf64_Shdr *sym;
            Elf64_Sym *symp;
            sym = shdr;
            for (int j = 0; j < sym->sh_size / sym->sh_entsize; j++) {
                symp = (Elf64_Sym *)(head + sym->sh_offset + sym->sh_entsize * j);
                if (!symp->st_name) continue;
                if (symp->st_value) {
                    std::string symbol_name = std::string((char *)(head + str->sh_offset + symp->st_name));
                    struct symbol s = {symp->st_value, symp->st_size};
                    (*symbols)[symbol_name] = s;
                }
            }
        }
    }
    else { // Unsupported
        // TODO: 32-bit elf support
        fprintf(stderr, "Unsupported elf class");
    }

    munmap(head, sb.st_size);
    close(fd);
    return ERR_OK;
}

void print_header(header *header)
{
    printf("entry point: 0x%x\n", header->entry_point);
}

void print_sections(sections *sections)
{
    puts("=== [sections] ===");
    for(auto itr = sections->begin(); itr != sections->end(); ++itr) {
        printf("%s (addr=0x%x, offset=0x%x, size=0x%x)\n", 
            itr->first.c_str(), itr->second.addr, itr->second.offset, itr->second.size
            );
    }
}

void print_segments(segments *segments)
{
    puts("=== [segments] ===");
    for(auto itr = segments->begin(); itr != segments->end(); ++itr) {
        printf("%d (addr=0x%x, offset=0x%x, size=0x%x, type=%d)\n", 
            itr->first, itr->second.addr, itr->second.offset, itr->second.size, itr->second.type
            );
    }
}

void print_symbols(symbols *symbols)
{
    puts("=== [symbols] ===");
    for(auto itr = symbols->begin(); itr != symbols->end(); ++itr) {
        printf("%s (addr/offset=0x%x, size=0x%x)\n", 
            itr->first.c_str(), itr->second.value, itr->second.size
            );
    }
}

void usage(char* argv[])
{
    puts("Simple ELF Parser");
    printf("usage: %s ELF_FILE", argv[0]);
    exit(1);
}

int main(int argc, char* argv[])
{
    err_t err;
    
    if (argc < 2) {
        usage(argv);
    }
    
    char *ELF_FILE = argv[1];
    header header;
    sections sections;
    segments segments;
    symbols symbols;
    err = parse_elf(ELF_FILE, &header, &sections, &segments, &symbols);
    if (err) {
        if (err == ERR_EXIST) perror("file not exists.");
        if (err == ERR_FORMAT) fprintf(stdout, "this is not elf.");
        return 1;
    }
    print_header(&header);
    print_sections(&sections);
    print_segments(&segments);
    print_symbols(&symbols);

    return 0;
}