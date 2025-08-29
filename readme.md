# 相机标定软件V1.0
![演示](.\doc\screenshot\MyVideo_4.gif)

*TODO:*
- [x] 支持海康机器人工业相机
- [x] 单目标定
- [x] 棋盘格支持
- [ ] 双目标定
- [ ] 圆点标定支持
- [ ] 性能优化

### 项目结构：
```plaintext
UWC/
├─3rdparty/     # 第三方插件
├─build/        # 编译后项目文件
├─doc/
│  └─dabao      # inno setup 打包脚本
├─modules/      # 子模块
└─resources/    # 资源文件
    ├─imgs/
    └─qss/
```


### 所需环境
- Qt 6.6.3
- opencv 4.12
- MSVC 2019
- cmake 3.28.1

### 配置教程
1. 拉取仓库代码
```bash
git clone https://github.com/wangnimo/UWC.git
```
2. 检查环境变量
```
D:\Qt\6.6.3\msvc2019_64\bin
D:\opencv\build\x64\vc16\bin
```
3. 使用Qt Creator 构建
