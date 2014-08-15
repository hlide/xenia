/*
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <alloy/frontend/ppc/ppc_emit-private.h>

#include <alloy/frontend/ppc/ppc_context.h>
#include <alloy/frontend/ppc/ppc_hir_builder.h>

namespace alloy {
namespace frontend {
namespace ppc {

// TODO(benvanik): remove when enums redefined.
using namespace alloy::hir;

using alloy::hir::Value;

Value* CalculateEA_0(PPCHIRBuilder& f, uint32_t ra, uint32_t rb);

#define SHUFPS_SWAP_DWORDS 0x1B

// Most of this file comes from:
// http://biallas.net/doc/vmx128/vmx128.txt
// https://github.com/kakaroto/ps3ida/blob/master/plugins/PPCAltivec/src/main.cpp

#define OP(x) ((((uint32_t)(x)) & 0x3f) << 26)
#define VX128(op, xop) (OP(op) | (((uint32_t)(xop)) & 0x3d0))
#define VX128_1(op, xop) (OP(op) | (((uint32_t)(xop)) & 0x7f3))
#define VX128_2(op, xop) (OP(op) | (((uint32_t)(xop)) & 0x210))
#define VX128_3(op, xop) (OP(op) | (((uint32_t)(xop)) & 0x7f0))
#define VX128_4(op, xop) (OP(op) | (((uint32_t)(xop)) & 0x730))
#define VX128_5(op, xop) (OP(op) | (((uint32_t)(xop)) & 0x10))
#define VX128_P(op, xop) (OP(op) | (((uint32_t)(xop)) & 0x630))

#define VX128_VD128 (i.VX128.VD128l | (i.VX128.VD128h << 5))
#define VX128_VA128 \
  (i.VX128.VA128l | (i.VX128.VA128h << 5) | (i.VX128.VA128H << 6))
#define VX128_VB128 (i.VX128.VB128l | (i.VX128.VB128h << 5))
#define VX128_1_VD128 (i.VX128_1.VD128l | (i.VX128_1.VD128h << 5))
#define VX128_2_VD128 (i.VX128_2.VD128l | (i.VX128_2.VD128h << 5))
#define VX128_2_VA128 \
  (i.VX128_2.VA128l | (i.VX128_2.VA128h << 5) | (i.VX128_2.VA128H << 6))
#define VX128_2_VB128 (i.VX128_2.VB128l | (i.VX128_2.VD128h << 5))
#define VX128_2_VC (i.VX128_2.VC)
#define VX128_3_VD128 (i.VX128_3.VD128l | (i.VX128_3.VD128h << 5))
#define VX128_3_VB128 (i.VX128_3.VB128l | (i.VX128_3.VB128h << 5))
#define VX128_3_IMM (i.VX128_3.IMM)
#define VX128_5_VD128 (i.VX128_5.VD128l | (i.VX128_5.VD128h << 5))
#define VX128_5_VA128 \
  (i.VX128_5.VA128l | (i.VX128_5.VA128h << 5)) | (i.VX128_5.VA128H << 6)
#define VX128_5_VB128 (i.VX128_5.VB128l | (i.VX128_5.VB128h << 5))
#define VX128_5_SH (i.VX128_5.SH)
#define VX128_R_VD128 (i.VX128_R.VD128l | (i.VX128_R.VD128h << 5))
#define VX128_R_VA128 \
  (i.VX128_R.VA128l | (i.VX128_R.VA128h << 5) | (i.VX128_R.VA128H << 6))
#define VX128_R_VB128 (i.VX128_R.VB128l | (i.VX128_R.VB128h << 5))

unsigned int xerotl(unsigned int value, unsigned int shift) {
  assert_true(shift < 32);
  return shift == 0 ? value : ((value << shift) | (value >> (32 - shift)));
}

XEEMITTER(dst, 0x7C0002AC, XDSS)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(dstst, 0x7C0002EC, XDSS)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(dss, 0x7C00066C, XDSS)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(lvebx, 0x7C00000E, X)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(lvehx, 0x7C00004E, X)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_lvewx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                     uint32_t rb) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(lvewx, 0x7C00008E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvewx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(lvewx128, VX128_1(4, 131), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvewx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}

int InstrEmit_lvsl_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                    uint32_t rb) {
  Value* ea = CalculateEA_0(f, ra, rb);
  Value* sh = f.Truncate(f.And(ea, f.LoadConstant((int64_t)0xF)), INT8_TYPE);
  Value* v = f.LoadVectorShl(sh);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(lvsl, 0x7C00000C, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvsl_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(lvsl128, VX128_1(4, 3), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvsl_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}

int InstrEmit_lvsr_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                    uint32_t rb) {
  Value* ea = CalculateEA_0(f, ra, rb);
  Value* sh = f.Truncate(f.And(ea, f.LoadConstant((int64_t)0xF)), INT8_TYPE);
  Value* v = f.LoadVectorShr(sh);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(lvsr, 0x7C00004C, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvsr_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(lvsr128, VX128_1(4, 67), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvsr_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}

int InstrEmit_lvx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                   uint32_t rb) {
  Value* ea = f.And(CalculateEA_0(f, ra, rb), f.LoadConstant(~0xFull));
  f.StoreVR(vd, f.ByteSwap(f.Load(ea, VEC128_TYPE)));
  return 0;
}
XEEMITTER(lvx, 0x7C0000CE, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(lvx128, VX128_1(4, 195), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}
XEEMITTER(lvxl, 0x7C0002CE, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvx(f, i);
}
XEEMITTER(lvxl128, VX128_1(4, 707), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvx128(f, i);
}

XEEMITTER(stvebx, 0x7C00010E, X)(PPCHIRBuilder& f, InstrData& i) {
  Value* ea = CalculateEA_0(f, i.X.RA, i.X.RB);
  Value* el = f.And(ea, f.LoadConstant(0xFull));
  Value* v = f.Extract(f.LoadVR(i.X.RT), el, INT8_TYPE);
  f.Store(ea, v);
  return 0;
}

XEEMITTER(stvehx, 0x7C00014E, X)(PPCHIRBuilder& f, InstrData& i) {
  Value* ea = CalculateEA_0(f, i.X.RA, i.X.RB);
  ea = f.And(ea, f.LoadConstant(~0x1ull));
  Value* el = f.Shr(f.And(ea, f.LoadConstant(0xFull)), 1);
  Value* v = f.Extract(f.LoadVR(i.X.RT), el, INT16_TYPE);
  f.Store(ea, f.ByteSwap(v));
  return 0;
}

int InstrEmit_stvewx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                      uint32_t rb) {
  Value* ea = CalculateEA_0(f, ra, rb);
  ea = f.And(ea, f.LoadConstant(~0x3ull));
  Value* el = f.Shr(f.And(ea, f.LoadConstant(0xFull)), 2);
  Value* v = f.Extract(f.LoadVR(vd), el, INT32_TYPE);
  f.Store(ea, f.ByteSwap(v));
  return 0;
}
XEEMITTER(stvewx, 0x7C00018E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvewx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(stvewx128, VX128_1(4, 387), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvewx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}

int InstrEmit_stvx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                    uint32_t rb) {
  Value* ea = f.And(CalculateEA_0(f, ra, rb), f.LoadConstant(~0xFull));
  f.Store(ea, f.ByteSwap(f.LoadVR(vd)));
  return 0;
}
XEEMITTER(stvx, 0x7C0001CE, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(stvx128, VX128_1(4, 451), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}
XEEMITTER(stvxl, 0x7C0003CE, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvx(f, i);
}
XEEMITTER(stvxl128, VX128_1(4, 963), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvx128(f, i);
}

// The lvlx/lvrx/etc instructions are in Cell docs only:
// https://www-01.ibm.com/chips/techlib/techlib.nsf/techdocs/C40E4C6133B31EE8872570B500791108/$file/vector_simd_pem_v_2.07c_26Oct2006_cell.pdf
int InstrEmit_lvlx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                    uint32_t rb) {
  Value* ea = CalculateEA_0(f, ra, rb);
  Value* eb = f.And(f.Truncate(ea, INT8_TYPE), f.LoadConstant((int8_t)0xF));
  // ea &= ~0xF
  ea = f.And(ea, f.LoadConstant(~0xFull));
  // v = (new << eb)
  Value* v = f.Permute(f.LoadVectorShl(eb), f.ByteSwap(f.Load(ea, VEC128_TYPE)),
                       f.LoadZero(VEC128_TYPE), INT8_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(lvlx, 0x7C00040E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvlx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(lvlx128, VX128_1(4, 1027), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvlx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}
XEEMITTER(lvlxl, 0x7C00060E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvlx(f, i);
}
XEEMITTER(lvlxl128, VX128_1(4, 1539), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvlx128(f, i);
}

int InstrEmit_lvrx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                    uint32_t rb) {
  Value* ea = CalculateEA_0(f, ra, rb);
  Value* eb = f.And(f.Truncate(ea, INT8_TYPE), f.LoadConstant((int8_t)0xF));
  // ea &= ~0xF
  ea = f.And(ea, f.LoadConstant(~0xFull));
  // v = (new >> (16 - eb))
  Value* v = f.Permute(f.LoadVectorShr(f.Sub(f.LoadConstant((int8_t)16), eb)),
                       f.LoadZero(VEC128_TYPE),
                       f.ByteSwap(f.Load(ea, VEC128_TYPE)), INT8_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(lvrx, 0x7C00044E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvrx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(lvrx128, VX128_1(4, 1091), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvrx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}
XEEMITTER(lvrxl, 0x7C00064E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvrx(f, i);
}
XEEMITTER(lvrxl128, VX128_1(4, 1603), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_lvrx128(f, i);
}

int InstrEmit_stvlx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                     uint32_t rb) {
  // NOTE: if eb == 0 (so 16b aligned) this equals new_value
  //       we could optimize this to prevent the other load/mask, in that case.
  Value* ea = CalculateEA_0(f, ra, rb);
  Value* eb = f.And(f.Truncate(ea, INT8_TYPE), f.LoadConstant((int8_t)0xF));
  Value* new_value = f.LoadVR(vd);
  // ea &= ~0xF
  ea = f.And(ea, f.LoadConstant(~0xFull));
  Value* old_value = f.ByteSwap(f.Load(ea, VEC128_TYPE));
  // v = (new >> eb) | (old & (ONE << (16 - eb)))
  Value* v = f.Permute(f.LoadVectorShr(eb), f.LoadZero(VEC128_TYPE), new_value,
                       INT8_TYPE);
  v = f.Or(
      v, f.And(old_value,
               f.Permute(f.LoadVectorShl(f.Sub(f.LoadConstant((int8_t)16), eb)),
                         f.Not(f.LoadZero(VEC128_TYPE)),
                         f.LoadZero(VEC128_TYPE), INT8_TYPE)));
  // ea &= ~0xF (handled above)
  f.Store(ea, f.ByteSwap(v));
  return 0;
}
XEEMITTER(stvlx, 0x7C00050E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvlx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(stvlx128, VX128_1(4, 1283), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvlx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}
XEEMITTER(stvlxl, 0x7C00070E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvlx(f, i);
}
XEEMITTER(stvlxl128, VX128_1(4, 1795), VX128_1)(PPCHIRBuilder& f,
                                                InstrData& i) {
  return InstrEmit_stvlx128(f, i);
}

int InstrEmit_stvrx_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t ra,
                     uint32_t rb) {
  // NOTE: if eb == 0 (so 16b aligned) this equals new_value
  //       we could optimize this to prevent the other load/mask, in that case.
  Value* ea = CalculateEA_0(f, ra, rb);
  Value* eb = f.And(f.Truncate(ea, INT8_TYPE), f.LoadConstant((int8_t)0xF));
  Value* new_value = f.LoadVR(vd);
  // ea &= ~0xF
  ea = f.And(ea, f.LoadConstant(~0xFull));
  Value* old_value = f.ByteSwap(f.Load(ea, VEC128_TYPE));
  // v = (new << (16 - eb)) | (old & (ONE >> eb))
  Value* v = f.Permute(f.LoadVectorShl(f.Sub(f.LoadConstant((int8_t)16), eb)),
                       new_value, f.LoadZero(VEC128_TYPE), INT8_TYPE);
  v = f.Or(v, f.And(old_value,
                    f.Permute(f.LoadVectorShr(eb), f.LoadZero(VEC128_TYPE),
                              f.Not(f.LoadZero(VEC128_TYPE)), INT8_TYPE)));
  // ea &= ~0xF (handled above)
  f.Store(ea, f.ByteSwap(v));
  return 0;
}
XEEMITTER(stvrx, 0x7C00054E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvrx_(f, i, i.X.RT, i.X.RA, i.X.RB);
}
XEEMITTER(stvrx128, VX128_1(4, 1347), VX128_1)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvrx_(f, i, VX128_1_VD128, i.VX128_1.RA, i.VX128_1.RB);
}
XEEMITTER(stvrxl, 0x7C00074E, X)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_stvrx(f, i);
}
XEEMITTER(stvrxl128, VX128_1(4, 1859), VX128_1)(PPCHIRBuilder& f,
                                                InstrData& i) {
  return InstrEmit_stvrx128(f, i);
}

XEEMITTER(mfvscr, 0x10000604, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(mtvscr, 0x10000644, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vaddcuw, 0x10000180, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_vaddfp_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD) <- (VA) + (VB) (4 x fp)
  Value* v = f.Add(f.LoadVR(va), f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vaddfp, 0x1000000A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vaddfp_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vaddfp128, VX128(5, 16), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vaddfp_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vaddsbs, 0x10000300, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_SATURATE);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vaddshs, 0x10000340, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_SATURATE);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vaddsws, 0x10000380, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_SATURATE);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vaddubm, 0x10000000, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vaddubs, 0x10000200, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_UNSIGNED | ARITHMETIC_SATURATE);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vadduhm, 0x10000040, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vadduhs, 0x10000240, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_UNSIGNED | ARITHMETIC_SATURATE);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vadduwm, 0x10000080, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vadduws, 0x10000280, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorAdd(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_UNSIGNED | ARITHMETIC_SATURATE);
  f.StoreSAT(f.DidSaturate(v));
  f.StoreVR(i.VX.VD, v);
  return 0;
}

int InstrEmit_vand_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // VD <- (VA) & (VB)
  Value* v = f.And(f.LoadVR(va), f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vand, 0x10000404, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vand_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vand128, VX128(5, 528), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vand_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

int InstrEmit_vandc_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // VD <- (VA) & ¬(VB)
  Value* v = f.And(f.LoadVR(va), f.Not(f.LoadVR(vb)));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vandc, 0x10000444, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vandc_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vandc128, VX128(5, 592), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vandc_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vavgsb, 0x10000502, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vavgsh, 0x10000542, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vavgsw, 0x10000582, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vavgub, 0x10000402, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vavguh, 0x10000442, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vavguw, 0x10000482, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_vcfsx_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb,
                     uint32_t uimm) {
  // (VD) <- float(VB as signed) / 2^uimm
  uimm = uimm ? (2 << (uimm - 1)) : 1;
  Value* v = f.Div(f.VectorConvertI2F(f.LoadVR(vb)),
                   f.Splat(f.LoadConstant((float)uimm), VEC128_TYPE));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vcfsx, 0x1000034A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcfsx_(f, i.VX.VD, i.VX.VB, i.VX.VA);
}
XEEMITTER(vcsxwfp128, VX128_3(6, 688), VX128_3)(PPCHIRBuilder& f,
                                                InstrData& i) {
  return InstrEmit_vcfsx_(f, VX128_3_VD128, VX128_3_VB128, VX128_3_IMM);
}

int InstrEmit_vcfux_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb,
                     uint32_t uimm) {
  // (VD) <- float(VB as unsigned) / 2^uimm
  uimm = uimm ? (2 << (uimm - 1)) : 1;
  Value* v = f.Div(f.VectorConvertI2F(f.LoadVR(vb), ARITHMETIC_UNSIGNED),
                   f.Splat(f.LoadConstant((float)uimm), VEC128_TYPE));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vcfux, 0x1000030A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcfux_(f, i.VX.VD, i.VX.VB, i.VX.VA);
}
XEEMITTER(vcuxwfp128, VX128_3(6, 752), VX128_3)(PPCHIRBuilder& f,
                                                InstrData& i) {
  return InstrEmit_vcfux_(f, VX128_3_VD128, VX128_3_VB128, VX128_3_IMM);
}

int InstrEmit_vctsxs_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb,
                      uint32_t uimm) {
  // (VD) <- int_sat(VB as signed * 2^uimm)
  uimm = uimm ? (2 << (uimm - 1)) : 1;
  Value* v =
      f.Mul(f.LoadVR(vb), f.Splat(f.LoadConstant((float)uimm), VEC128_TYPE));
  v = f.VectorConvertF2I(v, ARITHMETIC_SATURATE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vctsxs, 0x100003CA, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vctsxs_(f, i.VX.VD, i.VX.VB, i.VX.VA);
}
XEEMITTER(vcfpsxws128, VX128_3(6, 560), VX128_3)(PPCHIRBuilder& f,
                                                 InstrData& i) {
  return InstrEmit_vctsxs_(f, VX128_3_VD128, VX128_3_VB128, VX128_3_IMM);
}

int InstrEmit_vctuxs_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb,
                      uint32_t uimm) {
  // (VD) <- int_sat(VB as unsigned * 2^uimm)
  uimm = uimm ? (2 << (uimm - 1)) : 1;
  Value* v =
      f.Mul(f.LoadVR(vb), f.Splat(f.LoadConstant((float)uimm), VEC128_TYPE));
  v = f.VectorConvertF2I(v, ARITHMETIC_UNSIGNED | ARITHMETIC_SATURATE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vctuxs, 0x1000038A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vctuxs_(f, i.VX.VD, i.VX.VB, i.VX.VA);
}
XEEMITTER(vcfpuxws128, VX128_3(6, 624), VX128_3)(PPCHIRBuilder& f,
                                                 InstrData& i) {
  return InstrEmit_vctuxs_(f, VX128_3_VD128, VX128_3_VB128, VX128_3_IMM);
}

int InstrEmit_vcmpbfp_(PPCHIRBuilder& f, InstrData& i, uint32_t vd, uint32_t va,
                       uint32_t vb, uint32_t rc) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vcmpbfp, 0x100003C6, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpbfp_(f, i, i.VXR.VD, i.VXR.VA, i.VXR.VB, i.VXR.Rc);
}
XEEMITTER(vcmpbfp128, VX128(6, 384), VX128_R)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpbfp_(f, i, VX128_R_VD128, VX128_R_VA128, VX128_R_VB128,
                            i.VX128_R.Rc);
}

enum vcmpxxfp_op {
  vcmpxxfp_eq,
  vcmpxxfp_gt,
  vcmpxxfp_ge,
};
int InstrEmit_vcmpxxfp_(PPCHIRBuilder& f, InstrData& i, vcmpxxfp_op cmpop,
                        uint32_t vd, uint32_t va, uint32_t vb, uint32_t rc) {
  // (VD.xyzw) = (VA.xyzw) OP (VB.xyzw) ? 0xFFFFFFFF : 0x00000000
  // if (Rc) CR6 = all_equal | 0 | none_equal | 0
  // If an element in either VA or VB is NaN the result will be 0x00000000
  Value* v;
  switch (cmpop) {
    case vcmpxxfp_eq:
      v = f.VectorCompareEQ(f.LoadVR(va), f.LoadVR(vb), FLOAT32_TYPE);
      break;
    case vcmpxxfp_gt:
      v = f.VectorCompareSGT(f.LoadVR(va), f.LoadVR(vb), FLOAT32_TYPE);
      break;
    case vcmpxxfp_ge:
      v = f.VectorCompareSGE(f.LoadVR(va), f.LoadVR(vb), FLOAT32_TYPE);
      break;
    default:
      assert_unhandled_case(cmpop);
      return 1;
  }
  if (rc) {
    f.UpdateCR6(v);
  }
  f.StoreVR(vd, v);
  return 0;
}

XEEMITTER(vcmpeqfp, 0x100000C6, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxfp_(f, i, vcmpxxfp_eq, i.VXR.VD, i.VXR.VA, i.VXR.VB,
                             i.VXR.Rc);
}
XEEMITTER(vcmpeqfp128, VX128(6, 0), VX128_R)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxfp_(f, i, vcmpxxfp_eq, VX128_R_VD128, VX128_R_VA128,
                             VX128_R_VB128, i.VX128_R.Rc);
}
XEEMITTER(vcmpgefp, 0x100001C6, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxfp_(f, i, vcmpxxfp_ge, i.VXR.VD, i.VXR.VA, i.VXR.VB,
                             i.VXR.Rc);
}
XEEMITTER(vcmpgefp128, VX128(6, 128), VX128_R)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxfp_(f, i, vcmpxxfp_ge, VX128_R_VD128, VX128_R_VA128,
                             VX128_R_VB128, i.VX128_R.Rc);
}
XEEMITTER(vcmpgtfp, 0x100002C6, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxfp_(f, i, vcmpxxfp_gt, i.VXR.VD, i.VXR.VA, i.VXR.VB,
                             i.VXR.Rc);
}
XEEMITTER(vcmpgtfp128, VX128(6, 256), VX128_R)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxfp_(f, i, vcmpxxfp_gt, VX128_R_VD128, VX128_R_VA128,
                             VX128_R_VB128, i.VX128_R.Rc);
}

enum vcmpxxi_op {
  vcmpxxi_eq,
  vcmpxxi_gt_signed,
  vcmpxxi_gt_unsigned,
};
int InstrEmit_vcmpxxi_(PPCHIRBuilder& f, InstrData& i, vcmpxxi_op cmpop,
                       uint32_t width, uint32_t vd, uint32_t va, uint32_t vb,
                       uint32_t rc) {
  // (VD.xyzw) = (VA.xyzw) OP (VB.xyzw) ? 0xFFFFFFFF : 0x00000000
  // if (Rc) CR6 = all_equal | 0 | none_equal | 0
  // If an element in either VA or VB is NaN the result will be 0x00000000
  Value* v;
  switch (cmpop) {
    case vcmpxxi_eq:
      switch (width) {
        case 1:
          v = f.VectorCompareEQ(f.LoadVR(va), f.LoadVR(vb), INT8_TYPE);
          break;
        case 2:
          v = f.VectorCompareEQ(f.LoadVR(va), f.LoadVR(vb), INT16_TYPE);
          break;
        case 4:
          v = f.VectorCompareEQ(f.LoadVR(va), f.LoadVR(vb), INT32_TYPE);
          break;
        default:
          assert_unhandled_case(width);
          return 1;
      }
      break;
    case vcmpxxi_gt_signed:
      switch (width) {
        case 1:
          v = f.VectorCompareSGT(f.LoadVR(va), f.LoadVR(vb), INT8_TYPE);
          break;
        case 2:
          v = f.VectorCompareSGT(f.LoadVR(va), f.LoadVR(vb), INT16_TYPE);
          break;
        case 4:
          v = f.VectorCompareSGT(f.LoadVR(va), f.LoadVR(vb), INT32_TYPE);
          break;
        default:
          assert_unhandled_case(width);
          return 1;
      }
      break;
    case vcmpxxi_gt_unsigned:
      switch (width) {
        case 1:
          v = f.VectorCompareUGT(f.LoadVR(va), f.LoadVR(vb), INT8_TYPE);
          break;
        case 2:
          v = f.VectorCompareUGT(f.LoadVR(va), f.LoadVR(vb), INT16_TYPE);
          break;
        case 4:
          v = f.VectorCompareUGT(f.LoadVR(va), f.LoadVR(vb), INT32_TYPE);
          break;
        default:
          assert_unhandled_case(width);
          return 1;
      }
      break;
    default:
      assert_unhandled_case(cmpop);
      return 1;
  }
  if (rc) {
    f.UpdateCR6(v);
  }
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vcmpequb, 0x10000006, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_eq, 1, i.VXR.VD, i.VXR.VA, i.VXR.VB,
                            i.VXR.Rc);
}
XEEMITTER(vcmpequh, 0x10000046, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_eq, 2, i.VXR.VD, i.VXR.VA, i.VXR.VB,
                            i.VXR.Rc);
}
XEEMITTER(vcmpequw, 0x10000086, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_eq, 4, i.VXR.VD, i.VXR.VA, i.VXR.VB,
                            i.VXR.Rc);
}
XEEMITTER(vcmpequw128, VX128(6, 512), VX128_R)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_eq, 4, VX128_R_VD128, VX128_R_VA128,
                            VX128_R_VB128, i.VX128_R.Rc);
}
XEEMITTER(vcmpgtsb, 0x10000306, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_gt_signed, 1, i.VXR.VD, i.VXR.VA,
                            i.VXR.VB, i.VXR.Rc);
}
XEEMITTER(vcmpgtsh, 0x10000346, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_gt_signed, 2, i.VXR.VD, i.VXR.VA,
                            i.VXR.VB, i.VXR.Rc);
}
XEEMITTER(vcmpgtsw, 0x10000386, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_gt_signed, 4, i.VXR.VD, i.VXR.VA,
                            i.VXR.VB, i.VXR.Rc);
}
XEEMITTER(vcmpgtub, 0x10000206, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_gt_unsigned, 1, i.VXR.VD, i.VXR.VA,
                            i.VXR.VB, i.VXR.Rc);
}
XEEMITTER(vcmpgtuh, 0x10000246, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_gt_unsigned, 2, i.VXR.VD, i.VXR.VA,
                            i.VXR.VB, i.VXR.Rc);
}
XEEMITTER(vcmpgtuw, 0x10000286, VXR)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vcmpxxi_(f, i, vcmpxxi_gt_unsigned, 4, i.VXR.VD, i.VXR.VA,
                            i.VXR.VB, i.VXR.Rc);
}

int InstrEmit_vexptefp_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- pow2(VB)
  Value* v = f.Pow2(f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vexptefp, 0x1000018A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vexptefp_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vexptefp128, VX128_3(6, 1712), VX128_3)(PPCHIRBuilder& f,
                                                  InstrData& i) {
  return InstrEmit_vexptefp_(f, VX128_3_VD128, VX128_3_VB128);
}

int InstrEmit_vlogefp_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- log2(VB)
  Value* v = f.Log2(f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vlogefp, 0x100001CA, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vlogefp_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vlogefp128, VX128_3(6, 1776), VX128_3)(PPCHIRBuilder& f,
                                                 InstrData& i) {
  return InstrEmit_vlogefp_(f, VX128_3_VD128, VX128_3_VB128);
}

int InstrEmit_vmaddfp_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb,
                       uint32_t vc) {
  // (VD) <- ((VA) * (VC)) + (VB)
  Value* v = f.MulAdd(f.LoadVR(va), f.LoadVR(vc), f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vmaddfp, 0x1000002E, VXA)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- ((VA) * (VC)) + (VB)
  return InstrEmit_vmaddfp_(f, i.VXA.VD, i.VXA.VA, i.VXA.VB, i.VXA.VC);
}
XEEMITTER(vmaddfp128, VX128(5, 208), VX128)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- ((VA) * (VB)) + (VD)
  // NOTE: this resuses VD and swaps the arg order!
  return InstrEmit_vmaddfp_(f, VX128_VD128, VX128_VA128, VX128_VD128,
                            VX128_VB128);
}

XEEMITTER(vmaddcfp128, VX128(5, 272), VX128)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- ((VA) * (VD)) + (VB)
  Value* v = f.MulAdd(f.LoadVR(VX128_VA128), f.LoadVR(VX128_VD128),
                      f.LoadVR(VX128_VB128));
  f.StoreVR(VX128_VD128, v);
  return 0;
}

int InstrEmit_vmaxfp_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD) <- max((VA), (VB))
  Value* v = f.Max(f.LoadVR(va), f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vmaxfp, 0x1000040A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vmaxfp_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vmaxfp128, VX128(6, 640), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vmaxfp_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vmaxsb, 0x10000102, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- max((VA), (VB)) (signed int8)
  Value* v = f.VectorMax(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vmaxsh, 0x10000142, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- max((VA), (VB)) (signed int16)
  Value* v = f.VectorMax(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vmaxsw, 0x10000182, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- max((VA), (VB)) (signed int32)
  Value* v = f.VectorMax(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vmaxub, 0x10000002, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- max((VA), (VB)) (unsigned int8)
  Value* v = f.VectorMax(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vmaxuh, 0x10000042, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- max((VA), (VB)) (unsigned int16)
  Value* v = f.VectorMax(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vmaxuw, 0x10000082, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- max((VA), (VB)) (unsigned int32)
  Value* v = f.VectorMax(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vmhaddshs, 0x10000020, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmhraddshs, 0x10000021, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_vminfp_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD) <- min((VA), (VB))
  Value* v = f.Min(f.LoadVR(va), f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vminfp, 0x1000044A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vminfp_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vminfp128, VX128(6, 704), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vminfp_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vminsb, 0x10000302, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- min((VA), (VB)) (signed int8)
  Value* v = f.VectorMin(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vminsh, 0x10000342, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- min((VA), (VB)) (signed int16)
  Value* v = f.VectorMin(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vminsw, 0x10000382, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- min((VA), (VB)) (signed int32)
  Value* v = f.VectorMin(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vminub, 0x10000202, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- min((VA), (VB)) (unsigned int8)
  Value* v = f.VectorMin(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vminuh, 0x10000242, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- min((VA), (VB)) (unsigned int16)
  Value* v = f.VectorMin(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vminuw, 0x10000282, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- min((VA), (VB)) (unsigned int32)
  Value* v = f.VectorMin(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vmladduhm, 0x10000022, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmrghb, 0x1000000C, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmrghh, 0x1000004C, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_vmrghw_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD.x) = (VA.x)
  // (VD.y) = (VB.x)
  // (VD.z) = (VA.y)
  // (VD.w) = (VB.y)
  Value* v = f.Permute(f.LoadConstant(0x00040105), f.LoadVR(va), f.LoadVR(vb),
                       INT32_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vmrghw, 0x1000008C, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vmrghw_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vmrghw128, VX128(6, 768), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vmrghw_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vmrglb, 0x1000010C, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmrglh, 0x1000014C, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_vmrglw_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD.x) = (VA.z)
  // (VD.y) = (VB.z)
  // (VD.z) = (VA.w)
  // (VD.w) = (VB.w)
  Value* v = f.Permute(f.LoadConstant(0x02060307), f.LoadVR(va), f.LoadVR(vb),
                       INT32_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vmrglw, 0x1000018C, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vmrglw_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vmrglw128, VX128(6, 832), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vmrglw_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vmsummbm, 0x10000025, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmsumshm, 0x10000028, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmsumshs, 0x10000029, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmsumubm, 0x10000024, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmsumuhm, 0x10000026, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmsumuhs, 0x10000027, VXA)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmsum3fp128, VX128(5, 400), VX128)(PPCHIRBuilder& f, InstrData& i) {
  // Dot product XYZ.
  // (VD.xyzw) = (VA.x * VB.x) + (VA.y * VB.y) + (VA.z * VB.z)
  Value* v = f.DotProduct3(f.LoadVR(VX128_VA128), f.LoadVR(VX128_VB128));
  v = f.Splat(v, VEC128_TYPE);
  f.StoreVR(VX128_VD128, v);
  return 0;
}

XEEMITTER(vmsum4fp128, VX128(5, 464), VX128)(PPCHIRBuilder& f, InstrData& i) {
  // Dot product XYZW.
  // (VD.xyzw) = (VA.x * VB.x) + (VA.y * VB.y) + (VA.z * VB.z) + (VA.w * VB.w)
  Value* v = f.DotProduct4(f.LoadVR(VX128_VA128), f.LoadVR(VX128_VB128));
  v = f.Splat(v, VEC128_TYPE);
  f.StoreVR(VX128_VD128, v);
  return 0;
}

XEEMITTER(vmulesb, 0x10000308, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmulesh, 0x10000348, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmuleub, 0x10000208, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmuleuh, 0x10000248, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmulosb, 0x10000108, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmulosh, 0x10000148, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmuloub, 0x10000008, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmulouh, 0x10000048, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vmulfp128, VX128(5, 144), VX128)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (VA) * (VB) (4 x fp)
  Value* v = f.Mul(f.LoadVR(VX128_VA128), f.LoadVR(VX128_VB128));
  f.StoreVR(VX128_VD128, v);
  return 0;
}

int InstrEmit_vnmsubfp_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb,
                        uint32_t vc) {
  // (VD) <- -(((VA) * (VC)) - (VB))
  // NOTE: only one rounding should take place, but that's hard...
  // This really needs VFNMSUB132PS/VFNMSUB213PS/VFNMSUB231PS but that's AVX.
  Value* v = f.Neg(f.MulSub(f.LoadVR(va), f.LoadVR(vc), f.LoadVR(vb)));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vnmsubfp, 0x1000002F, VXA)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vnmsubfp_(f, i.VXA.VD, i.VXA.VA, i.VXA.VB, i.VXA.VC);
}
XEEMITTER(vnmsubfp128, VX128(5, 336), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vnmsubfp_(f, VX128_VD128, VX128_VA128, VX128_VB128,
                             VX128_VD128);
}

int InstrEmit_vnor_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // VD <- ¬((VA) | (VB))
  Value* v = f.Not(f.Or(f.LoadVR(va), f.LoadVR(vb)));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vnor, 0x10000504, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vnor_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vnor128, VX128(5, 656), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vnor_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

int InstrEmit_vor_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // VD <- (VA) | (VB)
  if (va == vb) {
    // Copy VA==VB into VD.
    f.StoreVR(vd, f.LoadVR(va));
  } else {
    Value* v = f.Or(f.LoadVR(va), f.LoadVR(vb));
    f.StoreVR(vd, v);
  }
  return 0;
}
XEEMITTER(vor, 0x10000484, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vor_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vor128, VX128(5, 720), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vor_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

int InstrEmit_vperm_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb,
                     uint32_t vc) {
  Value* v = f.Permute(f.LoadVR(vc), f.LoadVR(va), f.LoadVR(vb), INT8_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vperm, 0x1000002B, VXA)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vperm_(f, i.VXA.VD, i.VXA.VA, i.VXA.VB, i.VXA.VC);
}
XEEMITTER(vperm128, VX128_2(5, 0), VX128_2)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vperm_(f, VX128_2_VD128, VX128_2_VA128, VX128_2_VB128,
                          VX128_2_VC);
}

XEEMITTER(vpermwi128, VX128_P(6, 528), VX128_P)(PPCHIRBuilder& f,
                                                InstrData& i) {
  // (VD.x) = (VB.uimm[6-7])
  // (VD.y) = (VB.uimm[4-5])
  // (VD.z) = (VB.uimm[2-3])
  // (VD.w) = (VB.uimm[0-1])
  const uint32_t vd = i.VX128_P.VD128l | (i.VX128_P.VD128h << 5);
  const uint32_t vb = i.VX128_P.VB128l | (i.VX128_P.VB128h << 5);
  uint32_t uimm = i.VX128_P.PERMl | (i.VX128_P.PERMh << 5);
  Value* v = f.Swizzle(f.LoadVR(vb), INT32_TYPE, uimm);
  f.StoreVR(vd, v);
  return 0;
}

int InstrEmit_vrefp_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- 1/(VB)
  vec128_t one = {{{1, 1, 1, 1}}};
  Value* v = f.Div(f.LoadConstant(one), f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vrefp, 0x1000010A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrefp_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vrefp128, VX128_3(6, 1584), VX128_3)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrefp_(f, VX128_3_VD128, VX128_3_VB128);
}

int InstrEmit_vrfim_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- RndToFPInt32Floor(VB)
  Value* v = f.Round(f.LoadVR(vb), ROUND_TO_MINUS_INFINITY);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vrfim, 0x100002CA, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfim_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vrfim128, VX128_3(6, 816), VX128_3)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfim_(f, VX128_3_VD128, VX128_3_VB128);
}

int InstrEmit_vrfin_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- RoundToNearest(VB)
  Value* v = f.Round(f.LoadVR(vb), ROUND_TO_NEAREST);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vrfin, 0x1000020A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfin_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vrfin128, VX128_3(6, 880), VX128_3)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfin_(f, VX128_3_VD128, VX128_3_VB128);
}

int InstrEmit_vrfip_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- RndToFPInt32Ceil(VB)
  Value* v = f.Round(f.LoadVR(vb), ROUND_TO_POSITIVE_INFINITY);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vrfip, 0x1000028A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfip_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vrfip128, VX128_3(6, 944), VX128_3)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfip_(f, VX128_3_VD128, VX128_3_VB128);
}

int InstrEmit_vrfiz_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- RndToFPInt32Trunc(VB)
  Value* v = f.Round(f.LoadVR(vb), ROUND_TO_ZERO);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vrfiz, 0x1000024A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfiz_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vrfiz128, VX128_3(6, 1008), VX128_3)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrfiz_(f, VX128_3_VD128, VX128_3_VB128);
}

XEEMITTER(vrlb, 0x10000004, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- ROTL((VA), (VB)&0x3)
  Value* v = f.VectorRotateLeft(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vrlh, 0x10000044, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- ROTL((VA), (VB)&0xF)
  Value* v = f.VectorRotateLeft(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

int InstrEmit_vrlw_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD) <- ROTL((VA), (VB)&0x1F)
  Value* v = f.VectorRotateLeft(f.LoadVR(va), f.LoadVR(vb), INT32_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vrlw, 0x10000084, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrlw_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vrlw128, VX128(6, 80), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrlw_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vrlimi128, VX128_4(6, 1808), VX128_4)(PPCHIRBuilder& f,
                                                InstrData& i) {
  const uint32_t vd = i.VX128_4.VD128l | (i.VX128_4.VD128h << 5);
  const uint32_t vb = i.VX128_4.VB128l | (i.VX128_4.VB128h << 5);
  uint32_t blend_mask_src = i.VX128_4.IMM;
  uint32_t blend_mask = 0;
  for (int n = 3; n >= 0; n--) {
    blend_mask |= ((blend_mask_src & 0x1) ? n : (4 + n)) << ((3 - n) * 8);
    blend_mask_src >>= 1;
  }
  uint32_t rotate = i.VX128_4.z;
  // This is just a fancy permute.
  // X Y Z W, rotated left by 2 = Z W X Y
  // Then mask select the results into the dest.
  // Sometimes rotation is zero, so fast path.
  Value* v;
  if (rotate) {
    // TODO(benvanik): constants need conversion.
    uint32_t swizzle_mask;
    switch (rotate) {
      case 1:
        // X Y Z W -> Y Z W X
        swizzle_mask = SWIZZLE_XYZW_TO_YZWX;
        break;
      case 2:
        // X Y Z W -> Z W X Y
        swizzle_mask = SWIZZLE_XYZW_TO_ZWXY;
        break;
      case 3:
        // X Y Z W -> W X Y Z
        swizzle_mask = SWIZZLE_XYZW_TO_WXYZ;
        break;
      default:
        assert_always();
        return 1;
    }
    v = f.Swizzle(f.LoadVR(vb), FLOAT32_TYPE, swizzle_mask);
  } else {
    v = f.LoadVR(vb);
  }
  if (blend_mask != 0x00010203) {
    v = f.Permute(f.LoadConstant(blend_mask), v, f.LoadVR(vd), INT32_TYPE);
  }
  f.StoreVR(vd, v);
  return 0;
}

int InstrEmit_vrsqrtefp_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // (VD) <- 1 / sqrt(VB)
  // There are a lot of rules in the Altivec_PEM docs for handlings that
  // result in nan/infinity/etc. They are ignored here. I hope games would
  // never rely on them.
  Value* v = f.RSqrt(f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vrsqrtefp, 0x1000014A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vrsqrtefp_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vrsqrtefp128, VX128_3(6, 1648), VX128_3)(PPCHIRBuilder& f,
                                                   InstrData& i) {
  return InstrEmit_vrsqrtefp_(f, VX128_3_VD128, VX128_3_VB128);
}

int InstrEmit_vsel_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb,
                    uint32_t vc) {
  Value* a = f.LoadVR(va);
  Value* v = f.Xor(f.And(f.Xor(a, f.LoadVR(vb)), f.LoadVR(vc)), a);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vsel, 0x1000002A, VXA)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsel_(f, i.VXA.VD, i.VXA.VA, i.VXA.VB, i.VXA.VC);
}
XEEMITTER(vsel128, VX128(5, 848), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsel_(f, VX128_VD128, VX128_VA128, VX128_VB128, VX128_VD128);
}

XEEMITTER(vsl, 0x100001C4, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vslb, 0x10000104, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorShl(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vslh, 0x10000144, VX)(PPCHIRBuilder& f, InstrData& i) {
  Value* v = f.VectorShl(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

int InstrEmit_vslw_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // VA = |xxxxx|yyyyy|zzzzz|wwwww|
  // VB = |...sh|...sh|...sh|...sh|
  // VD = |x<<sh|y<<sh|z<<sh|w<<sh|
  Value* v = f.VectorShl(f.LoadVR(va), f.LoadVR(vb), INT32_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vslw, 0x10000184, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vslw_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vslw128, VX128(6, 208), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vslw_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

static uint8_t __vsldoi_table[16][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
    {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
    {3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18},
    {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19},
    {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
    {6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21},
    {7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22},
    {8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23},
    {9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24},
    {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25},
    {11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26},
    {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27},
    {13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28},
    {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
    {15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
};
int InstrEmit_vsldoi_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb,
                      uint32_t sh) {
  // (VD) <- ((VA) || (VB)) << (SH << 3)
  if (!sh) {
    f.StoreVR(vd, f.LoadVR(va));
    return 0;
  } else if (sh == 16) {
    f.StoreVR(vd, f.LoadVR(vb));
    return 0;
  }
  // TODO(benvanik): optimize for the rotation case:
  // vsldoi128 vr63,vr63,vr63,4
  // (ABCD ABCD) << 4b = (BCDA)
  // (VA << SH) OR (VB >> (16 - SH))
  vec128_t shift = *((vec128_t*)(__vsldoi_table[sh]));
  for (int i = 0; i < 4; ++i) {
    shift.i4[i] = poly::byte_swap(shift.i4[i]);
  }
  Value* control = f.LoadConstant(shift);
  Value* v = f.Permute(control, f.LoadVR(va), f.LoadVR(vb), INT8_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vsldoi, 0x1000002C, VXA)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsldoi_(f, i.VXA.VD, i.VXA.VA, i.VXA.VB, i.VXA.VC & 0xF);
}
XEEMITTER(vsldoi128, VX128_5(4, 16), VX128_5)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsldoi_(f, VX128_5_VD128, VX128_5_VA128, VX128_5_VB128,
                           VX128_5_SH);
}

XEEMITTER(vslo, 0x1000040C, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vslo128, VX128(5, 912), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vspltb, 0x1000020C, VX)(PPCHIRBuilder& f, InstrData& i) {
  // b <- UIMM*8
  // do i = 0 to 127 by 8
  //  (VD)[i:i+7] <- (VB)[b:b+7]
  Value* b = f.Extract(f.LoadVR(i.VX.VB), (i.VX.VA & 0xF), INT8_TYPE);
  Value* v = f.Splat(b, VEC128_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsplth, 0x1000024C, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD.xyzw) <- (VB.uimm)
  Value* h = f.Extract(f.LoadVR(i.VX.VB), (i.VX.VA & 0x7), INT16_TYPE);
  Value* v = f.Splat(h, VEC128_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

int InstrEmit_vspltw_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb,
                      uint32_t uimm) {
  // (VD.xyzw) <- (VB.uimm)
  Value* w = f.Extract(f.LoadVR(vb), (uimm & 0x3), INT32_TYPE);
  Value* v = f.Splat(w, VEC128_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vspltw, 0x1000028C, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vspltw_(f, i.VX.VD, i.VX.VB, i.VX.VA);
}
XEEMITTER(vspltw128, VX128_3(6, 1840), VX128_3)(PPCHIRBuilder& f,
                                                InstrData& i) {
  return InstrEmit_vspltw_(f, VX128_3_VD128, VX128_3_VB128, VX128_3_IMM);
}

XEEMITTER(vspltisb, 0x1000030C, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD.xyzw) <- sign_extend(uimm)
  Value* v;
  if (i.VX.VA) {
    // Sign extend from 5bits -> 8 and load.
    int8_t simm = (i.VX.VA & 0x10) ? (i.VX.VA | 0xF0) : i.VX.VA;
    v = f.Splat(f.LoadConstant(simm), VEC128_TYPE);
  } else {
    // Zero out the register.
    v = f.LoadZero(VEC128_TYPE);
  }
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vspltish, 0x1000034C, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD.xyzw) <- sign_extend(uimm)
  Value* v;
  if (i.VX.VA) {
    // Sign extend from 5bits -> 16 and load.
    int16_t simm = (i.VX.VA & 0x10) ? (i.VX.VA | 0xFFF0) : i.VX.VA;
    v = f.Splat(f.LoadConstant(simm), VEC128_TYPE);
  } else {
    // Zero out the register.
    v = f.LoadZero(VEC128_TYPE);
  }
  f.StoreVR(i.VX.VD, v);
  return 0;
}

int InstrEmit_vspltisw_(PPCHIRBuilder& f, uint32_t vd, uint32_t uimm) {
  // (VD.xyzw) <- sign_extend(uimm)
  Value* v;
  if (uimm) {
    // Sign extend from 5bits -> 32 and load.
    int32_t simm = (uimm & 0x10) ? (uimm | 0xFFFFFFF0) : uimm;
    v = f.Splat(f.LoadConstant(simm), VEC128_TYPE);
  } else {
    // Zero out the register.
    v = f.LoadZero(VEC128_TYPE);
  }
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vspltisw, 0x1000038C, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vspltisw_(f, i.VX.VD, i.VX.VA);
}
XEEMITTER(vspltisw128, VX128_3(6, 1904), VX128_3)(PPCHIRBuilder& f,
                                                  InstrData& i) {
  return InstrEmit_vspltisw_(f, VX128_3_VD128, VX128_3_IMM);
}

XEEMITTER(vsr, 0x100002C4, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vsrab, 0x10000304, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (VA) >>a (VB) by bytes
  Value* v = f.VectorSha(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsrah, 0x10000344, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (VA) >>a (VB) by halfwords
  Value* v = f.VectorSha(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

int InstrEmit_vsraw_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD) <- (VA) >>a (VB) by words
  Value* v = f.VectorSha(f.LoadVR(va), f.LoadVR(vb), INT32_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vsraw, 0x10000384, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsraw_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vsraw128, VX128(6, 336), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsraw_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vsrb, 0x10000204, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (VA) >> (VB) by bytes
  Value* v = f.VectorShr(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsrh, 0x10000244, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (VA) >> (VB) by halfwords
  Value* v = f.VectorShr(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

int InstrEmit_vsro_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  return 1;
}
XEEMITTER(vsro, 0x1000044C, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsro_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vsro128, VX128(5, 976), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsro_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

int InstrEmit_vsrw_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD) <- (VA) >> (VB) by words
  Value* v = f.VectorShr(f.LoadVR(va), f.LoadVR(vb), INT32_TYPE);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vsrw, 0x10000284, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsrw_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vsrw128, VX128(6, 464), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsrw_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vsubcuw, 0x10000580, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_vsubfp_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // (VD) <- (VA) - (VB) (4 x fp)
  Value* v = f.Sub(f.LoadVR(va), f.LoadVR(vb));
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vsubfp, 0x1000004A, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsubfp_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vsubfp128, VX128(5, 80), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vsubfp_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

XEEMITTER(vsubsbs, 0x10000700, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- clamp(EXTS(VA) + ¬EXTS(VB) + 1, -128, 127)
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_SATURATE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsubshs, 0x10000740, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- clamp(EXTS(VA) + ¬EXTS(VB) + 1, -2^15, 2^15-1)
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_SATURATE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsubsws, 0x10000780, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- clamp(EXTS(VA) + ¬EXTS(VB) + 1, -2^31, 2^31-1)
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_SATURATE);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsububm, 0x10000400, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (EXTZ(VA) + ¬EXTZ(VB) + 1) % 256
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsubuhm, 0x10000440, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (EXTZ(VA) + ¬EXTZ(VB) + 1) % 2^16
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsubuwm, 0x10000480, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- (EXTZ(VA) + ¬EXTZ(VB) + 1) % 2^32
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsububs, 0x10000600, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- clamp(EXTZ(VA) + ¬EXTZ(VB) + 1, 0, 256)
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT8_TYPE,
                         ARITHMETIC_SATURATE | ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsubuhs, 0x10000640, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- clamp(EXTZ(VA) + ¬EXTZ(VB) + 1, 0, 2^16-1)
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT16_TYPE,
                         ARITHMETIC_SATURATE | ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsubuws, 0x10000680, VX)(PPCHIRBuilder& f, InstrData& i) {
  // (VD) <- clamp(EXTZ(VA) + ¬EXTZ(VB) + 1, 0, 2^32-1)
  Value* v = f.VectorSub(f.LoadVR(i.VX.VA), f.LoadVR(i.VX.VB), INT32_TYPE,
                         ARITHMETIC_SATURATE | ARITHMETIC_UNSIGNED);
  f.StoreVR(i.VX.VD, v);
  return 0;
}

XEEMITTER(vsumsws, 0x10000788, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vsum2sws, 0x10000688, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vsum4sbs, 0x10000708, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vsum4shs, 0x10000648, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vsum4ubs, 0x10000608, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkpx, 0x1000030E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkshss, 0x1000018E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkshss128, VX128(5, 512), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkswss, 0x100001CE, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkswss128, VX128(5, 640), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkswus, 0x1000014E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkswus128, VX128(5, 704), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkuhum, 0x1000000E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkuhum128, VX128(5, 768), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkuhus, 0x1000008E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkuhus128, VX128(5, 832), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkshus, 0x1000010E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkshus128, VX128(5, 576), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkuwum, 0x1000004E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkuwum128, VX128(5, 896), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vpkuwus, 0x100000CE, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}
XEEMITTER(vpkuwus128, VX128(5, 960), VX128)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vupkhpx, 0x1000034E, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

XEEMITTER(vupklpx, 0x100003CE, VX)(PPCHIRBuilder& f, InstrData& i) {
  XEINSTRNOTIMPLEMENTED();
  return 1;
}

int InstrEmit_vupkhsh_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // halfwords 0-3 expanded to words 0-3 and sign extended
  Value* v = f.Unpack(f.LoadVR(vb), PACK_TYPE_S16_IN_32_HI);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vupkhsh, 0x1000024E, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vupkhsh_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vupkhsh128, 0x100002CE, VX)(PPCHIRBuilder& f, InstrData& i) {
  uint32_t va = VX128_VA128;
  assert_zero(va);
  return InstrEmit_vupkhsh_(f, VX128_VD128, VX128_VB128);
}

int InstrEmit_vupklsh_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // halfwords 4-7 expanded to words 0-3 and sign extended
  Value* v = f.Unpack(f.LoadVR(vb), PACK_TYPE_S16_IN_32_LO);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vupklsh, 0x100002CE, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vupklsh_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vupklsh128, 0x100002CE, VX)(PPCHIRBuilder& f, InstrData& i) {
  uint32_t va = VX128_VA128;
  assert_zero(va);
  return InstrEmit_vupklsh_(f, VX128_VD128, VX128_VB128);
}

int InstrEmit_vupkhsb_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // bytes 0-7 expanded to halfwords 0-7 and sign extended
  Value* v = f.Unpack(f.LoadVR(vb), PACK_TYPE_S8_IN_16_HI);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vupkhsb, 0x1000020E, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vupkhsb_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vupkhsb128, VX128(6, 896), VX128)(PPCHIRBuilder& f, InstrData& i) {
  uint32_t va = VX128_VA128;
  if (va == 0x60) {
    // Hrm, my instruction tables suck.
    return InstrEmit_vupkhsh_(f, VX128_VD128, VX128_VB128);
  }
  return InstrEmit_vupkhsb_(f, VX128_VD128, VX128_VB128);
}

int InstrEmit_vupklsb_(PPCHIRBuilder& f, uint32_t vd, uint32_t vb) {
  // bytes 8-15 expanded to halfwords 0-7 and sign extended
  Value* v = f.Unpack(f.LoadVR(vb), PACK_TYPE_S8_IN_16_LO);
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vupklsb, 0x1000028E, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vupklsb_(f, i.VX.VD, i.VX.VB);
}
XEEMITTER(vupklsb128, VX128(6, 960), VX128)(PPCHIRBuilder& f, InstrData& i) {
  uint32_t va = VX128_VA128;
  if (va == 0x60) {
    // Hrm, my instruction tables suck.
    return InstrEmit_vupklsh_(f, VX128_VD128, VX128_VB128);
  }
  return InstrEmit_vupklsb_(f, VX128_VD128, VX128_VB128);
}

XEEMITTER(vpkd3d128, VX128_4(6, 1552), VX128_4)(PPCHIRBuilder& f,
                                                InstrData& i) {
  const uint32_t vd = i.VX128_4.VD128l | (i.VX128_4.VD128h << 5);
  const uint32_t vb = i.VX128_4.VB128l | (i.VX128_4.VB128h << 5);
  uint32_t type = i.VX128_4.IMM >> 2;
  uint32_t shift = i.VX128_4.IMM & 0x3;
  uint32_t pack = i.VX128_4.z;
  Value* v = f.LoadVR(vb);
  switch (type) {
    case 0:  // VPACK_D3DCOLOR
      v = f.Pack(v, PACK_TYPE_D3DCOLOR);
      break;
    case 1:  // VPACK_NORMSHORT2
      v = f.Pack(v, PACK_TYPE_SHORT_2);
      break;
    case 3:  // VPACK_... 2 FLOAT16s DXGI_FORMAT_R16G16_FLOAT
      v = f.Pack(v, PACK_TYPE_FLOAT16_2);
      break;
    case 5:  // VPACK_... 4 FLOAT16s DXGI_FORMAT_R16G16B16A16_FLOAT
      v = f.Pack(v, PACK_TYPE_FLOAT16_4);
      break;
    default:
      assert_unhandled_case(type);
      return 1;
  }
  // http://hlssmod.net/he_code/public/pixelwriter.h
  // control = prev:0123 | new:4567
  uint32_t control = 0x00010203;  // original
  uint32_t src = xerotl(0x04050607, shift * 8);
  uint32_t mask = 0;
  switch (pack) {
    case 1:  // VPACK_32
      // VPACK_32 & shift = 3 puts lower 32 bits in x (leftmost slot).
      mask = 0x000000FF << (shift * 8);
      control = (control & ~mask) | (src & mask);
      break;
    case 2:  // 64bit
      if (shift < 3) {
        mask = 0x0000FFFF << (shift * 8);
      } else {
        // w
        src = 0x00000007;
        mask = 0x000000FF;
      }
      control = (control & ~mask) | (src & mask);
      break;
    case 3:  // 64bit
      if (shift < 3) {
        mask = 0x0000FFFF << (shift * 8);
      } else {
        // z
        src = 0x00000006;
        mask = 0x000000FF;
      }
      control = (control & ~mask) | (src & mask);
      break;
    default:
      assert_unhandled_case(pack);
      return 1;
  }
  v = f.Permute(f.LoadConstant(control), f.LoadVR(vd), v, INT32_TYPE);
  f.StoreVR(vd, v);
  return 0;
}

XEEMITTER(vupkd3d128, VX128_3(6, 2032), VX128_3)(PPCHIRBuilder& f,
                                                 InstrData& i) {
  // Can't find many docs on this. Best reference is
  // http://worldcraft.googlecode.com/svn/trunk/src/qylib/math/xmmatrix.inl,
  // which shows how it's used in some cases. Since it's all intrinsics,
  // finding it in code is pretty easy.
  const uint32_t vd = i.VX128_3.VD128l | (i.VX128_3.VD128h << 5);
  const uint32_t vb = i.VX128_3.VB128l | (i.VX128_3.VB128h << 5);
  const uint32_t type = i.VX128_3.IMM >> 2;
  Value* v = f.LoadVR(vb);
  switch (type) {
    case 0:  // VPACK_D3DCOLOR
      v = f.Unpack(v, PACK_TYPE_D3DCOLOR);
      break;
    case 1:  // VPACK_NORMSHORT2
      v = f.Unpack(v, PACK_TYPE_SHORT_2);
      break;
    case 3:  // VPACK_... 2 FLOAT16s DXGI_FORMAT_R16G16_FLOAT
      v = f.Unpack(v, PACK_TYPE_FLOAT16_2);
      break;
    case 5:  // VPACK_... 4 FLOAT16s DXGI_FORMAT_R16G16B16A16_FLOAT
      v = f.Unpack(v, PACK_TYPE_FLOAT16_4);
      break;
    default:
      assert_unhandled_case(type);
      return 1;
  }
  f.StoreVR(vd, v);
  return 0;
}

int InstrEmit_vxor_(PPCHIRBuilder& f, uint32_t vd, uint32_t va, uint32_t vb) {
  // VD <- (VA) ^ (VB)
  Value* v;
  if (va == vb) {
    // Fast clear.
    v = f.LoadZero(VEC128_TYPE);
  } else {
    v = f.Xor(f.LoadVR(va), f.LoadVR(vb));
  }
  f.StoreVR(vd, v);
  return 0;
}
XEEMITTER(vxor, 0x100004C4, VX)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vxor_(f, i.VX.VD, i.VX.VA, i.VX.VB);
}
XEEMITTER(vxor128, VX128(5, 784), VX128)(PPCHIRBuilder& f, InstrData& i) {
  return InstrEmit_vxor_(f, VX128_VD128, VX128_VA128, VX128_VB128);
}

void RegisterEmitCategoryAltivec() {
  XEREGISTERINSTR(dst, 0x7C0002AC);
  XEREGISTERINSTR(dstst, 0x7C0002EC);
  XEREGISTERINSTR(dss, 0x7C00066C);
  XEREGISTERINSTR(lvebx, 0x7C00000E);
  XEREGISTERINSTR(lvehx, 0x7C00004E);
  XEREGISTERINSTR(lvewx, 0x7C00008E);
  XEREGISTERINSTR(lvewx128, VX128_1(4, 131));
  XEREGISTERINSTR(lvsl, 0x7C00000C);
  XEREGISTERINSTR(lvsl128, VX128_1(4, 3));
  XEREGISTERINSTR(lvsr, 0x7C00004C);
  XEREGISTERINSTR(lvsr128, VX128_1(4, 67));
  XEREGISTERINSTR(lvx, 0x7C0000CE);
  XEREGISTERINSTR(lvx128, VX128_1(4, 195));
  XEREGISTERINSTR(lvxl, 0x7C0002CE);
  XEREGISTERINSTR(lvxl128, VX128_1(4, 707));
  XEREGISTERINSTR(stvebx, 0x7C00010E);
  XEREGISTERINSTR(stvehx, 0x7C00014E);
  XEREGISTERINSTR(stvewx, 0x7C00018E);
  XEREGISTERINSTR(stvewx128, VX128_1(4, 387));
  XEREGISTERINSTR(stvx, 0x7C0001CE);
  XEREGISTERINSTR(stvx128, VX128_1(4, 451));
  XEREGISTERINSTR(stvxl, 0x7C0003CE);
  XEREGISTERINSTR(stvxl128, VX128_1(4, 963));
  XEREGISTERINSTR(lvlx, 0x7C00040E);
  XEREGISTERINSTR(lvlx128, VX128_1(4, 1027));
  XEREGISTERINSTR(lvlxl, 0x7C00060E);
  XEREGISTERINSTR(lvlxl128, VX128_1(4, 1539));
  XEREGISTERINSTR(lvrx, 0x7C00044E);
  XEREGISTERINSTR(lvrx128, VX128_1(4, 1091));
  XEREGISTERINSTR(lvrxl, 0x7C00064E);
  XEREGISTERINSTR(lvrxl128, VX128_1(4, 1603));
  XEREGISTERINSTR(stvlx, 0x7C00050E);
  XEREGISTERINSTR(stvlx128, VX128_1(4, 1283));
  XEREGISTERINSTR(stvlxl, 0x7C00070E);
  XEREGISTERINSTR(stvlxl128, VX128_1(4, 1795));
  XEREGISTERINSTR(stvrx, 0x7C00054E);
  XEREGISTERINSTR(stvrx128, VX128_1(4, 1347));
  XEREGISTERINSTR(stvrxl, 0x7C00074E);
  XEREGISTERINSTR(stvrxl128, VX128_1(4, 1859));

  XEREGISTERINSTR(mfvscr, 0x10000604);
  XEREGISTERINSTR(mtvscr, 0x10000644);
  XEREGISTERINSTR(vaddcuw, 0x10000180);
  XEREGISTERINSTR(vaddfp, 0x1000000A);
  XEREGISTERINSTR(vaddfp128, VX128(5, 16));
  XEREGISTERINSTR(vaddsbs, 0x10000300);
  XEREGISTERINSTR(vaddshs, 0x10000340);
  XEREGISTERINSTR(vaddsws, 0x10000380);
  XEREGISTERINSTR(vaddubm, 0x10000000);
  XEREGISTERINSTR(vaddubs, 0x10000200);
  XEREGISTERINSTR(vadduhm, 0x10000040);
  XEREGISTERINSTR(vadduhs, 0x10000240);
  XEREGISTERINSTR(vadduwm, 0x10000080);
  XEREGISTERINSTR(vadduws, 0x10000280);
  XEREGISTERINSTR(vand, 0x10000404);
  XEREGISTERINSTR(vand128, VX128(5, 528));
  XEREGISTERINSTR(vandc, 0x10000444);
  XEREGISTERINSTR(vandc128, VX128(5, 592));
  XEREGISTERINSTR(vavgsb, 0x10000502);
  XEREGISTERINSTR(vavgsh, 0x10000542);
  XEREGISTERINSTR(vavgsw, 0x10000582);
  XEREGISTERINSTR(vavgub, 0x10000402);
  XEREGISTERINSTR(vavguh, 0x10000442);
  XEREGISTERINSTR(vavguw, 0x10000482);
  XEREGISTERINSTR(vcfsx, 0x1000034A);
  XEREGISTERINSTR(vcsxwfp128, VX128_3(6, 688));
  XEREGISTERINSTR(vcfpsxws128, VX128_3(6, 560));
  XEREGISTERINSTR(vcfux, 0x1000030A);
  XEREGISTERINSTR(vcuxwfp128, VX128_3(6, 752));
  XEREGISTERINSTR(vcfpuxws128, VX128_3(6, 624));
  XEREGISTERINSTR(vcmpbfp, 0x100003C6);
  XEREGISTERINSTR(vcmpbfp128, VX128(6, 384));
  XEREGISTERINSTR(vcmpeqfp, 0x100000C6);
  XEREGISTERINSTR(vcmpeqfp128, VX128(6, 0));
  XEREGISTERINSTR(vcmpgefp, 0x100001C6);
  XEREGISTERINSTR(vcmpgefp128, VX128(6, 128));
  XEREGISTERINSTR(vcmpgtfp, 0x100002C6);
  XEREGISTERINSTR(vcmpgtfp128, VX128(6, 256));
  XEREGISTERINSTR(vcmpgtsb, 0x10000306);
  XEREGISTERINSTR(vcmpgtsh, 0x10000346);
  XEREGISTERINSTR(vcmpgtsw, 0x10000386);
  XEREGISTERINSTR(vcmpequb, 0x10000006);
  XEREGISTERINSTR(vcmpgtub, 0x10000206);
  XEREGISTERINSTR(vcmpequh, 0x10000046);
  XEREGISTERINSTR(vcmpgtuh, 0x10000246);
  XEREGISTERINSTR(vcmpequw, 0x10000086);
  XEREGISTERINSTR(vcmpequw128, VX128(6, 512));
  XEREGISTERINSTR(vcmpgtuw, 0x10000286);
  XEREGISTERINSTR(vctsxs, 0x100003CA);
  XEREGISTERINSTR(vctuxs, 0x1000038A);
  XEREGISTERINSTR(vexptefp, 0x1000018A);
  XEREGISTERINSTR(vexptefp128, VX128_3(6, 1712));
  XEREGISTERINSTR(vlogefp, 0x100001CA);
  XEREGISTERINSTR(vlogefp128, VX128_3(6, 1776));
  XEREGISTERINSTR(vmaddfp, 0x1000002E);
  XEREGISTERINSTR(vmaddfp128, VX128(5, 208));
  XEREGISTERINSTR(vmaddcfp128, VX128(5, 272));
  XEREGISTERINSTR(vmaxfp, 0x1000040A);
  XEREGISTERINSTR(vmaxfp128, VX128(6, 640));
  XEREGISTERINSTR(vmaxsb, 0x10000102);
  XEREGISTERINSTR(vmaxsh, 0x10000142);
  XEREGISTERINSTR(vmaxsw, 0x10000182);
  XEREGISTERINSTR(vmaxub, 0x10000002);
  XEREGISTERINSTR(vmaxuh, 0x10000042);
  XEREGISTERINSTR(vmaxuw, 0x10000082);
  XEREGISTERINSTR(vmhaddshs, 0x10000020);
  XEREGISTERINSTR(vmhraddshs, 0x10000021);
  XEREGISTERINSTR(vminfp, 0x1000044A);
  XEREGISTERINSTR(vminfp128, VX128(6, 704));
  XEREGISTERINSTR(vminsb, 0x10000302);
  XEREGISTERINSTR(vminsh, 0x10000342);
  XEREGISTERINSTR(vminsw, 0x10000382);
  XEREGISTERINSTR(vminub, 0x10000202);
  XEREGISTERINSTR(vminuh, 0x10000242);
  XEREGISTERINSTR(vminuw, 0x10000282);
  XEREGISTERINSTR(vmladduhm, 0x10000022);
  XEREGISTERINSTR(vmrghb, 0x1000000C);
  XEREGISTERINSTR(vmrghh, 0x1000004C);
  XEREGISTERINSTR(vmrghw, 0x1000008C);
  XEREGISTERINSTR(vmrghw128, VX128(6, 768));
  XEREGISTERINSTR(vmrglb, 0x1000010C);
  XEREGISTERINSTR(vmrglh, 0x1000014C);
  XEREGISTERINSTR(vmrglw, 0x1000018C);
  XEREGISTERINSTR(vmrglw128, VX128(6, 832));
  XEREGISTERINSTR(vmsummbm, 0x10000025);
  XEREGISTERINSTR(vmsumshm, 0x10000028);
  XEREGISTERINSTR(vmsumshs, 0x10000029);
  XEREGISTERINSTR(vmsumubm, 0x10000024);
  XEREGISTERINSTR(vmsumuhm, 0x10000026);
  XEREGISTERINSTR(vmsumuhs, 0x10000027);
  XEREGISTERINSTR(vmsum3fp128, VX128(5, 400));
  XEREGISTERINSTR(vmsum4fp128, VX128(5, 464));
  XEREGISTERINSTR(vmulesb, 0x10000308);
  XEREGISTERINSTR(vmulesh, 0x10000348);
  XEREGISTERINSTR(vmuleub, 0x10000208);
  XEREGISTERINSTR(vmuleuh, 0x10000248);
  XEREGISTERINSTR(vmulosb, 0x10000108);
  XEREGISTERINSTR(vmulosh, 0x10000148);
  XEREGISTERINSTR(vmuloub, 0x10000008);
  XEREGISTERINSTR(vmulouh, 0x10000048);
  XEREGISTERINSTR(vmulfp128, VX128(5, 144));
  XEREGISTERINSTR(vnmsubfp, 0x1000002F);
  XEREGISTERINSTR(vnmsubfp128, VX128(5, 336));
  XEREGISTERINSTR(vnor, 0x10000504);
  XEREGISTERINSTR(vnor128, VX128(5, 656));
  XEREGISTERINSTR(vor, 0x10000484);
  XEREGISTERINSTR(vor128, VX128(5, 720));
  XEREGISTERINSTR(vperm, 0x1000002B);
  XEREGISTERINSTR(vperm128, VX128_2(5, 0));
  XEREGISTERINSTR(vpermwi128, VX128_P(6, 528));
  XEREGISTERINSTR(vpkpx, 0x1000030E);
  XEREGISTERINSTR(vpkshss, 0x1000018E);
  XEREGISTERINSTR(vpkshss128, VX128(5, 512));
  XEREGISTERINSTR(vpkshus, 0x1000010E);
  XEREGISTERINSTR(vpkshus128, VX128(5, 576));
  XEREGISTERINSTR(vpkswss, 0x100001CE);
  XEREGISTERINSTR(vpkswss128, VX128(5, 640));
  XEREGISTERINSTR(vpkswus, 0x1000014E);
  XEREGISTERINSTR(vpkswus128, VX128(5, 704));
  XEREGISTERINSTR(vpkuhum, 0x1000000E);
  XEREGISTERINSTR(vpkuhum128, VX128(5, 768));
  XEREGISTERINSTR(vpkuhus, 0x1000008E);
  XEREGISTERINSTR(vpkuhus128, VX128(5, 832));
  XEREGISTERINSTR(vpkuwum, 0x1000004E);
  XEREGISTERINSTR(vpkuwum128, VX128(5, 896));
  XEREGISTERINSTR(vpkuwus, 0x100000CE);
  XEREGISTERINSTR(vpkuwus128, VX128(5, 960));
  XEREGISTERINSTR(vpkd3d128, VX128_4(6, 1552));
  XEREGISTERINSTR(vrefp, 0x1000010A);
  XEREGISTERINSTR(vrefp128, VX128_3(6, 1584));
  XEREGISTERINSTR(vrfim, 0x100002CA);
  XEREGISTERINSTR(vrfim128, VX128_3(6, 816));
  XEREGISTERINSTR(vrfin, 0x1000020A);
  XEREGISTERINSTR(vrfin128, VX128_3(6, 880));
  XEREGISTERINSTR(vrfip, 0x1000028A);
  XEREGISTERINSTR(vrfip128, VX128_3(6, 944));
  XEREGISTERINSTR(vrfiz, 0x1000024A);
  XEREGISTERINSTR(vrfiz128, VX128_3(6, 1008));
  XEREGISTERINSTR(vrlb, 0x10000004);
  XEREGISTERINSTR(vrlh, 0x10000044);
  XEREGISTERINSTR(vrlw, 0x10000084);
  XEREGISTERINSTR(vrlw128, VX128(6, 80));
  XEREGISTERINSTR(vrlimi128, VX128_4(6, 1808));
  XEREGISTERINSTR(vrsqrtefp, 0x1000014A);
  XEREGISTERINSTR(vrsqrtefp128, VX128_3(6, 1648));
  XEREGISTERINSTR(vsel, 0x1000002A);
  XEREGISTERINSTR(vsel128, VX128(5, 848));
  XEREGISTERINSTR(vsl, 0x100001C4);
  XEREGISTERINSTR(vslb, 0x10000104);
  XEREGISTERINSTR(vslh, 0x10000144);
  XEREGISTERINSTR(vslo, 0x1000040C);
  XEREGISTERINSTR(vslo128, VX128(5, 912));
  XEREGISTERINSTR(vslw, 0x10000184);
  XEREGISTERINSTR(vslw128, VX128(6, 208));
  XEREGISTERINSTR(vsldoi, 0x1000002C);
  XEREGISTERINSTR(vsldoi128, VX128_5(4, 16));
  XEREGISTERINSTR(vspltb, 0x1000020C);
  XEREGISTERINSTR(vsplth, 0x1000024C);
  XEREGISTERINSTR(vspltw, 0x1000028C);
  XEREGISTERINSTR(vspltw128, VX128_3(6, 1840));
  XEREGISTERINSTR(vspltisb, 0x1000030C);
  XEREGISTERINSTR(vspltish, 0x1000034C);
  XEREGISTERINSTR(vspltisw, 0x1000038C);
  XEREGISTERINSTR(vspltisw128, VX128_3(6, 1904));
  XEREGISTERINSTR(vsr, 0x100002C4);
  XEREGISTERINSTR(vsrab, 0x10000304);
  XEREGISTERINSTR(vsrah, 0x10000344);
  XEREGISTERINSTR(vsraw, 0x10000384);
  XEREGISTERINSTR(vsraw128, VX128(6, 336));
  XEREGISTERINSTR(vsrb, 0x10000204);
  XEREGISTERINSTR(vsrh, 0x10000244);
  XEREGISTERINSTR(vsro, 0x1000044C);
  XEREGISTERINSTR(vsro128, VX128(5, 976));
  XEREGISTERINSTR(vsrw, 0x10000284);
  XEREGISTERINSTR(vsrw128, VX128(6, 464));
  XEREGISTERINSTR(vsubcuw, 0x10000580);
  XEREGISTERINSTR(vsubfp, 0x1000004A);
  XEREGISTERINSTR(vsubfp128, VX128(5, 80));
  XEREGISTERINSTR(vsubsbs, 0x10000700);
  XEREGISTERINSTR(vsubshs, 0x10000740);
  XEREGISTERINSTR(vsubsws, 0x10000780);
  XEREGISTERINSTR(vsububm, 0x10000400);
  XEREGISTERINSTR(vsubuhm, 0x10000440);
  XEREGISTERINSTR(vsubuwm, 0x10000480);
  XEREGISTERINSTR(vsububs, 0x10000600);
  XEREGISTERINSTR(vsubuhs, 0x10000640);
  XEREGISTERINSTR(vsubuws, 0x10000680);
  XEREGISTERINSTR(vsumsws, 0x10000788);
  XEREGISTERINSTR(vsum2sws, 0x10000688);
  XEREGISTERINSTR(vsum4sbs, 0x10000708);
  XEREGISTERINSTR(vsum4shs, 0x10000648);
  XEREGISTERINSTR(vsum4ubs, 0x10000608);
  XEREGISTERINSTR(vupkhpx, 0x1000034E);
  XEREGISTERINSTR(vupkhsb, 0x1000020E);
  XEREGISTERINSTR(vupkhsb128, VX128(6, 896));
  XEREGISTERINSTR(vupkhsh, 0x1000024E);
  XEREGISTERINSTR(vupklpx, 0x100003CE);
  XEREGISTERINSTR(vupklsb, 0x1000028E);
  XEREGISTERINSTR(vupklsb128, VX128(6, 960));
  XEREGISTERINSTR(vupklsh, 0x100002CE);
  XEREGISTERINSTR(vupkd3d128, VX128_3(6, 2032));
  XEREGISTERINSTR(vxor, 0x100004C4);
  XEREGISTERINSTR(vxor128, VX128(5, 784));
}

}  // namespace ppc
}  // namespace frontend
}  // namespace alloy
