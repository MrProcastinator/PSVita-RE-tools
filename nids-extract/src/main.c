// nids_extract
// Made by: @dots_tb @CelesteBlue123
// Thanks to: team_molecule, theflow

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "self.h"
#include "elf.h"


void getExports(SceModuleInfo *mod_info, uint8_t *segment0, uint32_t vaddr, uint32_t seg0_sz) {
	uint32_t i = (uint32_t)mod_info->expTop;
	while (i < mod_info->expBtm) {
		SceExportsTable *exp_table = (SceExportsTable *)(segment0 + i);
		if (exp_table->nid_table - vaddr < seg0_sz && exp_table->lib_name - vaddr < seg0_sz) {
			char *lib_name = (char *)(segment0 + exp_table->lib_name - vaddr);
			uint32_t *nid_table = (uint32_t *)(segment0 + exp_table->nid_table - vaddr);	
			if (exp_table->lib_name) {
				printf("      %s:\n", lib_name);
				printf("        kernel: %s\n", (exp_table->attribute&0x4000) ? "false" : "true");
				printf("        nid: 0x%08X\n", exp_table->module_nid);
				printf("        functions:\n");
				for (int j = 0; j < exp_table->num_functions; j++) {
					uint32_t nid = nid_table[j];
					printf("          %s_%08X: 0x%08X\n", lib_name, nid, nid);
				}
			}
		}/* else 
			printf("Out of bounds vaddr %x %x\n", exp_table->lib_name, exp_table->nid_table);*/
		i += exp_table->size;
	 }
}

static void usage(char *argv[]) {
	printf("Usage: %s ver file1.elf path/file2.elf... > db_lookup.yml\n", argv[0]);
}

int main(int argc, char **argv) {
	int ret = 0;
	FILE *fin = NULL;
	if (argc < 3) {
		usage(argv);
		return 1;
	}

	printf("version: 2\n");
	printf("firmware: %s\n", argv[2]);
	printf("modules:\n");

	for (int i=1; i < 2; i++) {
		fprintf(stderr, "Opening %s\n", argv[i]);
		fin = fopen(argv[i], "rb");
		
		if (!fin) {
			perror("Failed to open input file");
			goto error;
		}
		fseek(fin, 0, SEEK_END);
		size_t sz = ftell(fin);
		fseek(fin, 0, SEEK_SET);
		uint8_t *input = calloc(1, sz);	
		if (!input) {
			perror("Failed to allocate buffer for input file");
			goto error;
		}
		if (fread(input, sz, 1, fin) != 1) {
			static const char s[] = "Failed to read input file";
			if (feof(fin))
				fprintf(stderr, "%s: unexpected end of file\n", s);
			else
				perror(s);
			goto error;
		}
		fclose(fin);
		fin = NULL;
		
		
		ELF_header *ehdr = (ELF_header *)(input);
		Elf32_Phdr *phdr = (Elf32_Phdr *)(input + ehdr->e_phoff);
		SceModuleInfo *mod_info = (SceModuleInfo *)(input + phdr[0].p_offset + ehdr->e_entry);
		
		printf("  %s: # %s\n", mod_info->name, argv[i]);
		printf("    nid: 0x%08X\n", mod_info->nid);
		printf("    libraries:\n");
		getExports(mod_info, input + phdr[0].p_offset, phdr[0].p_vaddr, phdr[0].p_memsz);	
	}

end:
	if (fin)
		fclose(fin);
	return ret;
error:
	ret = 1;
	goto end;
}
