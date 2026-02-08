# External Dependencies

This directory contains external repositories used by the EngEmil STM32 Bootloader project, managed as **Git submodules**.

- ChibiOS (RTOS + USB Stack + CMSIS). https://github.com/ChibiOS/ChibiOS
- Unity (Unit Testing Framework). https://github.com/ThrowTheSwitch/Unity
- CMock (Mocking Framework). https://github.com/ThrowTheSwitch/CMock


## Version Information

| Repository | Pinned Commit | Branch Tracking | Date Added |
|------------|---------------|-----------------|------------|
| ChibiOS | `7c19a48` | master | 2026-02-08 |
| ChibiOS_21.11.x | `0d4b08c` | stable_21.11.x | 2026-02-05 |
| Unity | `d1fe18b` | master | 2026-02-04 |
| CMock | `f368744` | master | 2026-02-04 |

**NB!** ChibiOS_21.11.x is used by bootloader, while the test-firmwares are using the newer version.


## Installation

These repositories are managed as **Git submodules**. 

### For New Submodules

```bash
# Add submodule
git submodule add <repository-url> ext/<name-submodule-folder>

# or, add submodule with specific branch
git submodule add -b <branch-name> <repository-url> ext/<name-submodule-folder>
```

### For Existing Clones

If you already cloned the repository without submodules, initialize and update them:

```bash
# Initialize all submodules (one-time setup)
git submodule init

# Update all submodules to their pinned commits
git submodule update
```

Or do both in one command:

```bash
git submodule update --init --recursive
```

### Updating Submodules

To update a specific submodule to the latest commit on its tracked branch:

```bash
# Update ChibiOS to latest on master branch
cd ext/ChibiOS
git checkout master
git pull
cd ../..
git add ext/ChibiOS
```

Or use the shorthand:

```bash
# Update ChibiOS submodule
git submodule update --remote ext/ChibiOS
git add ext/ChibiOS
```

To update all submodules at once:

```bash
git submodule update --remote
git add ext/
```

### Checking Submodule Status

```bash
# View current commit for each submodule
git submodule status

# View detailed information
git submodule foreach 'echo $name: $(git log -1 --oneline)'
```



## License Information

Each repository maintains its own license:
- **ChibiOS:** Apache 2.0 (includes CMSIS from ARM which is Apache 2.0)
- **Unity:** MIT License
- **CMock:** MIT License

See individual repository LICENSE files for details.

