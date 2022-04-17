这个repo用于记录不发环境, 测试, 示例代码片段等(基于osx). 主要参考:
- [配置 Visual Studio Code](https://tensorflow.google.cn/io/vscode?hl=zh-cn)
- [Debug Tensorflow的C++代码](https://blog.codescv.com/debug-tf-cpp.html)
- [tensorflow二次开发](https://leslie-fang.github.io/2019/02/27/tensorflow%E4%BA%8C%E6%AC%A1%E5%BC%80%E5%8F%91/)

# 1. 软件安装
```bash
# 1) install python
wget https://repo.anaconda.com/miniconda/Miniconda3-py38_4.9.2-MacOSX-x86_64.sh
chmod a+x Miniconda3-py38_4.9.2-MacOSX-x86_64.sh
./Miniconda3-py38_4.9.2-MacOSX-x86_64.sh 

# 2) create python env and install tensorflow
conda create -n py8 python=3.8
conda activate py8
pip install tensorflow==2.4.0

# 3) install bazel
# get bazel version from tf: https://github.com/tensorflow/tensorflow/blob/v2.4.0/.bazelversion
export BAZEL_VERSION=3.1.0
curl -fLO "https://github.com/bazelbuild/bazel/releases/download/${BAZEL_VERSION}/bazel-${BAZEL_VERSION}-installer-darwin-x86_64.sh"
chmod +x "bazel-${BAZEL_VERSION}-installer-darwin-x86_64.sh"
./bazel-${BAZEL_VERSION}-installer-darwin-x86_64.sh --user

# 4) install vscode
# download from https://code.visualstudio.com/download
# install plugin: C/C++, Python, ...
```

# 2. 环境配置

## 2.1 C++环境配置
### 2.1.1 c_cpp_properties.json文件配置
```json
{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "${workspaceFolder}",
                "${workspaceFolder}/tfext/**",
                "/Users/fitz/opt/anaconda3/envs/py8/lib/python3.8/site-packages/tensorflow/include/**"
            ],
            "defines": [],
            "macFrameworkPath": [],
            "compilerPath": "/usr/local/bin/gcc-11",
            "cStandard": "gnu17",
            "cppStandard": "gnu++17",
            "intelliSenseMode": "macos-gcc-x64"
        }
    ],
    "version": 4
}
```
其中, includePath 中要包括tf的头文件, 可以用如下方式获得:
> python -c "import tensorflow as tf; print(tf.sysconfig.get_include())"
对于, `compilerPath`, `cStandard`, `cppStandard`, `intelliSenseMode`, 不同环境可能不一样, 这里是macos的例子. 一般用默认值就好, 可以通过setting先看默认值, 然后填在这里.

### 2.1.2 task.json文件配置
```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Example (Debug)",
            "type": "shell",
            "command": "bazel build //tfext:example -c dbg --spawn_strategy=standalone",
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": []
        },
    ]
}
```
由于配置task用要用于调试, 如果是运行, 直接`bazel run`就好, 所以`command`配置时加`-c dbg --spawn_strategy=standalone`. 如果有多个task, 在tasks列表中增加就好. `label`中在lauch中引用task的标签, 不能重名.

### 2.1.2 lauch.json文件配置
```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Example",
            "preLaunchTask": "Build Example (Debug)",
            "program": "${workspaceFolder}/bazel-out/darwin-dbg/bin/tfext/example",
            "cwd": "${workspaceFolder}/bazel-out/darwin-dbg/bin/tfext/example.runfiles/notebook/",
            "args": [],
            "type": "cppdbg",
            "request": "launch",
            "MIMode": "lldb",
            "stopAtEntry": false,
            "externalConsole": false,
            "sourceFileMap": {
                "/private/var/tmp/_bazel_fitz/3558c7b26f0678a7a819549aed6127a8/execroot/notebook/": "${workspaceFolder}",
            },
        }
    ]
}
```
其中有几个地方比较关键:
- preLaunchTask: 对应task中的label
- sourceFileMap: `/private/var/tmp/_bazel_fitz/3558c7b26f0678a7a819549aed6127a8/execroot/notebook/`, notebook是项目名, 要 WORKSPACE 中通过`workspace(name = "notebook")`指定, 如果没有指定, 就是`__main__`
- program: `${workspaceFolder}/bazel-out/darwin-dbg/bin/{path_to_binary}`, 例如 tfext/example, 相对WORKSPACE目录的路径
- cwd: `${workspaceFolder}/bazel-out/darwin-dbg/bin/{path_to_binary}.runfiles/notebook/`, 其中 path_to_binary 与上面一样, notebook是项目名, 如果没有指定, 就是`__main__`

这样就可以用vscode调试bazel编的程序了



## 2.2 Python环境配置
```json
{
    "python.defaultInterpreterPath": "/Users/fitz/opt/anaconda3/envs/py8/bin/python"
}
```
