/* SPDX-License-Identifier: MIT */
extern int first(int);
extern int second(int);
extern int combine(int, int);
int nested(int x) { return combine(first(x), second(x + 1)); }
extern unsigned long long collect(unsigned long long, unsigned long long);
unsigned long long preserve(unsigned long long a, double v)
{ return collect(a, (unsigned long long)v); }
