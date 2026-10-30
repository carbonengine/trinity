# Copyright © 2026 CCP ehf.

import argparse
import ctypes
import fnmatch
import json
import os
import random
import sys
import time
import unittest
from xml.sax.saxutils import escape as xml_escape

GTEST_DESCRIPTION = (
    "Runs Python unittest tests and reports results using GoogleTest's "
    "command-line interface and output format."
)

GTEST_EPILOG = """\
Supported GoogleTest flags:
  --gtest_list_tests
  --gtest_filter=PATTERN
  --gtest_also_run_disabled_tests
  --gtest_repeat=NUMBER
  --gtest_shuffle
  --gtest_random_seed=NUMBER
  --gtest_color=(yes|no|auto)
  --gtest_print_time=0|1
  --gtest_output=(xml|json)[:PATH]
  --gtest_break_on_failure
  --gtest_throw_on_failure
  --gtest_fail_fast
  --gtest_brief=0|1

Extra flags (not part of the GoogleTest CLI, needed to locate the Python
tests since this is a generic wrapper rather than a fixed test binary):
  --test_path PATH       directory to search for tests (default: '.')
  --test_pattern GLOB    unittest discovery filename pattern (default: 'test*.py')
  test_names...          explicit dotted test names/modules to load instead
                          of discovery (e.g. "my_module.MyCase.test_foo")
"""


class Colorizer:
    def __init__(self, enabled):
        self.enabled = enabled

    def _wrap(self, code, text):
        if not self.enabled:
            return text
        return f"\033[0;{code}m{text}\033[0m"

    def green(self, text):
        return self._wrap(32, text)

    def red(self, text):
        return self._wrap(31, text)

    def yellow(self, text):
        return self._wrap(33, text)


def _enable_windows_ansi():
    if os.name != "nt":
        return
    try:
        kernel32 = ctypes.windll.kernel32
        handle = kernel32.GetStdHandle(-11)
        mode = ctypes.c_uint32()
        if kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
            kernel32.SetConsoleMode(handle, mode.value | 0x0004)
    except Exception:
        pass


def should_use_color(mode):
    if mode == "yes":
        return True
    if mode == "no":
        return False
    return sys.stdout.isatty() and os.environ.get("TERM") != "dumb"


def _plural(n):
    return "" if n == 1 else "s"


def build_arg_parser():
    parser = argparse.ArgumentParser(
        prog="gtest_reporter.py",
        description=GTEST_DESCRIPTION,
        epilog=GTEST_EPILOG,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--gtest_list_tests", action="store_true")
    parser.add_argument("--gtest_filter", default="*")
    parser.add_argument("--gtest_also_run_disabled_tests", action="store_true")
    parser.add_argument("--gtest_repeat", type=int, default=1)
    parser.add_argument("--gtest_shuffle", action="store_true")
    parser.add_argument("--gtest_random_seed", type=int, default=0)
    parser.add_argument(
        "--gtest_color", type=lambda s: s.lower(), choices=["yes", "no", "auto"], default="no"
    )
    parser.add_argument("--gtest_print_time", type=int, choices=[0, 1], default=1)
    parser.add_argument("--gtest_output", nargs="?", const="xml", default=None)
    parser.add_argument("--gtest_break_on_failure", type=int, choices=[0, 1], default=0)
    parser.add_argument("--gtest_throw_on_failure", type=int, choices=[0, 1], default=0)
    parser.add_argument("--gtest_catch_exceptions", type=int, choices=[0, 1], default=0)
    parser.add_argument("--gtest_fail_fast", type=int, choices=[0, 1], default=0)
    parser.add_argument("--gtest_brief", type=int, choices=[0, 1], default=0)
    parser.add_argument("--test_path", default=".")
    parser.add_argument("--test_pattern", default="test*.py")
    parser.add_argument("test_names", nargs="*")
    return parser


def discover_tests(args):
    loader = unittest.TestLoader()
    if args.test_names:
        return loader.loadTestsFromNames(args.test_names)
    return loader.discover(args.test_path, pattern=args.test_pattern)


def flatten(suite):
    tests = []
    for item in suite:
        if isinstance(item, unittest.TestSuite):
            tests.extend(flatten(item))
        else:
            tests.append(item)
    return tests


def is_disabled(test):
    suite_name = type(test).__name__
    method_name = getattr(test, "_testMethodName", "")
    return suite_name.startswith("DISABLED_") or method_name.startswith("DISABLED_")


def gtest_pattern_match(name, pattern):
    positive, sep, negative = pattern.partition("-")
    positive = positive or "*"
    positive_patterns = [p for p in positive.split(":") if p]
    negative_patterns = [p for p in negative.split(":") if p] if sep else []
    if not any(fnmatch.fnmatchcase(name, p) for p in positive_patterns):
        return False
    if any(fnmatch.fnmatchcase(name, p) for p in negative_patterns):
        return False
    return True


def group_by_suite(tests):
    groups = {}
    for test in tests:
        groups.setdefault(type(test).__name__, []).append(test)
    return groups


def print_test_list(tests, colorizer):
    groups = group_by_suite(tests)
    for suite_name, items in groups.items():
        print(f"{suite_name}.")
        for test in items:
            print(f"  {getattr(test, '_testMethodName', test.id())}")
    print()


class GTestResult(unittest.TestResult):
    def __init__(self, suite_name, colorizer, args, out_results):
        super().__init__()
        self.suite_name = suite_name
        self.colorizer = colorizer
        self.args = args
        self.out_results = out_results
        self._start_time = 0.0

    def _test_name(self, test):
        method_name = getattr(test, "_testMethodName", None)
        if method_name is None:
            return f"{self.suite_name}.{test.id()}"
        return f"{self.suite_name}.{method_name}"

    def startTest(self, test):
        super().startTest(test)
        self._start_time = time.time()
        print(f"[{self.colorizer.green(' RUN      ')}] {self._test_name(test)}")

    def _record(self, test, status, message=None):
        elapsed_ms = int((time.time() - self._start_time) * 1000)
        name = self._test_name(test)
        method_name = getattr(test, "_testMethodName", test.id())
        self.out_results.append(
            {
                "suite": self.suite_name,
                "name": method_name,
                "full_name": name,
                "status": status,
                "time_ms": elapsed_ms,
                "message": message,
            }
        )
        time_str = f" ({elapsed_ms} ms)" if self.args.gtest_print_time else ""
        if status == "FAILED":
            if message:
                print(message.rstrip())
            print(f"[{self.colorizer.red('  FAILED  ')}] {name}{time_str}")
            if self.args.gtest_break_on_failure or self.args.gtest_fail_fast:
                self.stop()
            if self.args.gtest_throw_on_failure:
                sys.exit(1)
        elif status == "SKIPPED":
            if message:
                print(str(message).rstrip())
            print(f"[{self.colorizer.yellow('  SKIPPED ')}] {name}{time_str}")
        else:
            if self.args.gtest_brief:
                return
            print(f"[{self.colorizer.green('       OK ')}] {name}{time_str}")

    def addSuccess(self, test):
        super().addSuccess(test)
        self._record(test, "OK")

    def addFailure(self, test, err):
        super().addFailure(test, err)
        self._record(test, "FAILED", self._exc_info_to_string(err, test))

    def addError(self, test, err):
        super().addError(test, err)
        self._record(test, "FAILED", self._exc_info_to_string(err, test))

    def addSkip(self, test, reason):
        super().addSkip(test, reason)
        self._record(test, "SKIPPED", reason)

    def addExpectedFailure(self, test, err):
        super().addExpectedFailure(test, err)
        self._record(test, "OK")

    def addUnexpectedSuccess(self, test):
        super().addUnexpectedSuccess(test)
        self._record(test, "FAILED", "Test unexpectedly succeeded (marked as expectedFailure).")


def run_tests(groups, args, colorizer):
    all_results = []
    total_tests = sum(len(v) for v in groups.values())
    total_suites = len(groups)
    print(
        f"[{colorizer.green('==========')}] Running {total_tests} test{_plural(total_tests)} "
        f"from {total_suites} test suite{_plural(total_suites)}."
    )
    print(f"[{colorizer.green('----------')}] Global test environment set-up.")
    overall_start = time.time()
    stopped_early = False
    for suite_name, tests in groups.items():
        if stopped_early:
            break
        n = len(tests)
        print(f"[{colorizer.green('----------')}] {n} test{_plural(n)} from {suite_name}")
        suite_start = time.time()
        result = GTestResult(suite_name, colorizer, args, all_results)
        unittest.TestSuite(tests).run(result)
        suite_elapsed_ms = int((time.time() - suite_start) * 1000)
        time_str = f" ({suite_elapsed_ms} ms total)" if args.gtest_print_time else ""
        print(f"[{colorizer.green('----------')}] {n} test{_plural(n)} from {suite_name}{time_str}")
        print()
        if result.shouldStop:
            stopped_early = True
    print(f"[{colorizer.green('----------')}] Global test environment tear-down")
    overall_elapsed_ms = int((time.time() - overall_start) * 1000)
    ran = len(all_results)
    time_str = f" ({overall_elapsed_ms} ms total)" if args.gtest_print_time else ""
    print(
        f"[{colorizer.green('==========')}] {ran} test{_plural(ran)} from {total_suites} "
        f"test suite{_plural(total_suites)} ran.{time_str}"
    )

    passed = [r for r in all_results if r["status"] == "OK"]
    failed = [r for r in all_results if r["status"] == "FAILED"]
    skipped = [r for r in all_results if r["status"] == "SKIPPED"]

    print(f"[{colorizer.green('  PASSED  ')}] {len(passed)} test{_plural(len(passed))}.")
    if skipped:
        print(
            f"[{colorizer.yellow('  SKIPPED ')}] {len(skipped)} test{_plural(len(skipped))}, "
            f"listed below:"
        )
        for r in skipped:
            print(f"[{colorizer.yellow('  SKIPPED ')}] {r['full_name']}")
    if failed:
        print(
            f"[{colorizer.red('  FAILED  ')}] {len(failed)} test{_plural(len(failed))}, "
            f"listed below:"
        )
        for r in failed:
            print(f"[{colorizer.red('  FAILED  ')}] {r['full_name']}")
        print()
        print(f"{len(failed)} FAILED TEST{'S' if len(failed) != 1 else ''}")

    return all_results, overall_elapsed_ms


def write_xml(path, results, total_time_ms):
    suites = {}
    for r in results:
        suites.setdefault(r["suite"], []).append(r)
    total_failures = sum(1 for r in results if r["status"] == "FAILED")
    lines = ['<?xml version="1.0" encoding="UTF-8"?>']
    lines.append(
        f'<testsuites tests="{len(results)}" failures="{total_failures}" disabled="0" '
        f'errors="0" time="{total_time_ms / 1000.0:.3f}" name="AllTests">'
    )
    for suite_name, items in suites.items():
        s_failures = sum(1 for r in items if r["status"] == "FAILED")
        s_time = sum(r["time_ms"] for r in items) / 1000.0
        lines.append(
            f'  <testsuite name="{xml_escape(suite_name)}" tests="{len(items)}" '
            f'failures="{s_failures}" disabled="0" errors="0" time="{s_time:.3f}">'
        )
        for r in items:
            t_time = r["time_ms"] / 1000.0
            if r["status"] == "FAILED":
                lines.append(
                    f'    <testcase name="{xml_escape(r["name"])}" status="run" '
                    f'result="completed" time="{t_time:.3f}" classname="{xml_escape(suite_name)}">'
                )
                message = r["message"] or "Failed"
                lines.append(
                    f'      <failure message="{xml_escape(message.splitlines()[0])}" type="">'
                    f"<![CDATA[{message}]]></failure>"
                )
                lines.append("    </testcase>")
            elif r["status"] == "SKIPPED":
                lines.append(
                    f'    <testcase name="{xml_escape(r["name"])}" status="run" '
                    f'result="skipped" time="{t_time:.3f}" classname="{xml_escape(suite_name)}">'
                )
                message = r["message"] or "Skipped"
                lines.append(
                    f'      <skipped message="{xml_escape(message.splitlines()[0])}">'
                    f"<![CDATA[{message}]]></skipped>"
                )
                lines.append("    </testcase>")
            else:
                lines.append(
                    f'    <testcase name="{xml_escape(r["name"])}" status="run" '
                    f'result="completed" time="{t_time:.3f}" classname="{xml_escape(suite_name)}" />'
                )
        lines.append("  </testsuite>")
    lines.append("</testsuites>")
    with open(path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")


def write_json(path, results, total_time_ms):
    suites = {}
    for r in results:
        suites.setdefault(r["suite"], []).append(r)
    total_failures = 0
    testsuites = []
    for suite_name, items in suites.items():
        s_failures = sum(1 for r in items if r["status"] == "FAILED")
        total_failures += s_failures
        s_time = sum(r["time_ms"] for r in items) / 1000.0
        testsuite = {
            "name": suite_name,
            "tests": len(items),
            "failures": s_failures,
            "disabled": 0,
            "errors": 0,
            "time": f"{s_time:.3f}s",
            "testsuite": [],
        }
        for r in items:
            testcase = {
                "name": r["name"],
                # Every entry here actually ran (disabled tests never reach this list),
                # so status is always RUN; only the outcome ("result") differs.
                "status": "RUN",
                "result": "SKIPPED" if r["status"] == "SKIPPED" else "COMPLETED",
                "time": f"{r['time_ms'] / 1000.0:.3f}s",
                "classname": suite_name,
            }
            if r["status"] == "FAILED":
                testcase["failures"] = [{"failure": r["message"] or "", "type": ""}]
            testsuite["testsuite"].append(testcase)
        testsuites.append(testsuite)
    data = {
        "tests": len(results),
        "failures": total_failures,
        "disabled": 0,
        "errors": 0,
        "time": f"{total_time_ms / 1000.0:.3f}s",
        "name": "AllTests",
        "testsuites": testsuites,
    }
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
        f.write("\n")


def write_output_file(spec, results, total_time_ms, working_dir):
    fmt, sep, path = spec.partition(":")
    if not sep:
        fmt, path = "xml", ""
    if not path:
        path = "test_detail.xml" if fmt == "xml" else "test_detail.json"
    wd = os.getcwd()
    os.chdir(working_dir)
    if os.path.isdir(path) or path.endswith(("/", "\\")):
        path = os.path.join(path, "test_detail.xml" if fmt == "xml" else "test_detail.json")
    directory = os.path.dirname(path)
    if directory and not os.path.exists(directory):
        os.makedirs(directory, exist_ok=True)
    if fmt == "json":
        write_json(path, results, total_time_ms)
    else:
        write_xml(path, results, total_time_ms)
    os.chdir(wd)
    return path

main_exit_code = 0

def main(argv=None):
    global main_exit_code
    try:
        args = build_arg_parser().parse_args(argv)
        working_dir = os.getcwd()
        os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

        color_enabled = should_use_color(args.gtest_color)
        if color_enabled:
            _enable_windows_ansi()
        colorizer = Colorizer(color_enabled)

        all_tests = flatten(discover_tests(args))
        filtered = [
            t
            for t in all_tests
            if gtest_pattern_match(f"{type(t).__name__}.{getattr(t, '_testMethodName', t.id())}", args.gtest_filter)
        ]

        if args.gtest_list_tests:
            print_test_list(filtered, colorizer)
            return 0

        if args.gtest_filter != "*":
            print(f"Note: Google Test filter = {args.gtest_filter}\n")

        disabled_tests = [t for t in filtered if is_disabled(t)]
        if args.gtest_also_run_disabled_tests:
            runnable = filtered
        else:
            runnable = [t for t in filtered if not is_disabled(t)]

        seed = args.gtest_random_seed
        if args.gtest_shuffle:
            if seed == 0:
                seed = int(time.time() * 1000) % 100000
            random.Random(seed).shuffle(runnable)
            print(f"Note: Randomizing tests' orders with a seed of {seed} .\n")

        repeat = args.gtest_repeat
        infinite = repeat < 0
        iteration = 0
        exit_code = 0
        while infinite or iteration < repeat:
            iteration += 1
            if repeat != 1 and iteration > 1:
                print(f"Repeating all tests (iteration {iteration}) . . .\n")

            groups = group_by_suite(runnable)
            results, elapsed_ms = run_tests(groups, args, colorizer)

            if not args.gtest_also_run_disabled_tests and disabled_tests:
                n = len(disabled_tests)
                print(f"\n  YOU HAVE {n} DISABLED TEST{'S' if n != 1 else ''}")

            if args.gtest_output:
                write_output_file(args.gtest_output, results, elapsed_ms, working_dir)

            if any(r["status"] == "FAILED" for r in results):
                exit_code = 1
                if infinite:
                    break
    except:
        main_exit_code = 1
        raise
    main_exit_code = exit_code


if __name__ == "__main__":
    import blue
    sys.modules['_scheduler'] = blue.LoadExtension('_scheduler')
    import scheduler

    tasklet = scheduler.tasklet(main)
    tasklet()
    while tasklet.alive:
        blue.os.Pump()

    sys.exit(main_exit_code)
