load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utils",
    srcs = ["utils.cc"],
    hdrs = ["utils.h"],
    deps = [
        "@com_google_glog//:glog",
        "@org_tensorflow//:tensorflow_framework"
        # "@com_google_absl//absl/synchronization",
        # "@com_google_absl//absl/random",
    ]
)

cc_test(
    name = "utils_test",
    srcs = ["utils_test.cc"],
    deps = [
        ":utils",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "queue",
    hdrs = ["queue.h"],
    deps = [],
)

cc_test(
    name = "queue_test",
    srcs = ["queue_test.cc"],
    deps = [
        ":queue",
        "@com_google_googletest//:gtest_main",
    ],
)
