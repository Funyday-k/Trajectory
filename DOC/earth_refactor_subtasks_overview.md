# 地球模拟改造子任务总览

本文档把 `earth_simulation_refactor_plan.md` 拆成若干可独立执行、可验收的子任务书。拆分原则是先保护现有太阳模拟，再抽象通用天体接口，随后接入地球模型，最后完善输出、文档和物理校验。

## 0. 审核结论与修订重点

本轮审核后，原子任务拆分方向保持不变，但需要补强几个会影响实现成败的点：

1. 子任务 01 需要增加可复现随机种子策略，否则太阳 smoke 基线只能做日志样例，难以做回归对比。
2. 子任务 02 不能只替换 `Solar_Model&` 类型，还必须同步处理 `Initial_Conditions` 的渐近距离参数、`Hyperbolic_Kepler_Shift` 的天体质量参数，以及 `TrajectoryBincount` 的太阳半径编译期常量。
3. 子任务 03 的地球数据要明确“工程 smoke 数据”和“正式物理数据”的边界，并要求数据文件带元信息或配套文档。
4. 子任务 04 的输出目录兼容策略要和子任务 01 的默认 `body` 策略保持一致，避免旧太阳任务输出路径突然变化。
5. 子任务 05 需要把 sanity checks 变成可重复的检查命令或记录模板，不能只停留在人工检查列表。

## 1. 总体目标

把当前 `DaMaSCUS-SUN Trajectory TXT` 从只支持太阳模型，逐步改造成可由配置选择天体模型的轨迹模拟程序。第一轮改造完成后，至少应支持：

- `body = "Sun"`：保持现有太阳模拟行为。
- `body = "Earth"`：使用地球半径、质量、径向结构、元素组成和逃逸速度运行 smoke 级别轨迹模拟。
- 同一套轨迹传播、散射采样、MPI 并行和 TXT 输出框架服务不同天体。

## 2. 子任务拆分

| 顺序 | 子任务书 | 目标 | 主要产物 |
| --- | --- | --- | --- |
| 01 | `earth_refactor_task_01_baseline_guardrails.md` | 建立太阳模拟基线，保证旧配置继续可运行 | smoke 基线记录、`body = "Sun"` 默认策略、README 兼容说明 |
| 02 | `earth_refactor_task_02_celestial_model_abstraction.md` | 把轨迹代码从 `Solar_Model` 具体类型解耦 | `Celestial_Model.hpp`、`Solar_Model` 继承接口、轨迹/工具函数改签名 |
| 03 | `earth_refactor_task_03_earth_model_data.md` | 新增地球模型类和基础数据 | `Earth_Model.hpp/.cpp`、`earth_prem.dat`、地球组成和数密度实现 |
| 04 | `earth_refactor_task_04_runtime_integration.md` | 让主程序按配置选择 Sun 或 Earth 并正确输出 | `body` 配置入口、`earth_smoke.cfg`、body 前缀输出目录 |
| 05 | `earth_refactor_task_05_validation_and_docs.md` | 做物理 sanity checks 并补齐使用文档 | README 地球示例、数据格式文档、太阳/地球 smoke 验收记录 |

## 3. 任务边界修正

为减少后续返工，各任务边界按以下规则执行：

- 子任务 01 只允许新增兼容型配置和基线记录；任何缺省行为必须与当前太阳版本一致。
- 子任务 02 是“算法层去太阳化”的关键任务，必须把轨迹层内的 `rSun`、`mSun`、`R_SUN_KM`、`BIN_MAX_KM` 等太阳常量全部迁走或参数化。太阳专属文件 `Solar_Model.*` 中允许保留太阳常量。
- 子任务 03 只负责让 `Earth_Model` 可构造、可编译、基础量级正确；不要在此任务接入主程序运行分支。
- 子任务 04 负责把配置、模型工厂、输出路径和地球 smoke 串起来；如果发现任务 02 遗留太阳常量，应先回补任务 02 的抽象，不要在主程序里做临时绕路。
- 子任务 05 负责最终文档和验证闭环，包括记录无法运行验证时的具体依赖缺口。

## 4. 推荐执行顺序

1. 先执行子任务 01，冻结太阳版本的可运行基线。
2. 执行子任务 02，只做抽象和签名替换，不引入地球运行分支。
3. 执行子任务 03，添加 `Earth_Model` 和数据，但尽量让新增代码能独立编译验证。
4. 执行子任务 04，把 `Earth_Model` 接到主程序、配置和输出路径。
5. 执行子任务 05，补物理检查、文档和最终 smoke 记录。

## 5. 跨任务约束

- 保持 C++11 兼容，不引入需要 C++14 或更高版本的库特性。
- 第一轮不重命名现有可执行文件 `DaMaSCUS-SUN-TrajectoryTXT`，避免破坏脚本兼容。
- 旧字段 `initial_radius_rsun` 在太阳配置中继续可用；新字段使用 `initial_radius_body_radius`。
- 轨迹 TXT 列格式保持不变：`time_s x_km y_km z_km vx_km_s vy_km_s vz_km_s E_eV`。
- `body` 缺失时必须沿用旧太阳输出目录规则；只有显式配置 `body` 时才允许启用新的 body 前缀输出目录。
- 随机种子字段必须是可选字段，缺失时保持现有随机行为；用于基线时应记录 seed 和 MPI rank 的派生规则。
- 地球数据第一版如果是简化数据，必须在配置、数据格式文档或验收记录中明确标注“仅用于 smoke / 工程验证”。
- 每个子任务完成后都至少运行一次窄范围验证；如果验证命令暂时跑不通，需要记录原因和阻塞点。

## 6. 里程碑验收

### 里程碑 M1：太阳兼容基线

- 旧 `config/smoke.cfg` 不修改即可运行。
- README 明确默认 `body` 是 `Sun`。
- 有一份可对比的 smoke 输出统计。
- 如果加入固定 seed，有一份固定 seed 的 smoke 基线；如果无法固定 seed，明确记录原因。

### 里程碑 M2：通用接口可用

- `Simulation_Trajectory` 和 `Simulation_Utilities` 不再依赖 `Solar_Model&` 作为核心接口类型。
- 太阳 smoke 行为仍然通过。
- 轨迹层不再直接依赖 `rSun`、`mSun`、`R_SUN_KM` 或 `BIN_MAX_KM` 这类太阳专属常量。

### 里程碑 M3：地球模型可构造

- `Earth_Model` 能读入地球径向表。
- `Mass(R_earth)`、`Local_Escape_Speed(R_earth)` 等基础量级合理。
- 地球数据文件和组成模型的精度等级已标注。

### 里程碑 M4：地球 smoke 可运行

- `config/earth_smoke.cfg` 能完成 1 rank 运行。
- 输出目录明确包含 body 信息，不与太阳结果混在一起。

### 里程碑 M5：文档和物理检查齐备

- README 给出太阳和地球两套运行命令。
- `DOC/earth_model_data_format.md` 描述数据来源、字段和单位。
- 记录太阳/地球 smoke 验证结果。
