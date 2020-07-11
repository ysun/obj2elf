#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h> 

 struct obj_field_info_t
 {
     unsigned char id;
     unsigned long addr_len;
     unsigned long length_len;
     unsigned long checksum_len;
 };

enum {
    OBJ_FILE_TYPE       = 0x40,
    OBJ_MEM_32          = 0x41,
    OBJ_IO_MEM_32       = 0x42,
    OBJ_SYMBOL_32       = 0x43,
    OBJ_MEM_64          = 0x44,
    OBJ_IO_MEM_64       = 0x45,
    OBJ_SYMBOL_64       = 0x46,
    OBJ_EOF             = 0x4f,
};

static struct obj_field_info_t obj_fields_info[] = {
    { OBJ_FILE_TYPE,    0,     0,      0},
    { OBJ_MEM_32,       4,     4,      0},
    { OBJ_IO_MEM_32,    4,     4,      0},
    { OBJ_SYMBOL_32,    4,     1,      0},
    { OBJ_MEM_64,       8,     4,      0},
    { OBJ_IO_MEM_64,    8,     4,      0},
    { OBJ_SYMBOL_64,    8,     1,      0},
    { 0 /* 0x47 */,     0,     0,      0},
    { 0 /* 0x48 */,     0,     0,      0},
    { 0 /* 0x49 */,     0,     0,      0},
    { 0 /* 0x4a */,     0,     0,      0},
    { 0 /* 0x4b */,     0,     0,      0},
    { 0 /* 0x4c */,     0,     0,      0},
    { 0 /* 0x4e */,     0,     0,      0},
    { OBJ_EOF,          0,     0,      4},
};

static struct obj_field_info_t *get_obj_fields_info(unsigned short id)
{
    if(id < OBJ_FILE_TYPE || id > OBJ_EOF) {
		fprintf(stderr, "Failed ID (%x) \n", id);
		assert(id >= OBJ_FILE_TYPE && id <= OBJ_EOF);
	}
    return &obj_fields_info[id - OBJ_FILE_TYPE];
}

static int fread_non_zero_len(unsigned long *pval, size_t len, FILE *fp)
{
    *pval = 0;
    if (len == 0)
        return 0;
    return (fread(pval, len, 1, fp) == len);
}

int
main (int argc, char **argv)
{
	char *filename_obj, *filename_elf;
	FILE *fp_obj, *fp_elf;

    unsigned long addr, length, checksum, r, i;
    unsigned char id = 0;

    const struct obj_field_info_t *obj_field_info;
    u_int8_t *buf = NULL;

	int c;
	opterr = 0;
	while ((c = getopt (argc, argv, "i:o:")) != -1)
		switch (c)
		{
		case 'i':
			filename_obj = optarg;
			break;
		case 'o':
			filename_elf = optarg;
			break;
		case '?':
			fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
			return 1;
		default:
			abort ();
		}

	printf ("filename_obj = %s, filename_elf = %s\n",
          filename_obj, filename_elf);

	fp_obj = fopen(filename_obj, "r");
	if (fp_obj == NULL) {
		fprintf(stderr, "Failed to open seed file: %s\n", filename_obj);
		return -2;
	}

	fp_elf = fopen(filename_elf, "w");
	if (fp_elf == NULL) {
		fprintf(stderr, "Failed to open seed file: %s\n", filename_elf);
		return -2;
	}

	do {
		addr = length = checksum = id = 0;

		if (fread(&id, 1, 1, fp_obj) != 1) {
			fprintf(stderr, "Failed to read ID: %s\n", filename_obj);
			goto done;
		}
		obj_field_info = get_obj_fields_info(id);
		fread_non_zero_len(&addr, obj_field_info->addr_len, fp_obj);
		fread_non_zero_len(&length, obj_field_info->length_len, fp_obj);
		fread_non_zero_len(&checksum, obj_field_info->checksum_len, fp_obj);

	    buf = (u_int8_t*) malloc(length);
	    if (!buf) 
	    	fprintf(stderr, "Failed to create buffer (%s)\n", __func__);

		if (length > 0)
			if ((r = fread(buf, 1, length, fp_obj)) != length) {
				fprintf(stderr, "Failed to read buf: %s\n", filename_obj);
				goto done;
			}
		if (id == OBJ_MEM_64 || id == OBJ_MEM_32) {
			fseek(fp_elf, addr, SEEK_SET);
			fwrite(buf, length, 1, fp_elf);

//			fprintf(stderr, "ysun: Cafe id:%d, addr:%lx, length:%ld\n",
//					id, addr, length);

//			for(i = 0; i<length; i++) {
//				fprintf(stderr, "%x ", buf[i]);
//				i++;
//				if( i == 16) fprintf(stderr, "\n");
//			}
//			fprintf(stderr, "\n");
		}

		free(buf);
	} while (id != OBJ_EOF);

done:
    fclose(fp_elf);
    fclose(fp_obj);

    return 0;
}
