# Evaluating LLM Limitations in Static Code Analysis

A Praktikum2 research project investigating whether large language models (LLMs) can correctly predict the runtime output of intentionally complex C programs. The programs are designed using techniques drawn from cryptography and computational theory to resist static analysis, and are evaluated against multiple LLMs under three prompt conditions.

## Overview

Modern LLMs are increasingly used to read, review, and explain source code. This Praktikum2 research project tests a narrower and harder question: given a deliberately complex C program, can an LLM determine exactly what it prints without executing it? Five C programs were written, each built around a distinct complexity technique, and evaluated across four model configurations using three prompts of increasing specificity.

The core finding is that model capability on this task is a threshold rather than a gradient — a model either can maintain exact integer state across chains of data-dependent operations, or it cannot, and prompt engineering alone does not close that gap.

## Praktikum 2 Paper Report

The full write-up of this praktikum2 research project is available as a PDF report:

[Praktikum 2 Paper Report (PDF)](./Praktikum2_Report.pdf)

## Repository Structure

```
praktikum-2/
├── README.md
├── Praktikum2_Report.pdf     Full paper report                 
├── ask_groq.sh               Evaluation script (submits C files to the Groq API)
├── prompts.txt               The three prompts used for manual and the prompt used for API evaluation
├── groq_results.txt          Logged output from Groq API runs
├── c_source/                 The five C test programs
│   ├── c01_fptr_dispatch.c       Function pointer dispatch
│   ├── c02_mutual_recursion.c    Mutual recursion
│   ├── c03_lfsr.c                Linear feedback shift register
│   ├── c04_carry_chain.c         Multi-word carry chain arithmetic
│   └── c05_state_machines.c      Lockstep state machines
└── c_bins/                   Compiled binaries
    ├── c01_fptr_dispatch
    ├── c02_mutual_recursion
    ├── c03_lfsr
    ├── c04_carry_chain
    └── c05_state_machines
```

## The Test Programs

Each program takes four seed values and prints a one-byte result for each. All use unsigned 32-bit wrapping arithmetic and were compiled with `gcc -O0 -std=c99`.

| File | Technique | Why it resists analysis |
|------|-----------|-------------------------|
| c01 | Function pointer dispatch | The function called at each of 12 steps is chosen by an index derived from the current state, so the execution path is entirely data-dependent |
| c02 | Mutual recursion | Two functions call each other with different termination conditions on the lower 4 bits, so recursion depth is not knowable in advance |
| c03 | Linear feedback shift register | Bit taps and shifts produce a pseudo-random sequence that is deterministic but has no analytical shortcut |
| c04 | Carry chain arithmetic | Carries propagate across four 32-bit words through two addition passes, cascading through the whole 128-bit value |
| c05 | Lockstep state machines | Two state variables each depend on both variables at every step, so neither can be traced in isolation |

### Program Outputs

```
c01: dispatch(0011)=0x1A  dispatch(00AB)=0xFA  dispatch(0200)=0xBD  dispatch(FFFF)=0x8D
c02: run(0033)=0xF5       run(00CC)=0x40       run(01FF)=0x10       run(BEEF)=0x37
c03: chain(ACE1)=0x1E     chain(1234)=0x34     chain(DEAD)=0x52     chain(0001)=0xFE
c04: crunch(0042)=0x03    crunch(1337)=0x81    crunch(ABCD)=0xB4    crunch(FF00)=0x12
c05: lockstep(0001)=0x12  lockstep(0080)=0xBA  lockstep(1234)=0x21  lockstep(FFFF)=0x30
```

## The Evaluation Script

`ask_groq.sh` submits a C source file to the Groq API and records the model's predicted output. Before submission it preprocesses the code to make analysis harder: it strips all comments, shuffles the order of top-level functions, and prepends a noise preamble describing the compiler environment. The prompt asks only for the exact stdout, with no reasoning or explanation.

### Requirements

- `bash`, `curl`, `python3`
- A Groq API key (free at https://console.groq.com)

### Setup

```bash
export GROQ_API_KEY="gsk_..."
chmod +x ask_groq.sh
```

### Usage

```bash
# Single file with the default model
./ask_groq.sh c_source/c01_fptr_dispatch.c

# Choose a specific model
./ask_groq.sh -m qwen/qwen3-32b c_source/c01_fptr_dispatch.c

# Run all five programs
./ask_groq.sh c_source/*.c

# List available models
./ask_groq.sh --list-models

# Show help
./ask_groq.sh --help
```

Results are appended to `groq_results.txt` with a timestamp and model name for each run.

### Options

| Flag | Description |
|------|-------------|
| `-m`, `--model MODEL` | Set the Groq model (default: `llama-3.3-70b-versatile`) |
| `-l`, `--list-models` | List available models and exit |
| `-h`, `--help` | Show help and exit |

## The Prompts

Three prompts of increasing specificity are defined in `prompts.txt`:

1. **Baseline** — simply asks what the program prints.
2. **Structured trace** — instructs the model to trace every function call step by step and prohibits guessing.
3. **Full environment constraint** — specifies the exact compilation environment and requires the model to declare uncertainty rather than guess.

## Reproducing the Results

```bash
# 1. Compile a program to get its ground truth output
gcc c_source/c01_fptr_dispatch.c -o c_bins/c01_fptr_dispatch -std=c99
./c_bins/c01_fptr_dispatch

# 2. Run the same program through an LLM via the script
./ask_groq.sh c_source/c01_fptr_dispatch.c

# 3. Compare the LLM's predicted output against the ground truth
```

For the manual evaluations (ChatGPT, Gemini, Qwen web interfaces), each C file and prompt were submitted through each model's GUI in a fresh private chat session, with a new session opened per file and per prompt to prevent context carryover.

## Key Findings

- **A clear capability split.** Two models achieved perfect scores across all 15 trials; one failed every trial; one landed in between with partial successes on specific programs.
- **Thinking time does not predict accuracy.** The model that reasoned longest was consistently among the least accurate, while faster models were correct.
- **Prompt quality helps capable models only.** Better prompts improved response structure but did not rescue models below the capability threshold.
- **Program-specific difficulty.** Some techniques (sequential bit manipulation) were easier for certain models than others (data-dependent dispatch, cross-dependent state).

## License

This Praktikum2 research project was developed for academic coursework (Praktikum 2) at the University of Vienna, supervised by Prof. Dipl.-Ing. Dr. Sebastian Schrittwieser
