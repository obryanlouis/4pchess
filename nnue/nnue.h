#ifndef __NNUE_H__
#define __NNUE_H__

#include <immintrin.h>
#include <memory>
#include <vector>

#include "../types.h"

namespace chess {

class NNUE {
 public:
  NNUE(std::string weights_dir,
       std::shared_ptr<NNUE> copy_weights_from = nullptr);
  ~NNUE();

  void InitializeWeights(const std::vector<PlacedPiece>& placed_pieces);
  void SetPiece(Piece piece, BoardLocation location);
  void RemovePiece(Piece piece, BoardLocation location);
  // Outputs the eval score with respect to team of 'turn'.
  int32_t Evaluate(PlayerColor turn);

 private:
  void ComputeLayer0Activation();
  void CopyWeights(const NNUE& copy_from);
  void LoadWeightsFromFile(const std::string& weights_dir);
  void CopyWeightsToAvxVectors();

  int num_layers_ = 0;
  int* layer_sizes_ = nullptr;
  int* input_sizes_ = nullptr;

  float** linear_output_0_ = nullptr; // [4][layer_sizes[0]]
  float** l0_output_ = nullptr;  // [4][layer_sizes[0]]
  float** l_output_ = nullptr; // [num_layers][layer_sizes[i]]
  // note that the first kernel input size is 4*layer_sizes[0]
  float*** kernel_ = nullptr; // [num_layers][layer_sizes[i-1]][layer_sizes[i]]
  float** bias_ = nullptr; // [num_layers][layer_sizes[i]]

#ifdef __AVX2__

  // __m256 == 8 floats

  __m256** avx2_linear_output_0_ = nullptr; // [4][layer_sizes[0]]
  __m256** avx2_l0_output_ = nullptr;  // [4][layer_sizes[0]]
  __m256** avx2_l_output_ = nullptr; // [num_layers][layer_sizes[i]]
  __m256*** avx2_kernel_rowwise_ = nullptr; // [num_layers][layer_sizes[i-1]][layer_sizes[i]]
  __m256*** avx2_kernel_colwise_ = nullptr; // [num_layers][layer_sizes[i]][layer_sizes[i-1]]
  __m256** avx2_bias_ = nullptr; // [num_layers][layer_sizes[i]]
  __m256 avx2_zero_ = _mm256_set_ps(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

#endif

};

}  // namespace chess

#endif // __NNUE_H__
