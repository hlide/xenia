/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include <xenia/emulator.h>

#include <xenia/apu/apu.h>
#include <xenia/cpu/cpu.h>
#include <xenia/cpu/xenon_memory.h>
#include <xenia/gpu/gpu.h>
#include <xenia/hid/hid.h>
#include <xenia/kernel/kernel.h>
#include <xenia/kernel/kernel_state.h>
#include <xenia/kernel/modules.h>
#include <xenia/kernel/fs/filesystem.h>
#include <xenia/ui/window.h>


using namespace xe;
using namespace xe::apu;
using namespace xe::cpu;
using namespace xe::gpu;
using namespace xe::hid;
using namespace xe::kernel;
using namespace xe::kernel::fs;
using namespace xe::ui;


Emulator::Emulator(const xechar_t* command_line) :
    main_window_(0),
    memory_(0),
    processor_(0),
    audio_system_(0), graphics_system_(0), input_system_(0),
    export_resolver_(0), file_system_(0),
    kernel_state_(0), xam_(0), xboxkrnl_(0) {
  XEIGNORE(xestrcpy(command_line_, XECOUNT(command_line_), command_line));
}

Emulator::~Emulator() {
  // Note that we delete things in the reverse order they were initialized.

  if (main_window_) {
    main_window_->Close();
  }

  delete xam_;
  delete xboxkrnl_;
  delete kernel_state_;

  delete file_system_;

  delete input_system_;

  // Give the systems time to shutdown before we delete them.
  graphics_system_->Shutdown();
  audio_system_->Shutdown();
  delete graphics_system_;
  delete audio_system_;

  delete processor_;

  delete export_resolver_;
}

X_STATUS Emulator::Setup() {
  X_STATUS result = X_STATUS_UNSUCCESSFUL;

  // Create memory system first, as it is required for other systems.
  memory_ = new XenonMemory();
  XEEXPECTNOTNULL(memory_);
  result = memory_->Initialize();
  XEEXPECTZERO(result);

  // Shared export resolver used to attach and query for HLE exports.
  export_resolver_ = new ExportResolver();
  XEEXPECTNOTNULL(export_resolver_);

  // Initialize the CPU.
  processor_ = new Processor(this);
  XEEXPECTNOTNULL(processor_);

  // Initialize the APU.
  audio_system_ = xe::apu::Create(this);
  XEEXPECTNOTNULL(audio_system_);

  // Initialize the GPU.
  graphics_system_ = xe::gpu::Create(this);
  XEEXPECTNOTNULL(graphics_system_);

  // Initialize the HID.
  input_system_ = xe::hid::Create(this);
  XEEXPECTNOTNULL(input_system_);

  // Setup the core components.
  result = processor_->Setup();
  XEEXPECTZERO(result);
  result = audio_system_->Setup();
  XEEXPECTZERO(result);
  result = graphics_system_->Setup();
  XEEXPECTZERO(result);
  result = input_system_->Setup();
  XEEXPECTZERO(result);

  // Bring up the virtual filesystem used by the kernel.
  file_system_ = new FileSystem();
  XEEXPECTNOTNULL(file_system_);

  // Shared kernel state.
  kernel_state_ = new KernelState(this);
  XEEXPECTNOTNULL(kernel_state_);

  // HLE kernel modules.
  xboxkrnl_ = new XboxkrnlModule(this, kernel_state_);
  XEEXPECTNOTNULL(xboxkrnl_);
  xam_ = new XamModule(this, kernel_state_);
  XEEXPECTNOTNULL(xam_);

  return result;

XECLEANUP:
  return result;
}

void Emulator::set_main_window(Window* window) {
  assert_null(main_window_);
  main_window_ = window;

  window->closed.AddListener([](UIEvent& e) {
    // TODO(benvanik): call module API to kill? this is a bad shutdown.
    exit(1);
  });
}

X_STATUS Emulator::LaunchXexFile(const xechar_t* path) {
  // We create a virtual filesystem pointing to its directory and symlink
  // that to the game filesystem.
  // e.g., /my/files/foo.xex will get a local fs at:
  // \\Device\\Harddisk0\\Partition1
  // and then get that symlinked to game:\, so
  // -> game:\foo.xex

  int result_code = 0;

  // Get just the filename (foo.xex).
  const xechar_t* file_name = xestrrchr(path, poly::path_separator);
  if (file_name) {
    // Skip slash.
    file_name++;
  } else {
    // No slash found, whole thing is a file.
    file_name = path;
  }

  // Get the parent path of the file.
  xechar_t parent_path[poly::max_path];
  XEIGNORE(xestrcpy(parent_path, XECOUNT(parent_path), path));
  parent_path[file_name - path] = 0;

  // Register the local directory in the virtual filesystem.
  result_code = file_system_->RegisterHostPathDevice(
      "\\Device\\Harddisk1\\Partition0", parent_path);
  if (result_code) {
    XELOGE("Unable to mount local directory");
    return result_code;
  }

  // Create symlinks to the device.
  file_system_->CreateSymbolicLink(
      "game:", "\\Device\\Harddisk1\\Partition0");
  file_system_->CreateSymbolicLink(
      "d:", "\\Device\\Harddisk1\\Partition0");

  // Get the file name of the module to load from the filesystem.
  char fs_path[poly::max_path];
  XEIGNORE(xestrcpya(fs_path, XECOUNT(fs_path), "game:\\"));
  char* fs_path_ptr = fs_path + xestrlena(fs_path);
  *fs_path_ptr = 0;
#if XE_WCHAR
  XEIGNORE(xestrnarrow(fs_path_ptr, XECOUNT(fs_path), file_name));
#else
  XEIGNORE(xestrcpya(fs_path_ptr, XECOUNT(fs_path), file_name));
#endif

  // Launch the game.
  return xboxkrnl_->LaunchModule(fs_path);
}

X_STATUS Emulator::LaunchDiscImage(const xechar_t* path) {
  int result_code = 0;

  // Register the disc image in the virtual filesystem.
  result_code = file_system_->RegisterDiscImageDevice(
      "\\Device\\Cdrom0", path);
  if (result_code) {
    XELOGE("Unable to mount disc image");
    return result_code;
  }

  // Create symlinks to the device.
  file_system_->CreateSymbolicLink(
      "game:",
      "\\Device\\Cdrom0");
  file_system_->CreateSymbolicLink(
      "d:",
      "\\Device\\Cdrom0");

  // Launch the game.
  return xboxkrnl_->LaunchModule("game:\\default.xex");
}

X_STATUS Emulator::LaunchSTFSTitle(const xechar_t* path) {
  int result_code = 0;

  // TODO(benvanik): figure out paths.

  // Register the disc image in the virtual filesystem.
  result_code = file_system_->RegisterSTFSContainerDevice(
      "\\Device\\Cdrom0", path);
  if (result_code) {
    XELOGE("Unable to mount STFS container");
    return result_code;
  }

  // Create symlinks to the device.
  file_system_->CreateSymbolicLink(
      "game:",
      "\\Device\\Cdrom0");
  file_system_->CreateSymbolicLink(
      "d:",
      "\\Device\\Cdrom0");

  // Launch the game.
  return xboxkrnl_->LaunchModule("game:\\default.xex");
}
