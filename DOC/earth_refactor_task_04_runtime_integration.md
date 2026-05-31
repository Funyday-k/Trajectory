# 子任务 04：主程序接入 Earth 运行路径

## 1. 目标

把 `Earth_Model` 接入主程序和配置系统，让用户可以通过 `body = "Earth"` 运行地球 smoke 模拟，同时保持 `body = "Sun"` 和旧太阳配置兼容。

## 2. 范围

建议涉及文件：

- `src/main.cpp`
- `vendor/damascus/include/Simulation_Utilities.hpp`
- `vendor/damascus/src/Simulation_Utilities.cpp`
- `vendor/damascus/include/Simulation_Trajectory.hpp`
- `vendor/damascus/src/Simulation_Trajectory.cpp`
- `config/earth_smoke.cfg`
- `config/earth_example.cfg`
- `CMakeLists.txt`
- `README.md`

不在本任务中做：

- 不继续提升地球组成模型精度。
- 不重命名可执行文件。
- 不改变 TXT 轨迹列格式。
- 不把旧太阳字段立即删除。

## 3. 配置方案

新增地球配置字段：

```text
body = "Earth";
initial_radius_body_radius = 2.0;
asymptotic_distance_body_radius = 1000.0;
bincount_max_radius_body_radius = 2.0;
body_model_file = "vendor/damascus/data/earth_prem.dat";
body_composition = "layered";
```

建议同时支持可复现 smoke 字段：

```text
random_seed = 12345;
```

兼容策略：

- `body` 不存在：默认 `Sun`。
- `body = "Sun"`：优先支持旧字段 `initial_radius_rsun`，也可接受新字段 `initial_radius_body_radius`。
- `body = "Earth"`：优先读取 `initial_radius_body_radius`。
- 旧字段 `initial_radius_rsun` 不用于地球配置，避免语义混乱。
- `body` 缺失时沿用旧输出目录规则；`body` 显式存在时使用 body 前缀输出目录。

## 4. 实施步骤

1. 在 `main.cpp` 中新增 body 读取和模型工厂逻辑。
2. 使用 `std::unique_ptr<Celestial_Model>` 保存当前模型实例。
3. 当 `body = "Sun"` 时构造 `Solar_Model`；当 `body = "Earth"` 时构造 `Earth_Model`。
4. 把散射率插值、轨迹模拟、初始条件采样全部传入 `Celestial_Model&`。
5. 把初始边界从太阳专属字段推广为 `initial_radius_body_radius * body.Radius()`。
6. 把渐近采样距离配置化为 `asymptotic_distance_body_radius * body.Radius()`，避免地球版本继续使用太阳代码中的 `1000 AU` 假设。
7. 把轨迹 bin 最大半径改为 `bincount_max_radius_body_radius * body.Radius()` 或等价配置值。
8. 检查 `Hyperbolic_Kepler_Shift` 已在子任务 02 中使用 `body.Total_Mass()` 和 `body.Radius()`；如果尚未完成，应回补抽象层，不要在主程序中写特殊分支。
9. 新增 `config/earth_smoke.cfg`，用于 1 rank、少量轨迹、较少插值点的快速验证。
10. 新增 `config/earth_example.cfg`，作为正常地球运行示例。
11. 输出启动信息增加 body 名称、半径、质量、模型文件。
12. 输出目录增加 body 前缀，例如 `output/earth/results_<mass>_<cross_section>`。
13. 地球 smoke 配置应使用较小 `sample_size`、`max_trajectories` 和 `interpolation_points`，并启用 `random_seed` 以便复现。

## 5. 输出目录规则

推荐规则：

```text
<output_dir>/<body_lowercase>/results_<log10_dm_mass>_<log10_cross_section>/trajectory_<id>_task<rank>.txt
```

示例：

```text
smoke_output/earth/results_-0.3010_-39.0000/trajectory_1_task0.txt
output/sun/results_-0.3010_-36.0000/trajectory_1_task0.txt
```

如果为了兼容暂时保留太阳旧目录，可以采用过渡策略：

- `body` 缺失时沿用旧目录。
- `body` 显式存在时使用 body 前缀。

该策略需要在 README 中说清楚，避免用户误判输出位置。

## 6. 验收标准

- `mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg` 仍可运行。
- `mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_smoke.cfg` 可运行完成。
- 地球启动日志包含 body、半径、质量和模型文件路径。
- 地球输出目录不会与太阳输出目录混在一起。
- 地球表面逃逸速度日志或检查值数量级接近 `11.2 km/s`。
- 未知 `body` 失败并输出清晰错误。
- `body` 缺失的旧太阳配置仍写到旧目录；显式 `body = "Sun"` 的新配置按 README 说明写入 body 前缀目录或明确采用兼容目录。

## 7. 建议验证命令

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_smoke.cfg
head -5 smoke_output/earth/results_*/trajectory_1_task0.txt
```

## 8. 风险与注意事项

- `std::unique_ptr` 需要 C++11，确保包含 `<memory>`。
- 如果 `Solar_Model` 和 `Earth_Model` 构造参数不同，不要把 factory 写成过度复杂的抽象；先在 `main.cpp` 做清晰分支即可。
- 初始条件采样的半径、逃逸速度和渐近距离必须全部来自当前 body，不能只替换一部分。
- 输出路径改动会影响脚本，必要时采用过渡兼容策略。
- 如果地球 smoke 很难俘获粒子，不要靠无限增大 `max_trajectories` 来掩盖问题；先确认初始条件采样、散射率和逃逸速度量级正确。
