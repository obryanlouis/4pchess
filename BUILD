
cc_library(
    name = "board",
    hdrs = ["board.h"],
    srcs = ["board.cc"],
)

cc_test(
    name = "board_test",
    srcs = ["board_test.cc"],
    deps = [
        ":board",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "player",
    hdrs = ["player.h"],
    srcs = ["player.cc"],
    deps = [
        ":board",
    ],
)

cc_test(
    name = "player_test",
    srcs = ["player_test.cc"],
    deps = [
        ":board",
        ":player",
        "@com_google_googletest//:gtest_main",
    ],
)


cc_test(
    name = "speed_test",
    srcs = ["speed_test.cc"],
    deps = [
        ":board",
        ":player",
        "@com_google_googletest//:gtest_main",
    ],
)


cc_binary(
    name = "strength_test",
    srcs = ["strength_test.cc"],
    deps = [
        ":board",
        ":player",
    ],
)

