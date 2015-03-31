/* Copyright 2014 HSA Foundation Inc.  All Rights Reserved.
*
* HSAF is granting you permission to use this software and documentation (if
* any) (collectively, the "Materials") pursuant to the terms and conditions
* of the Software License Agreement included with the Materials.  If you do
* not have a copy of the Software License Agreement, contact the  HSA Foundation
*for a copy.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
* CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*WITH THE SOFTWARE.
*/

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hsa.h"
#include "hsa_ext_finalize.h"
#include "hsa_ext_amd.h"

#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__((aligned(x)))
#endif
#endif

#define MULTILINE(...) #__VA_ARGS__

#define WIDTH 4
#define HEIGHT 4

#define INDEX(x, y, width) (y * width) + x

#pragma pack(push, 1)
typedef struct args_t {
  /*
   * Clover generates code with additional nine parameters.
   * DWORDS 0-2: Number of work groups in each dimension (x,y,z)
   * DWORDS 3-5: Number of global work items in each dimension (x,y,z)
   * DWORDS 6-8: Number of work items within each work group in each dimensions
   *             (x,y,z)
   * DWORDS 9+ : Actual kernel parameters
   */
  uint32_t workgroup_count_x;
  uint32_t workgroup_count_y;
  uint32_t workgroup_count_z;
  uint32_t global_size_x;
  uint32_t global_size_y;
  uint32_t global_size_z;
  uint32_t workgroup_size_x;
  uint32_t workgroup_size_y;
  uint32_t workgroup_size_z;
  int *input_matrix_a;
  int *input_matrix_b;
  int *output_matrix_c;
  int width_a;
  int width_b;
} args_t;
#pragma pack(pop)

void PrintMatrix(int *matrix, int width, int height);
void FillRandomValue(int *matrix, const int range_min, const int range_max,
                     int width, int height, int seed);
hsa_status_t RetrieveGpuAgent(hsa_agent_t agent, void *data);
int SetupKernelCode();
int Setup();
int Execute();
int Verify(int *input_a, int *input_b, int *output_c, int width_a, int height_a,
           int width_b);
void Cleanup();

/*
 * The following string contains the OpenCL kernel used in this sample.
 * The program does not compile /finalize this kernel string programmatically.
 * Instead, an external ".CL" file containing an identical string needs to be
 * created and compiled offline using LLVM toolchain with R600 backend.
 * The offline compilation result is stored in a separate file, which will then
 * be read by the program.
 */
const char *g_matrix_multiply_kernel = MULTILINE(__kernel void matmul(
    __global int *input_matrix_a, __global int *input_matrix_b,
    __global int *output_matrix_c, const int width_a, const int width_b) {
  int i = get_global_id(0);
  int j = get_global_id(1);

  int idx_out = (j * width_b) + i;
  output_matrix_c[idx_out] = 0;

  int k = 0;
  for (k = 0; k < width_a; ++k) {
    output_matrix_c[idx_out] +=
        input_matrix_a[(j * width_a) + k] * input_matrix_b[(k * width_b) + i];
  }
});

hsa_agent_t g_gpu_agent = 0;
hsa_queue_t *g_queue = NULL;

hsa_amd_code_unit_t g_code_unit = 0;
hsa_amd_code_t g_code_handle = 0;

int g_input_matrix_a[HEIGHT][WIDTH] = {0};
int g_input_matrix_b[WIDTH][HEIGHT] = {0};
int g_output_matrix_c[HEIGHT][HEIGHT] = {0};

int main(int argc, char *argv[]) {
  /* Initialize HSA runtime. */
  if (HSA_STATUS_SUCCESS != hsa_init()) {
    perror("Failed initializing HSA\n");
    return 1;
  }

  if (Setup() == 0) {
    assert(g_gpu_agent != 0);
    assert(g_queue != NULL);

    if (Execute() == 0) {
      if (Verify((int *)g_input_matrix_a, (int *)g_input_matrix_b,
                 (int *)g_output_matrix_c, WIDTH, HEIGHT, HEIGHT) == 0) {
        printf("PASSED.\n");
      } else {
        printf("FAILED.\n");
      }
    }

    Cleanup();
  }

  /* Finish using HSA runtime. */
  hsa_shut_down();

  return 0;
}

void PrintMatrix(int *matrix, int width, int height) {
  int i, j;
  for (i = 0; i < height; i++) {
    for (j = 0; j < width; j++) {
      printf(" %d", matrix[INDEX(j, i, width)]);
    }
    printf("\n");
  }
}

void FillRandomValue(int *matrix, const int range_min, const int range_max,
                     int width, int height, int seed) {
  assert(matrix != NULL);

  srand(seed);

  double range = (double)(range_max - range_min) + 1.0;

  int i = 0;
  for (i = 0; i < height; i += 1) {
    int j = 0;
    for (j = 0; j < width; j += 1) {
      matrix[INDEX(j, i, width)] =
          range_min + (int)(range * rand() / (RAND_MAX + 1.0));
    }
  }
}

hsa_status_t RetrieveGpuAgent(hsa_agent_t agent, void *data) {
  if (data == NULL) {
    return HSA_STATUS_ERROR_INVALID_ARGUMENT;
  }

  hsa_device_type_t type;

  hsa_status_t stat = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &type);
  if (stat != HSA_STATUS_SUCCESS) {
    return stat;
  }

  hsa_agent_t *agent_ptr = (hsa_agent_t *)data;
  if (type == HSA_DEVICE_TYPE_GPU && *agent_ptr == 0) {
    *agent_ptr = agent;
  }

  return HSA_STATUS_SUCCESS;
}

int SetupKernelCode() {
  /* Open the file containing the kernel object code. */
  FILE *file = fopen("MatMul.cl.o", "rb");

  if (file == NULL) {
    perror("Fail opening filename MatMul.cl.o\n");
    return -1;
  }

  fseek(file, 0, SEEK_END);
  const size_t file_size = (size_t)ftell(file);
  fseek(file, 0, SEEK_SET);

  char *elf_mem = (char *)malloc(file_size);
  fread(elf_mem, 1, file_size, file);

  hsa_runtime_caller_t caller;
  caller.caller = 0;

  /* Extract the kernel object code. */
  if (HSA_STATUS_SUCCESS != hsa_ext_code_unit_load(caller, NULL, 0, elf_mem,
                                                   file_size, NULL, NULL,
                                                   &g_code_unit)) {
    perror("Failed loading elf file.\n");
    return -1;
  }

  printf("Loaded ELF successfully.\n");

  if (HSA_STATUS_SUCCESS !=
      hsa_ext_code_unit_get_info(g_code_unit,
                                 HSA_EXT_CODE_UNIT_INFO_CODE_ENTITY_CODE, 0,
                                 &g_code_handle)) {
    printf("Failed getting code unit information.\n");
    return -1;
  }

  const uint32_t *amd_version_major = (const uint32_t *)(g_code_handle);
  const uint32_t *amd_version_minor = (const uint32_t *)(g_code_handle + 4);
  const uint32_t *struct_byte_size = (const uint32_t *)(g_code_handle + 8);
  const uint32_t *target_chip = (const uint32_t *)(g_code_handle + 12);
  const int64_t *kernel_code_entry = (const int64_t *)(g_code_handle + 16);

  printf("----------------------\n");
  printf("amd_version_major: %u\n", *amd_version_major);
  printf("amd_version_minor: %u\n", *amd_version_minor);
  printf("struct_byte_size:  %u\n", *struct_byte_size);
  printf("target_chip:       %u\n", *target_chip);
  printf("kernel_code_entry: %ld\n", *kernel_code_entry);
  printf("----------------------\n\n");

  free(elf_mem);

  fclose(file);

  return 0;
}

int Setup() {
  /* Retrieve HSA gpu agent. */
  if (HSA_STATUS_SUCCESS !=
      hsa_iterate_agents(RetrieveGpuAgent, &g_gpu_agent)) {
    perror("Failed retrieving GPU agent\n");
    return -1;
  }

  if (g_gpu_agent == 0) {
    perror("Could not find GPU agent in the platform\n");
    return -1;
  }

  /* Print out name of the gpu. */
  char name[64] = {0};
  if (HSA_STATUS_SUCCESS !=
      hsa_agent_get_info(g_gpu_agent, HSA_AGENT_INFO_NAME, name)) {
    perror("Failed retrieving device name\n");
    return -1;
  }
  printf("Using agent: %s\n", name);

  if (SetupKernelCode() != 0) {
    perror("Failed compiling the kernel\n");
    return -1;
  }

  /* Get queue size. */
  uint32_t queue_size = 0;
  if (HSA_STATUS_SUCCESS != hsa_agent_get_info(g_gpu_agent,
                                               HSA_AGENT_INFO_QUEUE_MAX_SIZE,
                                               &queue_size)) {
    perror("Failed retrieving max queue size\n");
    return -1;
  }

  /* Create queue with maximum allowed size. */
  if (HSA_STATUS_SUCCESS != hsa_queue_create(g_gpu_agent, queue_size,
                                             HSA_QUEUE_TYPE_MULTI, NULL, NULL,
                                             &g_queue)) {
    perror("Failed creating queue\n");
    return -1;
  }

  printf("Matrix multiplication. C = A * B\n");

  /* Populate input with random values. */
  FillRandomValue((int *)g_input_matrix_a, 0, 9, WIDTH, HEIGHT, 1);
  printf("Matrix input A = \n");
  PrintMatrix((int *)g_input_matrix_a, WIDTH, HEIGHT);

  printf("\n");

  FillRandomValue((int *)g_input_matrix_b, 0, 9, HEIGHT, WIDTH, 2);
  printf("Matrix input B = \n");
  PrintMatrix((int *)g_input_matrix_b, HEIGHT, WIDTH);

  printf("\n");

  return 0;
}

int Execute() {
  /* Setup kernel arguments. */
  args_t args = {0};
  args.global_size_x = HEIGHT;
  args.global_size_y = HEIGHT;
  args.global_size_z = 1;
  args.workgroup_count_x = HEIGHT;
  args.workgroup_count_y = HEIGHT;
  args.workgroup_count_z = 1;
  args.workgroup_size_x = 1;
  args.workgroup_size_y = 1;
  args.workgroup_size_z = 1;
  args.input_matrix_a = (int *)g_input_matrix_a;
  args.input_matrix_b = (int *)g_input_matrix_b;
  args.output_matrix_c = (int *)g_output_matrix_c;
  args.width_a = WIDTH;
  args.width_b = HEIGHT;

  /* Setup completion signal. */
  const hsa_signal_value_t kInitVal = 1;
  hsa_signal_t completion_signal = 0;
  if (HSA_STATUS_SUCCESS !=
      hsa_signal_create(kInitVal, 0, NULL, &completion_signal)) {
    perror("Failed on creating completion signal\n");
    return -1;
  }

  /* Setup dispatch packet. */
  hsa_dispatch_packet_t aql;
  memset(&aql, 0, sizeof(aql));

  aql.dimensions = 2;
  aql.workgroup_size_x = 1;
  aql.workgroup_size_y = 1;
  aql.workgroup_size_z = 1;
  aql.grid_size_x = HEIGHT;
  aql.grid_size_y = HEIGHT;
  aql.grid_size_z = 1;
  aql.header.type = HSA_PACKET_TYPE_DISPATCH;
  aql.header.acquire_fence_scope = HSA_FENCE_SCOPE_SYSTEM;
  aql.header.release_fence_scope = HSA_FENCE_SCOPE_SYSTEM;
  aql.header.barrier = 1;
  aql.group_segment_size = 0;
  aql.private_segment_size = 0;
  aql.kernel_object_address = g_code_handle;
  aql.completion_signal = completion_signal;
  aql.kernarg_address = (uint64_t)&args;

  /* Submit kernel. */
  const uint32_t queue_mask = g_queue->size - 1;
  uint64_t index = hsa_queue_load_write_index_relaxed(g_queue);
  ((hsa_dispatch_packet_t *)(g_queue->base_address))[index & queue_mask] = aql;
  hsa_queue_store_write_index_relaxed(g_queue, index + 1);
  hsa_signal_store_relaxed(g_queue->doorbell_signal, index);

  /* Wait til the kernel is finished. */
  hsa_signal_value_t val =
      hsa_signal_wait_acquire(completion_signal, HSA_EQ, 0, (uint64_t)(-1),
                              HSA_WAIT_EXPECTANCY_UNKNOWN);

  return 0;
}

int Verify(int *input_a, int *input_b, int *output_c, int width_a, int height_a,
           int width_b) {
  int output_ref[HEIGHT][HEIGHT] = {0};

  /* Calculate reference result. */
  int i = 0;
  for (i = 0; i < height_a; i += 1) {
    int j = 0;
    for (j = 0; j < width_b; j += 1) {
      int k = 0;
      for (k = 0; k < width_a; k += 1) {
        output_ref[i][j] +=
            (input_a[INDEX(k, i, width_a)] * input_b[INDEX(j, k, width_b)]);
      }
    }
  }

  const size_t memory_size = sizeof(int) * width_a * height_a;
  const int ret = memcmp(output_c, output_ref, memory_size);

  printf("Matrix output C = \n");
  PrintMatrix((int *)g_output_matrix_c, HEIGHT, HEIGHT);

  if (ret != 0) {
    printf("\nExpected output matrix = \n");
    PrintMatrix((int *)output_ref, HEIGHT, HEIGHT);
  }

  printf("\n");

  return ret;
}

void Cleanup() {
  /* Destroy queue. */
  hsa_queue_destroy(g_queue);

  /* Destroy kernel object. */
  hsa_ext_code_unit_destroy(g_code_unit);
}
