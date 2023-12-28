import argparse
import scipy

parser = argparse.ArgumentParser()

parser.add_argument('-n_wins', '--n_wins', type=int, required=True)
parser.add_argument('-n_losses', '--n_losses', type=int, required=True)
parser.add_argument('-n_draws', '--n_draws', type=int, required=False,
    default=0)
args = parser.parse_args()

print(scipy.stats.mstats.ttest_1samp(
      [0] * args.n_losses + [1] * args.n_wins + [0.5] * args.n_draws, 0.5,
      alternative='greater'))

