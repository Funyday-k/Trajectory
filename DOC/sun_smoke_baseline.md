# 太阳 Smoke 基线记录

## 1. 基线范围

本记录对应子任务 01：太阳基线与兼容护栏。目标是在新增 `body` 默认值和可选 `random_seed` 后，确认旧太阳 smoke 路径仍可运行，并记录一个可复现的最小输出基线。

## 2. 运行环境

- 日期：2026-05-31
- 分支：`earth-task-01-baseline-guardrails`
- 可执行文件：`build/DaMaSCUS-SUN-TrajectoryTXT`
- 配置文件：`config/smoke.cfg`
- MPI rank 数：1
- body：缺省配置，程序按 `Sun` 处理
- seed：`random_seed = 12345`，实际规则为 `random_seed + mpi_rank`

## 3. 构建与运行命令

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DBoost_NO_BOOST_CMAKE=ON \
  -DBoost_NO_SYSTEM_PATHS=ON \
  -DBOOST_ROOT=$HOME/opt/boost_1_77_0 \
  -DBoost_INCLUDE_DIR=$HOME/opt/boost_1_77_0
cmake --build build --target DaMaSCUS-SUN-TrajectoryTXT --config Release -j4
mpirun -np 1 ./build/DaMaSCUS-SUN-TrajectoryTXT config/smoke.cfg
```

## 4. 关键日志

```text
DaMaSCUS-SUN-TrajectoryTXT 0.1.0
Trajectory TXT runner
MPI processes: 1
Body: Sun
Output mode: trajectory txt files only
PRNG seed: random_seed + mpi_rank, base random_seed = 12345
Trajectory directory: ./smoke_output/results_-0.301030_-39.000000
Trajectory files: 1
Captured: 0
Free: 1
Reflected: 0
Aborted: 0
Text rows written: 9
RK45 steps: 83
EARLY STOP: max_trajectories reached
```

## 5. 输出产物

- 轨迹目录：`smoke_output/results_-0.301030_-39.000000`
- 轨迹文件数：1
- 轨迹文件：`trajectory_1_task0.txt`
- 文件总行数：10
- 轨迹文件 MD5：`7ce9aae522dca681d9d9c95611bd685f`

前 5 行：

```text
# columns: time_s x_km y_km z_km vx_km_s vy_km_s vz_km_s E_eV
0.0000000000e+00        2.6599388430e+05        1.3416790529e+06        -2.5522251583e+05       -1.2551065882e+02       -4.9870526841e+02       1.0306315114e+022.3454752304e+02
1.1196703547e+03        1.1423674345e+05        7.2300493701e+05        -1.2858062735e+05       -1.4969716723e+02       -6.3455345792e+02       1.2806017596e+022.3454405109e+02
1.7850094203e+03        2.8751820813e+03        2.0478472304e+05        -2.7348813072e+04       -1.8959567718e+02       -1.0725678468e+03       1.9694005830e+022.3453989111e+02
1.9144005989e+03        -2.0761059706e+04       4.8126251051e+04        1.6821208406e+02        -1.6058310052e+02       -1.3493630947e+03       2.2378386707e+022.3454349529e+02
```

后 5 行：

```text
1.9417964322e+03        -2.4858346326e+04       1.0678614856e+04        6.2681568068e+03        -1.3726471123e+02       -1.3788798720e+03       2.2042395217e+022.3455350571e+02
1.9601739927e+03        -2.7210174384e+04       -1.4676777595e+04       1.0268404194e+04        -1.1847448896e+02       -1.3774734988e+03       2.1446103449e+022.3453551730e+02
1.9951415154e+03        -3.0730303956e+04       -6.2187053477e+04       1.7491164409e+04        -8.3734701154e+01       -1.3325505919e+03       1.9796731073e+022.3454524484e+02
2.1956597200e+03        -3.6503624073e+04       -2.8929436501e+05       4.8616380176e+04        1.6781552608e+00        -9.6584901967e+02       1.2429977814e+022.3454563661e+02
3.2795448641e+03        -1.1772918963e+04       -1.0451307359e+06       1.3868378864e+05        2.7718627928e+01        -5.7529321328e+02       6.5816631629e+012.3454906019e+02
```

## 6. 兼容分支检查

- `config/smoke.cfg` 不写 `body`：通过，程序日志显示 `Body: Sun`。
- 临时配置显式加入 `body = "Sun";`：通过，summary 与默认 smoke 一致。
- 临时配置设置 `body = "Earth";`：失败符合预期，输出 `Unsupported body: Earth`，退出码为 1。
- 同一 `random_seed` 连续运行两次：通过，轨迹文件 MD5 均为 `7ce9aae522dca681d9d9c95611bd685f`。

## 7. 已知说明

- 构建过程中出现第三方依赖和既有代码警告，未见任务 01 引入的编译错误。
- 本任务不改变输出目录规则，不改变轨迹 TXT 列定义，不引入地球模型。