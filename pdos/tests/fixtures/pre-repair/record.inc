static int pdos64PdsRecord(PDOS *pdos, PDOS64PDSWRITE *state,
                            DCB *dcb, const void *data, int len)
{
    unsigned char tbuf[MAXBLKSZ + 8];
    int cyl, head, rec, count, track;
    if (len < 0 || len > MAXBLKSZ || state->failed) return 0;
    split_cchhr(dcb->dcbfdad + 3, &cyl, &head, &rec);
    for (;;)
    {
        rec++;
        track = (cyl - state->basecyl) * 15 + head - state->basehead;
        if (track < 0 || cyl > state->endcyl
            || (cyl == state->endcyl && head > state->endhead))
            return 0;
        *(short *)tbuf = cyl;
        *(short *)(tbuf + 2) = head;
        tbuf[4] = rec;
        tbuf[5] = 0;
        *(short *)(tbuf + 6) = len;
        if (len) memcpy(tbuf + 8, data, len);
        count = wrblock(pdos->ipldev, cyl, head, rec - 1,
                        tbuf, len + 8, 0x1d);
        if (count > 0) break;
        rec = 0;
        head++;
        if (head == 15) { head = 0; cyl++; }
    }
    if (state->records == 0)
    {
        state->first_ttr[0] = track >> 8;
        state->first_ttr[1] = track;
        state->first_ttr[2] = rec;
    }
    state->records++;
    join_cchhr(dcb->dcbfdad + 3, cyl, head, rec);
    return 1;
}
