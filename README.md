# NYU OS Labs (C / Linux)

A compact overview of four systems-programming labs written in **C** and designed to run in a minimal Linux container. Each lab focuses on a core OS concept (strings & memory, shells & processes, concurrency, and filesystems).

---

## What’s here

* **Lab 1 — `nyuc`**: C warm-up. Safe string manipulation, heap allocation, and variadic APIs.
* **Lab 2 — `nyush`**: A small Unix-like shell with pipelines, redirection, signals, and job control.
* **Lab 3 — `nyuenc`**: Run-length encoder (RLE). Sequential + parallel (thread pool with POSIX threads).
* **Lab 4 — `nyufile`**: FAT32 undelete utility. Recovers files (incl. non-contiguous) with SHA-1 verification.

All code is **portable C (GNU17)**, built with **gcc 12** inside a Docker image that mirrors the grading environment.

---

## Quick start

```bash
# 1) Start the container (replace PATH_TO_LABS)
docker run -it --rm --name 2250 \
  -v "$PWD":/2250 -w /2250 ytang/os bash

# 2) Build a lab (each lab has its own Makefile)
cd lab1-nyuc    && make   # produces ./nyuc
cd ../lab2-nyush && make  # produces ./nyush
cd ../lab3-nyuenc && make # produces ./nyuenc  (links pthread)
cd ../lab4-nyufile && make # produces ./nyufile (links -lcrypto)

# 3) Run a few smoke tests
./lab1-nyuc/nyuc Hello, world
./lab2-nyush/nyush         # interactive shell
./lab3-nyuenc/nyuenc file.txt > out.enc
./lab4-nyufile/nyufile fat32.disk -i
```

---

## Repo layout

```
.
├─ lab1-nyuc/        # arg manipulation in C (malloc + variadic free)
│  ├─ Makefile
│  ├─ nyuc.c         # provided main (unchanged)
│  ├─ argmanip.h     # provided interface (unchanged)
│  └─ argmanip.c     # implemented
├─ lab2-nyush/       # tiny shell
│  ├─ Makefile
│  ├─ *.c *.h        # parser, executor, builtins, jobs, signals
├─ lab3-nyuenc/      # RLE encoder (sequential + threads)
│  ├─ Makefile       # links with -pthread
│  ├─ *.c *.h        # thread pool, chunking, stitching
├─ lab4-nyufile/     # FAT32 undelete tool
│  ├─ Makefile       # links with -lcrypto (OpenSSL SHA-1)
│  ├─ *.c *.h        # mmap, on-disk structs, recovery logic
└─ README.md
```

---

## Highlights by lab

### Lab 1 — `nyuc`: argv-style transformation

* **What it does:** prints each CLI arg in original, UPPER, lower.
* **Engineering focus:** careful heap layout that exactly mirrors `argv`:

  * One `malloc` for the `char*` vector (NULL-terminated),
  * One `malloc` per string + `manip()` (`toupper` / `tolower`),
  * Variadic `free_copied_args(args1, args2, ..., NULL)` that frees all lists safely.
* **Memory safety:** validated with Valgrind (0 leaks / 0 invalid accesses).

**Sample**

```bash
./nyuc Hello, world
[./nyuc] -> [./NYUC] [./nyuc]
[Hello,] -> [HELLO,] [hello,]
[world] -> [WORLD] [world]
```

---

### Lab 2 — `nyush`: a small shell

* **Prompt:** `[nyush <cwd-basename>]$ ` (stdout flushed immediately).
* **Parsing rules:** single-space separators; no spaces inside tokens; supports `<`, `>`, `>>`, and `|`.
* **Execution:**

  * Program search: absolute `/x`, relative `a/b`, or basename in `/usr/bin` (ignore `$PATH`).
  * Pipelines via `pipe()` + `dup2()`, redirection via `open()/creat()`.
  * Child processes use default signal handlers; the shell ignores `SIGINT`, `SIGQUIT`, `SIGTSTP`.
* **Built-ins:** `cd`, `jobs`, `fg <index>`, `exit` (with precise error texts).
* **Job control:** track stopped processes; display and resume by index; no zombies (uses `waitpid()`).

**Examples**

```bash
[nyush lab2]$ /usr/bin/ls -a -l
[nyush lab2]$ cat < in.txt | grep main | wc -l >> out.txt
[nyush lab2]$ jobs
[1] /usr/bin/top -c
[nyush lab2]$ fg 1
```

---

### Lab 3 — `nyuenc`: RLE encoder with a thread pool

* **Format:** ASCII byte followed by **1-byte count** (0–255). Multiple input files concatenate logically before encoding.
* **Sequential path:** single scan, zero copying of input.
* **Parallel path (`-j N`):**

  * Fixed **4KB chunking**,
  * A **thread pool** (mutex + condition variables) consumes tasks,
  * Results are **stitched** at chunk boundaries (e.g., `a5` + `a3` → `a8`).
  * Main thread **submits & collects concurrently** (no busy wait; no `sleep()`).
* **Correctness & concurrency checks:** Valgrind (Helgrind/DRD) → 0 errors; `strace` reveals proper futex-based synchronization and no excessive `clone()`.

**Examples**

```bash
echo -n "aaaaaabbbbbbbbba" > t.txt
./nyuenc t.txt > t.enc
xxd t.enc    # shows 61 06 62 09 61 01  => a6 b9 a1

time ./nyuenc -j 4 bigfile > /dev/null
```

---

### Lab 4 — `nyufile`: FAT32 undelete with SHA-1 disambiguation

* **Access pattern:** `mmap()` the disk image; operate on raw on-disk structs (boot sector / directory entries / FAT).
* **Operations:**

  * `-i`: print FS params (FAT count, bytes/sector, sectors/cluster, reserved sectors).
  * `-l`: list root directory (8.3 names only); shows size and starting cluster (or `/` for dirs).
  * `-r filename [-s sha1]`: recover **contiguous** deleted file; with optional SHA-1 for disambiguation.
  * `-R filename -s sha1`: recover **possibly non-contiguous** deleted file (bounded brute force over unallocated clusters).
* **Constraints honored:** update directory entry; do not rely on OS FS layer; handle empty files; detect multiple candidates; print exact status lines.

**Examples**

```bash
./nyufile fat32.disk -i
Number of FATs = 2
Number of bytes per sector = 512
Number of sectors per cluster = 1
Number of reserved sectors = 32

./nyufile fat32.disk -l
HELLO.TXT (size = 14, starting cluster = 3)
DIR/ (starting cluster = 4)
EMPTY (size = 0)
Total number of entries = 3

./nyufile fat32.disk -r TANG.TXT -s c91761a2cc1562d36585614c8c680ecf5712e875
TANG.TXT: successfully recovered with SHA-1
```

---

## Tooling & quality gates

* **Compilers/flags:** `-std=gnu17 -Wall -Wextra -Werror -pedantic -g`
* **Debugging:** `gdb` (core dumps, `bt`, `info locals`, `x/xb`), `strace`
* **Memory/thread checkers:**

  * `valgrind --leak-check=full --track-origins=yes ./nyuc ...`
  * `valgrind --tool=helgrind --read-var-info=yes ./nyuenc -j 4 ...`
  * `valgrind --tool=drd --read-var-info=yes ./nyuenc -j 4 ...`
* **Performance sanity:** `time`, `perf record` / `perf report`

---

## Design notes

* **Memory discipline (Lab 1):** argv-shape fidelity, explicit NULL terminators, variadic resource ownership and release.
* **Shell correctness (Lab 2):** strict parser/grammar; exact error strings; no `system()` or `/bin/sh`; signal isolation (shell vs. children); zombie avoidance.
* **Concurrency (Lab 3):** true thread pool (bounded `clone()` count), condition-variable scheduling (no busy-wait), deterministic stitching across chunk seams, work queue out-of-order completion with in-order writeback.
* **Filesystem (Lab 4):** byte-accurate on-disk structs (`#pragma pack`), safe `mmap` access, directory/FAT walking, ambiguity handling via SHA-1, recovery under contiguous and small bounded non-contiguous search.

---

## Minimal usage cheatsheet

```bash
# Lab 1
./lab1-nyuc/nyuc Hello, world

# Lab 2
./lab2-nyush/nyush

# Lab 3
./lab3-nyuenc/nyuenc file1 file2 > out.enc
./lab3-nyuenc/nyuenc -j 4 bigfile > /dev/null

# Lab 4
./lab4-nyufile/nyufile fat32.disk -i
./lab4-nyufile/nyufile fat32.disk -l
./lab4-nyufile/nyufile fat32.disk -r HELLO.TXT
./lab4-nyufile/nyufile fat32.disk -r TANG.TXT -s <sha1>
./lab4-nyufile/nyufile fat32.disk -R NAME.TXT -s <sha1>
```

---

## Notes

* Projects are designed to run inside the provided Docker image; building on other platforms may require adjusting toolchains.
* All prompts/error messages are intentionally exact (useful for automated grading and black-box testing).
