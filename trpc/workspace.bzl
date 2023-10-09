"""This module contains some dependency"""

# buildifier: disable=load
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def clean_dep(dep):
    return str(Label(dep))

# buildifier: disable=function-docstring-args
def naming_polarismesh_workspace(path_prefix = "", repo_name = "", **kwargs):
    """Build rules for the trpc project

    Note: The main idea is to determine the required version of dependent software during the build process
          by passing in parameters.
    Args:
        path_prefix: Path prefix.
        repo_name: Repository name of the dependency.
        kwargs: Keyword arguments, dictionary type, mainly used to specify the version and sha256 value of
                dependent software, where the key of the keyword is constructed by the `name + version`.
                eg: protobuf_ver,zlib_ver...
    Example:
        trpc_workspace(path_prefix="", repo_name="", protobuf_ver="xxx", protobuf_sha256="xxx", ...)
        Here, `xxx` is the specific specified version. If the version is not specified through the key,
        the default value will be used. eg: protobuf_ver = kwargs.get("protobuf_ver", "3.8.0")
    """

    if not native.existing_rule("com_googlesource_code_re2"):
        http_archive(
            name = "com_googlesource_code_re2",
            sha256 = "0915741f524ad87debb9eb0429fe6016772a1569e21dc6d492039562308fcb0f",
            strip_prefix = "re2-2020-10-01",
            urls = ["https://github.com/google/re2/archive/2020-10-01.tar.gz"],
        )

    new_git_repository(
        name = "com_github_polarismesh_polaris",
        remote = "https://github.com/polarismesh/polaris-cpp.git",
        branch = "main",
        build_file = "//third_party/com_github_polarismesh_polaris:polaris.BUILD",
    )
