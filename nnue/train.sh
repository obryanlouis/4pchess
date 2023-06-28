python3 train.py --output_dir=../data --num_self_play_loops=1000 \
  --train_positions_per_loop=10000 --search_depth=8 --epochs_per_loop=1 \
  --train_last_n=1 --batch_size=32 --nnue_search_rate=0.0
