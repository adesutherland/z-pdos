/* SPDX-License-Identifier: MIT
 * Bounded Classic C31 service for the two-space nucleus qualification.
 * The caller supplies a K-space bounce-buffer pointer through the Classic
 * parameter list. No U virtual address is dereferenced in this function.
 */
unsigned int pdosTwoSpaceService(const unsigned int *request)
{
    if (request == 0) return 0xffffffffU;
    if (*request != 0xa1b2c3d4U) return 0xfffffffeU;
    return 0x2468ace0U;
}
