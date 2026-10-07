/* Shared maintained PDOS media commands.
 * Paul Edwards created PDOS; bounded media repairs retain the component licence.
 * The one-space kernel and K/U port supply their own checked I/O and storage.
 */
static int pdosVolumeIndex(PDOS *pdos, int device)
{
    int i;
    for (i = 0; i < pdos->volume_count; i++)
        if (pdos->volumes[i].device == device) return i;
    return -1;
}

/* Read the real VOL1 label. Never infer a volume serial from a Hercules
   device address. Dataset operations validate the VTOC when used. */
static int pdosReadVolumeLabel(int device, char serial[7])
{
    char label[80];
    int count = rdblock(device, 0, 0, 3, label, sizeof label, 0x0e);
    if (count < 24 || memcmp(label + 4, "VOL1", 4) != 0)
        return 0;
    memcpy(serial, label + 8, 6);
    serial[6] = '\0';
    return 1;
}

/* The first guest allocator is deliberately confined to dasdload's
   100-cylinder 3390 exchange layout: VOL1 at 0/0/3, a two-cylinder VTOC,
   one contiguous format-1 extent per dataset, and no format-5 free map.
   Data are initialized before the new DSCB is made discoverable. */
static int pdosAllocateDataset(PDOS *pdos, const char *input)
{
#ifndef ZARCH
    (void)pdos;
    (void)input;
    printf("ALLOC: unsupported on this build\n");
    return 8;
#else
    char name[48], format[8], extra;
    unsigned char wanted[44], label[80], f4[140], f5[140];
    unsigned char raw[140], check[140], blank[140], used[100], eof[8];
    DSCB1 dscb;
    int lrecl, blksize, cylinders, volume, device;
    int c, h, r, n, x, start, freec, freeh, freer, saw_blank;
    int low, high, lowhead, highhead, count, count4, count5;
#ifndef PDOS_MEDIA_PORT
    time_t now;
    struct tm *today;
#endif

    if (sscanf(input, "%47s %7s %d %d %d %c", name, format,
               &lrecl, &blksize, &cylinders, &extra) != 5)
    {
        printf("usage: ALLOC name FB|VB|U lrecl blksize cylinders\n");
        return 8;
    }
    for (n = 0; name[n]; n++) name[n] = toupper((unsigned char)name[n]);
    for (n = 0; format[n]; n++)
        format[n] = toupper((unsigned char)format[n]);
    n = strlen(name);
    if (n < 1 || n > 44 || cylinders < 1 || cylinders > 16
        || blksize < 1 || blksize > 32760
        || !(isalpha((unsigned char)name[0]) || name[0] == '#'
             || name[0] == '$' || name[0] == '@'))
    {
        printf("ALLOC: invalid name, cylinder count or block length\n");
        return 8;
    }
    for (x = 0; x < n; x++)
        if (!(isalnum((unsigned char)name[x]) || name[x] == '.'
              || name[x] == '#' || name[x] == '$' || name[x] == '@'))
        {
            printf("ALLOC: invalid dataset name\n");
            return 8;
        }
    if (strcmp(format, "FB") == 0)
    {
        if (lrecl < 1 || lrecl > blksize || blksize % lrecl)
        {
            printf("ALLOC: FB requires whole fixed records per block\n");
            return 8;
        }
    }
    else if (strcmp(format, "VB") == 0)
    {
        if (lrecl < 5 || lrecl + 4 > blksize)
        {
            printf("ALLOC: VB needs RDW and BDW space\n");
            return 8;
        }
    }
    else if (strcmp(format, "U") == 0)
    {
        if (lrecl != 0)
        {
            printf("ALLOC: U requires lrecl 0\n");
            return 8;
        }
    }
    else
    {
        printf("ALLOC: supported formats are FB, VB and U\n");
        return 8;
    }
    volume = pdosVolumeIndex(pdos, pdos->curdev);
    if (volume <= 0)
    {
        printf("ALLOC: select an exchange volume first\n");
        return 8;
    }
    device = pdos->curdev;
    memset(wanted, ' ', sizeof wanted);
    memcpy(wanted, name, n);
    count = rdblock(device, 0, 0, 3, label, sizeof label, 0x0e);
    count4 = rdblock(device, 1, 0, 1, f4, sizeof f4, 0x0e);
    count5 = rdblock(device, 1, 0, 2, f5, sizeof f5, 0x0e);
    if (count < 20 || memcmp(label + 4, "VOL1", 4) != 0
        || memcmp(label + 8, pdos->volumes[volume].volser, 6) != 0
        || memcmp(label + 15, "\0\1\0\0\1", 5) != 0
        || count4 != 140 || f4[44] != '4'
        || f4[62] != 0 || (unsigned char)f4[63] != 100
        || f4[107] != 0 || f4[108] != 1 || f4[109] != 0
        || f4[110] != 0 || f4[111] != 0 || f4[112] != 2
        || f4[113] != 0 || f4[114] != 14
        || count5 != 140 || f5[44] != '5')
    {
        printf("ALLOC: unsupported or damaged VTOC layout\n");
        return 8;
    }
    for (x = 45; x < 140; x++)
        if (f5[x] != 0)
        {
            printf("ALLOC: free-space map needs another allocator\n");
            return 8;
        }
    memset(used, 0, sizeof used);
    used[0] = used[1] = used[2] = 1;
    freec = freeh = freer = -1;
    saw_blank = 0;
    for (c = 1; c <= 2; c++)
        for (h = 0; h < 15; h++)
            for (r = (c == 1 && h == 0) ? 3 : 1; r <= 100; r++)
            {
                count = rdblock(device, c, h, r, raw, sizeof raw, 0x0e);
                if (count < 0) break;
                if (count != 140)
                {
                    printf("ALLOC: malformed VTOC record\n");
                    return 8;
                }
                if (raw[44] == 0)
                {
                    for (x = 0; x < 140; x++)
                        if (raw[x] != 0) return 8;
                    if (!saw_blank)
                    {
                        freec = c; freeh = h; freer = r;
                        saw_blank = 1;
                    }
                    continue;
                }
                if (saw_blank || raw[44] != '1' || raw[59] != 1
                    || raw[105] != 0x81)
                {
                    printf("ALLOC: unsupported VTOC entry order or extent\n");
                    return 8;
                }
                if (memcmp(raw, wanted, 44) == 0)
                {
                    printf("ALLOC: dataset already exists: %s\n", name);
                    return 8;
                }
                low = ((unsigned char)raw[107] << 8) | (unsigned char)raw[108];
                lowhead = ((unsigned char)raw[109] << 8) | (unsigned char)raw[110];
                high = ((unsigned char)raw[111] << 8) | (unsigned char)raw[112];
                highhead = ((unsigned char)raw[113] << 8) | (unsigned char)raw[114];
                if (low < 3 || high >= 100 || low > high
                    || lowhead >= 15 || highhead >= 15
                    || (low == high && lowhead > highhead)) return 8;
                for (x = low; x <= high; x++) used[x] = 1;
            }
    if (freec < 0)
    {
        printf("ALLOC: VTOC has no free DSCB\n");
        return 8;
    }
    for (start = 3; start + cylinders <= 100; start++)
    {
        for (x = start; x < start + cylinders && !used[x]; x++) ;
        if (x == start + cylinders) break;
    }
    if (start + cylinders > 100)
    {
        printf("ALLOC: insufficient free cylinders\n");
        return 8;
    }
    for (c = start; c < start + cylinders; c++)
        for (h = 0; h < 15; h++)
            if (rdblock(device, c, h, 1, check, sizeof check, 0x0e) >= 0)
            {
                printf("ALLOC: selected extent is not empty\n");
                return 8;
            }
    memset(eof, 0, sizeof eof);
    eof[0] = (unsigned char)(start >> 8);
    eof[1] = (unsigned char)start;
    eof[4] = 1;
    if (wrblock(device, start, 0, 0, eof, sizeof eof, 0x1d)
        != sizeof eof)
    {
        printf("ALLOC: could not initialize data extent\n");
        return 12;
    }
    memset(&dscb, 0, sizeof dscb);
    memset(dscb.ds1dsnam, ' ', sizeof dscb.ds1dsnam);
    memcpy(dscb.ds1dsnam, name, n);
    dscb.ds1fmtid = '1';
    memcpy(dscb.ds1dssn, pdos->volumes[volume].volser, 6);
    dscb.ds1volsq[1] = 1;
#ifdef PDOS_MEDIA_PORT
    media_creation_date((unsigned char *)dscb.ds1credt);
#else
    now = time(NULL);
    today = gmtime(&now);
    if (today != NULL)
    {
        int day = today->tm_yday + 1;
        dscb.ds1credt[0] = (unsigned char)today->tm_year;
        dscb.ds1credt[1] = (unsigned char)(day >> 8);
        dscb.ds1credt[2] = (unsigned char)day;
    }
#endif
    dscb.ds1noepv = 1;
    memset(dscb.ds1syscd, ' ', sizeof dscb.ds1syscd);
    memcpy(dscb.ds1syscd, "PDOS", 4);
    dscb.ds1dsorg = 0x4000;
    dscb.ds1recfm = strcmp(format, "FB") == 0 ? 0x90
                    : strcmp(format, "VB") == 0 ? 0x50 : 0xc0;
    dscb.ds1blkl = blksize;
    dscb.ds1lrecl = lrecl;
    dscb.ds1dsind = 0x80;
    dscb.ds1scal1 = 0xc0;
    dscb.ds1scal3[2] = 1;
    dscb.ds1lstar[2] = 1;
    dscb.ds1trbal[0] = 0xe2;
    dscb.ds1trbal[1] = 0xfa;
    dscb.ds1ext1[0] = 0x81;
    dscb.startcchh[0] = (unsigned char)(start >> 8);
    dscb.startcchh[1] = (unsigned char)start;
    dscb.endcchh[0] = (unsigned char)((start + cylinders - 1) >> 8);
    dscb.endcchh[1] = (unsigned char)(start + cylinders - 1);
    dscb.endcchh[3] = 14;
    if (wrblock(device, freec, freeh, freer, &dscb,
                sizeof dscb, 0x0d) != sizeof dscb
        || rdblock(device, freec, freeh, freer, check,
                   sizeof check, 0x0e) != sizeof check
        || memcmp(check, &dscb, sizeof dscb) != 0)
    {
        memset(blank, 0, sizeof blank);
        wrblock(device, freec, freeh, freer, blank, sizeof blank, 0x0d);
        printf("ALLOC: VTOC write/readback failed\n");
        return 12;
    }
    printf("ALLOC: %s %.6s cyl=%d..%d %s/%d/%d\n", name,
           pdos->volumes[volume].volser, start, start + cylinders - 1,
           format, lrecl, blksize);
    return 0;
#endif
}

/* Preserve physical FB/VB blocks, including a variable record whose data
   length is zero. The byte-stream COPY command cannot promise that case.
   This bounded path accepts only separate, one-cylinder PS extents whose
   entire logical file fits on its first track. */
static int pdosRecordCopy(PDOS *pdos, const char *input)
{
#ifndef ZARCH
    (void)pdos; (void)input;
    printf("RCOPY: unsupported on this build\n");
    return 8;
#else
    char source[48], target[48], extra;
    DSCB1 src, dst;
    unsigned char probe[MAXBLKSZ], verify[MAXBLKSZ], countbuf[MAXBLKSZ + 8];
    unsigned char *saved;
    int sizes[64], offsets[64];
    int i, j, n, sc, sh, tc, th, total, records, length, format, lrecl;
    int vc, vh, vr, rc;
    if (sscanf(input, "%47s %47s %c", source, target, &extra) != 2)
    {
        printf("usage: RCOPY source target\n");
        return 8;
    }
    for (i = 0; source[i]; i++) source[i] = toupper((unsigned char)source[i]);
    for (i = 0; target[i]; i++) target[i] = toupper((unsigned char)target[i]);
    if (strcmp(source, target) == 0 || pdosVolumeIndex(pdos, pdos->curdev) <= 0
        || !pdos64ReadDscbLoc(pdos->curdev, source, &src, NULL, NULL, NULL)
        || !pdos64ReadDscbLoc(pdos->curdev, target, &dst, &vc, &vh, &vr))
    {
        printf("RCOPY: separate existing datasets on exchange disk required\n");
        return 8;
    }
    sc = ((unsigned char)src.startcchh[0] << 8) | (unsigned char)src.startcchh[1];
    sh = ((unsigned char)src.startcchh[2] << 8) | (unsigned char)src.startcchh[3];
    tc = ((unsigned char)dst.startcchh[0] << 8) | (unsigned char)dst.startcchh[1];
    th = ((unsigned char)dst.startcchh[2] << 8) | (unsigned char)dst.startcchh[3];
    format = (unsigned char)src.ds1recfm;
    lrecl = (unsigned short)src.ds1lrecl;
    if (src.ds1dsorg != 0x4000 || dst.ds1dsorg != 0x4000
        || src.ds1noepv != 1 || dst.ds1noepv != 1
        || (format != 0x90 && format != 0x50)
        || dst.ds1recfm != src.ds1recfm
        || dst.ds1blkl != src.ds1blkl || dst.ds1lrecl != src.ds1lrecl
        || sc < 3 || tc < 3 || sc == tc || sc >= 100 || tc >= 100
        || sh != 0 || th != 0
        || src.endcchh[0] != src.startcchh[0]
        || src.endcchh[1] != src.startcchh[1]
        || dst.endcchh[0] != dst.startcchh[0]
        || dst.endcchh[1] != dst.startcchh[1]
        || (unsigned char)src.endcchh[3] != 14
        || (unsigned char)dst.endcchh[3] != 14
        || rdblock(pdos->curdev, tc, 0, 1, probe,
                   sizeof probe, 0x0e) != 0)
    {
        printf("RCOPY: matching FB/VB geometry and empty target required\n");
        return 8;
    }
    saved = malloc(50000);
    if (saved == NULL) return 12;
    records = total = 0; rc = 8;
    for (n = 1; n <= 64; n++)
    {
        length = rdblock(pdos->curdev, sc, 0, n, probe,
                         sizeof probe, 0x0e);
        if (length == 0) { rc = 0; break; }
        if (length < 0 || length > (unsigned short)src.ds1blkl
            || total + length > 50000) break;
        if (format == 0x90)
        {
            if (lrecl < 1 || length % lrecl != 0) break;
        }
        else
        {
            if (length < 4 || (((unsigned char)probe[0] << 8)
                               | (unsigned char)probe[1]) != length) break;
            for (j = 4; j < length; )
            {
                int rdw;
                if (j + 4 > length) break;
                rdw = ((unsigned char)probe[j] << 8)
                      | (unsigned char)probe[j + 1];
                if (rdw < 4 || rdw > lrecl || j + rdw > length) break;
                j += rdw;
            }
            if (j != length) break;
        }
        offsets[records] = total; sizes[records] = length;
        memcpy(saved + total, probe, length);
        total += length; records++;
    }
    if (rc != 0)
    {
        printf("RCOPY: source framing, length or EOF invalid\n");
        free(saved); return 8;
    }
    if (records == 0)
    {
        printf("RCOPY: empty source; empty target retained\n");
        free(saved); return 0;
    }
    for (n = 1; n <= records + 1; n++)
    {
        length = n <= records ? sizes[n - 1] : 0;
        countbuf[0] = (unsigned char)(tc >> 8);
        countbuf[1] = (unsigned char)tc;
        countbuf[2] = countbuf[3] = countbuf[5] = 0;
        countbuf[4] = (unsigned char)n;
        countbuf[6] = (unsigned char)(length >> 8);
        countbuf[7] = (unsigned char)length;
        if (length) memcpy(countbuf + 8, saved + offsets[n - 1], length);
        if (wrblock(pdos->curdev, tc, 0, n - 1, countbuf,
                    length + 8, 0x1d) != length + 8)
        {
            printf("RCOPY: output write failed at block %d\n", n);
            free(saved); return 12;
        }
    }
    for (n = 1; n <= records; n++)
        if (rdblock(pdos->curdev, tc, 0, n, verify,
                    sizeof verify, 0x0e) != sizes[n - 1]
            || memcmp(verify, saved + offsets[n - 1], sizes[n - 1]) != 0)
        {
            printf("RCOPY: output readback failed at block %d\n", n);
            free(saved); return 12;
        }
    free(saved);
    dst.ds1lstar[0] = dst.ds1lstar[1] = 0;
    dst.ds1lstar[2] = (unsigned char)(records + 1);
    if (wrblock(pdos->curdev, vc, vh, vr,
                ((char *)&dst) + 44, sizeof dst - 44, 0x05)
        != sizeof dst - 44) return 12;
    printf("RCOPY: %s -> %s blocks=%d bytes=%d\n",
           source, target, records, total);
    return 0;
#endif
}

static int pdosTapeCommand(PDOS *pdos, const char *input, char *first)
{
    int address, device, i, count;
    char extra;
    unsigned int hash;
    char block[MAXBLKSZ];
        if (strcmp(first, "STATUS") == 0)
        {
            if (pdos->tape_device)
                printf("TAPE: mounted %04X %s\n", pdos->tape_address,
                       pdos->tape_writable ? "write" : "read");
            else
                printf("TAPE: off\n");
            return 0;
        }
        if (strcmp(first, "OFF") == 0)
        {
#ifdef PDOS_MEDIA_PORT
            if (media_busy((unsigned int)pdos->tape_device)) return 8;
#endif
#if defined(ZARCH) && !defined(PDOS_MEDIA_PORT)
            TIOT *tiot = pdos->psa->psatold->tcbtio;
            for (i = 0; i < tiot->count; i++)
                if (tiot->device[i] == pdos->tape_device
                    && pdos->tape_device && tiot->open_count[i] != 0)
                    return 8;
            for (i = tiot->count - 1; i >= 0; i--)
                if (tiot->device[i] == pdos->tape_device
                    && pdos->tape_device)
                    pdos64FreeDd(pdos, tiot,
                                 (char *)tiot->ddentry[i] + 4);
#endif
            pdos->tape_device = pdos->tape_address = pdos->tape_writable = 0;
            printf("TAPE: off\n");
            return 0;
        }
        if (strcmp(first, "MOUNT") == 0)
        {
            char mode[16];
            if (pdos->tape_device) return 8;
            mode[0] = '\0';
            count = sscanf(input, "%15s %x %15s %c",
                           first, &address, mode, &extra);
            if (count < 2 || count > 3
                || address <= 0 || address > 0xffff) return 8;
            for (i = 0; mode[i]; i++) mode[i] = toupper((unsigned char)mode[i]);
            if (mode[0] && strcmp(mode, "WRITE") != 0) return 8;
            device = getssid(address);
            if (!device || pdosVolumeIndex(pdos, device) >= 0) return 8;
            pdos->tape_device = device;
            pdos->tape_address = address;
            pdos->tape_writable = strcmp(mode, "WRITE") == 0;
            printf("TAPE: %04X %s; position is device current position\n",
                   address, pdos->tape_writable ? "write" : "read");
            return 0;
        }
        if (strcmp(first, "READ") == 0 && pdos->tape_device)
        {
            count = rdtape(pdos->tape_device, block, sizeof block);
            if (count < 0) { printf("TAPE: read error\n"); return 12; }
            hash = 2166136261U;
            for (i = 0; i < count; i++)
                hash = (hash ^ (unsigned char)block[i]) * 16777619U;
            printf("TAPE: record length=%d fnv32=%08X", count, hash);
            for (i = 0; i < count && i < 16; i++)
                printf(" %02X", (unsigned char)block[i]);
            printf("\n");
            return 0;
        }
        if (strcmp(first, "REWIND") == 0 && pdos->tape_device)
        {
            if (tapectl(pdos->tape_device, 0x07) != 0)
            { printf("TAPE: rewind error\n"); return 12; }
            printf("TAPE: rewound %04X\n", pdos->tape_address);
            return 0;
        }
        if (strcmp(first, "SCAN") == 0 && pdos->tape_device)
        {
            unsigned long total = 0;
            int records = 0;
            while (records < 100000 && total < 512UL * 1024 * 1024)
            {
                count = rdtape(pdos->tape_device, block, sizeof block);
                if (count < 0) { printf("TAPE: scan read error\n"); return 12; }
                if (count == 0)
                {
                    printf("TAPE: filemark records=%d bytes=%lu\n",
                           records, total);
                    return 0;
                }
                records++;
                total += count;
            }
            printf("TAPE: scan limit records=%d bytes=%lu\n", records, total);
            return 8;
        }
        if (strcmp(first, "MARK") == 0 && pdos->tape_device
            && pdos->tape_writable)
        {
            if (tapectl(pdos->tape_device, 0x1f) != 0)
            { printf("TAPE: mark error\n"); return 12; }
            printf("TAPE: filemark written\n");
            return 0;
        }
        if (strcmp(first, "WRITE") == 0 && pdos->tape_device
            && pdos->tape_writable)
        {
            const char *hex = strchr(input, ' ');
            const char *digits = "0123456789ABCDEF";
            int n, hi, lo;
            if (hex == NULL) return 8;
            while (*hex == ' ') hex++;
            n = strlen(hex);
            if (n < 2 || n > 80 || n % 2) return 8;
            for (i = 0; i < n / 2; i++)
            {
                const char *p = strchr(digits,
                                       toupper((unsigned char)hex[2 * i]));
                const char *q = strchr(digits,
                                       toupper((unsigned char)hex[2 * i + 1]));
                if (p == NULL || q == NULL) return 8;
                hi = p - digits;
                lo = q - digits;
                block[i] = (hi << 4) | lo;
            }
            count = wrtape(pdos->tape_device, block, n / 2);
            if (count != n / 2)
            { printf("TAPE: write error\n"); return 12; }
            printf("TAPE: wrote record length=%d\n", count);
            return 0;
        }
        printf("usage: TAPE STATUS, MOUNT addr(hex), REWIND, READ, SCAN\n");
        printf("       TAPE MOUNT addr WRITE, WRITE hex, MARK, OFF\n");
        return 8;
}

/* DEVICES discovers channel addresses without issuing CKD reads to a tape.
   MOUNT validates an explicitly named DASD. TAPE is a separate device class. */
static int pdosMediaCommand(PDOS *pdos, const char *verb, char *parm)
{
    char input[96], first[16], second[16], serial[7];
    int length = *(short *)parm;
    int address, device, i;
    char extra;
    char block[80];

    if (length < 0 || length >= sizeof input) return 8;
    memcpy(input, parm + sizeof(short), length);
    input[length] = '\0';
    first[0] = second[0] = '\0';
    sscanf(input, "%15s %15s", first, second);
    for (i = 0; first[i]; i++) first[i] = toupper((unsigned char)first[i]);
    if (strcmp(verb, "DEVICES") == 0)
    {
        for (i = 0; i < 4096; i++)
        {
            address = getdevn(0x10000 + i);
            if (address == 0) break;
            printf("DEVICE %04X %s\n", address,
                   pdosVolumeIndex(pdos, 0x10000 + i) >= 0 ? "DASD" :
                   pdos->tape_device == 0x10000 + i ? "TAPE" : "unclassified");
        }
        return 0;
    }
    if (strcmp(verb, "VOLUMES") == 0)
    {
        for (i = 0; i < pdos->volume_count; i++)
            printf("VOLUME %04X %.6s %s%s\n", pdos->volumes[i].address,
                   pdos->volumes[i].volser,
                   i == 0 ? "IPL protected" : "exchange",
                   pdos->curdev == pdos->volumes[i].device ? " selected" : "");
        return 0;
    }
    if (strcmp(verb, "ALLOC") == 0)
        return pdosAllocateDataset(pdos, input);
    if (strcmp(verb, "RCOPY") == 0)
        return pdosRecordCopy(pdos, input);
    if (strcmp(verb, "CMS") == 0)
        return pdosCmsCommand(pdos, input);
    if (strcmp(verb, "MOUNT") == 0)
    {
        if (sscanf(input, "%x %15s %c", &address, second, &extra) != 2
            || address <= 0 || address > 0xffff || strlen(second) != 6)
        {
            printf("usage: MOUNT address(hex) expected-volser(6)\n");
            return 8;
        }
        memcpy(serial, second, 7);
        for (i = 0; i < 6; i++) serial[i] = toupper((unsigned char)serial[i]);
        device = getssid(address);
        if (device == 0 || pdos->volume_count >= PDOS_MAX_VOLUMES
            || pdosVolumeIndex(pdos, device) >= 0)
        {
            printf("MOUNT: device absent, already mounted or table full\n");
            return 8;
        }
        if (!pdosReadVolumeLabel(device, block)
            || memcmp(block, serial, 6) != 0)
        {
            printf("MOUNT: VOL1 missing or serial mismatch at %04X\n", address);
            return 8;
        }
        for (i = 0; i < pdos->volume_count; i++)
            if (memcmp(pdos->volumes[i].volser, serial, 6) == 0)
            {
                printf("MOUNT: duplicate volume serial %.6s\n", serial);
                return 8;
            }
        i = pdos->volume_count++;
        pdos->volumes[i].device = device;
        pdos->volumes[i].address = address;
        memcpy(pdos->volumes[i].volser, serial, 7);
#if defined(ZARCH) && !defined(PDOS_MEDIA_PORT)
        memset(pdos64_dasd_ucb[i], 0, 64);
        pdos64_dasd_ucb[i][0x12] = 0x20;
        memcpy(pdos64_dasd_ucb[i] + 0x1c, serial, 6);
#endif
        printf("MOUNT: %04X %.6s ready\n", address, serial);
        return 0;
    }
    if (strcmp(verb, "SELECT") == 0 || strcmp(verb, "UNMOUNT") == 0)
    {
        if (strlen(first) != 6 || second[0] != '\0') return 8;
        for (i = 0; i < pdos->volume_count; i++)
            if (memcmp(pdos->volumes[i].volser, first, 6) == 0) break;
        if (i == pdos->volume_count) return 8;
        if (strcmp(verb, "SELECT") == 0)
        {
            pdos->curdev = pdos->volumes[i].device;
            printf("SELECT: %.6s\n", first);
            return 0;
        }
        if (i == 0 || i != pdos->volume_count - 1
            || pdos->curdev == pdos->volumes[i].device)
        {
            printf("UNMOUNT: IPL, selected or not the last mount\n");
            return 8;
        }
#ifdef PDOS_MEDIA_PORT
        if (media_busy((unsigned int)pdos->volumes[i].device)) return 8;
#endif
#if defined(ZARCH) && !defined(PDOS_MEDIA_PORT)
        {
            TIOT *tiot = pdos->psa->psatold->tcbtio;
            int n;
            for (n = 0; n < tiot->count; n++)
                if (tiot->device[n] == pdos->volumes[i].device)
                {
                    if (tiot->open_count[n] != 0)
                    {
                        printf("UNMOUNT: volume has open DD\n");
                        return 8;
                    }
                }
            for (n = 0; n < PDOS64_PDS_WRITES; n++)
                if (pdos64_pds_writes[n].dcb != NULL
                    && pdos64_pds_writes[n].device == pdos->volumes[i].device)
                    return 8;
            for (n = tiot->count - 1; n >= 0; n--)
                if (tiot->device[n] == pdos->volumes[i].device)
                    pdos64FreeDd(pdos, tiot,
                                 (char *)tiot->ddentry[n] + 4);
        }
#endif
        for (; i + 1 < pdos->volume_count; i++)
        {
            pdos->volumes[i] = pdos->volumes[i + 1];
#if defined(ZARCH) && !defined(PDOS_MEDIA_PORT)
            memcpy(pdos64_dasd_ucb[i], pdos64_dasd_ucb[i + 1], 64);
#endif
        }
        pdos->volume_count--;
        printf("UNMOUNT: %.6s\n", first);
        return 0;
    }
    if (strcmp(verb, "TAPE") == 0)
        return pdosTapeCommand(pdos, input, first);
    return 8;
}
