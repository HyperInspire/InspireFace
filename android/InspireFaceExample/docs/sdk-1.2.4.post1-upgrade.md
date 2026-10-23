# SDK 1.2.4.post1 迁移与验证

## 升级输入与版本

- 初次本地适配使用 SDK 仓库：`/Users/tunm/work/inspireface-android-sdk`，提交 `7675e3e`（当时工作区干净）。
- 初次本地包：`inspireface/build/outputs/aar/inspireface-release.aar`。
- 初次本地 AAR SHA-256：`7cd90439171d0db48b7bb62aca58b17b2cbeb9ea46b56a611a0340d01dba78c8`。
- Android SDK 版本：`1.2.4.post1`；原生内核版本：`1.2.4`；C API level：`2`。
- 正式依赖：`com.github.HyperInspire:inspireface-android-sdk:v1.2.4.post1`，Maven 版本保留发布 tag 中的 `v`。
- 发布页面：[JitPack v1.2.4.post1](https://jitpack.io/#HyperInspire/inspireface-android-sdk/v1.2.4.post1)。
- 支持 `arm64-v8a`、`armeabi-v7a`、`x86_64`，每个 ABI 只有一个 `libInspireFace.so`。

对比基线是本项目原来的 **1.2.0 Java/模型 + 本地 DTO 修补 + 1.2.4 原生库**。
SDK 自身的兼容性审计以 1.2.3.post5 为基线，所以不能直接把其中的“所有旧接口兼容”
套用到本项目的 1.2.0 基础代码。

## 接口和行为差异

| 范围 | 新版变化 | 本项目适配 |
| --- | --- | --- |
| 完整依赖 | Java、portable JNI、原生库、模型和 consumer rules 配套发布 | 删除旧 `inspireface-sdk` 拼装模块，直接消费完整 AAR；不再下载 1.2.0 sources/AAR |
| 旧接口 | 相对 1.2.0，移除七个 boolean 的 `CreateCustomParameter`、`setEnableInteractionLiveness(int)` 和 `InspireFaceVersion.information` | 全项目没有使用这些接口；保留无参工厂与链式配置 |
| Session/管线 | 高层实现转为 portable JNI 与显式参数转换；完整映射十个选项，pose 与旧 landmark 独立 | 原有检测模式、姿态、活体、质量、识别与属性配置继续使用高层 API |
| 检测结果 | 新增 `MultipleFaceData.trackCounts`；结果数组/token 为 Java 自有副本，空检测返回空数组 | 保留现有空检测处理；增加跨帧 token、跟踪计数和 snapshot 副本验证 |
| 资源生命周期 | `Session`、`ImageStream` 实现 `AutoCloseable`；Session 释放后 handle 归零，重复释放无操作，释放失败可抛异常 | 四条图像处理路径采用 try-with-resources；`FaceEngine` 对空/已关闭 Session 不再误减活跃计数 |
| 图像流释放 | Android Bitmap/byte[] facade 和 portable API 对输入缓冲区管理不同 | app 保持 facade 创建和 `ImageStream.close()` 释放，不能混用 `Native.HFReleaseImageStream` |
| FeatureHub | 单结果查询以 V2 明确匹配状态；无匹配为 `id=-1, feature=null`；保留 64 位 ID | app 继续 TopK+阈值与 metadata 联结；搜索/CRUD 与 enable/disable 共用进程锁；存储格式不变 |
| Snapshot/Capture | `getFaces()` 返回自有副本；配置校验更严格；capture 必须先于父 Session 关闭 | 更新集成测试；现有相机注册、PLUS 采集状态机保持业务流程 |
| 新增 API | 完整 `Native` C API、V2 Session、资源校验、组件/诊断、CPU 控制、五点关键点、跟踪控制等 | 验证 portable JNI、C API level、诊断查询；业务按需使用，无需为了升级重写为底层 API |
| 版本诊断 | 原生版本不能区分 Android `post1` 修订 | 启动日志同时记录构建配置的 Android 版本/来源、实测 native 版本与 C API level |

## 模型与已有数据

旧资产来自 1.2.0 AAR，Pikachu/Megatron 资源元数据为 t3.1（2025-02-19）；新完整 AAR
为 t4.0（2025-06-08），新增 `_10_emotion_fp16` 和检测尺寸声明。逐个比较 tar 内成员，
两套模型所有共有权重（包括识别权重）字节一致；推荐识别阈值仍分别为 0.48 和 0.32。
因此本次不删除已有特征库、不修改 ID 序列或图片/metadata 格式。

`GlobalLaunch(Context, model)` 会把本次 AAR 资产覆盖复制到 app 的模型目录。
两个模型的人脸库继续按目录隔离。集成测试用独立临时目录检查持久化和 64 位 ID，
不会修改用户已经注册的人脸。

## 本地与 JitPack 切换

默认 `gradle.properties` 中 `inspirefaceSdkSource=jitpack`，正式联网解析
`com.github.HyperInspire:inspireface-android-sdk:v1.2.4.post1`。
`jitpack` 模式不会读取本地 AAR 作为依赖或自动回退。

```sh
./gradlew --refresh-dependencies \
  :app:assembleDebug :app:assembleRelease :app:testDebugUnitTest \
  :app:lintDebug :app:assembleDebugAndroidTest
./gradlew :app:connectedDebugAndroidTest
./gradlew :app:dependencyInsight \
  --dependency inspireface-android-sdk --configuration debugRuntimeClasspath
```

需要测试本地 SDK 修改时，复制根目录 `sdk.local.properties.example` 为
`sdk.local.properties`，把 `aarPath` 指向 SDK 仓库构建出的 release AAR，并显式传入
`-PinspirefaceSdkSource=local`。该配置文件被 Git 忽略；本机已配置。
也支持同时传入 `-PinspirefaceSdkAar=/absolute/path/to/inspireface-release.aar`。
本项目不会自动重编外部 SDK；修改 SDK 后需先在 SDK 仓库重新构建 AAR。

```sh
./gradlew -PinspirefaceSdkSource=local :app:assembleDebug :app:testDebugUnitTest
```

## 正式 JitPack 接入验证（2026-09-29）

用户已发布 `v1.2.4.post1`，项目默认依赖已切换到该 JitPack 版本。以下结果均基于
联网取得的正式发布包，单独记录，不沿用下面的历史本地包测试结果。

| 检查 | 正式发布包结果 |
| --- | --- |
| 联网依赖解析 | `--refresh-dependencies` 与 `dependencyInsight` 确认解析 `com.github.HyperInspire:inspireface-android-sdk:v1.2.4.post1`，选择 `releaseVariantReleaseRuntimePublication` |
| 构建与版本标识 | Debug / Release / Android instrumentation APK 完整构建成功；两个 app variant 的 `BuildConfig` 均为 source=`jitpack`、version=`v1.2.4.post1` |
| JVM 单元测试 | 53 项：52 通过、0 失败、1 跳过（需显式开启的 PLUS 云服务测试） |
| `lintDebug` | 0 errors；65 warnings |
| 发布包来源 | Gradle 实际缓存 AAR、独立下载 AAR 与官方 `.module` 的 SHA-256 完全一致；JitPack 构建提交与初次本地适配相同 |
| 发布包内容 | `classes.jar`、模型、consumer rules、manifest 与本地包逐字节相同；三个 ABI 的原生库在 NDK strip 后与本地包一致 |
| 最终 APK | Debug / 正常 Release 均包含全部 74 个 SDK 类和四个相同 SDK assets；每个 ABI 一份 SDK 库，与远端 strip 后一致；全部九个 native 库 ELF LOAD 16 KiB 对齐，两个 APK 的 `zipalign -c -P 16 4` 均通过 |
| 临时 R8 混淆构建 | 构建成功；66 个受保护 SDK 类保留原名；`zipalign -c -P 16 4` 通过；正式 minify 配置保持不变 |
| 真机安装与自动测试 | **未执行**：当前 `adb` 无连接设备；不能用初次本地包的安装结果替代发布包验证 |

正式发布 AAR SHA-256：`93c6c2634eb9775c688d10ec67f0148ad8d1e061d96fdab7b8597957999cd25f`。
构建提交：`7675e3eb339bfb93d07586a6bfbff24a94cd85fa`。
远端 AAR 携带的三个 `.so` 未 strip，因此 AAR 比初次本地 release 包大；上述比对
确认其差异来自符号剥离，Java API、模型和原生代码版本一致。

完整构建日志保留在 `build/sdk-upgrade/jitpack-build.log`，R8 验证日志在
`build/sdk-upgrade/jitpack-r8-build.log`。发布包审计 JSON、混淆验证 APK
`app-release-minified.apk` 和 `r8-mapping.txt` 位于 `build/sdk-upgrade/jitpack/`；
这些生成产物不提交。

## 初次本地包验证记录（2026-09-29）

本节保留正式发布前的本地包验证结果；可复用测试保留在 `app/src/test` 与 `app/src/androidTest`。
生成的 Gradle 报告、APK 和日志位于忽略的 `app/build/`，不提交生成产物。

已完成的静态 SDK 审计：SDK 的 `scripts/verify-sdk-api.py` 通过，核对 125 个 C/Java
接口、三 ABI 中的 180 个 JNI 符号、配套库哈希和 ABI 契约。
AAR 模型与 SDK 源目录一致；原生库与 SDK release 构建的 stripped 输出逐字节一致
（AAR 已剥离调试符号，因此不直接等于 `inspireface/libs` 中未剥离的文件）。

| 检查 | 本次结果 |
| --- | --- |
| Debug / Release APK | 构建成功；Release 保持项目原来的不混淆配置 |
| JVM 单元测试 | 53 项：52 通过、0 失败、1 跳过（需显式开启的 PLUS 云服务测试） |
| `lintDebug` | 0 errors；65 warnings |
| Android instrumentation APK | 编译成功，包括新增双模型、FeatureHub 和生命周期测试 |
| SDK 打包来源 | Debug/Release 的三个 SDK `.so` 和四个模型目录资产与 AAR SHA-256 完全相同 |
| Native 数量 | 每个 ABI 只有一个 SDK `libInspireFace.so`；整个 APK 还包含 CameraX 库，共 9 个 `.so` |
| 16 KiB 对齐 | 两 APK 全部 native 库 ELF LOAD 对齐通过，`zipalign -c -P 16 4` 通过 |
| 临时 R8 混淆构建 | `assembleRelease` 成功；66 个受保护 SDK 类保留原名，混淆 APK ZIP 对齐通过；未修改正式 minify 配置 |
| 真机安装/启动 | 手机重连后，使用原有 Debug 签名覆盖安装成功，保留数据；`HomeActivity` 冷启动成功并进入前台，交由用户手动测试 |
| 真机自动测试 | **未完成**：之前 `connectedDebugAndroidTest` 报告 `No connected devices!`；本次仅按用户要求安装启动，未运行自动测试 |
| JitPack | 初次本地适配时未执行；正式发布后的验证见上节 |

主机测试与 lint 报告分别在 `app/build/reports/tests/testDebugUnitTest/index.html` 和
`app/build/reports/lint-results-debug.html`。标准 APK 在 `app/build/outputs/apk/debug/`、
`app/build/outputs/apk/release/`；临时混淆验证 APK 和 mapping 保留在
`build/sdk-upgrade/app-release-minified.apk`、`build/sdk-upgrade/r8-mapping.txt`。

已准备但尚未在设备上运行的测试覆盖：两套模型的各业务 Session、106 点关键点、
识别和活体/属性管线、PLUS 本地预处理；trackCounts 与跨帧 token；snapshot/capture
副本和生命周期；portable direct buffer 路径；重复释放 Session 的计数；双模型独立
临时库的 40-bit ID 增删改查、持久化重开与无匹配语义。每个 SDK 测试前后还检查
app/native Session 和 stream 均释放，避免与 UI 测试的全局运行时状态互相污染。

上述初次记录不代表“本地测试全部通过”，其中真机自动测试当时未完成。
发布包的设备验证状态以上面的正式 JitPack 接入验证为准。

## 验证边界

初次验证以本地完整 AAR 为准，正式接入验证使用 JitPack 发布的完整 AAR。
静态图片推理、相机预览和生命周期验证不等于真人活体
准确率验证；前后摄真人动作、长时间性能、其他设备/ABI 和 16 KiB 页面设备仍需专项实测。
PLUS 的本地预处理与 UI 可离线验证；本次不调用收费云活体接口。
