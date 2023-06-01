import argparse
import gc
import glob
import matplotlib.pyplot as plt
import numpy as np
import os
import subprocess
import tensorflow as tf
import time

parser = argparse.ArgumentParser(
    prog='NNUE trainer',
    description='Trains NNUE using self-play and moderate depth search')

parser.add_argument('--output_dir', required=True)
parser.add_argument('--num_self_play_loops', required=False, default=10)
parser.add_argument('--train_positions_per_loop', required=False,
                    default=100000)
parser.add_argument('--search_depth', required=False, default=8)


def generate_training_data(
    output_dir, search_depth, num_threads, num_positions, nnue_weights_filepath):
  cwd = os.getcwd()
  prog_filepath = os.path.join(cwd, '../bazel-bin/gen_data')
  args = [
    output_dir,
    search_depth,
    num_threads,
    num_positions,
  ]
  if nnue_weights_filepath:
    args.append(nnue_weights_filepath)
  args = [prog_filepath] + args
  subprocess.run(args)


def train_model(train_data_dirs, model_save_dir):
  # copy code from the jupyter notebook...

  pass


def train():
  args = parser.parse_args()
  output_dir = args.output_dir
  model_dir = os.path.join(output_dir, 'models')
  data_dir = os.path.join(output_dir, 'data')
  os.makedirs(model_dir, exist_ok=True)
  os.makedirs(data_dir, exist_ok=True)

  # Generate gen-0 without NNUE
  generate_training_data(
      os.path.join(data_dir, 'gen_0'),
      args.search_depth,
      num_threads=12,
      num_positions=args.train_positions_per_loop,
      nnue_weights_filepath=None)

  # Train
  train_data_dirs = [os.path.join(data_dir, 'gen_0')]
  train_model(train_data_dirs, model_dir)

  for gen_id in range(1, args.num_self_play_loops + 1):
    # Generate gen-0 without NNUE
    gen_data_dir = os.path.join(data_dir, f'gen_{i}')
    generate_training_data(
        gen_data_dir,
        args.search_depth,
        num_threads=12,
        num_positions=args.train_positions_per_loop,
        nnue_weights_filepath=model_dir)

    # Train
    train_data_dirs.append(gen_data_dir)
    train_data_dirs = train_data_dirs[-3:]
    train_model(train_data_dirs, model_dir)



if __name__ == '__main__':
  train()

