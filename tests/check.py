#!/usr/bin/env python3
import argparse
import subprocess
import sys
from pathlib import Path


def normalize(text: str) -> str:
    return text.replace("\r\n", "\n").rstrip()


def main():
    parser = argparse.ArgumentParser(
        description="Run a program on tests test{i}.txt and compare stdout with ans{i}.txt"
    )

    parser.add_argument("program", help="Path to program or command to run")
    parser.add_argument("n", type=int, help="Number of tests")

    parser.add_argument(
        "--tests-dir",
        default=".",
        help="Directory with test{i}.txt files"
    )

    parser.add_argument(
        "--ans-dir",
        default=".",
        help="Directory with ans{i}.txt files"
    )

    args = parser.parse_args()

    program = args.program
    tests_dir = Path(args.tests_dir)
    ans_dir = Path(args.ans_dir)

    passed = 0

    for i in range(1, args.n + 1):
        test_file = tests_dir / f"test{i}.txt"
        ans_file = ans_dir / f"ans{i}.txt"

        if not test_file.exists():
            print(f"[{i}] ERROR: файл теста не найден: {test_file}")
            continue

        if not ans_file.exists():
            print(f"[{i}] ERROR: файл ответа не найден: {ans_file}")
            continue

        try:
            result = subprocess.run(
                [program, str(test_file)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
        except FileNotFoundError:
            print(f"ERROR: программа не найдена: {program}")
            sys.exit(1)

        expected = ans_file.read_text(encoding="utf-8")
        actual = result.stdout

        if result.returncode != 0:
            print(f"[{i}] RUNTIME ERROR")
            print("stderr:")
            print(result.stderr)
            continue

        if normalize(actual) == normalize(expected):
            print(f"[{i}] OK")
            passed += 1
        else:
            print(f"[{i}] WRONG ANSWER")
            print("Expected:")
            print(expected)
            print("Actual:")
            print(actual)

    print()
    print(f"Passed {passed}/{args.n}")


if __name__ == "__main__":
    main()