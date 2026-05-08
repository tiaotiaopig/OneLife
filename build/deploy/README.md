# build/deploy 部署模板

本目录保存 `dist/` 部署包的手写模板，构建产物不落在本目录。

## 内容

- `client-linux-x64/` — 客户端部署包模板（启动脚本、包内 README）
- `server-linux-x64/` — 服务端部署包模板（启动脚本、停止脚本、包内 README）

## 构建入口

在项目根目录执行：

```bash
scripts/build-dist.sh
```

该脚本会：

1. 编译服务端与客户端二进制
2. 组装 `dist/client-linux-x64/` 与 `dist/server-linux-x64/`
3. 生成 `dist/onelife-client-linux-x64.tar.gz` 与 `dist/onelife-server-linux-x64.tar.gz`

## 修改流程

1. 修改本目录下模板文件
2. 重新执行 `scripts/build-dist.sh`
3. 验证 `dist/` 下新产物
4. 提交模板改动（`dist/` 生成内容不需要提交）

## 与 build/ 其他脚本的关系

`build/deploy/` 仅服务于项目根 `dist/` 部署包生成流程，与官方发布构建脚本
（`build/source/makeLinuxBuild`、`build/makeDistributionMacOSX` 等）职责分离：

- 官方发布脚本用于跨平台正式发行、Steam 发布、增量更新
- 本目录用于 Linux 私服/自部署场景下的一键组包
