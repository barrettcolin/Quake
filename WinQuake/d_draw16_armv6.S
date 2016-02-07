#include "asm_i386.h"
#include "quakeasm.h"
#include "asm_draw.h"

#if idarm
.global D_DrawZSpans
    @ r0 = pspan
D_DrawZSpans:
    @ Calculate izistep in r8
    ldr r1, =d_zistepu
    @ s13 = d_zistepu
    vldr s13, [r1]
    @ s9 = (float)0x8000
    vldr s9, DZS_0x8000f
    @ s10 = (float)0x10000
    vldr s10, DZS_0x10000f
    @ s15 = d_zistepu * 32768.0
    vmul.f32 s15, s13, s9
    @ s15 = (d_zistepu * 32768.0) * 65536.0 (= izistep)
    vmul.f32 s15, s15, s10
    vcvt.s32.f32 s15, s15

    @ sl(r10), fp(r11), ip(r12), sp(r13), lr(r14), pc(r15)
    @ push (r4, r5), (r6, r7), (r8, r9), (r10, lr)
    push {r4-r10, lr}

    @ s11 = d_zistepv
    ldr r3, =d_zistepv
    vldr s11, [r3]
    @ s12 = d_ziorigin
    ldr r2, =d_ziorigin
    vldr s12, [r2]
    @ r8: izistep
    vmov r8, s15
    @ r9: d_pzbuffer
    ldr r1, =d_pzbuffer
    ldr r9, [r1]
    @ r10: d_zwidth
    ldr r3, =d_zwidth
    ldr r10, [r3]

    @ r8: izistep
    @ r9: d_pzbuffer
    @ r10: d_zwidth
    @ s9: (float)0x8000
    @ s10: (float)0x10000
    @ s11: d_zistepv
    @ s12: d_ziorigin
    @ s13: d_zistepu
DZS_SpanLoop:
    @ float zi = d_ziorigin + dv * d_zistepv + du * d_zistepu
    ldr r4, [r0, +#espan_t_u]
    ldr r5, [r0, +#espan_t_v]
    @ s14: du
    vmov s8, r4
    vcvt.f32.s32 s14, s8
    @ s15: dv
    vmov s8, r5
    vcvt.f32.s32 s15, s8
    @ s8 = d_ziorigin
    vmov.f32 s8, s12
    @ s8 = d_ziorigin + dv * d_zistepv
    vmla.f32 s8, s15, s11
    @ s15 = d_ziorigin + dv * d_zistepv
    vmov.f32 s15, s8
    @ s15 = (d_ziorigin + dv * d_zistepv) + d_zistepu * du
    vmla.f32 s15, s13, s14

    @ s15 = (int)(((d_ziorigin + dv * d_zistepv) + d_zistepu * du) * 0x8000 * 0x10000)
    vmul.f32 s15, s15, s9
    vmul.f32 s15, s15, s10
    vcvt.s32.f32 s15, s15

    @ short* pdest = d_pzbuffer + (d_zwidth * pspan->v) + pspan->u
    @ r6 <- d_zwidth * pspan->v + pspan->u
    mla r6, r10, r5, r4
    ldr r2, [r0, +#espan_t_count]
    ldr r0, [r0, +#espan_t_pnext]
    pld [r0]

    @ r3 <- d_pzbuffer + (d_zwidth * pspan->v + pspan->u) * 2
    add r3, r9, r6, lsl #1

    @ r1 = izi
    vmov r1, s15

    @ outer loop - r8: izistep
    @ inner loop - r0: pspan, r1: izi, r2: count, r3: pdest

    @ Write to word boundary
    tst r3, #2
    subne r2, r2, #1
    asrne r4, r1, #16
    strneh r4, [r3], #2
    addne r1, r1, r8

    @ Write to quadword boundary
DZS_WriteToQuadwordBoundary:
    tst r3, #15
    beq DZS_WriteQuadwords
    cmp r2, #1
    ble DZS_WriteLastShort
    subs r2, #2
    add r4, r1, r8              @ r4 <- izi + izistep
    pkhtb r4, r4, r1, asr #16   @ r4 <- (izi + izistep) & 0xffff0000 | (izi >> 16) & 0xffff
    add r1, r1, r8, lsl #1      @ r1 <- izi + izistep * 2
    str r4, [r3], #4            @ r4 -> [r3], r3 += 4
    bgt DZS_WriteToQuadwordBoundary

DZS_WriteQuadwords:
    cmp r2, #7
    ble DZS_WriteLastWords
    subs r2, #8

    add r4, r1, r8              @ r4 <- izi + izistep
    pkhtb r4, r4, r1, asr #16   @ r4 <- (izi + izistep) & 0xffff0000 | (izi >> 16) & 0xffff
    add r1, r1, r8, lsl #1      @ r1 <- izi + izistep * 2

    add r5, r1, r8              @ r5 <- izi + izistep
    pkhtb r5, r5, r1, asr #16   @ r5 <- (izi + izistep) & 0xffff0000 | (izi >> 16) & 0xffff
    add r1, r1, r8, lsl #1      @ r1 <- izi + izistep * 2

    add r6, r1, r8              @ r6 <- izi + izistep
    pkhtb r6, r6, r1, asr #16   @ r6 <- (izi + izistep) & 0xffff0000 | (izi >> 16) & 0xffff
    add r1, r1, r8, lsl #1      @ r1 <- izi + izistep * 2

    add r7, r1, r8              @ r7 <- izi + izistep
    pkhtb r7, r7, r1, asr #16   @ r7 <- (izi + izistep) & 0xffff0000 | (izi >> 16) & 0xffff
    add r1, r1, r8, lsl #1      @ r1 <- izi + izistep * 2

    stmia r3!, {r4, r5, r6, r7} @ {r4, r5, r6, r7} -> [r3], r3 += 16

    bgt DZS_WriteQuadwords

DZS_WriteLastWords:
    cmp r2, #1
    ble DZS_WriteLastShort
    add r4, r1, r8              @ r4 <- izi + izistep
    pkhtb r4, r4, r1, asr #16   @ r4 <- (izi + izistep) & 0xffff0000 | (izi >> 16) & 0xffff
    add r1, r1, r8, lsl #1      @ r1 <- izi + izistep * 2
    str r4, [r3], #4            @ r4 -> [r3], r3 += 4
    subs r2, #2
    bgt DZS_WriteLastWords

DZS_WriteLastShort:
    cmp r2, #1
    asreq r4, r1, #16
    streqh r4, [r3], #2

    cmp r0, #0
    bne DZS_SpanLoop

    pop {r4-r10, pc}

DZS_0x8000f:	.word	0x47000000
DZS_0x10000f:	.word	0x47800000
#endif

