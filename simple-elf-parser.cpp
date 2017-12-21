#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <string>
#include <map>

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
    long unsigned int size;
};
typedef std::map<std::string, struct section> sections;

#define IS_ELF(h) (h->e_ident[0] == 0x7f && h->e_ident[1] == 'E' && h->e_ident[2] == 'L' && h->e_ident[3] == 'F')

err_t parse_elf(char *file_name, header *header, sections *sections)
{
    // open elf file
    int fd = open(file_name, O_RDONLY);
    if (fd == 0) {
        return ERR_EXIST;
    }

    // map file to memory
    struct stat sb;
    fstat(fd, &sb);
    void *head = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

    // check the file's magic number
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)head; // parse this file as 32-bit elf
    if (!IS_ELF(ehdr)) {
        return ERR_FORMAT;
    }

    if (ehdr->e_ident[EI_CLASS] == ELFCLASS64) { // this is 64-bit elf
        Elf64_Ehdr *e64hdr;
        Elf64_Shdr *shdr;
        Elf64_Shdr *shstr;

        // re-parse elf header
        e64hdr = (Elf64_Ehdr *) ehdr;

        // parse header
        header->entry_point = e64hdr->e_entry;
        
        // parse sections
        shstr = (Elf64_Shdr *)(head + e64hdr->e_shoff + e64hdr->e_shentsize * e64hdr->e_shstrndx);
        for (int i = 0; i < e64hdr->e_shnum; i++) {
            shdr = (Elf64_Shdr *)(head + e64hdr->e_shoff + e64hdr->e_shentsize * i);
            std::string section_name = std::string((char*) head + shstr->sh_offset + shdr->sh_name);
            struct section s = {(long unsigned int) shdr->sh_addr, (long unsigned int) shdr->sh_size};
            (*sections)[section_name] = s;           
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
    for(auto itr = sections->begin(); itr != sections->end(); ++itr) {
        printf("%s (addr=0x%x, size=0x%x)\n", 
            itr->first.c_str(), itr->second.addr, itr->second.size
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
    err = parse_elf(ELF_FILE, &header, &sections);
    if (err) {
        if (err == ERR_EXIST) perror("file not exists.");
        if (err == ERR_FORMAT) fprintf(stdout, "this is not elf.");
    }
    print_header(&header);
    print_sections(&sections);
    return 0;
}