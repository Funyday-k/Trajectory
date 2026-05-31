# 子任务 03：Earth_Model 与地球数据

## 1. 目标

新增一个实现 `Celestial_Model` 的 `Earth_Model`，提供地球半径、质量、径向密度/温度/包围质量、核素组成、核数密度、电子数密度和散射率接口。完成后，地球模型应能被构造并参与编译，但是否接入主程序运行由子任务 04 完成。

## 2. 范围

建议涉及文件：

- `vendor/damascus/include/Earth_Model.hpp`
- `vendor/damascus/src/Earth_Model.cpp`
- `vendor/damascus/data/earth_prem.dat`
- `CMakeLists.txt`
- 可选新增：`DOC/earth_model_data_format.md`

不在本任务中做：

- 不改主程序 body factory。
- 不新增 `config/earth_smoke.cfg` 的运行入口。
- 不改输出目录结构。
- 不进行大规模物理精度优化；第一版重点是工程可运行和量级正确。

## 3. 数据文件要求

建议新增：

```text
vendor/damascus/data/earth_prem.dat
```

最低字段：

```text
r_km  enclosed_mass_kg  density_g_cm3  temperature_K
```

字段约束：

- `r_km` 单调递增，覆盖 `0 <= r <= 6371`。
- `enclosed_mass_kg` 在中心接近 0，在表面接近 `5.9722e24 kg`。
- `density_g_cm3` 非负。
- `temperature_K` 必须为正，避免热速度采样出现除零或非法数值。
- 文件应允许注释行，并在文件头部或配套文档中标明数据来源、生成方式和适用范围。

如果暂时使用简化表，文件头部要写明它是工程 smoke 数据，不可作为正式物理结果依据。

## 4. 元素组成第一版

第一版建议使用分层常数丰度，至少覆盖：

- O
- Mg
- Si
- Fe
- Ni
- S
- Ca
- Al

推荐实现方式：

1. 在 `Earth_Model` 中按半径划分地核、地幔、地壳。
2. 每层提供元素质量分数。
3. 由质量密度和元素质量分数计算核数密度。
4. 由核数密度乘以原子序数 `Z` 汇总电子数密度。

第一版可以使用天然元素的主同位素或与 Obscura/DaMaSCUS 现有 isotope 构造方式一致的同位素列表；关键是接口和单位正确。

组成模型必须同时记录：

- 每一层的半径范围。
- 每个元素质量分数的归一化方式。
- 使用天然同位素丰度还是单一代表同位素。
- 该组成是否仅用于 smoke。

## 5. 实施步骤

1. 新增 `Earth_Model.hpp`，声明继承 `Celestial_Model` 的地球模型类。
2. 定义地球常量：平均半径 `6371 km`、总质量 `5.9722e24 kg`。
3. 读取 `earth_prem.dat`，建立半径、包围质量、密度、温度的插值表。
4. 实现 `Radius()`、`Total_Mass()`、`Mass(r)`、`Mass_Density(r)`、`Temperature(r)`。
5. 实现 `Local_Escape_Speed(r)`：地球外部应满足 `sqrt(2 G M_earth / r)`；地球内部建议沿用太阳模型的势能积分思路，使用 `Mass(x) / x^2` 从 `r` 积到 `R_earth`，再加表面势能项，避免只用局部包围质量近似导致逃逸速度偏低。
6. 实现 `Number_Density_Nucleus(r, index)` 和 `Number_Density_Electron(r)`，在 `r > Radius()` 时返回 0。
7. 复用 `Solar_Model` 中散射率计算和插值结构，但不要复制太阳专属数据路径或太阳专属常量。
8. 更新 CMake，把 `Earth_Model.cpp` 加入 `damascus_trajectory_core`。
9. 新增或预留一个轻量检查入口，能打印 `Radius`、`Total_Mass`、表面逃逸速度、中心/表面密度和电子数密度，用于子任务 05 自动化记录。

## 6. 物理 sanity checks

本任务至少要能人工或小程序检查：

- `Earth_Model::Radius()` 返回约 `6371 km`。
- `Earth_Model::Total_Mass()` 返回约 `5.9722e24 kg`。
- `Earth_Model::Mass(0)` 接近 0。
- `Earth_Model::Mass(R_earth)` 接近 `M_earth`。
- `Earth_Model::Local_Escape_Speed(R_earth)` 约为 `11.2 km/s`。
- `Number_Density_Electron(r)` 在地球内部为正，在地球外部为 0。
- `Total_DM_Scattering_Rate` 在 `r > R_earth` 为 0。

## 7. 验收标准

- 项目能成功编译，且 `Earth_Model.cpp` 参与构建。
- `Earth_Model` 不依赖太阳数据文件 `model_agss09.dat`。
- 地球径向表缺失时给出明确错误。
- 所有半径、质量、密度、温度单位在代码和数据文档中一致。
- 地球外部数密度和散射率为 0。
- `Earth_Model` 的数据来源和精度等级有文档记录；简化数据不能被描述为正式 PREM 结果。

## 8. 建议验证命令

```bash
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
```

如果已经有测试 target，可以新增或复用一个只构造 `Earth_Model` 的窄测试。没有测试框架时，可以在子任务 05 中补正式检查，本任务至少保证编译和基础人工检查。

## 9. 风险与注意事项

- 地球组成如果使用 bulk Earth 常数丰度，只能作为工程 smoke 版本；正式物理结果需要更严格的分层或径向组成表。
- 温度不能设为 0。
- `enclosed_mass_kg` 如果由密度积分生成，要确认积分单位从 `g/cm^3`、`km` 到 `kg` 的换算没有遗漏。
- 不要让 `Earth_Model` 直接修改太阳模型行为；两个模型应并行存在。

## 10. 完成情况

完成日期：2026-05-31

执行分支：`earth-branch`

已完成内容：

- 新增 `vendor/damascus/include/Earth_Model.hpp` 和 `vendor/damascus/src/Earth_Model.cpp`。
- `Earth_Model` 已继承 `Celestial_Model`，实现半径、总质量、包围质量、密度、温度、逃逸速度、数密度、散射率和插值接口。
- 新增 `vendor/damascus/data/earth_prem.dat`，作为第一版工程 smoke 用简化 PREM-like 径向表。
- 新增三层常数组成模型：地核、地幔、地壳；目标元素覆盖 O、Mg、Si、Fe、Ni、S、Ca、Al，并使用代表同位素 O-16、Mg-24、Si-28、Fe-56、Ni-58、S-32、Ca-40、Al-27。
- 地球外部 `Number_Density_Electron`、`Number_Density_Nucleus` 和 `Total_DM_Scattering_Rate` 返回 0。
- `CMakeLists.txt` 已把 `Earth_Model.cpp` 加入 `damascus_trajectory_core`，并检查 `earth_prem.dat` 是否存在。
- 新增轻量检查 target：`DaMaSCUS-EarthModelCheck`。
- 新增 `DOC/earth_model_data_format.md`，说明地球数据字段、单位、简化假设和 sanity checks。

验证结果：

- `cmake -S . -B build ...` 通过。
- `cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT DaMaSCUS-EarthModelCheck --config Release -j4` 通过。
- `./build/DaMaSCUS-EarthModelCheck` 通过，输出摘要：`radius_km = 6371`，`total_mass_kg = 5.9722e+24`，`mass_surface_kg = 5.9722e+24`，`surface_escape_speed_km_s = 11.186`，`electron_density_outside = 0`，`targets = 8`。
- 太阳 smoke 回归仍通过，确认新增 Earth 模型没有破坏现有太阳运行路径。

遗留说明：

- `earth_prem.dat` 是 smoke 级别简化数据，不能作为正式物理结果使用。
- `Earth_Model` 已参与编译并可独立构造，但尚未接入 `src/main.cpp` 的 `body = "Earth"` 运行路径；该内容留给子任务 04。
