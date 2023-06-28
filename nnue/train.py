import argparse
import gc
import glob
import matplotlib.pyplot as plt
import numpy as np
import os
import shutil
import subprocess
import tensorflow as tf
import time

parser = argparse.ArgumentParser(
    prog='NNUE trainer',
    description='Trains NNUE using self-play and moderate depth search')

parser.add_argument('--output_dir', required=True, type=str)
parser.add_argument('--num_self_play_loops', required=False, default=10,
                    type=int)
parser.add_argument('--train_positions_per_loop', required=False,
                    default=100000, type=int)
parser.add_argument('--train_last_n', required=False,
                    default=3, type=int)
parser.add_argument('--search_depth', required=False, default=8, type=int)
parser.add_argument('--epochs_per_loop', required=False, default=50, type=int)
parser.add_argument('--batch_size', required=False, default=256, type=int)
parser.add_argument('--nnue_search_rate', required=False, default=0.5,
                    type=float)
parser.add_argument('--mode', required=False, default='train-new',
                    type=str, choices=['train-new', 'train-existing'])
parser.add_argument('--existing_data_dirs', required=False, type=str)


def generate_training_data(
    output_dir, search_depth, num_threads, num_positions,
    nnue_weights_filepath, nnue_search_rate):
  print('generating training data...')
  os.makedirs(output_dir, exist_ok=True)
  cwd = os.getcwd()
  prog_filepath = os.path.join(cwd, '../bazel-bin/nnue/gen_data')
  args = [
    output_dir,
    str(search_depth),
    str(num_threads),
    str(num_positions),
    str(nnue_search_rate),
  ]
  if nnue_weights_filepath:
    args.append(nnue_weights_filepath)
  args = [prog_filepath] + args
  print('cmd:', ' '.join(args))
  completion = subprocess.run(args, stdout=subprocess.PIPE)
  print(completion.stdout.decode('utf-8'))
  if completion.returncode != 0:
    raise ValueError(
        f'Training data gen finished with nonzero return code: '
        f'{completion.returncode}')
  assert os.path.exists(output_dir), f'No data generated at {output_dir}'


def data_generator(train_data_dirs, immediate=False):
  def gen():
    data_dirs = train_data_dirs
    if not isinstance(data_dirs, list):
      data_dirs = [data_dirs]
    board_filepaths = []
    score_filepaths = []
    per_depth_score_filepaths = []
    for data_dir in data_dirs:
      board_filepaths.extend(glob.glob(os.path.join(data_dir, 'board*.csv')))
      score_filepaths.extend(glob.glob(os.path.join(data_dir, 'score*.csv')))
      per_depth_score_filepaths.extend(
          glob.glob(os.path.join(data_dir, 'per_depth_score*.csv')))
    assert len(board_filepaths) == len(score_filepaths)
    assert len(board_filepaths) == len(per_depth_score_filepaths)
    board_filepaths = sorted(board_filepaths)
    score_filepaths = sorted(score_filepaths)
    per_depth_score_filepaths = sorted(per_depth_score_filepaths)

    for board_filepath, score_filepath, per_depth_score_filepath in zip(
        board_filepaths, score_filepaths, per_depth_score_filepaths):
      print('Loading data from:', board_filepath, flush=True)
      board_np = np.genfromtxt(board_filepath, delimiter=',')
      score_np = np.genfromtxt(score_filepath, delimiter=',')
      per_depth_score_np = np.genfromtxt(per_depth_score_filepath, delimiter=',')
      yield (tf.convert_to_tensor(board_np, dtype=tf.int32),
             tf.convert_to_tensor(score_np, dtype=tf.int32),
             tf.convert_to_tensor(per_depth_score_np, dtype=tf.int32))
      gc.collect()

  if not immediate:
    return gen

  results_list = list(gen())
  def immediate_gen():
    for x in results_list:
      yield x
  
  return immediate_gen


def convert_scores_to_probs(scores):
  # Scores are in centipawns
  # A score of 10000 should be mostly winning (~90%)
  denom = -10 / tf.math.log(1/.9 - 1)
  score = tf.cast(scores, tf.float32) / 100 / denom
  return tf.math.sigmoid(score)


def convert_probs_to_scores(probs):
  epsilon = 1e-8
  prob = np.maximum(epsilon, np.minimum(1-epsilon, probs))
  d = -10 / np.log(1/.9-1)
  return 100 * d * np.log(prob/(1-prob))


def train_model(train_data_dirs, last_model_save_dir, model_save_dir,
    epochs_per_loop, train_positions_per_loop, batch_size):
  print('training model...')
  immediate = (train_positions_per_loop is None
               or train_positions_per_loop <= 500000)
  gen = data_generator(train_data_dirs, immediate=immediate)

  dataset = tf.data.Dataset.from_generator(
      gen,
      output_signature=(
        tf.TensorSpec(shape=(None, 784), dtype=tf.int32),
        tf.TensorSpec(shape=(None), dtype=tf.int32),
        tf.TensorSpec(shape=(None, None), dtype=tf.int32)))

  def map_fn(board, score, per_depth_score):
    board = tf.reshape(board, (-1, 4, 14*14))
    probs = convert_scores_to_probs(score)
    return board, probs

  dataset = dataset.map(map_fn)

  def one_hot_layer(x):
    y = tf.one_hot(x, 7)
    y = tf.where(tf.expand_dims(x, -1) != 0, y, 0)
    return tf.reshape(y, (-1, 4, 14*14*7))

  model = tf.keras.Sequential(layers=[
    # (rows x cols,)
    tf.keras.Input(shape=(4, 14*14), dtype=tf.int32),
    tf.keras.layers.Lambda(one_hot_layer),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Flatten(),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dense(32, activation='relu'),
    tf.keras.layers.Dense(1),
  ])
  model.compile(
    optimizer='adam',
    loss=tf.keras.losses.MeanSquaredError(),
    metrics=[tf.keras.metrics.MeanAbsoluteError()])
  model.summary()

  if last_model_save_dir:
    print('Reloading weights from last model dir...')
    model.load_weights(os.path.join(last_model_save_dir, 'nnue.h5'))

  gc.collect()

  history = model.fit(dataset, epochs=epochs_per_loop, batch_size=batch_size,
      verbose=1)
  print('MAE:', history.history['mean_absolute_error'][-1])

  # Save the model and weights
  model.save(os.path.join(model_save_dir, 'nnue.h5'), save_format='h5')

  # Save the weights to CSV.
  # Ideally we would save them to a more performant file format like h5,
  # however reading h5 in C++ requires extra libraries.

  def save_weights(filename, np_arr):
    full_filepath = os.path.join(model_save_dir, filename)
    if os.path.exists(full_filepath):
      os.remove(full_filepath)
    with open(full_filepath, 'wt') as f:
      f.write(','.join([str(float(x)) for x in np_arr]))

  dense_layers = [
    x for x in model.layers
    if isinstance(x, tf.keras.layers.Dense)
  ]
  for i, layer in enumerate(dense_layers):
    kernel, bias = layer.weights
    assert 'kernel' in kernel.name
    assert 'bias' in bias.name
    save_weights(f'layer_{i}.kernel', kernel.numpy().reshape(-1))
    save_weights(f'layer_{i}.bias', bias.numpy().reshape(-1))

  gen = data_generator(train_data_dirs)()
  board_tf, score_tf, per_depth_score_tf = next(gen)
  board_np = board_tf.numpy()
  score_np = score_tf.numpy()
  per_depth_score_np = per_depth_score_tf.numpy()

  # How well do the model scores predict the actual (last-depth) score?

  # Filter out positions that are won/lost
  fltr = np.abs(score_np) < np.max(score_np)
  brd_f = board_np[fltr]
  scr_f = score_np[fltr]

  # Run model on `limit` samples
  pred_scores = []
  limit = min(10000, brd_f.shape[0])
  for i in range(limit):
      pred_score = convert_probs_to_scores(model(brd_f[i]).numpy().reshape(1).item())
      pred_scores.append(pred_score)
  pred_scores = np.array(pred_scores)

  print('How well do the model scores predict the actual (last-depth) score?')
  print(np.mean(np.abs(scr_f[:limit] - pred_scores)))

  # How well do the per-depth scores predict the last-depth score?
  print('How well do the per-depth scores predict the last-depth score?')
  fltr = np.amax(np.abs(per_depth_score_np), axis=1) < np.max(score_np)
  pds = per_depth_score_np[fltr]
  for i in range(pds.shape[1]):
    print('depth:', i)
    print(np.mean(np.abs(pds[:,i] - pds[:,-1])))


def train_new(args):
  """Train a model with newly-generated data."""
  output_dir = args.output_dir
  model_dir = os.path.join(output_dir, 'models')
  last_model_dir = None
  gen_model_dir = os.path.join(output_dir, 'gen_models')
  data_dir = os.path.join(output_dir, 'data')
  os.makedirs(model_dir, exist_ok=True)
  os.makedirs(gen_model_dir, exist_ok=True)
  os.makedirs(data_dir, exist_ok=True)

  gen_id = 1
  while os.path.exists(os.path.join(gen_model_dir, f'gen_{gen_id}')):
    last_model_dir = os.path.join(gen_model_dir, f'gen_{gen_id}')
    gen_id += 1

  train_data_dirs = []
  last_n = args.train_last_n
  while gen_id <= args.num_self_play_loops:
    print('loop:', gen_id)
    # Generate training data
    gen_data_dir = os.path.join(data_dir, f'gen_{gen_id}')
    generate_training_data(
        gen_data_dir,
        args.search_depth,
        num_threads=12,
        num_positions=args.train_positions_per_loop,
        # Generate gen-1 without NNUE
        nnue_weights_filepath=model_dir if gen_id > 1 else None,
        nnue_search_rate=args.nnue_search_rate)

    # Train
    train_data_dirs.append(gen_data_dir)
    train_data_dirs = train_data_dirs[-last_n:]
    train_model(train_data_dirs, last_model_dir, model_dir, args.epochs_per_loop,
        args.train_positions_per_loop, args.batch_size)

    # For debugging, copy the models during each gen
    gmdir = os.path.join(gen_model_dir, f'gen_{gen_id}')
    shutil.copytree(model_dir, gmdir)
    last_model_dir = model_dir
    gen_id += 1


def train_existing(args):
  """Train a model with existing data."""
  dirs = args.existing_data_dirs
  if not dirs:
    raise ValueError(
        f'Must provide --existing_data_dirs with training data location')
  data_dirs = glob.glob(dirs)
  if not data_dirs:
    raise ValueError(f'No data dirs to train on. Searched pattern: {dirs}')

  output_dir = args.output_dir
  model_dir = os.path.join(output_dir, 'models')
  os.makedirs(model_dir, exist_ok=True)

  start = time.time()
  train_model(data_dirs, last_model_save_dir=None, model_save_dir=model_dir,
    epochs_per_loop=args.epochs_per_loop,
    train_positions_per_loop=None,
    batch_size=args.batch_size)
  duration = time.time() - start

  print(f'Completed training in {duration:.3f} seconds')


def train():
  args = parser.parse_args()
  if args.mode == 'train-new':
    train_new(args)
  else:
    assert args.mode == 'train-existing'
    train_existing(args)



if __name__ == '__main__':
  train()

