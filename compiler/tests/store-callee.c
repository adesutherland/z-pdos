/* SPDX-License-Identifier: MIT
 * This callee may overwrite the caller-owned incoming argument area.
 */
void take(int x) { volatile int *p = &x; *p = 99; }
