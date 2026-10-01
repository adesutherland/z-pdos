#!/bin/sh
# Modified for Mainframe Classic C on 2026-10-01; see component SOURCES.md.
# cc370 host-side codegen regression tests.
#
# Uses the driver-private cc1 built by `make compiler` (build/gcc/cc1); override
# with CC1=/path/to/cc1.  Each case compiles a small C snippet to i370 HLASM and
# checks a property of the emitted assembler (no MVS required).
cd "$(dirname "$0")" || exit 2
ROOT=../..
CC1=${CC1:-$ROOT/build/gcc/cc1}
if [ ! -x "$CC1" ]; then
    echo "cc1 not found at $CC1 -- run 'make compiler' first (or set CC1=)" >&2
    exit 2
fi
WORK=$(mktemp -d "${TMPDIR:-/tmp}/cc370test.XXXXXX") || exit 2
trap 'rm -rf "$WORK"' 0
fail=0

# Compile $2 (a C file) with the extra flags in $3; capture diagnostics to
# $WORK/diag.  Returns the compiler exit status.
compile () { $CC1 -quiet -std=c89 $3 "$2" -o "$WORK/$1.s" >"$WORK/diag" 2>&1; }

# --- issue #17: external symbols collide silently when identifiers share a ---
# long prefix.  MVS/370 externals are limited to 8 characters; distinct C
# identifiers that truncate to the same name used to link to one another with
# no diagnostic.  cc370 must now warn.  An explicit __asm__ name (the issue's
# workaround) is emitted verbatim and must NOT warn.

# (1) Mike Beer's aliases replace truncation. Check independently calculated
# 32-bit DJB2/base36 names, rather than requiring the superseded warning.
cat > "$WORK/fn.c" <<'EOF'
void codec_stream_encode(int x);
void codec_stream_decode(int x);
void codec_stream_encode(int x) { codec_stream_decode(x); }
void codec_stream_decode(int x) { codec_stream_encode(x); }
EOF
compile fn "$WORK/fn.c"
if ! grep -q "collides with" "$WORK/diag" && grep -q 'C0EHKXH7' "$WORK/fn.s" && grep -q 'C0DNXCUP' "$WORK/fn.s"; then
    echo "long-fn: OK (distinct fixed-width aliases)"
else
    echo "long-fn: FAIL (incorrect aliases for codec_stream_encode/decode)"; fail=1
fi

# (2) two common (uninitialized) globals colliding -> warn (the other ESD path).
cat > "$WORK/cm.c" <<'EOF'
int codec_stream_state;
int codec_stream_stats;
EOF
compile cm "$WORK/cm.c"
if ! grep -q "collides with" "$WORK/diag" && grep -q 'C02VB0EM' "$WORK/cm.s" && grep -q 'C02VB0F0' "$WORK/cm.s"; then
    echo "long-common: OK (distinct fixed-width aliases)"
else
    echo "long-common: FAIL (incorrect common aliases)"; fail=1
fi

# (3) distinct 8-char prefixes must NOT warn (no false positive).
cat > "$WORK/ok.c" <<'EOF'
void codec_stream_encode(int x);
void codec_block_decode(int x);
void codec_stream_encode(int x) { codec_block_decode(x); }
void codec_block_decode(int x) { codec_stream_encode(x); }
EOF
compile ok "$WORK/ok.c"
if grep -q "collides with" "$WORK/diag"; then
    echo "distinct: FAIL (false positive on CODEC@ST vs CODEC@BL)"; fail=1
else
    echo "distinct: OK (no false positive)"
fi

# (4) the __asm__ workaround: two 8-char linkage names that differ are distinct
# object-deck symbols -> must NOT warn (no false positive on the workaround).
cat > "$WORK/asm.c" <<'EOF'
extern int cfg_p __asm__("FTPCFGDP");
extern int cfg_f __asm__("FTPCFGDF");
int cfg_p = 1;
int cfg_f = 2;
EOF
compile asm "$WORK/asm.c"
if grep -q "collides with" "$WORK/diag"; then
    echo "asm-name: FAIL (false positive on distinct 8-char __asm__ names)"; fail=1
else
    echo "asm-name: OK (distinct 8-char asm names)"
fi

# (5) __asm__ names LONGER than 8 chars that share their first 8: as370
# truncates the external symbol to 8 silently (diverging from Assembler XF,
# which diagnoses an over-length symbol), so PREFIXAB1 and PREFIXAB2 both
# become PREFIXAB. cc370 models as370 and must warn -- the workaround does not
# save a name that is itself too long.
cat > "$WORK/asmlong.c" <<'EOF'
extern int p __asm__("PREFIXAB1");
extern int f __asm__("PREFIXAB2");
int p = 1;
int f = 2;
EOF
compile asmlong "$WORK/asmlong.c"
if grep -q "collides with" "$WORK/diag" && grep -q "PREFIXAB" "$WORK/diag"; then
    echo "asm-long: OK (warned, both truncate to PREFIXAB)"
else
    echo "asm-long: FAIL (>8-char asm names truncating to PREFIXAB not caught)"; fail=1
fi

# (6) -Werror must promote the collision to a hard error (nonzero exit).
compile fnw "$WORK/asmlong.c" -Werror
if [ $? -ne 0 ]; then
    echo "werror: OK (collision is an error under -Werror)"
else
    echo "werror: FAIL (collision did not fail the build under -Werror)"; fail=1
fi

# --- issue #40: a 64-bit load through a dying pointer used to clobber its ---
# own base register: `L 2,0(2)` / `L 3,4+0(2)` -- the second load indexes off
# the loaded VALUE, a wild address on a 24-bit machine (S0C4 on MVS).  The
# fix loads the word the address still needs last.

# Scanner for the broken pair: `L d,disp(b)` with d==b immediately followed
# by `L d+1,4+disp(b)`.  (A lone self-clobbering L is ordinary pointer
# chasing and correct; only the pair is the defect.)
scan_pair () {
    awk '
    /^\* Function .* code/ { fn=$3 }
    { if (match($0, /^ +L +[0-9]+,[0-9]+\([0-9]+\)$/)) {
        split($0,a,/[ ,()]+/); d1=a[3];p1=a[4];b1=a[5]; prev=1; next }
      if (prev && match($0, /^ +L +[0-9]+,4\+[0-9]+\([0-9]+\)$/)) {
        split($0,c,/[ ,()+]+/); d2=c[3];p2=c[5];b2=c[6]
        if (d1==b1 && d2==d1+1 && b2==b1 && p2==p1) print fn }
      prev=0 }' "$1"
}

# (1) positive control: the scanner must flag a canned bad sequence --
# otherwise "no hits" below would also be the output of a broken scanner.
cat > "$WORK/canned.s" <<'EOF'
* Function bad code
         L     2,0(11)
         L     2,0(2)
         L     3,4+0(2)
EOF
if [ -n "$(scan_pair "$WORK/canned.s")" ]; then
    echo "pair-scanner: OK (flags the known-bad sequence)"
else
    echo "pair-scanner: FAIL (scanner does not detect the defect pattern)"; fail=1
fi

# (2) the issue's reproducer plus the correct-by-contrast variants: none may
# contain the broken pair.
cat > "$WORK/di.c" <<'EOF'
struct s { char pad[16]; unsigned long long v; int tail; };
unsigned f(const unsigned long long *p) { return (unsigned)(*p >> 32); }
unsigned a_dead(const struct s *p) { return (unsigned)(p->v >> 32); }
unsigned b_live(const struct s *p) { return (unsigned)(p->v >> 32)
                                          + (unsigned)p->tail; }
unsigned e_low (const struct s *p) { return (unsigned)p->v; }
int      d_cmp (const struct s *p, unsigned long long t) { return p->v < t; }
EOF
compile di "$WORK/di.c" "-std=gnu99 -O1"
bad=$(scan_pair "$WORK/di.s")
if [ -z "$bad" ]; then
    echo "di-load: OK (no self-clobbering load pair)"
else
    echo "di-load: FAIL (base-clobbering pair in: $bad)"; fail=1
fi

# (3) the fix path must actually have fired: when the pair overlaps the base,
# the low word is loaded first -- `L x,4+disp(b)` immediately followed by
# `L b,disp(b)`.  Guards against the allocator merely happening to avoid the
# overlap (which would leave the emit path untested).
rev=$(awk '
    { if (match($0, /^ +L +[0-9]+,4\+[0-9]+\([0-9]+\)$/)) {
        split($0,a,/[ ,()+]+/); d1=a[3];p1=a[5];b1=a[6]; prev=1; next }
      if (prev && match($0, /^ +L +[0-9]+,[0-9]+\([0-9]+\)$/)) {
        split($0,c,/[ ,()]+/); d2=c[3];p2=c[4];b2=c[5]
        if (d2==b1 && b2==b1 && p2==p1) print "rev" }
      prev=0 }' "$WORK/di.s")
if [ -n "$rev" ]; then
    echo "di-reversed: OK (overlapping pair loads the low word first)"
else
    echo "di-reversed: FAIL (no reversed pair found -- fix path never fired)"; fail=1
fi

# --- issue #468: a 64-bit shift left was emitted as SLDA, the ARITHMETIC ---
# double shift, which keeps bit 0 of the even register and shifts only the
# 63-bit magnitude -- so (unsigned long long)0xFFFFFFFF << 32 came back as
# 7FFFFFFF00000000.  A C left shift is logical on the bit pattern: SLDL.

# (1) the issue's three functions plus a variable shift count: no SLDA, and
# one SLDL each -- the count guards against the shift being lowered to
# something else entirely, which would also have no SLDA.
cat > "$WORK/shl.c" <<'EOF'
unsigned long long shl32u(unsigned long h) { return (unsigned long long)h << 32; }
long long          shl32s(long h)          { return (long long)h << 32; }
unsigned long long join(unsigned long h, unsigned long l) { return ((unsigned long long)h << 32) | l; }
unsigned long long shlv(unsigned long long x, int n) { return x << n; }
EOF
compile shl "$WORK/shl.c" "-std=gnu99 -O1"
slda=$(grep -c '^ *SLDA ' "$WORK/shl.s")
sldl=$(grep -c '^ *SLDL ' "$WORK/shl.s")
if [ "$slda" = 0 ] && [ "$sldl" -ge 4 ]; then
    echo "di-shl: OK (no SLDA, $sldl SLDL)"
else
    echo "di-shl: FAIL ($slda SLDA, $sldl SLDL; want 0 and >= 4)"; fail=1
fi

# (2) control: a signed 64-bit shift RIGHT is arithmetic and must stay SRDA.
cat > "$WORK/sra.c" <<'EOF'
long long sra3(long long x) { return x >> 3; }
EOF
compile sra "$WORK/sra.c" "-std=gnu99 -O1"
if grep -q '^ *SRDA ' "$WORK/sra.s"; then
    echo "di-sra: OK (signed shift right is still SRDA)"
else
    echo "di-sra: FAIL (no SRDA for a signed 64-bit shift right)"; fail=1
fi

# --- issue #470: a libcall's name is the libgcc name cut to 8 characters, ---
# and four pairs cut to the same one: __fixunssfdi/__fixunsdfdi -> @@FIXUNS,
# __floatdisf/__floatdidf -> @@FLOATD, __popcountsi2/__popcountdi2 ->
# @@POPCOU, __paritysi2/__paritydi2 -> @@PARITY.  One library member cannot
# serve two signatures.  Each call must now name its own helper.

# (1) each function below calls exactly one helper, and each a different one.
cat > "$WORK/lf.c" <<'EOF'
unsigned long long fxunsf(float f)  { return (unsigned long long)f; }
unsigned long long fxundf(double d) { return (unsigned long long)d; }
float              fltdsf(long long x) { return (float)x; }
double             fltddf(long long x) { return (double)x; }
int popcsi(unsigned x)           { return __builtin_popcount(x); }
int popcdi(unsigned long long x) { return __builtin_popcountll(x); }
int partsi(unsigned x)           { return __builtin_parity(x); }
int partdi(unsigned long long x) { return __builtin_parityll(x); }
EOF
compile lf "$WORK/lf.c" "-std=gnu99 -O1"
got=$(awk '/^\* X-func/{f=$3} /V\(@@/{match($0,/V\([^)]*\)/); print f "=" substr($0,RSTART+2,RLENGTH-3)}' "$WORK/lf.s" | tr '\n' ' ')
want="fxunsf=@@FXUNSF fxundf=@@FXUNDF fltdsf=@@FLTDSF fltddf=@@FLTDDF popcsi=@@POPCSI popcdi=@@POPCDI partsi=@@PARTSI partdi=@@PARTDI "
if [ "$got" = "$want" ]; then
    echo "libcall-names: OK (eight helpers, eight names)"
else
    echo "libcall-names: FAIL"; echo "   got:  $got"; echo "   want: $want"; fail=1
fi

# (2) control: the helpers whose names did not collide keep them -- libc370
# defines the first six, so moving one would break every consumer's link.
cat > "$WORK/lk.c" <<'EOF'
long long          mul(long long a, long long b) { return a * b; }
long long          dv(long long a, long long b)  { return a / b; }
long long          md(long long a, long long b)  { return a % b; }
unsigned long long ud(unsigned long long a, unsigned long long b) { return a / b; }
unsigned long long um(unsigned long long a, unsigned long long b) { return a % b; }
long long          ng(long long a) { return -a; }
long long          fxdf(double d) { return (long long)d; }
long long          fxsf(float f)  { return (long long)f; }
EOF
compile lk "$WORK/lk.c" "-std=gnu99 -O1"
got=$(grep -o 'V(@@[A-Z0-9@]*)' "$WORK/lk.s" | tr '\n' ' ')
want="V(@@MULDI3) V(@@DIVDI3) V(@@MODDI3) V(@@UDIVDI) V(@@UMODDI) V(@@NEGDI2) V(@@FIXDFD) V(@@FIXSFD) "
if [ "$got" = "$want" ]; then
    echo "libcall-keep: OK (the non-colliding names are unchanged)"
else
    echo "libcall-keep: FAIL"; echo "   got:  $got"; echo "   want: $want"; fail=1
fi

# --- issue #484: a wide character constant must carry the same EBCDIC value ---
# as a narrow one and as the element of a wide string literal.  The string
# element is emitted byte by byte through MAP_OUTCHAR, so the constant is
# mapped the same way, and a numeric escape is pre-imaged on the wide path as
# it is on the narrow one, so that it stays the literal value in both.
# Printable bytes leave as C'..' and become EBCDIC in the assembler: C'a' is
# X'81'.
cat > "$WORK/wc.c" <<'EOF'
int wa = L'a';
int na = 'a';
int wn = L'\n';
int nn = '\n';
int wx = L'\x81';
int nx = '\x81';
int wy = L'\x141';
const int sa[] = L"a";
const int sn[] = L"\n";
const int sx[] = L"\x81";
const int sy[] = L"\x141";
EOF
compile wc "$WORK/wc.c"
# One line per symbol: its DC operands, up to the terminating element.
got=$(awk '/ EQU /{if (s) print s; s=$1 ":"; next}
           /^[[:space:]]+DC[[:space:]]/{s=s " " $2}
           END{print s}' "$WORK/wc.s" | sed "s/ X'0' X'0' X'0' X'0'\$//")
want="WA: F'129'
NA: F'129'
WN: F'21'
NN: F'21'
WX: F'129'
NX: F'129'
WY: F'321'
SA: X'0' X'0' X'0' C'a'
SN: X'0' X'0' X'0' X'15'
SX: X'0' X'0' X'0' C'a'
SY: X'0' X'0' X'1' X'41'"
if [ "$got" = "$want" ]; then
    echo "wide-charconst: OK (L'x' == 'x' == L\"x\"[0], escapes literal)"
else
    echo "wide-charconst: FAIL"; echo "   got:"; echo "$got" | sed 's/^/      /'
    echo "   want:"; echo "$want" | sed 's/^/      /'; fail=1
fi

# --- issue #477: builtin folds must use the target (EBCDIC) value ---
# A string literal stays in the host charset until output, so a fold that
# turns a string byte into a value (putchar/fputc of a one-character string)
# or into an ordering (strcmp/strncmp/memcmp of two literals) must map it
# first.  One line per function: every operand that carries the folded value.
# The comparisons are chosen so that ASCII and EBCDIC disagree in both
# directions: 'a' < 'B' in EBCDIC (X'81' < X'C2'), '1' > 'a' (X'F1' > X'81').
cat > "$WORK/bf.c" <<'EOF'
typedef struct FILE FILE;
int printf(const char *, ...);
int fprintf(FILE *, const char *, ...);
int fputs(const char *, FILE *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, __SIZE_TYPE__);
int memcmp(const void *, const void *, __SIZE_TYPE__);
void pa(void)        { printf("A"); }
void pn(void)        { printf("\n"); }
void px(void)        { printf("\x81"); }
void fa(FILE *f)     { fputs("A", f); }
void fp(FILE *f)     { fprintf(f, "A"); }
int  c1(void)        { return strcmp("a", "B"); }
int  c2(void)        { return strcmp("1", "a"); }
int  c3(void)        { return strncmp("a", "B", 1); }
int  c4(void)        { return memcmp("a", "B", 1); }
int  c5(void)        { return strcmp("ab", "ab"); }
int  c6(void)        { return strncmp("ab", "ac", 1); }
EOF
compile bf "$WORK/bf.c" "-O1"
got=$(awk '/^\* X-func/{if (f) print s; f=$3; s=f ":"; next}
           /=F.-?[0-9]+.|LA +15,|SLR +15,15|L +15,=V\(/{sub(/^[ \t]+/, ""); gsub(/[ \t]+/, " "); s=s " [" $0 "]"}
           END{print s}' "$WORK/bf.s")
want="pa: [MVC 88(4,13),=F'193'] [L 15,=V(PUTCHAR)]
pn: [MVC 88(4,13),=F'21'] [L 15,=V(PUTCHAR)]
px: [MVC 88(4,13),=F'129'] [L 15,=V(PUTCHAR)]
fa: [MVC 88(4,13),=F'193'] [L 15,=V(FPUTC)]
fp: [MVC 88(4,13),=F'193'] [L 15,=V(FPUTC)]
c1: [L 15,=F'-1']
c2: [LA 15,1(0,0)]
c3: [L 15,=F'-1']
c4: [L 15,=F'-1']
c5: [SLR 15,15]
c6: [SLR 15,15]"
if [ "$got" = "$want" ]; then
    echo "builtin-fold-ebcdic: OK (putchar/fputc get EBCDIC, literal compares in EBCDIC order)"
else
    echo "builtin-fold-ebcdic: FAIL"; echo "   got:"; echo "$got" | sed 's/^/      /'
    echo "   want:"; echo "$want" | sed 's/^/      /'; fail=1
fi

# --- issue #487: a string byte read as a value must be the EBCDIC byte ---
# At -O1 a read of a known string byte folds to a constant (expr.c), and a
# designator into a string-initialised array splits the string into numbers
# (c-typeck.c).  Both took the host byte; the program reads the mapped one.
# char is unsigned on i370, so "a"[0] is 129.  "\x81" and "\xFF" check that
# the escape pre-image maps back to the literal value.
cat > "$WORK/sb.c" <<'EOF'
static const char tbl[] = "ABC";
const char gtbl[] = "ABC";
int s_idx(void)  { return "a"[0]; }
int s_ind(void)  { return *"a"; }
int s_ind1(void) { return *("ab" + 1); }
int s_stbl(void) { return tbl[0]; }
int s_gtbl(void) { return gtbl[2]; }
int s_nl(void)   { return "\n"[0]; }
int s_esc(void)  { return "\x81"[0]; }
int s_ff(void)   { return "\xFF"[0]; }
struct ds { char s[6]; } dsg = { .s = "abcd", .s[1] = (char)0xF0 };
EOF
compile sb "$WORK/sb.c" "-std=gnu99 -O1"
got=$(awk '/^\* X-func/{f=$3} /^[ \t]+LA +15,[0-9]+\(0,0\)/{print f ": " $2}
           /^DSG /{d=1; s="dsg:"; next} d && /^[ \t]+DC /{s=s " " $2} d && /^[ \t]+DS /{print s; d=0}' "$WORK/sb.s")
want="s_idx: 15,129(0,0)
s_ind: 15,129(0,0)
s_ind1: 15,130(0,0)
s_stbl: 15,193(0,0)
s_gtbl: 15,195(0,0)
s_nl: 15,21(0,0)
s_esc: 15,129(0,0)
s_ff: 15,255(0,0)
dsg: X'81' X'F0' X'83' X'84' X'00'"
if [ "$got" = "$want" ]; then
    echo "string-byte-fold: OK (folded string bytes and split initialisers are EBCDIC)"
else
    echo "string-byte-fold: FAIL"; echo "   got:"; echo "$got" | sed 's/^/      /'
    echo "   want:"; echo "$want" | sed 's/^/      /'; fail=1
fi

# --- issue #485 (1): a wide character above 0xFF keeps its code point ---
# Output maps every byte of a wide element, so a character above 0xFF that
# comes from source text or a UCN must be pre-imaged, as a numeric escape
# is (#484): L"Ł"[0] and L'Ł' are 0x141, not 0x1C1.  Up to 0xFF
# the byte mapping is wanted: U+00E4 is Latin-1 'a-umlaut', CP037 X'43'.
printf '%s\n' \
  'const int su[] = L"Ł";' \
  'const int s8[] = L"'"$(printf '\305\201')"'";' \
  'const int sl[] = L"'"$(printf '\303\244')"'";' \
  "int cu = L'\\u0141';" \
  "int c8 = L'$(printf '\305\201')';" \
  "int cl = L'$(printf '\303\244')';" > "$WORK/wh.c"
compile wh "$WORK/wh.c" "-std=gnu99"
got=$(awk '/ EQU /{if (s) print s; s=$1 ":"; next}
           /^[[:space:]]+DC[[:space:]]/{s=s " " $2}
           END{print s}' "$WORK/wh.s" | sed "s/ X'0' X'0' X'0' X'0'\$//")
want="SU: X'0' X'0' X'1' X'41'
S8: X'0' X'0' X'1' X'41'
SL: X'0' X'0' X'0' X'43'
CU: F'321'
C8: F'321'
CL: F'67'"
if [ "$got" = "$want" ]; then
    echo "wide-high: OK (U+0141 stays 0x141 from UCN and UTF-8, U+00E4 is CP037 X'43')"
else
    echo "wide-high: FAIL"; echo "   got:"; echo "$got" | sed 's/^/      /'
    echo "   want:"; echo "$want" | sed 's/^/      /'; fail=1
fi

# --- issue #485 (2): #if sees the same character value as the code ---
# cpp evaluates a character constant in #if itself; it must get the EBCDIC
# value lex_charconst gives the code, or '#if 'A' == 65' picks the ASCII
# branch on an EBCDIC target.
cat > "$WORK/pi.c" <<'EOF'
#if 'A' == 193
int ok_a;
#endif
#if 'A' == 65
int bad_a;
#endif
#if '\n' == 0x15
int ok_nl;
#endif
#if '\x81' == 0x81
int ok_esc;
#endif
#if L'a' == 129
int ok_wide;
#endif
#if 'ab' == ((129 << 8) | 130)
int ok_multi;
#endif
EOF
compile pi "$WORK/pi.c"
got=$(grep -o '^\* X-var [a-z_]*' "$WORK/pi.s" | sed 's/^\* X-var //' | tr '\n' ' ')
want="ok_a ok_nl ok_esc ok_wide ok_multi "
if [ "$got" = "$want" ]; then
    echo "if-charset: OK (#if evaluates character constants in EBCDIC)"
else
    echo "if-charset: FAIL"; echo "   got:  $got"; echo "   want: $want"; fail=1
fi

# --- issue #467: long long / % * by a constant must stay a libcall ---
# The DR/D and MR/M helper insns used to read as (div:DI ...), (mod:DI ...)
# and (mult:DI ...), so delete_trivially_dead_insns (dead_libcall_p, cse.c)
# replaced the __divdi3/__moddi3/__muldi3 libcall with its REG_EQUAL note --
# a single 32-bit DR or M on the 64-bit value, at -O0 as well as -O1.
# The pre-fix cc1 emits DR, DR and M for the three DI functions.  The SI
# functions are the control: they must keep the inline DR/MR.
cat > "$WORK/dq.c" <<'EOF'
long long d_div(long long a) { return a / 10; }
long long d_mod(long long a) { return a % 10; }
long long d_mul(long long a) { return a * 123456789; }
int s_div(int a) { return a / 10; }
int s_mod(int a) { return a % 10; }
unsigned u_div(unsigned a) { return a / 10; }
int s_mul(int a) { return a * 123456789; }
EOF
compile dq "$WORK/dq.c" "-O1"
got=$(awk '/^\* X-func/{f=$3} /=V\(@@|^[ \t]+(DR|D|MR|M)[ \t]/{print f ": " $1 " " $2}' "$WORK/dq.s")
want="d_div: L 15,=V(@@DIVDI3)
d_mod: L 15,=V(@@MODDI3)
d_mul: L 15,=V(@@MULDI3)
s_div: DR 2,4
s_mod: DR 2,4
u_div: DR 2,4
s_mul: MR 2,4"
if [ "$got" = "$want" ]; then
    echo "di-const: OK (long long / % * by a constant call the helpers, int keeps DR/MR)"
else
    echo "di-const: FAIL"; echo "   got:"; echo "$got" | sed 's/^/      /'
    echo "   want:"; echo "$want" | sed 's/^/      /'; fail=1
fi

# --- issue #511: a UTF-8 source is one byte per CHARACTER in a string -------
# The output pass maps each string byte Latin-1 -> CP037, and a UTF-8 source
# reached it unconverted: "a¬b" was X'62' X'5F' with sizeof 5, '¬' a two-byte
# multi-character constant (25183), and "\u00ac" two bytes as well.  The narrow
# execution set is now ISO-8859-1 and the input set is decided per file.
#   pre-fix cc1: sizeof 5/3, '¬' 25183, #if false, the >U+00FF literal assembles
#   (three junk bytes), a BOM is "stray '\357' in program".
# Controls: the same source in Latin-1 must compile identically (the ecosystem
# has Latin-1 files, and cc370 always read them right); a \x escape is a byte
# value and keeps its two bytes; a character above U+00FF is an error.
python3 - "$WORK" <<'EOF'
import sys
w = sys.argv[1]
src = ('#if \'\u00ac\' == 95\nint ifok = 1;\n#else\nint ifok = 0;\n#endif\n'
       'const char s[] = "a\u00acb";\nint n = sizeof s;\nint c = \'\u00ac\';\n'
       'const char e[] = "\\xC2\\xAC";\nint en = sizeof e;\n')
open(w + '/u8.c', 'w', encoding='utf-8').write(src)
open(w + '/l1.c', 'w', encoding='latin-1').write(src)
open(w + '/ucn.c', 'w').write('const char u[] = "\\u00ac";\nint un = sizeof u;\n')
open(w + '/bom.c', 'w', encoding='utf-8').write('\ufeffint bom = 1;\n')
open(w + '/wide.c', 'w', encoding='utf-8').write('const char w[] = "x\u2014y";\n')
open(w + '/widec.c', 'w', encoding='utf-8').write('int wc = \'\u2014\';\n')
EOF
dcs () { grep -E "^[ \t]+DC[ \t]" "$WORK/$1.s" | tr -s ' \t' ' ' | tr '\n' ';'; }
want=" DC F'1'; DC C'a'; DC X'5F'; DC C'b'; DC X'0'; DC F'4'; DC F'95'; DC C'B'; DC X'AC'; DC X'0'; DC F'3';"
u8fail=0
compile u8 "$WORK/u8.c";   [ "$(dcs u8)" = "$want" ] || { echo "utf8-src: FAIL -- UTF-8 source: $(dcs u8)"; cat "$WORK/diag"; u8fail=1; }
compile l1 "$WORK/l1.c";   [ "$(dcs l1)" = "$want" ] || { echo "utf8-src: FAIL -- Latin-1 control changed: $(dcs l1)"; u8fail=1; }
compile ucn "$WORK/ucn.c" "-std=c99"
[ "$(dcs ucn)" = " DC X'5F'; DC X'0'; DC F'2';" ] || { echo "utf8-src: FAIL -- \\u00ac: $(dcs ucn)"; u8fail=1; }
if ! compile bom "$WORK/bom.c" || grep -q stray "$WORK/diag"; then echo "utf8-src: FAIL -- a byte order mark is not dropped"; cat "$WORK/diag"; u8fail=1; fi
for f in wide widec; do
    if compile $f "$WORK/$f.c" || ! grep -q "U+2014 has no equivalent" "$WORK/diag"; then
        echo "utf8-src: FAIL -- U+2014 in $f.c was not refused"; cat "$WORK/diag"; u8fail=1
    fi
done
if [ $u8fail = 0 ]; then
    echo "utf8-src: OK (UTF-8 and Latin-1 sources give one byte per character; \\x keeps its bytes; above U+00FF is an error)"
else
    fail=1
fi

# --- issue #575: an unconditional B is judged against the page it lands on --
# The jump and indirect_jump patterns chose the short form (B label) when the
# target was on the current page, and only then let mvs_check_page start a new
# one -- so a backward B at the very end of a page was emitted behind the
# DROP/USING of the next page, and as370 (like IFOX00) answered IFO209. The
# conditional branches always asked mvs_check_page first. Seen at -Os in
# rexx370 irx#pars.c; it is a matter of page geometry, not of -Os: the sweep
# below hits the window at -O1. N filler stores push a small loop whose
# back-edge is a plain B across the first page end. Before the fix 6 of these
# variants (N+X = 284, N 279..284) put the B on page 2 with its label on
# page 1; after it, none does, and the same variants take L 14,=A()/BR 14 as
# the first instruction of the new page. Either form there means the sweep
# reached the window; with none, the geometry has moved and the test says so.
pgfail=0; pghit=0; pgn=0
N=260
while [ $N -le 300 ]; do
    X=0
    while [ $X -le 5 ]; do
        { echo 'extern int g(int); extern volatile int v[1000]; extern volatile int w;'
          echo 'void t(void) {'
          i=0; while [ $i -lt $N ]; do echo "  v[$i] = $i;"; i=$((i+1)); done
          echo '  for (;;) {'
          echo '    if (g(1)) { w = 5;'
          j=0; while [ $j -lt $X ]; do echo "      w = $j;"; j=$((j+1)); done
          echo '      continue; }'
          echo '    if (g(0)) return;'
          echo '    w = 7;'
          echo '  }'
          echo '}'; } > "$WORK/pg.c"
        if ! compile pg "$WORK/pg.c" -O1; then echo "page-jump: FAIL -- N=$N X=$X does not compile"; cat "$WORK/diag"; pgfail=1; fi
        pgn=$((pgn + 1))
        # a plain B to a local label must sit on the label's page (pages split at DROP 12)
        bad=$(awk '/DROP[ \t]+12/ { p++ }
                   /^@@L[0-9]+[ \t]+EQU/ { lp[$1] = p }
                   $1 == "B" && $2 ~ /^@@L[0-9]+$/ { n++; bt[n] = $2; bp[n] = p }
                   END { for (i = 1; i <= n; i++) if ((bt[i] in lp) && lp[bt[i]] != bp[i]) print bt[i] }' "$WORK/pg.s")
        if [ -n "$bad" ]; then echo "page-jump: FAIL -- N=$N X=$X: B $bad crosses a page boundary"; pgfail=1; fi
        # the window: an unconditional back-edge is the first instruction of a
        # new page -- B (the defect) or L 14,=A() + BR 14 (the fix); counted on
        # both binaries alike (6 of 246), so it says the sweep still reaches it
        awk '/^@@L[0-9]+[ \t]+EQU/ { seen[$1] = 1 }
             first && $1 == "B" && ($2 in seen) { f = 1 }
             first && $1 == "L" && $2 ~ /^14,=A\(@@L[0-9]+\)$/ { t = $2; sub(/^14,=A\(/, "", t); sub(/\)$/, "", t); if (t in seen) want = 1 }
             want && !first && $1 == "BR" && $2 == "14" { f = 1 }
             { if (!first) want = 0; first = ($0 ~ /^@P[0-9]+[ \t]+EQU/) }
             END { exit !f }' "$WORK/pg.s" && pghit=$((pghit + 1))
        X=$((X + 1))
    done
    N=$((N + 1))
done
if [ $pgfail = 0 ] && [ $pghit -gt 0 ]; then
    echo "page-jump: OK ($pgn variants, no B across a page; $pghit put the back-edge first on a new page)"
elif [ $pgfail = 0 ]; then
    echo "page-jump: FAIL -- no variant reached the page end; the sweep no longer tests #575"; fail=1
else
    fail=1
fi
# --- issue #590: unit-at-a-time is off by default at -O2/-Os ----------------
# GCC 3.4 turns it on for optimize >= 2, and on this target the callgraph
# (a) drops a static table whose only use is a global pointer's initializer
#     -- libc370 @@tolow.c became DC A(@V1+2) with no @V1, rc 8 -- and
# (b) writes top-level asm ahead of every function, so a DCB placed after the
#     code that addresses it lands outside that code's USING (IFO209).
# Both assemble at -O1. The default is now off; -funit-at-a-time still wins.
cat > "$WORK/uat.c" <<'EOF'
static short tabR[3] = { -1, 7, 9 };
short *tab = tabR + 1;
void f(void *p) { __asm__("MVC 0(PROTOLEN,%0),PROTODCB" : : "r"(p)); }
__asm__("\n" "PROTODCB DC CL8'X'\n" "PROTOLEN EQU *-PROTODCB");
EOF
uatfail=0
for o in -O2 -Os; do
    if ! compile uat "$WORK/uat.c" "$o"; then echo "unit-at-a-time: FAIL -- $o does not compile"; cat "$WORK/diag"; uatfail=1; continue; fi
    # (a) the table is emitted and tab points into it
    lbl=$(awk '$1 == "DC" && $2 ~ /^A\(@[A-Z0-9]+\+2\)$/ { s = $2; sub(/^A\(/, "", s); sub(/\+2\)$/, "", s); print s }' "$WORK/uat.s")
    if [ -z "$lbl" ] || ! grep -q "^$lbl " "$WORK/uat.s"; then echo "unit-at-a-time: FAIL -- $o: tab's table '$lbl' is not emitted"; uatfail=1; fi
    # (b) the top-level asm stays behind the function
    fl=$(grep -n 'PDPPRLG' "$WORK/uat.s" | head -1 | cut -d: -f1); dl=$(grep -n '^PROTODCB' "$WORK/uat.s" | head -1 | cut -d: -f1)
    if [ -z "$fl" ] || [ -z "$dl" ] || [ "$dl" -lt "$fl" ]; then echo "unit-at-a-time: FAIL -- $o: PROTODCB (line $dl) ahead of the function (line $fl)"; uatfail=1; fi
done
# -funit-at-a-time is still honoured: the asm moves ahead again
if compile uat "$WORK/uat.c" "-Os -funit-at-a-time"; then
    fl=$(grep -n 'PDPPRLG' "$WORK/uat.s" | head -1 | cut -d: -f1); dl=$(grep -n '^PROTODCB' "$WORK/uat.s" | head -1 | cut -d: -f1)
    [ -n "$dl" ] && [ -n "$fl" ] && [ "$dl" -lt "$fl" ] || { echo "unit-at-a-time: FAIL -- an explicit -funit-at-a-time is no longer honoured"; uatfail=1; }
else echo "unit-at-a-time: FAIL -- -Os -funit-at-a-time does not compile"; uatfail=1; fi
if [ $uatfail = 0 ]; then echo "unit-at-a-time: OK (off at -O2/-Os: the table stays, top-level asm stays behind the code; -funit-at-a-time still works)"; else fail=1; fi

# --- issue #592: strict aliasing is off by default at -O2/-Os ---------------
# Under -fstrict-aliasing the store through a float * cannot touch an int,
# so the function returns the constant 1 without reloading *i (LA 15,1).
# MVS code casts between control-block layouts constantly; over the ecosystem
# the rule moved code in 71 files for 0.1 % of size. The default must reload
# (L 15,0(r)); -fstrict-aliasing still gives the constant.
cat > "$WORK/sa.c" <<'EOF'
int f(int *i, float *fl) { *i = 1; *fl = 2.0f; return *i; }
EOF
safail=0
for o in -O2 -Os; do
    compile sa "$WORK/sa.c" "$o" && grep -qE '^ +L +15,0\(' "$WORK/sa.s" ||
        { echo "strict-aliasing: FAIL -- $o assumes no aliasing (no reload of *i)"; safail=1; }
done
compile sa "$WORK/sa.c" "-Os -fstrict-aliasing" && grep -qE '^ +LA +15,1\(' "$WORK/sa.s" ||
    { echo "strict-aliasing: FAIL -- an explicit -fstrict-aliasing is no longer honoured"; safail=1; }
if [ $safail = 0 ]; then echo "strict-aliasing: OK (off at -O2/-Os: *i is reloaded; -fstrict-aliasing still works)"; else fail=1; fi

[ $fail = 0 ] && echo "ALL CC370 TESTS PASSED" || echo "FAILURES"
exit $fail
