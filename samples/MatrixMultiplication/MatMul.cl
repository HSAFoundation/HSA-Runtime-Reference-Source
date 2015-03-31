  
__kernel void matmul(__global int *input_matrix_a, 
                     __global int *input_matrix_b,
                     __global int *output_matrix_c, 
                     const    int width_a, 
                     const    int width_b) 
{
  int i = get_global_id(0);
  int j = get_global_id(1);

  int idx_out = (j * width_b) + i;
  output_matrix_c[idx_out] = 0;

  int k = 0;
  for (k = 0; k < width_a; ++k) 
  {
    output_matrix_c[idx_out] +=
      input_matrix_a[(j * width_a) + k] * input_matrix_b[(k * width_b) + i];
  }
}

