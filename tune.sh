python3 tune.py \
  -scm_path /Users/louisobryan/simplechessmatch/scm \
  -engine_path /Users/louisobryan/4pchess/bazel-bin/cli \
  -fens_path /Users/louisobryan/simplechessmatch/FENs_4PC_balanced.txt \
  -tune_params "piece_eval_pawn,piece_eval_knight,piece_eval_bishop,piece_eval_rook,piece_eval_queen" \
  -n_calls 100

