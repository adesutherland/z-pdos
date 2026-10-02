static void write3270(char *buf, size_t lenbuf, int cr)
{
    static int first = 1;
    static int lineupto = 0;
    static char *hex = "0123456789ABCDEF";

    if (first)
    {
        memset(intbuf, ' ', sizeof intbuf);
        /* normally C3 is used to unlock the keyboard, but we lock it
           instead, with 41, and do an unlock before a read */
        memcpy(intbuf, "\x41\x11\x5d\x7f\x1d\xf0", 6);
        memcpy(intbuf + 6 + 22 * 80, "\x1d\x00\x13\x3c\x5d\x7f\x00", 7);
        if (cons_type == 3275)
        {
            int c;
            int x;

            if (lenbuf > 800)
            {
                lenbuf = 800;
            }
            for (x = 0; x < lenbuf; x++)
            {
                c = buf[x];
                intbuf[6 + x * 2 + 0] = hex[(c >> 4) & 0xf];
                intbuf[6 + x * 2 + 1] = hex[c & 0xf];
            }
            if (cr)
            {
                intbuf[6 + x * 2 + 0] = '1';
                intbuf[6 + x * 2 + 1] = '5';
                x++;
            }
            intbuf[6 + x * 2 + 0] = 'X';
            intbuf[6 + x * 2 + 1] = 'X';
            __conswr(sizeof intbuf, intbuf, 0);
            return;
        }
        first = 0;
    }
    memset(intbuf + 6 + lineupto * 80, ' ', 80);
    memcpy(intbuf + 6 + lineupto * 80, buf, lenbuf);
    __conswr(sizeof intbuf, intbuf, 0);
    lineupto++;
    if (lineupto == 22)
    {
        lineupto--;
        memmove(intbuf + 6, intbuf + 6 + 80, lineupto * 80);
    }
}
