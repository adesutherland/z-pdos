/* SPDX-License-Identifier: MIT
 * Original host byte writer/checker for the selected 100-cylinder 3390.
 * Coordinates and CCWs follow the owned s370/ipl3390.txt. No old disk input.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define TRACK 56832
#define HEADS 15
#define TRACKS 1500
#define REQUIRE(x) do { if (!(x)) { \
    fprintf(stderr,"IPL image:%d: %s\n",__LINE__,#x); exit(1); } } while (0)
static unsigned char track[TRACK], compare[TRACK];
static unsigned char one[24], two[144];

static unsigned be(const unsigned char *p)
{ return (unsigned)p[0]*256U+p[1]; }
static void ccw(unsigned char *p, unsigned op, unsigned long addr,
                unsigned flags, unsigned count)
{
    REQUIRE(addr <= 0xffffffUL && op <= 255 && flags <= 255 && count <= 65535);
    p[0] = (unsigned char)op; p[1] = (unsigned char)(addr >> 16);
    p[2] = (unsigned char)(addr >> 8); p[3] = (unsigned char)addr;
    p[4] = (unsigned char)flags; p[5] = 0;
    p[6] = (unsigned char)(count >> 8); p[7] = (unsigned char)count;
}
static void bootstrap(void)
{
    static const unsigned char first[24] = {
        0,0,0,0,0,0,0,0,6,0x10,0x3a,0x98,0x60,0,0,0x60,
        8,0x10,0x3a,0x98,0,0,0,0};
    static const unsigned char second[43] = {
        7,0x10,0x3a,0xb8,0x40,0,0,6,
        0x31,0x10,0x3a,0xbe,0x40,0,0,5,
        8,0x10,0x3a,0xa0,0,0,0,0,6,0,0,0,0x20,0,0x7f,0xff,
        0,0,0,0,0,1,0,0,0,1,1};
    memset(one,0,sizeof one); memset(two,0,sizeof two);
    ccw(one+8,6,0x103a98UL,0x60,96); ccw(one+16,8,0x103a98UL,0,0);
    ccw(two,7,0x103ab8UL,0x40,6); ccw(two+8,0x31,0x103abeUL,0x40,5);
    ccw(two+16,8,0x103aa0UL,0,0); ccw(two+24,6,0,0x20,32767);
    two[37] = two[41] = two[42] = 1;
    REQUIRE(!memcmp(one,first,sizeof first) && !memcmp(two,second,sizeof second));
}
static void read_track(FILE *disk, unsigned n)
{
    REQUIRE(n < TRACKS && fseek(disk,512L+(long)n*TRACK,SEEK_SET) == 0);
    REQUIRE(fread(track,1,TRACK,disk) == TRACK);
    REQUIRE(track[0] == 0 && be(track+1) == n/HEADS && be(track+3) == n%HEADS);
}
static unsigned char *record(unsigned number, unsigned *key, unsigned *length)
{
    static const unsigned char end[8] = {255,255,255,255,255,255,255,255};
    size_t at, size;
    for (at = 5; at+8 <= TRACK; at += size) {
        if (!memcmp(track+at,end,sizeof end)) return NULL;
        *key = track[at+5]; *length = be(track+at+6);
        size = 8+(size_t)*key+*length; REQUIRE(size <= TRACK-at);
        REQUIRE(be(track+at) == be(track+1) && be(track+at+2) == be(track+3));
        if (track[at+4] == number) return track+at+8;
    }
    REQUIRE(0); return NULL;
}
static void label(unsigned char *out, const char *name)
{
    static const unsigned char letters[26] = {
        0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,
        0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,
        0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9};
    size_t i; memset(out,0x40,44);
    for (i = 0; name[i]; ++i) {
        REQUIRE(i < 44);
        if (name[i] == '.') out[i] = 0x4b;
        else { REQUIRE(name[i] >= 'A' && name[i] <= 'Z'); out[i] = letters[name[i]-'A']; }
    }
}
static void dataset(FILE *disk, const char *name, const char *path,
                    unsigned first, unsigned limit, unsigned block)
{
    unsigned char keyname[44], *p; unsigned n, r, key, size, start, end, found;
    unsigned long count, logical; FILE *input; int c, eof; size_t i;
    label(keyname,name); found = 0;
    /* The selected control file reserves cylinders 1 and 2 for the VTOC. */
    for (n = 15; n < 45; ++n) {
        read_track(disk,n);
        for (r = 1; r <= 255; ++r) {
            p = record(r,&key,&size); if (!p) break;
            if (key != 44 || size != 96 || p[44] != 0xf1 || memcmp(p,keyname,44)) continue;
            start = be(p+107)*HEADS+be(p+109);
            end = be(p+111)*HEADS+be(p+113);
            REQUIRE(start == first && end == limit-1 && p[82] == 0x40 && p[84] == 0x80);
            REQUIRE(be(p+86) == block && be(p+88) == block); ++found;
        }
    }
    REQUIRE(found == 1); input = fopen(path,"rb"); REQUIRE(input != NULL);
    eof = 0; count = logical = 0;
    for (n = first; n < limit && !eof; ++n) {
        read_track(disk,n);
        for (r = 1; r <= 255; ++r) {
            p = record(r,&key,&size); if (!p) break;
            REQUIRE(key == 0); if (!size) { eof = 1; break; }
            REQUIRE(size == block);
            for (i = 0; i < size; ++i) {
                c = fgetc(input);
                if (c == EOF) REQUIRE(p[i] == 0);
                else { REQUIRE(p[i] == (unsigned char)c); ++logical; }
            }
            count += size;
        }
    }
    REQUIRE(eof && fgetc(input) == EOF && !ferror(input) && fclose(input) == 0);
    REQUIRE(logical > 0 && count == (logical+block-1)/block*block);
    printf("%s: all %lu source bytes and %lu zero padding bytes read back\n",name,logical,count-logical);
}
static void validate(FILE *disk, const char **files, int installed)
{
    static const unsigned char keys[8] = {0xc9,0xd7,0xd3,0xf1,0xc9,0xd7,0xd3,0xf2};
    unsigned char header[512], psw[8], *p; unsigned key, size; unsigned long target; FILE *pload;
    REQUIRE(fseek(disk,0,SEEK_SET) == 0 && fread(header,1,sizeof header,disk) == sizeof header);
    REQUIRE(!memcmp(header,"CKD_P370",8) && header[8] == HEADS);
    REQUIRE(header[9] == 0 && header[10] == 0 && header[11] == 0);
    REQUIRE(header[12] == 0 && header[13] == 0xde && header[14] == 0 && header[15] == 0 && header[16] == 0x90);
    REQUIRE(fseek(disk,0,SEEK_END) == 0 && ftell(disk) == 512L+(long)TRACKS*TRACK);
    read_track(disk,0); p = record(1,&key,&size);
    REQUIRE(p && key == 4 && size == 24 && !memcmp(p,keys,4));
    if (installed) REQUIRE(!memcmp(p+4,one,sizeof one));
    p = record(2,&key,&size); REQUIRE(p && key == 4 && size == 144 && !memcmp(p,keys+4,4));
    if (installed) REQUIRE(!memcmp(p+4,two,sizeof two));
    read_track(disk,1); p = record(1,&key,&size); REQUIRE(p && key == 0 && size == 18452);
    pload = fopen(files[0],"rb"); REQUIRE(pload != NULL);
    REQUIRE(fread(psw,1,sizeof psw,pload) == sizeof psw && fclose(pload) == 0);
    REQUIRE(!memcmp(p,psw,sizeof psw) && !memcmp(psw,"\x00\x0c\x00\x00",4) && (psw[4] & 0x80));
    target = ((unsigned long)(psw[4]&0x7f) << 24) | ((unsigned long)psw[5] << 16) | ((unsigned long)psw[6] << 8) | psw[7];
    REQUIRE(target >= 512 && target+6 < size && !(target & 1));
    dataset(disk,"PLOAD.SYS",files[0],1,11,18452);
    dataset(disk,"PDOS.SYS",files[1],45,60,18452);
    dataset(disk,"COMMAND.EXE",files[2],75,90,18452);
    dataset(disk,"CONFIG.SYS",files[3],60,75,10);
}
static void unchanged(FILE *input, FILE *output, int transport)
{
    size_t n, got, i; unsigned long at, pos;
    REQUIRE(fseek(input,0,SEEK_SET) == 0 && fseek(output,0,SEEK_SET) == 0);
    at = 0;
    while ((n = fread(track,1,sizeof track,input)) != 0) {
        got = fread(compare,1,n,output); REQUIRE(got == n);
        for (i = 0; i < n; ++i) {
            pos = at+(unsigned long)i;
            if (transport && pos >= 20 && pos < 32 && track[i] != compare[i]) {
                /* Hercules regenerates only this host-container serial. */
                REQUIRE(track[i] >= 0x30 && track[i] <= 0x39);
                REQUIRE(compare[i] >= 0x30 && compare[i] <= 0x39);
                continue;
            }
            if (!transport && ((pos >= 512+0x21 && pos < 512+0x21+24) ||
                (pos >= 512+0x45 && pos < 512+0x45+144))) continue;
            REQUIRE(track[i] == compare[i]);
        }
        at += (unsigned long)n;
    }
    REQUIRE(!ferror(input) && fgetc(output) == EOF && !ferror(output));
}
int main(int argc, char **argv)
{
    FILE *input, *output, *existing; size_t n, i; int transport;
    const char *files[4];
    transport = argc == 8 && !strcmp(argv[1],"--compare");
    if (transport) { --argc; ++argv; }
    if (argc != 7) return 2;
    for (i = 0; i < 4; ++i) files[i] = argv[i+3];
    bootstrap();
    if (transport) {
        input = fopen(argv[1],"rb"); output = fopen(argv[2],"rb"); REQUIRE(input && output);
        validate(input,files,1); validate(output,files,1); unchanged(input,output,1);
        REQUIRE(fclose(input) == 0 && fclose(output) == 0);
        puts("CCKD roundtrip: every guest byte unchanged; only the 12-digit host serial may differ");
        return 0;
    }
    errno = 0; existing = fopen(argv[2],"rb");
    if (existing) { fclose(existing); fprintf(stderr,"IPL output already exists\n"); return 2; }
    if (errno != ENOENT || !strcmp(argv[1],argv[2])) return 2;
    input = fopen(argv[1],"rb"); REQUIRE(input != NULL);
    validate(input,files,0); REQUIRE(fseek(input,0,SEEK_SET) == 0);
    output = fopen(argv[2],"wb"); REQUIRE(output != NULL);
    while ((n = fread(track,1,sizeof track,input)) != 0) REQUIRE(fwrite(track,1,n,output) == n);
    REQUIRE(!ferror(input) && fclose(input) == 0);
    REQUIRE(fseek(output,512L+0x21,SEEK_SET) == 0 && fwrite(one,1,sizeof one,output) == sizeof one);
    REQUIRE(fseek(output,512L+0x45,SEEK_SET) == 0 && fwrite(two,1,sizeof two,output) == sizeof two);
    REQUIRE(fclose(output) == 0);
    input = fopen(argv[1],"rb"); output = fopen(argv[2],"rb"); REQUIRE(input && output);
    unchanged(input,output,0); REQUIRE(fclose(input) == 0);
    validate(output,files,1); REQUIRE(fclose(output) == 0);
    puts("100-cylinder CKD image: source IPL vectors, placement and complete payload readback pass");
    return 0;
}
