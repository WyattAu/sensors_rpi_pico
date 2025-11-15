#!/usr/bin/env python3
"""
Apache 2.0, Wyatt Au

Modified build script from my omniCppController.py, should be able to complete the following:
1. Parses CMakePresets.json to detect available configurations
2. Allows preset selection via command line arguments
3. Runs full build pipeline: CMake configure → Build → Package
4. Supports both release and debug builds with appropriate packaging
5. Can be called from VS Code tasks.json and launch.json

Usage:
      python build.py [--preset PRESET] [--clean] [--clean-all] [--package-only] [--run-tests] [--run-coverage] [--verbose]

Arguments:
     --preset PRESET    CMake preset to use (default: release)
     --clean           Clean build directory before building
     --package-only    Only create packages from existing build
     --run-tests       Run unit tests after building
     --run-coverage    Run code coverage analysis (implies --run-tests)
     --verbose         Enable verbose logging
     --help           Show this help message
"""

import argparse
import json
import logging
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import time
import urllib.request
import zipfile
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple


class BuildScript:
    """Main build script class for Raspberry Pi Pico sensors project."""

    def __init__(self, verbose: bool = False):
        self.verbose = verbose
        self.project_root = Path(__file__).parent.resolve()
        self.cmake_presets_file = self.project_root / "CMakePresets.json"
        self.build_dir: Optional[Path] = None
        self.preset: Optional[str] = None

        # Setup logging
        self._setup_logging()

    def _setup_logging(self) -> None:
        """Setup logging configuration."""
        level = logging.DEBUG if self.verbose else logging.INFO
        logging.basicConfig(
            level=level,
            format='%(asctime)s - %(levelname)s - %(message)s',
            datefmt='%Y-%m-%d %H:%M:%S'
        )
        self.logger = logging.getLogger(__name__)

    def download_file(self, url: str, dest_path: Path, expected_hash: Optional[str] = None) -> bool:
        """Download a file with optional hash verification."""
        try:
            self.logger.info(f"Downloading {url} to {dest_path}")
            urllib.request.urlretrieve(url, dest_path)

            if expected_hash:
                import hashlib
                sha256 = hashlib.sha256()
                with open(dest_path, 'rb') as f:
                    for chunk in iter(lambda: f.read(4096), b""):
                        sha256.update(chunk)
                actual_hash = sha256.hexdigest()
                if actual_hash != expected_hash:
                    self.logger.error(f"Hash mismatch for {dest_path}. Expected: {expected_hash}, Got: {actual_hash}")
                    return False

            return True
        except Exception as e:
            self.logger.error(f"Failed to download {url}: {e}")
            return False

    def extract_archive(self, archive_path: Path, extract_dir: Path) -> bool:
        """Extract an archive file."""
        try:
            self.logger.info(f"Extracting {archive_path} to {extract_dir}")
            if archive_path.suffix == '.zip':
                with zipfile.ZipFile(archive_path, 'r') as zip_ref:
                    zip_ref.extractall(extract_dir)
            elif archive_path.suffix in ['.tar', '.gz', '.xz']:
                with tarfile.open(archive_path, 'r:*') as tar_ref:
                    tar_ref.extractall(extract_dir)
            else:
                self.logger.error(f"Unsupported archive format: {archive_path}")
                return False
            return True
        except Exception as e:
            self.logger.error(f"Failed to extract {archive_path}: {e}")
            return False

    def find_msys2_ucrt64_clang(self) -> Optional[Path]:
        """Find Clang in MSYS2 UCRT64 environment."""
        if platform.system() != "Windows":
            return None

        # Try common MSYS2 UCRT64 paths
        possible_paths = [
            Path("C:/msys64/ucrt64/bin/clang.exe"),
            Path("C:/msys2/ucrt64/bin/clang.exe"),
        ]
        for clang_exe in possible_paths:
            if clang_exe.exists():
                return clang_exe.parent.parent  # return ucrt64 dir

        # Try running which in MSYS2 UCRT64 bash
        try:
            bash_exe = Path("C:/msys64/usr/bin/bash.exe")
            if bash_exe.exists():
                env = os.environ.copy()
                env['MSYSTEM'] = 'UCRT64'
                env['PATH'] = f"C:/msys64/ucrt64/bin;C:/msys64/usr/bin;{env.get('PATH', '')}"
                cmd = [str(bash_exe), '-c', 'which clang.exe 2>/dev/null']
                result = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=10)
                if result.returncode == 0:
                    clang_path_str = result.stdout.strip()
                    if clang_path_str:
                        clang_path = Path(clang_path_str)
                        if clang_path.exists():
                            return clang_path.parent.parent
        except Exception as e:
            self.logger.debug(f"Failed to check MSYS2 UCRT64 clang: {e}")

        return None

    def setup_gcc_toolchain(self, build_dir: Path) -> Optional[Path]:
        """Setup ARM GCC toolchain for Windows - download if not found."""
        if platform.system() != "Windows":
            return None

        toolchain_name = "arm-gnu-toolchain"
        toolchain_dir = build_dir / "toolchain" / toolchain_name
        gcc_exe_path = toolchain_dir / "bin" / "arm-none-eabi-gcc.exe"

        if gcc_exe_path.exists():
            self.logger.info(f"Downloaded {toolchain_name} toolchain already exists")
            return toolchain_dir

        toolchain_dir.mkdir(parents=True, exist_ok=True)

        # ARM GNU Toolchain for Windows
        toolchain_version = "14.3.rel1"
        toolchain_url = f"https://armkeil.blob.core.windows.net/developer/Files/downloads/gnu/{toolchain_version}/binrel/arm-gnu-toolchain-{toolchain_version}-mingw-w64-i686-arm-none-eabi.zip"
        toolchain_archive = toolchain_dir / "arm-gnu-toolchain.zip"

        self.logger.info(f"Downloading ARM GNU Toolchain {toolchain_version}...")
        if not self.download_file(toolchain_url, toolchain_archive):
            return None

        if not self.extract_archive(toolchain_archive, toolchain_dir):
            return None

        # The toolchain may extract to a directory like "arm-gnu-toolchain-14.3.rel1-mingw-w64-i686-arm-none-eabi"
        # or directly to the toolchain_dir
        extracted_dirs = list(toolchain_dir.glob("arm-gnu-toolchain-*"))

        if extracted_dirs:
            extracted_dir = extracted_dirs[0]
            # Move contents to toolchain/arm-gnu-toolchain
            for item in extracted_dir.iterdir():
                shutil.move(str(item), str(toolchain_dir / item.name))
            extracted_dir.rmdir()

        if not gcc_exe_path.exists():
            self.logger.error(f"ARM GCC executable not found at {gcc_exe_path} after extraction")
            return None

        self.logger.info(f"ARM GCC toolchain installed at {toolchain_dir}")
        return toolchain_dir

    def setup_clang_toolchain(self, build_dir: Path, use_embedded_toolchain: bool = False) -> Optional[Path]:
        """Setup Clang toolchain - use embedded if requested, else check MSYS2, then PATH, download if not found."""
        if platform.system() != "Windows":
            return None

        if use_embedded_toolchain:
            # Use LLVM Embedded Toolchain for Arm (includes llvm-objcopy and ARM runtime libraries)
            toolchain_name = "embedded-clang"
            self.logger.info(f"Using LLVM Embedded Toolchain for Arm...")
            toolchain_dir = build_dir / "toolchain" / toolchain_name
            clang_exe_path = toolchain_dir / "bin" / "clang.exe"

            if clang_exe_path.exists():
                self.logger.info(f"Downloaded {toolchain_name} toolchain already exists")
                return toolchain_dir

            toolchain_dir.mkdir(parents=True, exist_ok=True)

            embedded_version = "19.1.5"
            embedded_url = f"https://github.com/ARM-software/LLVM-embedded-toolchain-for-Arm/releases/download/release-{embedded_version}/LLVM-ET-Arm-{embedded_version}-Windows-x86_64.zip"
            embedded_archive = toolchain_dir / "llvm-embedded.zip"

            self.logger.info(f"Downloading LLVM Embedded Toolchain for Arm {embedded_version}...")
            if not self.download_file(embedded_url, embedded_archive):
                return None

            if not self.extract_archive(embedded_archive, toolchain_dir):
                return None

            # The embedded toolchain extracts to a directory like "LLVMEmbeddedToolchainForArm-18.1.3" or "LLVM-ET-Arm-19.1.5-Windows-x86_64"
            extracted_dirs = list(toolchain_dir.glob("LLVMEmbeddedToolchainForArm-*")) + list(toolchain_dir.glob("LLVM-ET-Arm-*"))
            if not extracted_dirs:
                self.logger.error("Failed to find extracted LLVM Embedded Toolchain directory")
                return None

            embedded_dir = extracted_dirs[0]
            # Move contents to toolchain/embedded-clang
            for item in embedded_dir.iterdir():
                shutil.move(str(item), str(toolchain_dir / item.name))
            embedded_dir.rmdir()

            if not clang_exe_path.exists():
                self.logger.error(f"Clang executable not found at {clang_exe_path}")
                return None

            self.logger.info(f"Clang toolchain installed at {toolchain_dir}")
            return toolchain_dir
        else:
            # First check if Clang is available in MSYS2 UCRT64
            clang_dir = self.find_msys2_ucrt64_clang()
            if clang_dir:
                # Check if objcopy (GNU binutils) is available (preferred for MSYS2)
                objcopy = clang_dir / "bin" / "objcopy.exe"
                if objcopy.exists():
                    self.logger.info(f"Found MSYS2 UCRT64 Clang with objcopy at {clang_dir}")
                    return clang_dir
                else:
                    self.logger.info(f"Found MSYS2 UCRT64 Clang at {clang_dir} but missing objcopy")

            # Fallback: check if Clang is already available in PATH
            clang_exe = shutil.which("clang.exe")
            if clang_exe:
                clang_dir = Path(clang_exe).parent.parent  # Get the installation directory
                # Check if objcopy is available
                objcopy = clang_dir / "bin" / "objcopy.exe"
                if objcopy.exists():
                    self.logger.info(f"Found existing Clang with objcopy in PATH at {clang_dir}")
                    return clang_dir
                else:
                    self.logger.info(f"Found existing Clang in PATH at {clang_dir} but missing objcopy")

            # If not found, download and setup standard LLVM
            toolchain_name = "clang"
            self.logger.info(f"Clang not found in MSYS2 UCRT64 or PATH, downloading {toolchain_name}...")
            toolchain_dir = build_dir / "toolchain" / toolchain_name
            clang_exe_path = toolchain_dir / "bin" / "clang.exe"

            if clang_exe_path.exists():
                self.logger.info(f"Downloaded {toolchain_name} toolchain already exists")
                return toolchain_dir

            toolchain_dir.mkdir(parents=True, exist_ok=True)

            # Use standard LLVM release for Windows
            clang_version = "18.1.8"
            clang_url = f"https://github.com/llvm/llvm-project/releases/download/llvmorg-{clang_version}/clang+llvm-{clang_version}-x86_64-pc-windows-msvc.tar.xz"
            clang_archive = toolchain_dir / "clang.tar.xz"

            self.logger.info(f"Downloading LLVM/Clang {clang_version}...")
            if not self.download_file(clang_url, clang_archive):
                return None

            if not self.extract_archive(clang_archive, toolchain_dir):
                return None

            # Find the extracted directory
            extracted_dirs = list(toolchain_dir.glob("clang+llvm-*"))
            if not extracted_dirs:
                self.logger.error("Failed to find extracted Clang directory")
                return None

            clang_dir = extracted_dirs[0]
            # Move contents to toolchain/clang
            for item in clang_dir.iterdir():
                item.rename(toolchain_dir / item.name)
            clang_dir.rmdir()

            if not clang_exe_path.exists():
                self.logger.error(f"Clang executable not found at {clang_exe_path}")
                return None

            self.logger.info(f"Clang toolchain installed at {toolchain_dir}")
            return toolchain_dir

    def setup_ninja_toolchain(self, build_dir: Path) -> Optional[Path]:
        """Setup Ninja build tool for MSYS2 - check PATH first, download if not found."""
        if platform.system() != "Windows":
            return None

        # First check if Ninja is already available in PATH
        ninja_exe = shutil.which("ninja.exe")
        if ninja_exe:
            self.logger.info(f"Found existing Ninja at {ninja_exe}")
            return Path(ninja_exe)

        # If not found, download and setup
        self.logger.info("Ninja not found in PATH, downloading...")
        toolchain_dir = build_dir / "toolchain" / "ninja"
        ninja_exe_path = toolchain_dir / "ninja.exe"

        if ninja_exe_path.exists():
            self.logger.info("Downloaded Ninja toolchain already exists")
            return ninja_exe_path

        toolchain_dir.mkdir(parents=True, exist_ok=True)

        # Ninja release for Windows
        ninja_version = "1.12.1"
        ninja_url = f"https://github.com/ninja-build/ninja/releases/download/v{ninja_version}/ninja-win.zip"
        ninja_archive = toolchain_dir / "ninja.zip"

        self.logger.info(f"Downloading Ninja {ninja_version}...")
        if not self.download_file(ninja_url, ninja_archive):
            return None

        if not self.extract_archive(ninja_archive, toolchain_dir):
            return None

        if not ninja_exe_path.exists():
            self.logger.error(f"Ninja executable not found at {ninja_exe_path}")
            return None

        self.logger.info(f"Ninja toolchain installed at {ninja_exe_path}")
        return ninja_exe_path

    def parse_cmake_presets(self) -> Dict[str, Any]:
        """Parse CMakePresets.json and return available presets."""
        if not self.cmake_presets_file.exists():
            raise FileNotFoundError(f"CMakePresets.json not found at {self.cmake_presets_file}")

        with open(self.cmake_presets_file, 'r') as f:
            presets_data = json.load(f)

        configure_presets = {}
        build_presets = {}

        # Parse configure presets
        for preset in presets_data.get('configurePresets', []):
            name = preset['name']
            configure_presets[name] = {
                'name': name,
                'displayName': preset.get('displayName', name),
                'description': preset.get('description', ''),
                'binaryDir': preset.get('binaryDir', f'${{sourceDir}}/build/{name}'),
                'inherits': preset.get('inherits', []),
                'cacheVariables': preset.get('cacheVariables', {}),
                'condition': preset.get('condition'),
                'environment': preset.get('environment', {})
            }

        # Parse build presets
        for preset in presets_data.get('buildPresets', []):
            name = preset['name']
            configure_preset = preset.get('configurePreset')
            if configure_preset and configure_preset in configure_presets:
                build_presets[name] = {
                    'name': name,
                    'configurePreset': configure_preset,
                    'configureData': configure_presets[configure_preset]
                }

        return {
            'configure': configure_presets,
            'build': build_presets
        }

    def _create_toolchain_file(self, configure_preset: str, build_dir: Path) -> None:
        """Create toolchain file for the given preset."""
        self.logger.info(f"Creating toolchain file for preset '{configure_preset}'")
        # Ensure build directory exists
        build_dir.mkdir(parents=True, exist_ok=True)

        if configure_preset == "msys2-clang":
            # Use full LLVM Embedded Toolchain
            clang_dir = build_dir / "toolchain" / "embedded-clang"
            if not clang_dir.exists() or not (clang_dir / "bin" / "clang.exe").exists():
                self.logger.error("LLVM Embedded Toolchain not found for msys2-clang")
                return

            toolchain_file = build_dir / "toolchain.cmake"
            clang_dir_str = str(clang_dir).replace('\\', '/')
            self.logger.info("Using full LLVM Embedded Toolchain for msys2-clang")

            toolchain_content = f"""
set(CMAKE_SYSTEM_NAME PICO)
set(CMAKE_SYSTEM_PROCESSOR cortex-m33)
set(CMAKE_C_COMPILER "{clang_dir_str}/bin/clang.exe")
set(CMAKE_CXX_COMPILER "{clang_dir_str}/bin/clang++.exe")
set(CMAKE_ASM_COMPILER "{clang_dir_str}/bin/clang.exe")
set(CMAKE_AR "{clang_dir_str}/bin/llvm-ar.exe")
set(CMAKE_OBJCOPY "{clang_dir_str}/bin/llvm-objcopy.exe")
set(CMAKE_OBJDUMP "{clang_dir_str}/bin/llvm-objdump.exe")
set(CMAKE_READELF "{clang_dir_str}/bin/llvm-readelf.exe")
set(CMAKE_C_COMPILER_TARGET arm-none-eabi)
set(CMAKE_CXX_COMPILER_TARGET arm-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET arm-none-eabi)
set(CMAKE_SYSROOT "{clang_dir_str}/lib/clang-runtimes/arm-none-eabi/armv8m.main_soft_nofp")
set(CMAKE_C_FLAGS "${{CMAKE_C_FLAGS}} --target=arm-none-eabi -mcpu=cortex-m33 -mthumb -mfloat-abi=soft -Wno-override-module -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS "${{CMAKE_CXX_FLAGS}} --target=arm-none-eabi -mcpu=cortex-m33 -mthumb -mfloat-abi=soft -Wno-override-module -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "${{CMAKE_ASM_FLAGS}} --target=arm-none-eabi -mcpu=cortex-m33 -mthumb -mfloat-abi=soft")
set(CMAKE_EXE_LINKER_FLAGS "${{CMAKE_EXE_LINKER_FLAGS}} -Wl,--build-id=none -nostdlib")
"""
            toolchain_file.write_text(toolchain_content)
            self.logger.info(f"Created toolchain file: {toolchain_file}")

        elif configure_preset == "msys2-gcc":
            # Setup gcc toolchain
            gcc_dir = self.setup_gcc_toolchain(build_dir)

            if gcc_dir:
                toolchain_file = build_dir / "toolchain.cmake"
                gcc_dir_str = str(gcc_dir).replace('\\', '/')
                toolchain_content = f"""
set(CMAKE_SYSTEM_NAME PICO)
set(CMAKE_SYSTEM_PROCESSOR cortex-m33)
set(CMAKE_C_COMPILER "{gcc_dir_str}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "{gcc_dir_str}/bin/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "{gcc_dir_str}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_AR "{gcc_dir_str}/bin/arm-none-eabi-ar.exe")
set(CMAKE_OBJCOPY "{gcc_dir_str}/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_C_COMPILER_TARGET arm-none-eabi)
set(CMAKE_CXX_COMPILER_TARGET arm-none-eabi)
set(CMAKE_ASM_COMPILER_TARGET arm-none-eabi)
set(CMAKE_C_FLAGS "-mcpu=cortex-m33 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m33 -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections -fno-stack-protector -fno-stack-clash-protection -fcf-protection=none -fno-PIE -fno-PIC -U_FORTIFY_SOURCE")
set(CMAKE_C_FLAGS_RELEASE "-O2 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -g")
set(CMAKE_ASM_FLAGS "-mcpu=cortex-m33 -mthumb")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--build-id=none")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
"""
                toolchain_file.write_text(toolchain_content)
                self.logger.info(f"Created toolchain file: {toolchain_file}")

    def get_available_presets(self) -> List[str]:
        """Get list of available build preset names."""
        presets = self.parse_cmake_presets()
        return list(presets['build'].keys())

    def validate_preset(self, preset: str) -> bool:
        """Validate if the given preset exists."""
        available = self.get_available_presets()
        return preset in available

    def resolve_binary_dir(self, preset: str) -> Path:
        """Resolve the binary directory for a given preset."""
        presets = self.parse_cmake_presets()
        if preset not in presets['build']:
            raise ValueError(f"Build preset '{preset}' not found")

        configure_preset = presets['build'][preset]['configureData']
        binary_dir = configure_preset['binaryDir']

        # Replace ${sourceDir} with actual path
        binary_dir = binary_dir.replace('${sourceDir}', str(self.project_root))

        return Path(binary_dir)

    def run_command(self, cmd: List[str], cwd: Optional[Path] = None,
                   env: Optional[Dict[str, str]] = None) -> Tuple[int, str, str]:
        """Run a command and return (returncode, stdout, stderr)."""
        self.logger.debug(f"Running command: {' '.join(cmd)}")
        if cwd:
            self.logger.debug(f"Working directory: {cwd}")

        try:
            result = subprocess.run(
                cmd,
                cwd=cwd or self.project_root,
                env=env,
                capture_output=True,
                text=True,
                check=False
            )
            return result.returncode, result.stdout, result.stderr
        except Exception as e:
            self.logger.error(f"Failed to run command: {e}")
            return -1, "", str(e)


    def get_configure_preset(self, build_preset: str) -> str:
        """Get the configure preset name for a given build preset."""
        presets = self.parse_cmake_presets()
        if build_preset not in presets['build']:
            raise ValueError(f"Build preset '{build_preset}' not found")
        return presets['build'][build_preset]['configurePreset']

    def cmake_configure(self, preset: str) -> bool:
        """Run CMake configure step."""
        configure_preset = self.get_configure_preset(preset)
        self.logger.info(f"Running CMake configure with preset '{configure_preset}'...")

        # Setup toolchains for MSYS2 builds
        env = None
        ninja_exe = None
        build_dir = self.resolve_binary_dir(preset)

        if configure_preset == "msys2-clang":
            self.logger.info("Setting up full LLVM Embedded Toolchain for Arm...")

            # Always use LLVM Embedded Toolchain for msys2-clang (no fallback to MSYS2 clang)
            clang_dir = self.setup_clang_toolchain(build_dir, use_embedded_toolchain=True)
            if not clang_dir:
                self.logger.error("Failed to setup LLVM Embedded Toolchain for MSYS2 Clang")
                return False

            ninja_exe = self.setup_ninja_toolchain(build_dir)

            if not ninja_exe:
                self.logger.error("Failed to setup Ninja for MSYS2 Clang")
                return False

            # Create toolchain file
            self._create_toolchain_file(configure_preset, build_dir)

            # For configure, don't override PATH to allow host compiler detection for tools like picotool
            # The toolchain.cmake will set the cross-compilers
            env = os.environ.copy()
            env['MSYSTEM'] = 'MINGW64'
            env['PICO_CLANG_RUNTIMES'] = 'arm-none-eabi'

        elif configure_preset == "msys2-gcc":
            self.logger.info("Setting up MSYS2 GCC toolchain with ARM GNU Toolchain...")

            gcc_dir = self.setup_gcc_toolchain(build_dir)
            ninja_exe = self.setup_ninja_toolchain(build_dir)

            if not gcc_dir or not ninja_exe:
                self.logger.error("Failed to setup required toolchains for MSYS2 GCC")
                return False

            # Create toolchain file
            self._create_toolchain_file(configure_preset, build_dir)

            # Set environment with updated PATH for MSYS2 MINGW64
            env = os.environ.copy()
            env['PATH'] = f"{gcc_dir / 'bin'};{Path(ninja_exe).parent};C:/msys64/mingw64/bin;C:/msys64/usr/bin;{env.get('PATH', '')}"
            env['MSYSTEM'] = 'MINGW64'

            self.logger.info(f"Updated PATH for MSYS2 GCC: {env['PATH']}")


        cmd = ['cmake', '--preset', configure_preset]
        if (configure_preset == "msys2-clang" or configure_preset == "msys2-gcc") and ninja_exe:
            # Ensure CMAKE_MAKE_PROGRAM is set for Ninja
            cmd.extend(['-DCMAKE_MAKE_PROGRAM=' + str(ninja_exe)])
        returncode, _, stderr = self.run_command(cmd, env=env)

        if returncode != 0:
            self.logger.error(f"CMake configure failed with return code {returncode}")
            if stderr:
                self.logger.error(f"CMake stderr: {stderr}")
            return False

        self.logger.info("CMake configure completed successfully")
        return True

    def cmake_build(self, preset: str) -> bool:
        """Run CMake build step."""
        self.logger.info(f"Running CMake build with preset '{preset}'...")

        # Setup toolchains for MSYS2 builds
        env = None
        configure_preset = self.get_configure_preset(preset)
        build_dir = self.resolve_binary_dir(preset)

        if configure_preset == "msys2-clang":
            # Use full LLVM Embedded Toolchain
            clang_dir = self.setup_clang_toolchain(build_dir, use_embedded_toolchain=True)
            if not clang_dir:
                self.logger.error("Embedded toolchain not available for MSYS2 Clang build")
                return False

            ninja_exe = self.setup_ninja_toolchain(build_dir)

            if clang_dir and ninja_exe:
                # Set PATH to prioritize LLVM Embedded Toolchain's ARM-aware tools
                embedded_toolchain_bin = clang_dir / "bin"
                env = os.environ.copy()
                env['PATH'] = f"{embedded_toolchain_bin};{env.get('PATH', '')}"
                env['MSYSTEM'] = 'MINGW64'
                env['PICO_CLANG_RUNTIMES'] = 'arm-none-eabi'
                self.logger.info(f"Updated PATH for full LLVM Embedded Toolchain build")

        elif configure_preset == "msys2-gcc":
            gcc_dir = self.setup_gcc_toolchain(build_dir)
            ninja_exe = self.setup_ninja_toolchain(build_dir)

            if gcc_dir and ninja_exe:
                env = os.environ.copy()
                env['PATH'] = f"{gcc_dir / 'bin'};{Path(ninja_exe).parent};C:/msys64/mingw64/bin;C:/msys64/usr/bin;{env.get('PATH', '')}"
                env['MSYSTEM'] = 'MINGW64'

        cmd = ['cmake', '--build', '--preset', preset]
        returncode, _, stderr = self.run_command(cmd, env=env)

        if returncode != 0:
            self.logger.error(f"CMake build failed with return code {returncode}")
            if stderr:
                self.logger.error(f"CMake stderr: {stderr}")
            return False

        self.logger.info("CMake build completed successfully")
        return True

    def clean_build_directory(self, preset: str) -> bool:
        """Clean the build directory for the given preset."""
        self.logger.info(f"Cleaning build directory for preset '{preset}'...")

        try:
            build_dir = self.resolve_binary_dir(preset)
            if build_dir.exists():
                shutil.rmtree(build_dir)
                self.logger.info(f"Removed build directory: {build_dir}")
            return True
        except Exception as e:
            self.logger.error(f"Failed to clean build directory: {e}")
            return False

    def clean_all_build_directories(self) -> bool:
        """Clean all build directories."""
        self.logger.info("Cleaning all build directories...")

        try:
            build_root = self.project_root / "build"
            if build_root.exists():
                shutil.rmtree(build_root, ignore_errors=True)
                self.logger.info(f"Removed all build directories: {build_root}")
            return True
        except Exception as e:
            self.logger.error(f"Failed to clean all build directories: {e}")
            return False

    def find_build_outputs(self, build_dir: Path) -> List[Path]:
        """Find build output files (elf, bin, hex, uf2)."""
        patterns = ['*.elf', '*.bin', '*.hex', '*.uf2']
        return [output for pattern in patterns for output in build_dir.glob(pattern)]

    def create_package(self, preset: str) -> bool:
        """Create tarball package from build outputs."""
        self.logger.info(f"Creating package for preset '{preset}'...")

        try:
            build_dir = self.resolve_binary_dir(preset)
            if not build_dir.exists():
                self.logger.error(f"Build directory does not exist: {build_dir}")
                return False

            outputs = self.find_build_outputs(build_dir)
            if not outputs:
                self.logger.warning(f"No build outputs found in {build_dir}")
                return True  # Not an error, just no outputs to package

            # Determine package name based on preset and build type
            presets = self.parse_cmake_presets()
            configure_data = presets['build'][preset]['configureData']
            build_type = configure_data['cacheVariables'].get('CMAKE_BUILD_TYPE', 'Release').lower()

            timestamp = time.strftime("%Y%m%d_%H%M%S")
            package_name = f"sensors_rpi_pico_{preset}_{build_type}_{timestamp}"
            package_dir = self.project_root / "packages"
            package_dir.mkdir(exist_ok=True)
            tarball_path = package_dir / f"{package_name}.tar.gz"

            self.logger.info(f"Creating package: {tarball_path}")

            with tarfile.open(tarball_path, "w:gz") as tar:
                # Add build outputs
                for output in outputs:
                    arcname = f"{package_name}/{output.name}"
                    tar.add(output, arcname=arcname)
                    self.logger.debug(f"Added to package: {output.name}")

                # Add version info if available
                version_file = build_dir / "version.txt"
                if version_file.exists():
                    tar.add(version_file, arcname=f"{package_name}/version.txt")

            self.logger.info(f"Package created successfully: {tarball_path}")
            return True

        except Exception as e:
            self.logger.error(f"Failed to create package: {e}")
            return False

    def cmake_test(self, preset: str) -> bool:
        """Run CMake test step."""
        self.logger.info(f"Running CMake tests with preset '{preset}'...")

        cmd = ['cmake', '--build', '--preset', preset, '--target', 'test']
        returncode, _, stderr = self.run_command(cmd)

        if returncode != 0:
            self.logger.error(f"CMake tests failed with return code {returncode}")
            if stderr:
                self.logger.error(f"CMake test stderr: {stderr}")
            return False

        self.logger.info("CMake tests completed successfully")
        return True

    def run_coverage(self, preset: str) -> bool:
        """Run code coverage analysis."""
        self.logger.info(f"Running code coverage analysis for preset '{preset}'...")

        build_dir = self.resolve_binary_dir(preset)
        coverage_cmd = self._get_coverage_command(build_dir)

        if coverage_cmd:
            returncode, _, stderr = self.run_command(coverage_cmd, cwd=build_dir)
            if returncode != 0:
                self.logger.error(f"Coverage analysis failed with return code {returncode}")
                if stderr:
                    self.logger.error(f"Coverage stderr: {stderr}")
                return False
            self.logger.info("Coverage analysis completed successfully")
            return True
        else:
            self.logger.warning("Coverage tool not available, skipping coverage analysis")
            return True

    def _get_coverage_command(self, build_dir: Path) -> Optional[List[str]]:
        """Get the appropriate coverage command for the current platform."""
        # Check for OpenCppCoverage on Windows
        opencppcoverage = shutil.which('OpenCppCoverage.exe')
        if opencppcoverage:
            test_exe = build_dir / 'sensors_tests.exe'
            if test_exe.exists():
                return [
                    opencppcoverage,
                    '--sources', str(self.project_root),
                    '--excluded_sources', str(self.project_root / 'tests'),
                    '--excluded_sources', str(self.project_root / 'build'),
                    '--excluded_sources', str(self.project_root / 'lib'),
                    '--', str(test_exe), '--gtest_output=xml:coverage.xml'
                ]

        # Check for gcov/lcov on Linux
        gcov = shutil.which('gcov')
        lcov = shutil.which('lcov')
        if gcov and lcov:
            return [
                'lcov', '--capture', '--directory', str(build_dir),
                '--output-file', 'coverage.info', '--test-name', 'sensors_tests'
            ]

        return None

    def build(self, preset: str, clean: bool = False, package_only: bool = False,
             run_tests: bool = False, run_coverage: bool = False) -> bool:
        """Run the complete build pipeline."""
        self.logger.info(f"Starting build pipeline for preset '{preset}'")

        if not self.validate_preset(preset):
            available = self.get_available_presets()
            self.logger.error(f"Invalid preset '{preset}'. Available presets: {', '.join(available)}")
            return False

        self.preset = preset
        self.build_dir = self.resolve_binary_dir(preset)

        success = True

        try:
            if clean:
                if not self.clean_build_directory(preset):
                    return False

            if not package_only:
                # CMake configure
                if not self.cmake_configure(preset):
                    success = False

                # CMake build
                if success and not self.cmake_build(preset):
                    success = False

                # Run tests if requested
                if success and run_tests and not self.cmake_test(preset):
                    success = False

                # Run coverage if requested
                if success and run_coverage and not self.run_coverage(preset):
                    success = False

            # Package
            if success and not self.create_package(preset):
                success = False

        except Exception as e:
            self.logger.error(f"Build pipeline failed: {e}")
            success = False

        if success:
            self.logger.info(f"Build pipeline completed successfully for preset '{preset}'")
        else:
            self.logger.error(f"Build pipeline failed for preset '{preset}'")

        return success

    def list_presets(self) -> None:
        """List all available presets."""
        try:
            presets = self.parse_cmake_presets()
            self.logger.info("Available build presets:")
            self.logger.info("-" * 50)

            for name, data in presets['build'].items():
                configure_data = data['configureData']
                display_name = configure_data.get('displayName', name)
                description = configure_data.get('description', '')
                build_type = configure_data['cacheVariables'].get('CMAKE_BUILD_TYPE', 'Unknown')

                self.logger.info(f"  {name}")
                self.logger.info(f"    Display Name: {display_name}")
                self.logger.info(f"    Description: {description}")
                self.logger.info(f"    Build Type: {build_type}")
                self.logger.info("")

        except Exception as e:
            self.logger.error(f"Failed to list presets: {e}")


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Build script for Raspberry Pi Pico sensors project",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )

    parser.add_argument(
        '--preset',
        default='release',
        help='CMake preset to use (default: release)'
    )


    parser.add_argument(
        '--clean',
        action='store_true',
        help='Clean build directory before building'
    )

    parser.add_argument(
        '--clean-all',
        action='store_true',
        help='Clean all build directories'
    )

    parser.add_argument(
        '--package-only',
        action='store_true',
        help='Only create packages from existing build'
    )

    parser.add_argument(
        '--run-tests',
        action='store_true',
        help='Run unit tests after building'
    )

    parser.add_argument(
        '--run-coverage',
        action='store_true',
        help='Run code coverage analysis (implies --run-tests)'
    )

    parser.add_argument(
        '--list-presets',
        action='store_true',
        help='List available presets and exit'
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='Enable verbose logging'
    )

    sys.argv = [arg for arg in sys.argv if arg]

    args = parser.parse_args()

    # Create build script instance
    script = BuildScript(verbose=args.verbose)

    # Clean all build directories if requested
    if args.clean_all:
        success = script.clean_all_build_directories()
        return 0 if success else 1

    # List presets if requested
    if args.list_presets:
        script.list_presets()
        return 0

    # Run build
    success = script.build(
        preset=args.preset,
        clean=args.clean,
        package_only=args.package_only,
        run_tests=args.run_tests or args.run_coverage,
        run_coverage=args.run_coverage
    )

    return 0 if success else 1


if __name__ == '__main__':
    sys.exit(main())