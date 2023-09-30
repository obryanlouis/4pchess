package(default_visibility=[
    "//visibility:public",
])


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
        ":utils",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "player",
    hdrs = ["player.h"],
    srcs = ["player.cc"],
    deps = [
        ":board",
        ":transposition_table",
        ":move_picker",
    ],
)

cc_test(
    name = "player_test",
    srcs = ["player_test.cc"],
    deps = [
        ":board",
        ":player",
        ":utils",
        "@com_google_googletest//:gtest_main",
    ],
)


cc_test(
    name = "speed_test",
    srcs = ["speed_test.cc"],
    deps = [
        ":board",
        ":player",
        ":transposition_table",
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

cc_binary(
    name = "cli",
    srcs = ["cli.cc"],
    deps = [
        ":command_line",
    ],
)


cc_library(
    name = "utils",
    srcs = ["utils.cc"],
    hdrs = ["utils.h"],
    deps = [
        ":board",
    ],
)

cc_library(
    name = "command_line",
    srcs = ["command_line.cc"],
    hdrs = ["command_line.h"],
    deps = [
        ":board",
        ":player",
        ":utils",
        ":transposition_table",
    ],
)

cc_test(
    name = "utils_test",
    srcs = ["utils_test.cc"],
    deps = [
        ":utils",
        ":board",
        ":player",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "transposition_table",
    srcs = ["transposition_table.cc"],
    hdrs = ["transposition_table.h"],
    deps = [
        ":board",
    ]
)

cc_library(
    name = "move_picker",
    srcs = ["move_picker.cc"],
    hdrs = ["move_picker.h"],
    deps = [
        ":board",
    ]
)

cc_test(
    name = "regression_test",
    srcs = ["regression_test.cc"],
    deps = [
        ":board",
        ":player",
        ":utils",
        "@com_google_googletest//:gtest_main",
    ],
)

