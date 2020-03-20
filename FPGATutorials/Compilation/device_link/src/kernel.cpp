//==============================================================
// Copyright © 2019 Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include "kernel.h"

#include <CL/sycl/intel/fpga_extensions.hpp>

auto exception_handler = [](cl::sycl::exception_list exceptions) {
  for (std::exception_ptr const &e : exceptions) {
    try {
      std::rethrow_exception(e);
    } catch (cl::sycl::exception const &e) {
      std::cout << "Caught asynchronous SYCL exception:\n"
                << e.what() << std::endl;
      std::terminate();
    }
  }
};

class SimpleAdd;

void run_kernel(std::vector<float> &vec_a, std::vector<float> &vec_b,
                std::vector<float> &vec_r) {
  try {
    // Device buffers
    buffer<float, 1> device_a(vec_a.data(), ARRAY_SIZE);
    buffer<float, 1> device_b(vec_b.data(), ARRAY_SIZE);
    buffer<float, 1> device_r(vec_r.data(), ARRAY_SIZE);

#if defined(FPGA_EMULATOR)
    intel::fpga_emulator_selector device_selector;
#elif defined(CPU_HOST)
    host_selector device_selector;
#else
    intel::fpga_selector device_selector;
#endif

    queue deviceQueue(device_selector, exception_handler);
    deviceQueue.submit([&](handler &cgh) {
      // Data accessors
      auto a = device_a.get_access<sycl_read>(cgh);
      auto b = device_b.get_access<sycl_read>(cgh);
      auto r = device_r.get_access<sycl_write>(cgh);
      // Kernel
      cgh.single_task<SimpleAdd>([=]() {
        for (int k = 0; k < ARRAY_SIZE; ++k) {
          r[k] = a[k] + b[k];
        }
      });
    });
  } catch (cl::sycl::exception const &e) {
    std::cout << "Caught synchronous SYCL exception:\n" << e.what() << std::endl;
    std::terminate();
  }
}