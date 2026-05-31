# 子任务 01：太阳基线与兼容护栏

## 1. 目标

在不改变现有太阳模拟物理行为的前提下，建立一条可重复检查的基线，并为后续 `body = "Sun"` / `body = "Earth"` 分支预留配置入口。

本任务完成后，旧配置应继续可运行；如果用户不写 `body` 字段，程序行为等价于当前太阳版本。

## 2. 范围

建议涉及文件：

- `src/main.cpp`
- `config/smoke.cfg`
- `config/example.cfg`
- `README.md`
- 可选新增：`DOC/sun_smoke_baseline.md`

不在本任务中做：

- 不新增 `Earth_Model`。
- 不改 `Simulation_Trajectory` 的模型类型。
- 不替换 `rSun` / `mSun` 硬编码。
- 不重命名 CMake target 或可执行文件。

## 3. 实施步骤

1. 记录当前 `config/smoke.cfg` 的运行方式、输出目录和轨迹文件格式。
2. 检查随机数入口。当前 `Trajectory_Simulator` 已有 `Fix_PRNG_Seed(int fixed_seed)`，建议新增可选配置字段 `random_seed`；字段缺失时保持现有 `random_device` 行为，字段存在时按 `random_seed + mpi_rank` 派生每个 rank 的 seed。
3. 在 `main.cpp` 配置读取中增加 `body` 字段，默认值为 `"Sun"`。
4. 当 `body` 缺失或等于 `"Sun"` 时，继续走当前 `Solar_Model` 路径。
5. 当 `body` 是未知值时，给出清晰错误信息，例如 `Unsupported body: <value>`。
6. 在 README 中说明：当前默认 body 是 `Sun`，旧太阳配置无需增加 `body` 字段；`random_seed` 仅用于可复现 smoke，不影响旧配置。
7. 新增或更新 `DOC/sun_smoke_baseline.md`，记录一次基线运行的命令、seed、MPI rank 数、轨迹文件数量、首行/末行样例和关键日志。

## 4. 配置兼容策略

第一阶段只需要支持：

```text
body = "Sun";
initial_radius_rsun = 2.0;
random_seed = 12345;  // optional, only for reproducible smoke checks
```

兼容要求：

- `body` 不存在时，按 `Sun` 处理。
- `body = "Sun"` 时，继续读取 `initial_radius_rsun`。
- `random_seed` 不存在时，随机数行为与当前版本一致。
- `random_seed` 存在时，必须在日志或基线文档里记录实际 seed 派生规则。
- 本任务不强制引入 `initial_radius_body_radius`，但不要阻碍后续新增该字段。

## 5. 验收标准

- `config/smoke.cfg` 不增加 `body` 字段也能运行。
- `config/smoke.cfg` 显式增加 `body = "Sun";` 后也能运行。
- 带 `random_seed` 的 smoke 能重复得到同一组轨迹统计；如果轨迹文件因浮点或 MPI 环境有细微差异，至少总数、分类统计和文件行数应稳定。
- 轨迹 TXT 表头和列顺序不变。
- 旧输出目录规则不变。
- 未知 `body` 会失败并输出明确错误，而不是静默回退到太阳模型。

## 6. 建议验证命令

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
head -5 smoke_output/results_*/trajectory_1_task0.txt
```

如果本地还没有 `build` 目录，先按 README 的 build 步骤配置 CMake。

## 7. 风险与注意事项

- 如果随机种子机制不存在，不要在本任务中顺手重构随机数系统；只记录缺口，后续单独处理。
- 固定 seed 不能成为默认行为；默认仍应适合正常大规模采样。
- 不要把 `body` 默认值写进旧配置后才算通过；真正的兼容点是配置缺失时程序仍能运行。
- 错误信息要避免继续写死 `Sun`，后续会统一改成 `body` 语义。

## 8. 完成情况

完成日期：2026-05-31

执行分支：`earth-task-01-baseline-guardrails`

已完成内容：

- `src/main.cpp` 新增 `body` 配置读取，缺省为 `Sun`。
- 当前阶段仅支持 `body = "Sun"`；未知 body 会输出 `Unsupported body: <value>` 并失败。
- `src/main.cpp` 新增可选 `random_seed` 配置；字段缺失时继续使用 `random_device`，字段存在时按 `random_seed + mpi_rank` 固定各 rank 的 PRNG seed。
- `config/smoke.cfg` 新增 `random_seed = 12345;`，未新增 `body` 字段，用于验证缺省 Sun 兼容路径。
- `README.md` 新增默认 body、显式 Sun 和 `random_seed` 派生规则说明。
- 新增 `DOC/sun_smoke_baseline.md`，记录本轮太阳 smoke 基线、输出统计、轨迹样例和兼容分支检查。

验证结果：

- CMake 配置通过，生成 `build/` 目录。
- `cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4` 通过。
- `mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg` 通过。
- 临时配置显式加入 `body = "Sun";` 后 smoke 通过。
- 临时配置设置 `body = "Earth";` 时输出 `Unsupported body: Earth`，退出码为 1。
- 同一 `random_seed` 连续运行两次，轨迹文件 MD5 均为 `7ce9aae522dca681d9d9c95611bd685f`。

基线输出摘要：

- 轨迹目录：`smoke_output/results_-0.301030_-39.000000`
- 轨迹文件数：1
- `trajectory_1_task0.txt` 行数：10
- Summary：`Captured = 0`，`Free = 1`，`Reflected = 0`，`Aborted = 0`，`Text rows written = 9`，`RK45 steps = 83`

遗留说明：

- 构建中仍有第三方依赖和既有代码警告，本任务未处理这些无关警告。
- 本任务未引入 `Earth_Model`，未修改 `Simulation_Trajectory` 的模型类型，未改变输出目录规则或 TXT 列格式。
