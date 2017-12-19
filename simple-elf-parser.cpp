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

struct section {
    long unsigned int addr;
    long unsigned int size;
};
typedef std::map<std::string, struct section> sections;

#define IS_ELF(h) (h->e_ident[0] == 0x7f && h->e_ident[1] == 'E' && h->e_ident[2] == 'L' && h->e_ident[3] == 'F')

int elf_bits(Elf32_Ehdr ehdr)
{
    switch (ehdr.e_ident[EI_CLASS]) {
		case ELFCLASS32:
			return 32;
		case ELFCLASS64:
			return 64;
		default:
			return 0;
    }
}

err_t parse_elf(char *file_name, sections *sections)
{
    int fd = open(file_name, O_RDONLY);
    if (fd == 0) {
        return ERR_EXIST;
    }
    struct stat sb;
    fstat(fd, &sb);
    void *head = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)head;
    if (!IS_ELF(ehdr)) {
        return ERR_FORMAT;
    }
    if (ehdr->e_ident[EI_CLASS] == ELFCLASS64) {
        Elf64_Ehdr *e64hdr;
        Elf64_Shdr *shdr;
        Elf64_Shdr *shstr;
        e64hdr = (Elf64_Ehdr *) ehdr;
        shstr = (Elf64_Shdr *)(head + e64hdr->e_shoff + e64hdr->e_shentsize * e64hdr->e_shstrndx);
        for (int i = 0; i < e64hdr->e_shnum; i++) {
            shdr = (Elf64_Shdr *)(head + e64hdr->e_shoff + e64hdr->e_shentsize * i);
            // printf("%s (addr=0x%x, size=0x%x)\n", 
            //     (char *)(head + shstr->sh_offset + shdr->sh_name),
            //     shdr->sh_addr,
            //     shdr->sh_size
            //     );
            std::string section_name = std::string((char*) head + shstr->sh_offset + shdr->sh_name);
            struct section s = {(long unsigned int) shdr->sh_addr, (long unsigned int) shdr->sh_size};
            (*sections)[section_name] = s;           
        }     
    }
    else {
        // Unsupported
        fprintf(stderr, "Unsupport elf class");
    }

    munmap(head, sb.st_size);
    close(fd);
    return ERR_OK;
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
    sections sections;
    err = parse_elf(ELF_FILE, &sections);
    if (err) {
        if (err == ERR_EXIST) perror("file not exists.");
        if (err == ERR_FORMAT) fprintf(stdout, "this is not elf.");
    }
    print_sections(&sections);
    return 0;
}