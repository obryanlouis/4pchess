bazel run -c opt depth_test -- \
  --fens_filepath="$(pwd)/FENs_4PC_balanced.txt" \
  --move_ms=1000 \
  --num_fens=300 \
