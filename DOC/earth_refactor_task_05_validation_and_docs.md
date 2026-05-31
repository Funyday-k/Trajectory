# 子任务 05：物理校验、文档与最终验收

## 1. 目标

在地球 smoke 能运行之后，补齐物理 sanity checks、数据格式说明、README 示例和最终验收记录，避免项目只是在工程上能跑但物理语义不清楚。

## 2. 范围

建议涉及文件：

- `README.md`
- `DOC/earth_model_data_format.md`
- `DOC/sun_smoke_baseline.md`
- `DOC/earth_smoke_validation.md`
- `config/earth_smoke.cfg`
- `config/earth_example.cfg`
- 可选：新增窄测试或检查脚本，位置按项目现有习惯确定

不在本任务中做：

- 不再大改模型接口。
- 不改变轨迹输出列格式。
- 不引入新的正式地球化学模型，除非前面任务明确留下阻塞。

## 3. 必做校验项

地球模型至少检查：

1. `Earth_Model::Mass(0)` 接近 0。
2. `Earth_Model::Mass(R_earth)` 接近 `M_earth`。
3. `Earth_Model::Local_Escape_Speed(R_earth)` 接近 `11.2 km/s`。
4. `Earth_Model::Local_Escape_Speed(r)` 在地球外满足 `sqrt(2GM/r)` 的量级和趋势。
5. `Total_DM_Scattering_Rate` 在 `r > R_earth` 为 0。
6. `Number_Density_Electron` 在 `r > R_earth` 为 0。
7. 散射率插值域覆盖 `0 <= r <= R_earth`。
8. `Initial_Conditions` 的最大撞击参数随 `R_earth` 缩放，而不是继续使用 `rSun`。
9. `TrajectoryBincount` 的 bin 最大半径和 bin 宽度按地球配置计算，而不是仍使用 `2 R_sun`。
10. `Hyperbolic_Kepler_Shift` 对地球外轨道使用 `M_earth`。

太阳兼容至少检查：

1. 旧 `config/smoke.cfg` 仍能运行。
2. 显式 `body = "Sun"` 的配置仍能运行。
3. 轨迹 TXT 列格式不变。
4. README 中旧太阳运行命令仍有效。
5. `body` 缺失时输出目录规则不变。
6. 固定 `random_seed` 的 smoke 基线可以重复生成稳定统计。

## 4. 文档补充

README 需要包含：

- 项目当前支持的 body 列表。
- 太阳 smoke 命令。
- 地球 smoke 命令。
- `body`、`initial_radius_body_radius`、`asymptotic_distance_body_radius`、`body_model_file` 等字段说明。
- 旧字段 `initial_radius_rsun` 的兼容和 deprecated 说明。
- 输出目录中 body 前缀的说明。

新增 `DOC/earth_model_data_format.md`，建议包含：

- `earth_prem.dat` 文件位置。
- 每列字段、单位和合法范围。
- 数据来源或简化假设。
- 第一版组成模型的层边界和质量分数。
- 哪些内容只适合 smoke，哪些内容可用于正式物理结果。

新增或更新 smoke 记录文档，建议包含：

- 构建命令。
- 运行命令。
- 运行日期。
- Git commit 或工作区状态摘要。
- body、随机 seed、MPI rank 数。
- 轨迹文件数量。
- 轨迹首几行样例。
- 关键 sanity check 输出，例如地球表面逃逸速度。
- 已知误差或限制。

## 5. 建议验证命令

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/earth_smoke.cfg
find smoke_output -name 'trajectory_*.txt' | sort | head
head -5 smoke_output/earth/results_*/trajectory_1_task0.txt
```

如果新增了检查脚本或测试 target，应把命令补到 README 或本任务验收记录中。

建议新增一个明确的检查记录模板：

```text
Body:
Config:
Command:
Random seed:
MPI ranks:
Trajectory files:
Captured / Free / Reflected / Aborted:
Surface escape speed:
Mass at radius:
Known limitations:
```

## 6. 最终验收标准

- 太阳和地球 smoke 都能在 1 rank 下完成。
- README 可以让新用户分别跑通太阳和地球示例。
- 地球数据格式、单位和简化假设有明确文档。
- 地球基础物理量通过 sanity checks。
- 输出目录不会混淆不同 body。
- `earth_simulation_refactor_plan.md` 与子任务书链接一致。
- 每一项无法自动验证的检查都有人工记录、数值和判断结论。

## 7. 风险与注意事项

- 如果地球 smoke 可以运行但物理检查失败，不要把结果标记为完成；应回到子任务 03 或 04 修正。
- 如果本地依赖不完整导致无法运行 smoke，需要记录缺少的依赖和替代验证结果。
- 文档中要明确第一版地球组成的精度等级，避免把工程 smoke 模型误用为正式分析模型。
