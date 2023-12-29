import re
import argparse
import skopt
import subprocess

parser = argparse.ArgumentParser(
    prog='Tuner',
    description='Tunes parameters of the chess program')
parser.add_argument('-tune_params', '--tune_params', type=str,
    required=False, default=None)
parser.add_argument('-scm_path', '--scm_path', type=str, required=True)
parser.add_argument('-engine_path', '--engine_path', type=str, required=True)
parser.add_argument('-fens_path', '--fens_path', type=str, required=False,
    default='')
parser.add_argument('-n_calls', '--n_calls', type=int,
    required=False, default=100)
args = parser.parse_args()

# NOTE: These params are outdated and we don't use this file (yet) to tune any
# hyper-params of the program.
_PARAMS = {
  'piece_eval_pawn': { 'space': skopt.space.space.Integer(10, 300), },
  'piece_eval_knight': { 'space': skopt.space.space.Integer(100, 600), },
  'piece_eval_bishop': { 'space': skopt.space.space.Integer(200, 600), },
  'piece_eval_rook': { 'space': skopt.space.space.Integer(200, 900), },
  'piece_eval_queen': { 'space': skopt.space.space.Integer(500, 1500), },
}



def tune_params(param_names):
  dimensions = []
  for param_name in param_names:
    if param_name not in _PARAMS:
      raise ValueError(
          'Parameter optimization not handled for parameter: ' + param_name)
    param_info = _PARAMS[param_name]
    dimensions.append(param_info['space'])

  def format_param_values(param_values):
    param_str = [f'{name}={value}' for name, value in zip(param_names, param_values)]
    param_str = ', '.join(param_str)
    return f'[{param_str}]'

  def f(param_values):
    assert len(param_names) == len(param_values)
    custom_args = []
    for param_name, param_value in zip(param_names, param_values):
      custom_args.extend([
          '--custom2', f'"setoption name {param_name} value {param_value}"',
      ])

    scm_args = [
      args.scm_path,
      '--e1', args.engine_path,
      '--e2', args.engine_path,
      '--fixed', '200',
      '--games', '100',
      '--threads', '10',
      '--maxmoves', '100',
    ] + custom_args
    if args.fens_path:
      scm_args.extend(['--fens', args.fens_path])
    full_cmd = ' '.join(scm_args)

    result = subprocess.run(scm_args, capture_output=True, encoding='utf-8')
    output = result.stdout

    lines = output.split('\n')
    for line in reversed(lines):
      pattern = 'Engine1.*?(\d+) wins.*?Engine2.*?(\d+) wins'
      match = re.search(pattern, line)
      if match is not None:
        win1 = float(match.group(1))
        win2 = float(match.group(2))
        print('win1:', win1, 'win2:', win2, 'param_values:', format_param_values(param_values))
        if win2 == 0:
          return 1
        return win1/(win1 + win2)
    print('output:', output)
    raise ValueError('chess match program did not return win status')

  def callback(result):
    print('#iters:', len(result.x_iters), ', '
          'new win rate:', 1.-result.fun,
          'params:', format_param_values(result.x)
          )

  result = skopt.gp_minimize(
      f,
      dimensions,
      n_calls=args.n_calls,
      noise=.5**2,
      callback=callback)

  print('===== Best params: ====')
  for param_name, best_value in zip(param_names, result.x):
    print(f'{param_name}={best_value}')
  print('Win rate with best params:', 1.0-result.fun)


def main():
  param_names = args.tune_params.split(',')
  print('Tuning params:', param_names)
  tune_params(param_names)
    

if __name__ == '__main__':
  main()


