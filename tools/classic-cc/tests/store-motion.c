/* SPDX-License-Identifier: MIT */
extern void take(int);
void twice(void) { take(17); take(17); }
void loop(int n) { while (n-- > 0) { take(17); take(23); } }
