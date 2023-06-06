#include "nnue.h"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>


#define USE_AVX2 true

namespace chess {
namespace {

int ceil_div(int x, int y) {
  return x / y + (x % y != 0);
}

}  // namespace

NNUE::NNUE(std::string weights_dir,
           std::shared_ptr<NNUE> copy_weights_from) {
  num_layers_ = 4;
  // NOTE: We could load the layer sizes from a file.
  layer_sizes_ = new int[4] { 32, 32, 32, 1 };
  input_sizes_ = new int[4] { 14*14*7, 4*32, 32, 32 };
  linear_output_0_ = new float*[4];
  l0_output_ = new float*[4];
  for (int player_id = 0; player_id < 4; player_id++) {
    linear_output_0_[player_id] = new float[layer_sizes_[0]];
    l0_output_[player_id] = new float[layer_sizes_[0]];
  }
  l_output_ = new float*[num_layers_];
  l_output_[0] = nullptr;
  for (int layer_id = 1; layer_id < num_layers_; layer_id++) {
    l_output_[layer_id] = new float[layer_sizes_[layer_id]];
  }
  kernel_ = new float**[num_layers_];
  bias_ = new float*[num_layers_];
  for (int layer_id = 0; layer_id < num_layers_; layer_id++) {
    int in_size = input_sizes_[layer_id];
    int out_size = layer_sizes_[layer_id];
    kernel_[layer_id] = new float*[in_size];
    for (int id = 0; id < in_size; id++) {
      kernel_[layer_id][id] = new float[out_size];
    }
    bias_[layer_id] = new float[out_size];
  }

  if (copy_weights_from != nullptr) {
    CopyWeights(*copy_weights_from.get());
  } else {
    LoadWeightsFromFile(weights_dir);
  }
  CopyWeightsToAvxVectors();

}

void NNUE::CopyWeights(const NNUE& copy_from) {
  for (int layer_id = 0; layer_id < num_layers_; layer_id++) {
    int in_size = input_sizes_[layer_id];
    int out_size = layer_sizes_[layer_id];
    for (int id = 0; id < in_size; id++) {
      std::memcpy(
          copy_from.kernel_[layer_id][id],
          kernel_[layer_id][id],
          out_size);
    }

    std::memcpy(
        copy_from.bias_[layer_id],
        bias_[layer_id],
        out_size);
  }
}

void NNUE::LoadWeightsFromFile(const std::string& weights_dir) {
  // load weights from files
  std::filesystem::path wpath(weights_dir);
  std::string line;

  for (int layer_id = 0; layer_id < num_layers_; layer_id++) {
    // kernel
    std::string kernel_filename = "layer_" + std::to_string(layer_id) + ".kernel";
    std::ifstream kernel_infile(wpath / kernel_filename);
    if (!kernel_infile.good()) {
      std::cout << "Can't open kernel file: " << kernel_filename << std::endl;
      abort();
    }
    while (std::getline(kernel_infile, line)) {
      std::istringstream ss(line);
      // shape: [layer_sizes[layer_id-1]][layer_sizes[layer_id]]
      for (int input_id = 0; input_id < input_sizes_[layer_id]; input_id++) {
        for (int output_id = 0; output_id < layer_sizes_[layer_id]; output_id++) {
          ss >> kernel_[layer_id][input_id][output_id];
          ss.ignore();  // ignore commas
        }
      }
    }

    // bias
    std::string bias_filename = "layer_" + std::to_string(layer_id) + ".bias";
    std::ifstream bias_infile(wpath / bias_filename);
    if (!bias_infile.good()) {
      std::cout << "Can't open bias file: " << bias_filename << std::endl;
      abort();
    }
    while (std::getline(bias_infile, line)) {
      std::istringstream ss(line);
      // shape: layer_sizes[layer_id]
      for (int output_id = 0; output_id < layer_sizes_[layer_id]; output_id++) {
        ss >> bias_[layer_id][output_id];
        ss.ignore();  // ignore commas
      }
    }
  }
}

void NNUE::CopyWeightsToAvxVectors() {

#if defined __AVX2__ && USE_AVX2
  avx2_linear_output_0_ = new __m256*[4];
  avx2_l0_output_ = new __m256*[4];
  for (int player_id = 0; player_id < 4; player_id++) {
    avx2_linear_output_0_[player_id] = new __m256[ceil_div(layer_sizes_[0], 8)];
    avx2_l0_output_[player_id] = new __m256[ceil_div(layer_sizes_[0], 8)];
  }
  avx2_l_output_ = new __m256*[num_layers_];
  avx2_l_output_[0] = nullptr;
  for (int layer_id = 1; layer_id < num_layers_; layer_id++) {
    avx2_l_output_[layer_id] = new __m256[ceil_div(layer_sizes_[layer_id], 8)];
  }
  avx2_kernel_rowwise_ = new __m256**[num_layers_];
  avx2_kernel_colwise_ = new __m256**[num_layers_];
  avx2_bias_ = new __m256*[num_layers_];
  for (int layer_id = 0; layer_id < num_layers_; layer_id++) {
    int in_size = input_sizes_[layer_id];
    int out_size = layer_sizes_[layer_id];
    int avx2_in_size = ceil_div(in_size, 8);
    int avx2_out_size = ceil_div(out_size, 8);
    avx2_kernel_rowwise_[layer_id] = new __m256*[in_size];
    for (int id = 0; id < in_size; id++) {
      avx2_kernel_rowwise_[layer_id][id] = new __m256[avx2_out_size];
      // load weights
      for (int avx2_id = 0; avx2_id < avx2_out_size; avx2_id++) {
        avx2_kernel_rowwise_[layer_id][id][avx2_id] = _mm256_loadu_ps(&kernel_[layer_id][id][8 * avx2_id]);
      }
    }
    if (layer_id > 0) { // we don't use the layer 0 colwise kernel
      avx2_kernel_colwise_[layer_id] = new __m256*[out_size];
      for (int id = 0; id < out_size; id++) {
        avx2_kernel_colwise_[layer_id][id] = new __m256[avx2_in_size];
        // load weights
        for (int avx2_id = 0; avx2_id < avx2_in_size; avx2_id++) {
          float colwise_values[8];
          for (int i = 0; i < 8; i++) {
            colwise_values[i] = kernel_[layer_id][8 * avx2_id + i][id];
          }
          avx2_kernel_colwise_[layer_id][id][avx2_id] = _mm256_loadu_ps(colwise_values);
        }
      }
    }
    avx2_bias_[layer_id] = new __m256[avx2_out_size];
    // load weights
    for (int avx2_id = 0; avx2_id < avx2_out_size; avx2_id++) {
      avx2_bias_[layer_id][avx2_id] = _mm256_loadu_ps(&bias_[layer_id][8 * avx2_id]);
    }
  }

#endif
}

NNUE::~NNUE() {
#if defined __AVX2__ && USE_AVX2
  for (int player_id = 0; player_id < 4; player_id++) {
    delete[] avx2_linear_output_0_[player_id];
    delete[] avx2_l0_output_[player_id];
  }
  delete[] avx2_linear_output_0_;
  delete[] avx2_l0_output_;

  for (int layer_id = 0; layer_id < num_layers_; layer_id++) {
    delete[] avx2_bias_[layer_id];
    if (avx2_l_output_[layer_id] != nullptr) {
      delete[] avx2_l_output_[layer_id];
    }
    if (avx2_kernel_rowwise_[layer_id] != nullptr) {
      int in_size = input_sizes_[layer_id];
      for (int id = 0; id < in_size; id++) {
        delete[] avx2_kernel_rowwise_[layer_id][id];
      }
      delete[] avx2_kernel_rowwise_[layer_id];
    }
    if (avx2_kernel_colwise_[layer_id] != nullptr) {
      if (layer_id > 0) {
        for (int id = 0; id < layer_sizes_[layer_id]; id++) {
          delete[] avx2_kernel_colwise_[layer_id][id];
        }
        delete[] avx2_kernel_colwise_[layer_id];
      }
    }
  }
  delete[] avx2_bias_;
  delete[] avx2_l_output_;
  delete[] avx2_kernel_rowwise_;
  delete[] avx2_kernel_colwise_;
#endif

  for (int player_id = 0; player_id < 4; player_id++) {
    delete[] linear_output_0_[player_id];
    delete[] l0_output_[player_id];
  }
  delete[] linear_output_0_;
  delete[] l0_output_;

  for (int layer_id = 0; layer_id < num_layers_; layer_id++) {
    delete[] bias_[layer_id];
    if (l_output_[layer_id] != nullptr) {
      delete[] l_output_[layer_id];
    }
    if (kernel_[layer_id] != nullptr) {
      int in_size = input_sizes_[layer_id];
      for (int id = 0; id < in_size; id++) {
        delete[] kernel_[layer_id][id];
      }
      delete[] kernel_[layer_id];
    }
  }
  delete[] bias_;
  delete[] l_output_;
  delete[] kernel_;
  delete[] layer_sizes_;
  delete[] input_sizes_;

}

void NNUE::InitializeWeights(const std::vector<PlacedPiece>& placed_pieces) {
#if defined __AVX2__ && USE_AVX2

  // 1. Initialize layer-0 linear outputs with biases
  for (int player_id = 0; player_id < 4; player_id++) {
    for (int id = 0; id < ceil_div(layer_sizes_[0], 8); id++) {
      avx2_linear_output_0_[player_id][id] = avx2_bias_[0][id];
    }
  }

#else

  // 1. Initialize layer-0 linear outputs with biases
  for (int player_id = 0; player_id < 4; player_id++) {
    for (int id = 0; id < layer_sizes_[0]; id++) {
      linear_output_0_[player_id][id] = bias_[0][id];
    }
  }

#endif

  // 2. Call SetPiece for each piece
  for (const auto& placed_piece : placed_pieces) {
    SetPiece(placed_piece.GetPiece(), placed_piece.GetLocation());
  }

}

void NNUE::SetPiece(Piece piece, BoardLocation location) {
  int color = piece.GetColor();
  int piece_type = 1 + (int)piece.GetPieceType();
  int row = location.GetRow();
  int col = location.GetCol();
  int s = layer_sizes_[0];

#if defined __AVX2__ && USE_AVX2

  __m256* k = avx2_kernel_rowwise_[0][row*14*7 + col*7 + piece_type];
  for (int i = 0; i < ceil_div(s, 8); i++) {
    // output: [4, layer_sizes[1]]
    avx2_linear_output_0_[color][i] =
      _mm256_add_ps(avx2_linear_output_0_[color][i], k[i]);
  }

#else

  float* k = kernel_[0][row*14*7 + col*7 + piece_type];
  for (int i = 0; i < s; i++) {
    // output: [4, layer_sizes[1]]
    linear_output_0_[color][i] += k[i];
  }

#endif

}

void NNUE::RemovePiece(Piece piece, BoardLocation location) {
  int color = piece.GetColor();
  int piece_type = 1 + (int)piece.GetPieceType();
  int row = location.GetRow();
  int col = location.GetCol();
  int s = layer_sizes_[0];

#if defined __AVX2__ && USE_AVX2

  __m256* k = avx2_kernel_rowwise_[0][row*14*7 + col*7 + piece_type];
  for (int i = 0; i < ceil_div(s, 8); i++) {
    // output: [4, layer_sizes[1]]
    avx2_linear_output_0_[color][i] =
      _mm256_sub_ps(avx2_linear_output_0_[color][i], k[i]);
  }

#else

  float* k = kernel_[0][row*14*7 + col*7 + piece_type];
  for (int i = 0; i < s; i++) {
    // output: [4, layer_sizes[1]]
    linear_output_0_[color][i] -= k[i];
  }

#endif
}

void NNUE::ComputeLayer0Activation() {
#if defined __AVX2__ && USE_AVX2

  for (int player_id = 0; player_id < 4; player_id++) {
    for (int id = 0; id < ceil_div(layer_sizes_[0], 8); id++) {
      // relu
      avx2_l0_output_[player_id][id] =
        _mm256_max_ps(avx2_zero_, avx2_linear_output_0_[player_id][id]);
    }
  }

#else

  for (int player_id = 0; player_id < 4; player_id++) {
    for (int id = 0; id < layer_sizes_[0]; id++) {
      // relu
      l0_output_[player_id][id] = std::max(0.0f, linear_output_0_[player_id][id]);
    }
  }

#endif
}

namespace {

// x = ( x7, x6, x5, x4, x3, x2, x1, x0 )
float sum8(__m256 x) {
    // hiQuad = ( x7, x6, x5, x4 )
    const __m128 hiQuad = _mm256_extractf128_ps(x, 1);
    // loQuad = ( x3, x2, x1, x0 )
    const __m128 loQuad = _mm256_castps256_ps128(x);
    // sumQuad = ( x3 + x7, x2 + x6, x1 + x5, x0 + x4 )
    const __m128 sumQuad = _mm_add_ps(loQuad, hiQuad);
    // loDual = ( -, -, x1 + x5, x0 + x4 )
    const __m128 loDual = sumQuad;
    // hiDual = ( -, -, x3 + x7, x2 + x6 )
    const __m128 hiDual = _mm_movehl_ps(sumQuad, sumQuad);
    // sumDual = ( -, -, x1 + x3 + x5 + x7, x0 + x2 + x4 + x6 )
    const __m128 sumDual = _mm_add_ps(loDual, hiDual);
    // lo = ( -, -, -, x0 + x2 + x4 + x6 )
    const __m128 lo = sumDual;
    // hi = ( -, -, -, x1 + x3 + x5 + x7 )
    const __m128 hi = _mm_shuffle_ps(sumDual, sumDual, 0x1);
    // sum = ( -, -, -, x0 + x1 + x2 + x3 + x4 + x5 + x6 + x7 )
    const __m128 sum = _mm_add_ss(lo, hi);
    return _mm_cvtss_f32(sum);
}

}  // namespace

int32_t NNUE::Evaluate(PlayerColor turn) {
  // Linear output 0 is already computed -- before RELU
  ComputeLayer0Activation();

#if defined __AVX2__ && USE_AVX2

  // Compute output of layer 1
  // Computing the first layer is special since we have to reorder layer-0
  // players by the current turn.
  int in_size = layer_sizes_[0];
  int avx2_in_size = ceil_div(in_size, 8);
  int out_size = layer_sizes_[1];
  float l1_results[out_size];
  for (int out_id = 0; out_id < out_size; out_id++) {
    __m256 result = avx2_zero_;
    for (int turn_addition = 0; turn_addition < 4; turn_addition++) {
      int player_id = (turn + turn_addition) % 4;
      for (int in_id = 0; in_id < avx2_in_size; in_id++) {
        result =
          _mm256_add_ps(
            result,
            _mm256_mul_ps(
                avx2_l0_output_[player_id][in_id],
                avx2_kernel_colwise_[1][out_id][player_id*avx2_in_size + in_id]));
      }
    }
    l1_results[out_id] = std::max(0.0f, sum8(result) + bias_[1][out_id]);
  }
  for (int out_id = 0; out_id < ceil_div(out_size, 8); out_id++) {
    avx2_l_output_[1][out_id] = _mm256_loadu_ps(&l1_results[out_id * 8]);
  }

  // Compute further layers
  for (int layer_id = 2; layer_id < num_layers_; layer_id++) {
    int in_size = layer_sizes_[layer_id - 1];
    int avx2_in_size = ceil_div(in_size, 8);
    int out_size = layer_sizes_[layer_id];
    int avx2_out_size = ceil_div(out_size, 8);
    float l_results[out_size];
    for (int out_id = 0; out_id < out_size; out_id++) {
      __m256 result = avx2_zero_;
      for (int in_id = 0; in_id < avx2_in_size; in_id++) {
        result =
          _mm256_add_ps(
            result,
            _mm256_mul_ps(
              avx2_l_output_[layer_id - 1][in_id],
              avx2_kernel_colwise_[layer_id][out_id][in_id]));
      }
      float f = sum8(result);
      f += bias_[layer_id][out_id];
      if (layer_id < num_layers_ - 1) { // no relu for last layer
        f = std::max(0.0f, f);
      }
      l_results[out_id] = f;
    }
    for (int out_id = 0; out_id < avx2_out_size; out_id++) {
      avx2_l_output_[layer_id][out_id] = _mm256_loadu_ps(&l_results[out_id * 8]);
    }
  }

  float prob = ((float*)avx2_l_output_[num_layers_ - 1])[0];

#else

  // Compute output of layer 1
  // Computing the first layer is special since we have to reorder layer-0
  // players by the current turn.
  int in_size = layer_sizes_[0];
  int out_size = layer_sizes_[1];
  for (int out_id = 0; out_id < out_size; out_id++) {
    l_output_[1][out_id] = 0;
    for (int turn_addition = 0; turn_addition < 4; turn_addition++) {
      int player_id = (turn + turn_addition) % 4;
      for (int in_id = 0; in_id < in_size; in_id++) {
        l_output_[1][out_id] += l0_output_[player_id][in_id]
          * kernel_[1][player_id*in_size + in_id][out_id];
      }
    }
    l_output_[1][out_id] = std::max(0.0f, l_output_[1][out_id] + bias_[1][out_id]);
  }

  // Compute further layers
  for (int layer_id = 2; layer_id < num_layers_; layer_id++) {
    int in_size = layer_sizes_[layer_id - 1];
    int out_size = layer_sizes_[layer_id];
    for (int out_id = 0; out_id < out_size; out_id++) {
      l_output_[layer_id][out_id] = 0;
      for (int in_id = 0; in_id < in_size; in_id++) {
        l_output_[layer_id][out_id] += l_output_[layer_id - 1][in_id]
          * kernel_[layer_id][in_id][out_id];
      }
      l_output_[layer_id][out_id] += bias_[layer_id][out_id];
      if (layer_id < num_layers_ - 1) { // no relu for last layer
        l_output_[layer_id][out_id] = std::max(0.0f, l_output_[layer_id][out_id]);
      }
    }
  }

  float prob = l_output_[num_layers_ - 1][0];

#endif

  // Map from probability to centipawns.
  constexpr float kEpsilon = 1e-8;
  prob = std::max(kEpsilon, std::min(1.0f - kEpsilon, prob));
  float d = -10.0f / std::log(1.0f/.9f-1.0f);
  float centipawns = 100.0f * d * std::log(prob/(1.0f-prob));
  return centipawns;
}


}  // namespace chess
