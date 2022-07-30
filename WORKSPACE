workspace(name = "notebook")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//third_party:repo.bzl", "tf_http_archive", "tf_mirror_urls")

http_archive(
    name = "bazel_skylib",
    sha256 = "2ef429f5d7ce7111263289644d233707dba35e39696377ebab8b0bc701f7818e",
    type = "tar.gz",
    url = "https://github.com/bazelbuild/bazel-skylib/releases/download/{version}/bazel-skylib.{version}.tar.gz".format(
        version = "0.8.0",
    ),
)

http_archive(
   name = "rules_proto",
   sha256 = "20b240eba17a36be4b0b22635aca63053913d5c1ee36e16be36499d167a2f533",
   strip_prefix = "rules_proto-11bf7c25e666dd7ddacbcd4d4c4a9de7a25175f8",
   urls = [
      "https://storage.googleapis.com/mirror.tensorflow.org/github.com/bazelbuild/rules_proto/archive/11bf7c25e666dd7ddacbcd4d4c4a9de7a25175f8.tar.gz",
      "https://github.com/bazelbuild/rules_proto/archive/11bf7c25e666dd7ddacbcd4d4c4a9de7a25175f8.tar.gz",
   ],
)

tf_http_archive(
   name = "rules_python",
   sha256 = "aa96a691d3a8177f3215b14b0edc9641787abaaa30363a080165d06ab65e1161",
   urls = tf_mirror_urls("https://github.com/bazelbuild/rules_python/releases/download/0.0.1/rules_python-0.0.1.tar.gz"),
)

tf_http_archive(
   name = "com_google_googletest",
   sha256 = "bc1cc26d1120f5a7e9eb450751c0b24160734e46a02823a573f3c6b6c0a574a7",
   strip_prefix = "googletest-e2c06aa2497e330bab1c1a03d02f7c5096eb5b0b",
   urls = tf_mirror_urls("https://github.com/google/googletest/archive/e2c06aa2497e330bab1c1a03d02f7c5096eb5b0b.zip"),
)

http_archive(
   name = "com_google_glog",
   sha256 = "1ee310e5d0a19b9d584a855000434bb724aa744745d5b8ab1855c85bff8a8e21",
   strip_prefix = "glog-028d37889a1e80e8a07da1b8945ac706259e5fd8",
   urls = [
      "https://mirror.bazel.build/github.com/google/glog/archive/028d37889a1e80e8a07da1b8945ac706259e5fd8.tar.gz",
      "https://github.com/google/glog/archive/028d37889a1e80e8a07da1b8945ac706259e5fd8.tar.gz",
   ],
)

tf_http_archive(
   name = "com_github_gflags_gflags",
   sha256 = "ae27cdbcd6a2f935baa78e4f21f675649271634c092b1be01469440495609d0e",
   strip_prefix = "gflags-2.2.1",
   urls = tf_mirror_urls("https://github.com/gflags/gflags/archive/v2.2.1.tar.gz"),
)

tf_http_archive(
   name = "com_google_absl",
   build_file = "//third_party/absl:com_google_absl.BUILD",
   patch_file = ["//third_party/absl:com_google_absl_fix_mac_and_nvcc_build.patch"],
   sha256 = "f368a8476f4e2e0eccf8a7318b98dafbe30b2600f4e3cf52636e5eb145aba06a",  # SHARED_ABSL_SHA
   strip_prefix = "abseil-cpp-df3ea785d8c30a9503321a3d35ee7d35808f190d",
   urls = [
      "https://storage.googleapis.com/mirror.tensorflow.org/github.com/abseil/abseil-cpp/archive/df3ea785d8c30a9503321a3d35ee7d35808f190d.tar.gz",
      "https://github.com/abseil/abseil-cpp/archive/df3ea785d8c30a9503321a3d35ee7d35808f190d.tar.gz",

   ],
)

tf_http_archive(
   name = "com_google_protobuf",
   patch_file = ["//third_party/protobuf:protobuf.patch"],
   sha256 = "cfcba2df10feec52a84208693937c17a4b5df7775e1635c1e3baffc487b24c9b",
   strip_prefix = "protobuf-3.9.2",
   system_build_file = "//third_party/systemlibs:protobuf.BUILD",
   system_link_files = {
       "//third_party/systemlibs:protobuf.bzl": "protobuf.bzl",
       "//third_party/systemlibs:protobuf_deps.bzl": "protobuf_deps.bzl",
   },
   urls = tf_mirror_urls("https://github.com/protocolbuffers/protobuf/archive/v3.9.2.zip"),
)

tf_http_archive(
   name = "com_github_grpc_grpc",
   sha256 = "b956598d8cbe168b5ee717b5dafa56563eb5201a947856a6688bbeac9cac4e1f",
   strip_prefix = "grpc-b54a5b338637f92bfcf4b0bc05e0f57a5fd8fadd",
   patch_file = ["//third_party/grpc:generate_cc_env_fix.patch"],
   system_build_file = "//third_party/systemlibs:grpc.BUILD",
   system_link_files = {
      "//third_party/systemlibs:BUILD": "bazel/BUILD",
      "//third_party/systemlibs:grpc.BUILD": "src/compiler/BUILD",
      "//third_party/systemlibs:grpc.bazel.grpc_deps.bzl": "bazel/grpc_deps.bzl",
      "//third_party/systemlibs:grpc.bazel.grpc_extra_deps.bzl": "bazel/grpc_extra_deps.bzl",
      "//third_party/systemlibs:grpc.bazel.cc_grpc_library.bzl": "bazel/cc_grpc_library.bzl",
      "//third_party/systemlibs:grpc.bazel.generate_cc.bzl": "bazel/generate_cc.bzl",
      "//third_party/systemlibs:grpc.bazel.protobuf.bzl": "bazel/protobuf.bzl",
  },
   urls = tf_mirror_urls("https://github.com/grpc/grpc/archive/b54a5b338637f92bfcf4b0bc05e0f57a5fd8fadd.tar.gz"),
)


# load("@rules_python//python:pip.bzl", "pip_install")
# Create a central external repo, @my_deps, that contains Bazel targets for all the
# third-party packages specified in the requirements.txt file.
# pip_install(
#    name = "py_deps",
#    requirements = "third_party/py_deps/requirements.txt",
# )

# python -c "import tensorflow as tf; print(tf.sysconfig.get_include())"
# python -c "import tensorflow as tf; print(tf.sysconfig.get_lib())"
new_local_repository(
   name = "org_tensorflow",
   path = "/Users/fitz/opt/anaconda3/envs/py8/lib/python3.8/site-packages/tensorflow",
   build_file = "third_party/org_tensorflow/BUILD",
)

http_archive(
   name = "gperftools",
   build_file = "//third_party:gperftools/gperftools.BUILD",
   sha256 = "81bb34f546ac8cddd064f8935805f8eb19e3c9661188e127b4c90526e944ebff",
   patches = ["//third_party:gperftools/gperftools.patch"],
   patch_args = ["-p1"],
   strip_prefix = "gperftools-2.7",
   urls = [
      "https://github.com/gperftools/gperftools/releases/download/gperftools-2.7/gperftools-2.7.zip",
   ],
)
