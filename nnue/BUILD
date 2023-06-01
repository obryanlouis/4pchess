package(default_visibility=[
    "//visibility:public",
])

cc_binary(
    name = "gen_data",
    srcs = ["gen_data.cc"],
    deps = [
        "//:board",
        "//:player",
    ],
)

cc_library(
    name = "nnue",
    hdrs = ["nnue.h"],
    srcs = ["nnue.cc"],
    deps = [
        "//:types",
    ],
)


cc_test(
    name = "nnue_test",
    srcs = ["nnue_test.cc"],
    deps = [
        ":nnue",
        "//:types",
        "//:board",
        "@com_google_googletest//:gtest_main",
    ],
)


