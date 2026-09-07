# T-400a post-fix target build 검증 기록

- 기준선: base `22222902e6ba9d06c3b0a8b2c9f4cb980c4e03f6`
- 원 candidate: `5821fcab613b2a544df4cde14281b898fac10388`
- 이전 post-fix review 식별자: temporary commit `85188da7321a656b6dc1a1c34cf6829fa6d41196`, tree `af8a92de71ac08161af3638e8d518f604bde1079`
- 주의: 이 alternate object는 이전 `.git` ACL 차단 중에만 사용한 임시 기준선이다. 아래 ESP32 build는 같은 post-fix working tree에서 새로 실행했으며, 실제 branch commit·push·CI 결과는 아직 아니다.

## 도구

- CMake `4.4.3`: `F:/dev/canview/.tools/cmake-4.4.3/.../cmake.exe`
- Ninja `1.13.2`: `F:/dev/canview/.tools/ninja-1.13.2/bin/ninja.exe`
- Arm GNU `15.3.1`: `C:/Users/digit/AppData/Local/CANView/toolchains/arm-gnu-toolchain-15.3.rel1/bin`
- STM32CubeG4 checkout: `C:/cv/STM32CubeG4-1.6.3`
- ESP-IDF `v6.0.3`: `C:/cv/esp-idf-6.0.3`

## STM32 target

실제 source와 pinned Arm/CMake/Ninja/Cube 경로를 명시해 다음 Debug/Release configure/build를 실행했다.

```text
cmake -S firmware/communicator/stm32 -B build/t400a-postfix-stm32-debug-2 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake -DSTM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3 -DCMAKE_MAKE_PROGRAM=<pinned ninja> -DCMAKE_C_COMPILER=<verified arm gcc> -DCMAKE_ASM_COMPILER=<verified arm gcc> -DCMAKE_OBJCOPY=<verified objcopy> -DCMAKE_SIZE=<verified size>
cmake --build build/t400a-postfix-stm32-debug-2 --parallel 2
cmake -S firmware/communicator/stm32 -B build/t400a-postfix-stm32-release-2 -G Ninja -DCMAKE_BUILD_TYPE=Release ...
cmake --build build/t400a-postfix-stm32-release-2 --parallel 2
```

두 build 모두 configure/build exit 0, compiler/linker/CMake warning 출력 0, ELF/HEX/BIN/MAP 및 map/stack checker PASS다.

| 구성 | text/data/bss | 산출물 |
|---|---:|---|
| Debug | `4296/4/8580` bytes | `build/t400a-postfix-stm32-debug-2/canview-communicator-stm32.{elf,hex,bin,map}` |
| Release | `3796/4/8580` bytes | `build/t400a-postfix-stm32-release-2/canview-communicator-stm32.{elf,hex,bin,map}` |

현재 산출물 SHA-256:

```text
18592ca77f5d374e8c3a29dac7d35d7c7352ab707569d63cc4f7cf787fecdc95  build/t400a-postfix-stm32-debug-2/canview-communicator-stm32.bin
583b6770d8c43a7849c030468bdd2558a34e47903502c846250957b312dd048c  build/t400a-postfix-stm32-debug-2/canview-communicator-stm32.elf
22ee31f7e3050f1c8e48c6f6764906ad99cd2c8fcb4d8a22c1bbb128a61b7fd8  build/t400a-postfix-stm32-debug-2/canview-communicator-stm32.hex
3c45131d23f79bdd455c3f4c2c3578a53e5f0845ec125152cbceeed34810ca21  build/t400a-postfix-stm32-debug-2/canview-communicator-stm32.map
17a6673aa65629ca51e873f012de080a5adb36c73a063c09540fbdd0053f87a6  build/t400a-postfix-stm32-release-2/canview-communicator-stm32.bin
f8d0946731091f80a7daf05b14e9634eb1849aa925ef62b726df14d6ba2f203f  build/t400a-postfix-stm32-release-2/canview-communicator-stm32.elf
820c3f11eac0c9020ad445d4f2a64188820e04bc4e89481528582e268c17e5fa  build/t400a-postfix-stm32-release-2/canview-communicator-stm32.hex
ef8d70454ef63d3660d433c36edbc935e589cbb8984849f9508593591834b4e9  build/t400a-postfix-stm32-release-2/canview-communicator-stm32.map
```

## ESP32 target

이전 compiler error 5는 해당 세션의 ACL/실행 환경 문제였고, 현재 Windows PowerShell에서 실제 IDF Python과 Xtensa compiler를 다시 확인한 뒤 post-fix working tree를 새 build directory에 build했다.

```text
. C:/cv/esp-idf-6.0.3/export.ps1
idf.py -C firmware/diagnostic-bridge -B F:/dev/canview/build/t400a-postfix-bridge-actual build
idf.py -C firmware/communicator/esp32 -B F:/dev/canview/build/t400a-postfix-communicator-esp32-actual build
idf.py -C firmware/controller -B F:/dev/canview/build/t400a-postfix-controller-actual build
python -B tools/check_sdkconfig.py firmware/communicator/esp32/sdkconfig --board comm-r2-n16r8
python -B tools/check_sdkconfig.py firmware/diagnostic-bridge/sdkconfig --board bridge-r1-n8r2
```

- ESP-IDF `v6.0.3`, IDF Python `3.14.3`, `xtensa-esp-elf-gcc 15.2.0`에서 세 build 모두 exit 0으로 완료됐다.
- Communicator와 Bridge의 실제 `sdkconfig` contract도 각각 PASS했다. Controller는 이 task에서 변경된 generated board header를 포함해 image를 생성했다.
- compiler/linker/CMake `warning:` 진단은 없었다. Bridge·Communicator·Controller bootloader configure 중 `CONFIG_ESP_INT_WDT_TIMEOUT_MS=800`의 Kconfig default `300`과의 차이를 알리는 notification은 출력됐으나, 명시 설정을 유지한다는 정보이며 compiler/linker/CMake warning은 아니다.
- `setup-windows.ps1 -VerifyOnly -ToolRoot C:/cv`의 managed Git shell `basename`/`sed`/`git-sh-setup` 실패는 남아 있다. 이 bootstrap 검증 실패는 별도 toolchain gate로 기록하며, 위 세 IDF target build 성공으로 대체하지 않는다.

| 대상 | BIN 크기 | ELF/MAP | SHA-256 (BIN / ELF / MAP) |
|---|---:|---|---|
| Diagnostic Bridge | `159,728` bytes | 생성됨 | `cddcea6502c6cb533ce2638ca5310b739766b06ab15846b3bdf9da48558bbe07` / `26b41d225eb90fcfd1506a233e9d48b948a7f8e6f04495111ca78a9ee2a34554` / `c4ee0c3c6149e99a0b2c0948dd6926748cc52904f48fb527a9c9d8d765937a55` |
| Communicator ESP32 | `164,176` bytes | 생성됨 | `5c428140d33d0165d04b4b6826df94594ccef4acfd7a96f1fef4ebdab95264a0` / `93b423ba835df20c6d181df82958451a18012ba17fc1c0a26290c14e6800f57c` / `9015e776fad6e3806f3d6c0dafc6e5d59011fab1805d9f57dc422a6c9706b752` |
| Controller | `159,712` bytes | 생성됨 | `5250a4ddb97eab33f6f34877cc100f6ae8f5621c437344e3b677bda363c463ae` / `6ec4bccf39f4e4ba30fb2bea5cd3c9606e309cc80185174867d493dde15830be` / `b81237a098fe7b7b83d2030d6d76bade9979d4f0024feff39d8b6382e25fe202` |

세 target 모두 app BIN/ELF/MAP와 bootloader BIN을 새로 만들었다. 실제 board flash/HIL 성공을 뜻하지 않으며, 보존된 pre-fix artifact를 재사용하지 않았다.

## Immutable branch candidate `5d6fac4` 재현 build

이 절은 PR branch에 push된 immutable candidate `5d6fac46287886529f9b4eb27c487ed07b7c70d4`에서 새 build directory로 재실행한 결과다. 위의 temporary alternate object 및 working-tree 산출물과 동일시하지 않는다.

```text
. C:/cv/esp-idf-6.0.3/export.ps1
. ./tools/environment/foundation-windows.ps1 -IncludeDocs
cmake -S . -B build/t400a-final-host-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/t400a-final-host-debug --parallel 4
ctest --test-dir build/t400a-final-host-debug --output-on-failure -E "^uart-fault-stream$" -j 4
cmake -S . -B build/t400a-final-host-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/t400a-final-host-release --parallel 4
ctest --test-dir build/t400a-final-host-release --output-on-failure -E "^uart-fault-stream$" -j 4

cmake -S firmware/communicator/stm32 -B build/t400a-5d6fac-stm32-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=F:/dev/canview/firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake -DSTM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3 -DCMAKE_MAKE_PROGRAM=<pinned ninja>
cmake --build build/t400a-5d6fac-stm32-debug --parallel 2
cmake -S firmware/communicator/stm32 -B build/t400a-5d6fac-stm32-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=F:/dev/canview/firmware/communicator/stm32/cmake/arm-none-eabi-gcc.cmake -DSTM32CUBE_G4_ROOT=C:/cv/STM32CubeG4-1.6.3 -DCMAKE_MAKE_PROGRAM=<pinned ninja>
cmake --build build/t400a-5d6fac-stm32-release --parallel 2

idf.py -C firmware/diagnostic-bridge -B F:/dev/canview/build/t400a-5d6fac-bridge build
idf.py -C firmware/communicator/esp32 -B F:/dev/canview/build/t400a-5d6fac-communicator-esp32 build
idf.py -C firmware/controller -B F:/dev/canview/build/t400a-5d6fac-controller build
```

- Host Debug/Release는 각각 `esp32*` 33/33, 전체(의도적으로 별도 장시간 fault stream을 제외) 106/106 CTest를 통과했다. `uart-fault-stream`은 86,400초 duration fixture이므로 이 기록의 fresh full-suite 수치에 포함하지 않았다.
- STM32 Debug/Release와 ESP-IDF `v6.0.3` Bridge/Communicator/Controller build는 모두 exit 0이다. 각 target은 ELF/MAP/BIN(ESP32는 bootloader BIN 포함)을 새로 생성했고 compiler/linker/CMake `warning:`은 0건이다.
- Bridge의 `CONFIG_SPIRAM_ECC_ENABLE=n` visibility notification은 board의 Quad PSRAM/ECC off 계약에 따른 Kconfig 정보이며 compiler/linker/CMake warning이 아니다. Bridge/Communicator의 `CONFIG_ESP_INT_WDT_TIMEOUT_MS=800` notification도 의도적으로 설정한 watchdog policy를 알리는 Kconfig 정보다.

| 대상 | 주요 산출물 SHA-256 (BIN / ELF / MAP) |
|---|---|
| STM32 Debug | `18592ca77f5d374e8c3a29dac7d35d7c7352ab707569d63cc4f7cf787fecdc95` / `f26443065f2796841bc7d93eca449cbc577d93bde692821d7bd77b26e37d7d16` / `5ee758ed791860f96d756258fedf8977cc29645d9b9a09eaa3b2dd753c72ad5e` |
| STM32 Release | `17a6673aa65629ca51e873f012de080a5adb36c73a063c09540fbdd0053f87a6` / `f8d0946731091f80a7daf05b14e9634eb1849aa925ef62b726df14d6ba2f203f` / `ef8d70454ef63d3660d433c36edbc935e589cbb8984849f9508593591834b4e9` |
| Diagnostic Bridge | `3991bf669215eb18e5eb4aa717ad6e8fc1e28d95c627d4ccaed110b7895e0b9b` / `68d41cc3e677c9d6faf07b068e6204153a2b251a7dc1d65dcbdf93455b1fd6b9` / `9f9b9d49776277c6969e7fa3160183ae0c2ce28ec6daaaca9a5d0166287e56ea` |
| Communicator ESP32 | `a820e0a91f3b4db701e9996f2dbf9c12a64afa518315faaafe22135551140cf5` / `c97cf98c360ce094c63685cb16727fa114d09d27ebde993fe86a95236bbe6267` / `ef4d1f66bb6d09e93139f95a1a035205e9bcdf43a42ffb091527e0e4305baf66` |
| Controller | `fd1229bd68586f1be731ca9d96aa5ffadee12b390bad247242c8aca6fe5d7aff` / `cdf38be106afef17a134427a650780637804fabe28cdd244ec3e5ede4058771c` / `5893f79d20be990ff46185bb638e62ac0dff904bf16a9c3e109487b1c67ab0ee` |

이 local immutable-candidate evidence는 CI의 clean checkout artifact hash/evidence gate를 대체하지 않으며, documentation-only 후속 commit의 app version/산출물과도 동일하다고 주장하지 않는다.

public-component fixture는 `build/t400a-5d6fac-public-component`에서 추가로 `fullclean`, `set-target esp32s3`, `build` exit 0과 warning scan 0을 확인했다. app BIN/ELF/MAP SHA-256은 각각 `26201bf36f5d51102c1257fd7675e5d6a2570e8ea16498c258485110acdfacd3`, `baa21b414f53566683b72a5064b2202fed52c394e3149f6ac00d2727baedae7f`, `902322a04cd360184e40ff9a99ee083c44de3468f089dc313e7e011e92346e58`이다. 단, 이 실행은 위 evidence Markdown을 편집한 dirty worktree에서 시작돼 IDF app version이 `5d6fac4-dirty`로 기록됐다. 따라서 이는 component integration 보조 증거일 뿐 clean immutable `5d6fac4` artifact evidence로 사용하지 않으며, 해당 clean target gate는 CI run `34112160182`의 결과로 별도 확인한다.

## physical gate

board flash, ST-LINK/USB serial, reset/brownout/power rail, PSRAM/ECC/clock/watchdog 장시간 측정, 차량 CAN/capture, security provisioning, TX release는 모두 `NOT_RUN`이다. host Bridge integration fixture는 SDK API model일 뿐 실제 보드/HIL이 아니다.
