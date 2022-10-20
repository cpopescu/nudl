load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")
load(
    "@rules_proto//proto:repositories.bzl",
    "rules_proto_dependencies",
    "rules_proto_toolchains",
)
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
load("@rules_python//python:pip.bzl", "pip_install")
load("@rules_antlr//antlr:lang.bzl", "CPP", "PYTHON")
load("@rules_antlr//antlr:repositories.bzl", "rules_antlr_dependencies")

def nuna_nudl_setup_workspace():
    pip_install(
        name = "nuna_nudl_pip_deps",
        requirements = "@nuna_nudl//:requirements.txt",
        timeout = 1200,
    )
    rules_cc_dependencies()
    rules_proto_dependencies()
    rules_proto_toolchains()
    protobuf_deps()
    rules_antlr_dependencies("4.10.1", CPP, PYTHON)