load("@rules_python//python:defs.bzl", "py_binary", "py_library", "py_test")
load("//third_party/org_tensorflow:tensorflow.bzl", "tf_cc_library", "tf_custom_op_library", "tf_cc_test")


tf_custom_op_library(
    name = "tf_deep_dot.so",
    srcs = [
        "runtime/kernels/deepdot_kernel.cc",
        "runtime/ops/deep_dot_ops.cc"
    ],
)

py_library(
    name = "deep_dot",
    srcs = ["deep_dot.py"],
    data = [":tf_deep_dot.so"]
)

py_test(
    name="deep_dot_test",
    srcs = ["deep_dot_test.py"],
    deps = [
        ":deep_dot"
    ]
)


tf_cc_library(
    name = "graph_optimization_pass.so",
    srcs = [
        "runtime/lib/graph_optimization_pass.cc",
    ],
    deps = ["//third_party/nlohmann:json",]
)


py_library(
    name = "pass_utils",
    srcs = ["pass_utils.py"],
    data = [":graph_optimization_pass.so"],
)


py_binary(
    name = "utils",
    srcs = ["utils.py"],
)


py_library(
    name = "local_cluster",
    srcs = ["local_cluster.py"],
    deps = [":utils"]
)

py_binary(
    name= "model",
    srcs = ["model.py"],
    deps = [
        ":local_cluster",
        ":pass_utils",
    ],
    data = ["//data:test_files"]
)