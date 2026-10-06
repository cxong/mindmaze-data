/*
 * Extract BMP images from the Encarta MindMaze MMBAG mediaview? files.
 *
 * Directory format reverse-engineered from MVUT21N.DLL:
 *
 *   uint8_t  name_length
 *   char     name[name_length]
 *   varint   file_offset
 *   varint   file_size
 *   uint8_t  flags
 *
 * The varint is the compact integer format used by MVUT21N.DLL.
 *
 * .DI$ resources are KWAJ-compressed Windows BMPs.
 *
 * Requires libmspack:
 *     https://github.com/kyz/libmspack
 *
 * Usage:
 *
 *   ./mmbag_extract MMBAG.* output_directory
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define mkdir_p(path) _mkdir(path)
#define PATH_SEP '\\'
#else
#include <unistd.h>
#define mkdir_p(path) mkdir(path, 0755)
#define PATH_SEP '/'
#endif

#include <mspack.h>

static uint16_t read_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t read_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t read_uleb128(const uint8_t *data, size_t data_size, size_t *pos)
{
    // https://en.wikipedia.org/wiki/LEB128
    uint64_t result = 0;
    int shift = 0;
    unsigned char byte;
    do
    {
        byte = data[(*pos)++];
        result |= (byte & 0x7f) << shift; /* low-order 7 bits of byte */
        shift += 7;
    } while ((byte & 0x80) != 0); /* get high-order bit of byte */
    return result;
}

struct mm_entry
{
    char *name;
    uint64_t offset;
    uint64_t size;
    uint8_t flags;
};

static char *read_file(const char *filename, size_t *size_out)
{
    FILE *f = fopen(filename, "rb");
    char *data = NULL;
    if (!f)
    {
        fprintf(stderr, "cannot open %s: %s\n", filename, strerror(errno));
        goto bail;
    }
    fseek(f, 0, SEEK_END);
    *size_out = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = malloc(*size_out);
    if (fread(data, 1, *size_out, f) != *size_out)
    {
        fprintf(stderr, "error reading %s\n", filename);
        goto bail;
    }
    fclose(f);
    return data;

bail:
    fclose(f);
    free(data);
    return data;
}

static void make_path(char *out, size_t out_size, const char *dir, const char *name)
{
    size_t len = strlen(dir);
    if (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\'))
    {
        snprintf(out, out_size, "%s%s", dir, name);
    }
    else
    {
        snprintf(out, out_size, "%s%c%s", dir, PATH_SEP, name);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s MMBAG.* output_directory\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    const char *output_dir = argv[2];
    if (mkdir_p(output_dir) != 0 && errno != EEXIST)
    {
        fprintf(stderr, "cannot create output directory %s: %s\n", output_dir, strerror(errno));
        return 1;
    }

    struct mskwaj_decompressor *kwaj = mspack_create_kwaj_decompressor(NULL);
    char temp_path[4096];
    size_t mmbag_size;
    char *mmbag = read_file(filename, &mmbag_size);
    if (!mmbag)
        goto bail;

    printf("MMBAG size: %zu bytes (0x%zx)\n", mmbag_size, mmbag_size);

    /*
     * MMBAG.* uses a VOO1 B-tree header:
     *
     *   +0x00  uint16  magic       0x293b
     *   +0x02  uint16  flags       0x0102
     *   +0x04  uint16  page_size   0x2000
     *   +0x06  char[4] "VOO1"
     *
     * The VOO1 string is therefore at header + 6.
     *
     * The 12-byte first-page header starts at header + 0x30,
     * and the first directory record starts at header + 0x3c.
     */
    size_t dir_offset = 0;
    size_t dir_size = 0;
    for (size_t i = 0; i + 0x3c <= mmbag_size; i++)
    {
        if (read_le16(mmbag + i) != 0x293b)
            continue;
        if (read_le16(mmbag + i + 2) != 0x0102)
            continue;
        if (read_le16(mmbag + i + 4) != 0x2000)
            continue;
        if (memcmp(mmbag + i + 6, "VOO1", 4) != 0)
            continue;
        const size_t records = i + 0x3c;
        if (records >= mmbag_size)
            continue;
        dir_offset = records;
        dir_size = mmbag_size - records;
        break;
    }
    if (dir_offset == 0)
    {
        fprintf(stderr, "could not locate VOO1 directory\n");
        goto bail;
    }

    printf("directory: 0x%zx - 0x%zx (%zu bytes)\n", dir_offset, dir_offset + dir_size, dir_size);

    // Parse directory
    size_t pos = dir_offset;
    size_t end = dir_offset + dir_size;
    struct mm_entry *entries = NULL;
    size_t entry_count = 0;
    size_t capacity = 0;

    while (pos < end)
    {
        /* Entries have
         *   name_len
         *   name
         *   offset
         *   size
         *   flags
         */
        const uint8_t name_len = mmbag[pos++];
        if (name_len == 0 || name_len > 255)
            break;

        if (entry_count == capacity)
        {
            size_t new_capacity = capacity ? capacity * 2 : 64;
            struct mm_entry *new_entries = realloc(entries, new_capacity * sizeof(*entries));
            entries = new_entries;
            capacity = new_capacity;
        }
        struct mm_entry *entry = &entries[entry_count];
        entry_count++;

        entry->name = malloc((size_t)name_len + 1);
        memcpy(entry->name, mmbag + pos, name_len);
        entry->name[name_len] = '\0';
        pos += name_len;

        entry->offset = read_uleb128(mmbag, end, &pos);
        entry->size = read_uleb128(mmbag, end, &pos);
        entry->flags = mmbag[pos++];
    }
    printf("entries: %zu\n\n", entry_count);

    /*
     * Temporary KWAJ file.
     *
     * libmspack's normal API works on filenames, so we give it
     * a temporary extracted resource.
     */
    make_path(temp_path, sizeof(temp_path), output_dir, ".mm_temp.kwaj");

    size_t bmp_count = 0;
    for (size_t i = 0; i < entry_count; i++)
    {
        const struct mm_entry *entry = &entries[i];

        // Only parse .DI$ for now
        const size_t name_len = strlen(entry->name);
        if (name_len < 4)
            continue;
        if (strcmp(entry->name + name_len - 4, ".DI$") != 0)
        {
            printf("Skipping %s\n", entry->name);
            continue;
        }

        printf("%-20s offset=0x%08llx size=0x%llx\n", entry->name, (unsigned long long)entry->offset,
               (unsigned long long)entry->size);

        const uint8_t *resource = mmbag + (size_t)entry->offset;
        const size_t resource_size = (size_t)entry->size;
        if (resource_size < 14 || memcmp(resource, "KWAJ", 4) != 0)
        {
            printf("  not KWAJ -- skipped\n");
            continue;
        }

        FILE *f = fopen(temp_path, "wb");
        fwrite(resource, 1, resource_size, f);
        fclose(f);

        // Convert PIC1-1.DI$ to PIC1-1.BMP
        char bmp_name[1024];
        snprintf(bmp_name, sizeof(bmp_name), "%.*s.bmp", (int)(name_len - 4), entry->name);
        char bmp_path[4096];
        make_path(bmp_path, sizeof(bmp_path), output_dir, bmp_name);

        // Decompress using libmspack
        int error = kwaj->decompress(kwaj, temp_path, bmp_path);
        if (error != MSPACK_ERR_OK)
        {
            fprintf(stderr, "  decompression failed\n");
            remove(bmp_path);
            continue;
        }

        size_t bmp_size;
        char *bmp = read_file(bmp_path, &bmp_size);
        if (bmp_size < 26 || bmp[0] != 'B' || bmp[1] != 'M')
        {
            fprintf(stderr, "  decompressed data is not a BMP\n");
            free(bmp);
            remove(bmp_path);
            continue;
        }

        uint32_t declared_size = read_le32(bmp + 2);
        uint32_t width = read_le32(bmp + 18);
        uint32_t height = read_le32(bmp + 22);
        // uint16_t planes = read_le16(bmp + 26 - 4);
        uint16_t bpp = read_le16(bmp + 28);
        printf("  -> %s (%ux%u, %u bpp, %zu bytes)\n", bmp_path, width, height, bpp, bmp_size);
        if (declared_size != 0 && declared_size != bmp_size)
        {
            printf("  warning: BMP header says %u bytes\n", declared_size);
        }
        free(bmp);

        bmp_count++;
    }

bail:
    mspack_destroy_kwaj_decompressor(kwaj);
    if (temp_path[0])
        remove(temp_path);

    for (size_t i = 0; i < entry_count; i++)
        free(entries[i].name);

    free(entries);
    free(mmbag);

    printf("\nExtracted %zu BMP files.\n", bmp_count);

    return 0;
}