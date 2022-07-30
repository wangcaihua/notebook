load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")
load("//third_party/org_tensorflow:tensorflow.bzl", "tf_cc_library", "tf_cc_test")

tf_cc_test(
  name = "str_test",
  size = "small",
  srcs = ["str_test.cc"],
)

cc_test(
  name = "vector_test",
  size = "small",
  srcs = ["vector_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)

cc_test(
  name = "set_test",
  size = "small",
  srcs = ["set_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)

cc_test(
  name = "ump_test",
  size = "small",
  srcs = ["ump_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)


cc_test(
  name = "iter_test",
  size = "small",
  srcs = ["iter_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)


cc_test(
  name = "lock_test",
  size = "small",
  srcs = ["lock_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)


cc_test(
  name = "thread_test",
  size = "small",
  srcs = ["thread_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)


cc_test(
  name = "algo_test",
  size = "small",
  srcs = ["algo_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)


tf_cc_test(
  name = "tensor_test",
  srcs = ["tensor_test.cc"],
)


cc_test(
  name = "io_test",
  size = "small",
  srcs = ["io_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)


cc_test(
  name = "random_test",
  size = "small",
  srcs = ["random_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)


cc_test(
  name = "time_test",
  size = "small",
  srcs = ["time_test.cc"],
  deps = ["@com_google_googletest//:gtest_main"],
)
