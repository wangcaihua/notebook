load("@rules_cc//cc:defs.bzl", "cc_library", "cc_proto_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "murmurhash",
    hdrs = ["murmurhash.h"],
    deps = [],
)

cc_library(
    name = "hopscotch_hash_set",
    srcs = ["hopscotch_hash_set.cc"],
    hdrs = ["hopscotch_hash_set.h"],
    deps = [
        ":murmurhash",
        "//concurrency:utils",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/hash",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)


cc_test(
    name = "hopscotch_hash_set_test",
    srcs = ["hopscotch_hash_set_test.cc"],
    deps = [
        ":hopscotch_hash_set",
        # "@gperftools//:libtcmalloc",
        "@com_google_googletest//:gtest_main",
    ],
)
