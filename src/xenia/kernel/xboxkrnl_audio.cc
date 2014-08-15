/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <xenia/kernel/xboxkrnl_audio.h>

#include <xenia/emulator.h>
#include <xenia/apu/apu.h>
#include <xenia/kernel/kernel_state.h>
#include <xenia/kernel/xboxkrnl_private.h>
#include <xenia/kernel/util/shim_utils.h>


using namespace xe;
using namespace xe::apu;
using namespace xe::kernel;
using namespace xe::kernel::xboxkrnl;


namespace xe {
namespace kernel {


SHIM_CALL XMACreateContext_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t context_ptr = SHIM_GET_ARG_32(0);

  XELOGD(
      "XMACreateContext(%.8X)",
      context_ptr);

  // TODO(benvanik): allocate and return -- see if size required or just dummy?
  // Games will call MmGetPhysicalAddress on the result.
  SHIM_SET_MEM_32(context_ptr, 0xAAAABABE);

  SHIM_SET_RETURN_32(X_STATUS_SUCCESS);
}


SHIM_CALL XMAReleaseContext_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t context_ptr = SHIM_GET_ARG_32(0);

  XELOGD(
      "XMAReleaseContext(%.8X)",
      context_ptr);

  // TODO(benvanik): free
}


SHIM_CALL XAudioGetSpeakerConfig_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t config_ptr = SHIM_GET_ARG_32(0);

  XELOGD(
      "XAudioGetSpeakerConfig(%.8X)",
      config_ptr);

  SHIM_SET_MEM_32(config_ptr, 0x00010001);

  SHIM_SET_RETURN_32(X_ERROR_SUCCESS);
}


SHIM_CALL XAudioGetVoiceCategoryVolumeChangeMask_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t driver_ptr = SHIM_GET_ARG_32(0);
  uint32_t out_ptr = SHIM_GET_ARG_32(1);

  XELOGD(
      "XAudioGetVoiceCategoryVolumeChangeMask(%.8X, %.8X)",
      driver_ptr, out_ptr);

  assert_true((driver_ptr & 0xFFFF0000) == 0x41550000);

  auto audio_system = state->emulator()->audio_system();

  // Checking these bits to see if any voice volume changed.
  // I think.
  SHIM_SET_MEM_32(out_ptr, 0);

  SHIM_SET_RETURN_32(X_ERROR_SUCCESS);
}


SHIM_CALL XAudioGetVoiceCategoryVolume_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t unk = SHIM_GET_ARG_32(0);
  uint32_t out_ptr = SHIM_GET_ARG_32(1);

  XELOGD(
      "XAudioGetVoiceCategoryVolume(%.8X, %.8X)",
      unk, out_ptr);

  // Expects a floating point single. Volume %?
  SHIM_SET_MEM_F32(out_ptr, 1.0f);

  SHIM_SET_RETURN_32(X_ERROR_SUCCESS);
}


SHIM_CALL XAudioEnableDucker_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t unk = SHIM_GET_ARG_32(0);

  XELOGD(
      "XAudioEnableDucker(%.8X)",
      unk);

  SHIM_SET_RETURN_32(X_ERROR_SUCCESS);
}


SHIM_CALL XAudioRegisterRenderDriverClient_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t callback_ptr = SHIM_GET_ARG_32(0);
  uint32_t driver_ptr = SHIM_GET_ARG_32(1);

  uint32_t callback = SHIM_MEM_32(callback_ptr + 0);
  uint32_t callback_arg = SHIM_MEM_32(callback_ptr + 4);

  XELOGD(
      "XAudioRegisterRenderDriverClient(%.8X(%.8X, %.8X), %.8X)",
      callback_ptr, callback, callback_arg, driver_ptr);

  auto audio_system = state->emulator()->audio_system();

  size_t index;
  auto result = audio_system->RegisterClient(callback, callback_arg, &index);
  if (XFAILED(result)) {
    SHIM_SET_RETURN_32(result);
    return;
  }

  assert_true(!(index & ~0x0000FFFF));
  SHIM_SET_MEM_32(driver_ptr, 0x41550000 | (static_cast<uint32_t>(index) & 0x0000FFFF));
  SHIM_SET_RETURN_32(X_ERROR_SUCCESS);
}


SHIM_CALL XAudioUnregisterRenderDriverClient_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t driver_ptr = SHIM_GET_ARG_32(0);

  XELOGD(
      "XAudioUnregisterRenderDriverClient(%.8X)",
      driver_ptr);

  assert_true((driver_ptr & 0xFFFF0000) == 0x41550000);

  auto audio_system = state->emulator()->audio_system();
  audio_system->UnregisterClient(driver_ptr & 0x0000FFFF);
  SHIM_SET_RETURN_32(X_ERROR_SUCCESS);
}


SHIM_CALL XAudioSubmitRenderDriverFrame_shim(
    PPCContext* ppc_state, KernelState* state) {
  uint32_t driver_ptr = SHIM_GET_ARG_32(0);
  uint32_t samples_ptr = SHIM_GET_ARG_32(1);

  XELOGD(
      "XAudioSubmitRenderDriverFrame(%.8X, %.8X)",
      driver_ptr, samples_ptr);

  assert_true((driver_ptr & 0xFFFF0000) == 0x41550000);

  auto audio_system = state->emulator()->audio_system();
  audio_system->SubmitFrame(driver_ptr & 0x0000FFFF, samples_ptr);

  SHIM_SET_RETURN_32(X_ERROR_SUCCESS);
}


}  // namespace kernel
}  // namespace xe


void xe::kernel::xboxkrnl::RegisterAudioExports(
    ExportResolver* export_resolver, KernelState* state) {
  SHIM_SET_MAPPING("xboxkrnl.exe", XMACreateContext, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAInitializeContext, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", XMAReleaseContext, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAEnableContext, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMADisableContext, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAGetOutputBufferWriteOffset, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetOutputBufferReadOffset, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAGetOutputBufferReadOffset, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetOutputBufferValid, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAIsOutputBufferValid, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetInputBuffer0Valid, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAIsInputBuffer0Valid, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetInputBuffer1Valid, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAIsInputBuffer1Valid, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetInputBuffer0, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetInputBuffer1, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAGetPacketMetadata, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMABlockWhileInUse, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetLoopData, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMASetInputBufferReadOffset, state);
  // SHIM_SET_MAPPING("xboxkrnl.exe", XMAGetInputBufferReadOffset, state);

  SHIM_SET_MAPPING("xboxkrnl.exe", XAudioGetSpeakerConfig, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", XAudioGetVoiceCategoryVolumeChangeMask, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", XAudioGetVoiceCategoryVolume, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", XAudioEnableDucker, state);

  SHIM_SET_MAPPING("xboxkrnl.exe", XAudioRegisterRenderDriverClient, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", XAudioUnregisterRenderDriverClient, state);
  SHIM_SET_MAPPING("xboxkrnl.exe", XAudioSubmitRenderDriverFrame, state);
}
